#Requires -Version 5.1

<#
.SYNOPSIS
    Checks the NimbyTapes audio files against the addon's config.cpp before a build.

.DESCRIPTION
    The static gate and the pack check pass when an audio file is missing, because they compare
    the PBO with the files that exist in the checkout (plan risk K8). This script is the check
    that the audio is there and is what the config says. Each check writes PASS or FAIL lines;
    any FAIL fails the run.
      1. config  : every tape class with a tracks[] list has a trackSeconds[] list of the same
                   length, and every track resolves sound set -> sound shader -> sample -> an
                   .ogg file inside the addon.
      2. format  : ffprobe reports Vorbis, mono, 44,100 Hz for every .ogg under sound\.
      3. length  : each track's length is within -ToleranceSeconds of its trackSeconds[] value.
      4. unused  : no .ogg under sound\ is left out of every tape (it would ship as dead weight).
    Before Phase 0 step 3 the config has no tape yet. -AllowMissingConfig then runs check 2
    only, on every .ogg under sound\, and still fails when there is none.

    Reads   : <addon>\config.cpp, <addon>\sound\*.ogg, ffprobe on PATH (or -FfprobePath),
              scripts\modules\NimbyCommon.psm1.
    Writes  : <repo>\logs\tapes-assets-<stamp>.log.
    Never   : changes the config or an audio file, or contacts a remote.

    Exit codes: 0 = every check passed, 1 = at least one failed.

    Runs in Windows PowerShell 5.1 and PowerShell 7. No elevation needed.

.PARAMETER AddonPath
    The NimbyTapes addon folder. Defaults to the parent of this script's folder.

.PARAMETER RepoRoot
    Repository root. Defaults to four levels above the addon folder.

.PARAMETER FfprobePath
    Full path of ffprobe. Found on PATH when omitted.

.PARAMETER ToleranceSeconds
    Allowed difference between a file's length and its trackSeconds[] value. Default 1.

.PARAMETER AllowMissingConfig
    Accept a config.cpp that defines no tape yet (Phase 0 step 2 only).

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server-wt\nimby-music\mods\own\nimby-pack\NimbyTapes\tools\Test-TapesAssets.ps1
#>
[CmdletBinding()]
param(
    [string]$AddonPath,
    [string]$RepoRoot,
    [string]$FfprobePath,
    [double]$ToleranceSeconds = 1,
    [switch]$AllowMissingConfig
)

Set-StrictMode -Version Latest
# Windows PowerShell 5.1 leaves $PSScriptRoot empty in a param() default when an advanced
# script is started with -File, so the defaults are resolved here.
if (-not $AddonPath) { $AddonPath = Split-Path -Parent $PSScriptRoot }
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $AddonPath))) }
$ErrorActionPreference = 'Stop'

$FfprobeInstallHint = 'ffprobe ships with ffmpeg. Install it with: winget install --id Gyan.FFmpeg -e   then open a new shell.'

function Get-ConfigTextWithoutComment {
    # config.cpp text without // and /* */ comments. Text inside double quotes is kept as is.
    param([Parameter(Mandatory)][AllowEmptyString()][string]$Text)

    $builder = New-Object System.Text.StringBuilder
    $inString = $false
    $index = 0
    $length = $Text.Length
    while ($index -lt $length) {
        $char = $Text[$index]
        if ($inString) {
            [void]$builder.Append($char)
            if ($char -eq [char]'"') { $inString = $false }
            $index++
            continue
        }
        if ($char -eq [char]'"') {
            $inString = $true
            [void]$builder.Append($char)
            $index++
            continue
        }
        if ($char -eq [char]'/' -and ($index + 1) -lt $length) {
            $next = $Text[$index + 1]
            if ($next -eq [char]'/') {
                while ($index -lt $length -and $Text[$index] -ne [char]"`n") { $index++ }
                continue
            }
            if ($next -eq [char]'*') {
                $end = $Text.IndexOf('*/', $index + 2)
                if ($end -lt 0) { break }
                $index = $end + 2
                continue
            }
        }
        [void]$builder.Append($char)
        $index++
    }
    return $builder.ToString()
}

function Find-ClosingBrace {
    # Index of the brace that closes the one at -OpenIndex, or -1. Braces inside double quotes
    # do not count.
    param(
        [Parameter(Mandatory)][AllowEmptyString()][string]$Text,
        [Parameter(Mandatory)][int]$OpenIndex
    )

    $depth = 0
    $inString = $false
    for ($index = $OpenIndex; $index -lt $Text.Length; $index++) {
        $char = $Text[$index]
        if ($char -eq [char]'"') { $inString = -not $inString; continue }
        if ($inString) { continue }
        if ($char -eq [char]'{') { $depth++; continue }
        if ($char -eq [char]'}') {
            $depth--
            if ($depth -eq 0) { return $index }
        }
    }
    return -1
}

function Get-ConfigClass {
    # Every class with a body in comment-free config text: Name, Parent ('' when none), and
    # Body, the class's own text with the bodies of nested classes blanked out.
    param([Parameter(Mandatory)][AllowEmptyString()][string]$Text)

    $found = New-Object 'System.Collections.Generic.List[object]'
    foreach ($match in [regex]::Matches($Text, '\bclass\s+(?<name>\w+)\s*(?::\s*(?<parent>\w+)\s*)?\{')) {
        $open = $match.Index + $match.Length - 1
        $close = Find-ClosingBrace -Text $Text -OpenIndex $open
        if ($close -lt 0) { continue }
        $found.Add([pscustomobject]@{ Name = $match.Groups['name'].Value; Parent = $match.Groups['parent'].Value; Open = $open; Close = $close; Body = '' })
    }
    foreach ($class in $found) {
        $chars = $Text.Substring($class.Open + 1, $class.Close - $class.Open - 1).ToCharArray()
        foreach ($other in $found) {
            if ($other.Open -gt $class.Open -and $other.Close -lt $class.Close) {
                for ($index = $other.Open; $index -le $other.Close; $index++) { $chars[$index - $class.Open - 1] = [char]' ' }
            }
        }
        $class.Body = New-Object string (, $chars)
    }
    return $found.ToArray()
}

function Get-ConfigArray {
    # The values of "<Name>[] = { ... };" in a class body, flattened: quoted strings without
    # their quotes, and numbers as text. $null when the body has no such array.
    param(
        [Parameter(Mandatory)][AllowEmptyString()][string]$Body,
        [Parameter(Mandatory)][string]$Name
    )

    $match = [regex]::Match($Body, ('(?<![\w])' + [regex]::Escape($Name) + '\s*\[\]\s*=\s*\{'))
    if (-not $match.Success) { return $null }
    $open = $match.Index + $match.Length - 1
    $close = Find-ClosingBrace -Text $Body -OpenIndex $open
    if ($close -lt 0) { return $null }
    $inner = $Body.Substring($open + 1, $close - $open - 1)
    $values = New-Object 'System.Collections.Generic.List[string]'
    foreach ($token in [regex]::Matches($inner, '"(?<text>[^"]*)"|(?<number>-?\d+(?:\.\d+)?)')) {
        if ($token.Groups['text'].Success) { $values.Add($token.Groups['text'].Value) }
        else { $values.Add($token.Groups['number'].Value) }
    }
    return , $values.ToArray()
}

function Get-TapeTrackPlan {
    # What the config says about every tape track. Returns an object with Tracks (Tape, Index,
    # SoundSet, Shader, Sample, File, Seconds) and Problems (text lines). -AddonPrefix is the
    # PBO prefix the sample paths start with, which is the addon folder's name.
    param(
        [Parameter(Mandatory)][AllowEmptyString()][string]$ConfigText,
        [Parameter(Mandatory)][string]$AddonPath,
        [Parameter(Mandatory)][string]$AddonPrefix
    )

    $classes = @(Get-ConfigClass -Text (Get-ConfigTextWithoutComment -Text $ConfigText))
    $byName = @{}
    foreach ($class in $classes) { $byName[$class.Name] = $class }
    $tracks = New-Object 'System.Collections.Generic.List[object]'
    $problems = New-Object 'System.Collections.Generic.List[string]'

    foreach ($class in $classes) {
        $setNames = Get-ConfigArray -Body $class.Body -Name 'tracks'
        if ($null -eq $setNames -or $setNames.Count -eq 0) { continue }
        $seconds = Get-ConfigArray -Body $class.Body -Name 'trackSeconds'
        if ($null -eq $seconds) { $seconds = @() }
        if ($seconds.Count -ne $setNames.Count) {
            $problems.Add(("{0} has {1} tracks[] and {2} trackSeconds[]" -f $class.Name, $setNames.Count, $seconds.Count))
            continue
        }
        for ($index = 0; $index -lt $setNames.Count; $index++) {
            $where = "{0} track {1}" -f $class.Name, $index
            $setName = $setNames[$index]
            if (-not $byName.ContainsKey($setName)) { $problems.Add("$where sound set '$setName' is not defined in config.cpp"); continue }
            $shaders = Get-ConfigArray -Body $byName[$setName].Body -Name 'soundShaders'
            if ($null -eq $shaders -or $shaders.Count -ne 1) { $problems.Add("$where sound set '$setName' must name exactly one sound shader"); continue }
            $shaderName = $shaders[0]
            if (-not $byName.ContainsKey($shaderName)) { $problems.Add("$where sound shader '$shaderName' is not defined in config.cpp"); continue }
            $samples = Get-ConfigArray -Body $byName[$shaderName].Body -Name 'samples'
            if ($null -eq $samples -or $samples.Count -lt 1) { $problems.Add("$where sound shader '$shaderName' has no samples[]"); continue }
            $sample = ($samples[0] -replace '/', '\').TrimStart('\')
            if (-not $sample.StartsWith($AddonPrefix + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
                $problems.Add("$where sample '$($samples[0])' is outside the addon prefix '$AddonPrefix'")
                continue
            }
            $relative = $sample.Substring($AddonPrefix.Length + 1) + '.ogg'
            $file = $AddonPath
            foreach ($part in ($relative -split '\\')) { $file = Join-Path $file $part }
            $secondsValue = 0.0
            if (-not [double]::TryParse($seconds[$index], [System.Globalization.NumberStyles]::Float, [System.Globalization.CultureInfo]::InvariantCulture, [ref]$secondsValue) -or $secondsValue -le 0) {
                $problems.Add("$where trackSeconds '$($seconds[$index])' is not a length above zero")
                continue
            }
            $tracks.Add([pscustomobject]@{ Tape = $class.Name; Index = $index; SoundSet = $setName; Shader = $shaderName; Sample = $samples[0]; File = $file; Seconds = $secondsValue })
        }
    }
    return [pscustomobject]@{ Tracks = $tracks.ToArray(); Problems = $problems.ToArray() }
}

function Resolve-FfprobePath {
    # The ffprobe to run: -FfprobePath when given, otherwise the first ffprobe on PATH.
    param([string]$Path)

    if ($Path) {
        if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "ffprobe not found at '$Path'. $FfprobeInstallHint" }
        return $Path
    }
    $command = Get-Command -Name 'ffprobe' -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $command) { throw "ffprobe is not on PATH. $FfprobeInstallHint" }
    return $command.Source
}

function Invoke-NativeTool {
    # Runs a native tool and returns its exit code and output lines. Windows PowerShell 5.1 turns
    # redirected stderr lines into error records, so the preference is relaxed inside this call.
    param(
        [Parameter(Mandatory)][string]$FilePath,
        [Parameter(Mandatory)][string[]]$ArgumentList
    )

    $ErrorActionPreference = 'Continue'
    $global:LASTEXITCODE = $null
    $output = @(& $FilePath @ArgumentList 2>&1 | ForEach-Object {
            if ($_ -is [System.Management.Automation.ErrorRecord]) { $_.Exception.Message } else { [string]$_ }
        })
    return [pscustomobject]@{ ExitCode = $LASTEXITCODE; Output = $output }
}

function ConvertFrom-FfprobeJson {
    # Codec, Channels, SampleRate, and Seconds of the first audio stream from ffprobe's JSON
    # text, or $null when the text holds no audio stream.
    param([Parameter(Mandatory)][AllowEmptyString()][string]$Json)

    if (-not $Json.Trim()) { return $null }
    try { $data = $Json | ConvertFrom-Json } catch { return $null }
    if ($null -eq $data -or $null -eq $data.PSObject.Properties['streams'] -or @($data.streams).Count -eq 0) { return $null }
    $stream = @($data.streams)[0]
    $seconds = 0.0
    $formatProperty = $data.PSObject.Properties['format']
    if ($null -ne $formatProperty -and $null -ne $formatProperty.Value -and $null -ne $formatProperty.Value.PSObject.Properties['duration']) {
        [void][double]::TryParse([string]$formatProperty.Value.duration, [System.Globalization.NumberStyles]::Float, [System.Globalization.CultureInfo]::InvariantCulture, [ref]$seconds)
    }
    $channels = 0
    if ($null -ne $stream.PSObject.Properties['channels']) { $channels = [int]$stream.channels }
    $sampleRate = 0
    if ($null -ne $stream.PSObject.Properties['sample_rate']) { $sampleRate = [int]$stream.sample_rate }
    $codec = ''
    if ($null -ne $stream.PSObject.Properties['codec_name']) { $codec = [string]$stream.codec_name }
    return [pscustomobject]@{ Codec = $codec; Channels = $channels; SampleRate = $sampleRate; Seconds = $seconds }
}

function Get-AudioFact {
    # ffprobe's view of one file (see ConvertFrom-FfprobeJson), or $null when ffprobe fails.
    param(
        [Parameter(Mandatory)][string]$Ffprobe,
        [Parameter(Mandatory)][string]$File
    )

    $arguments = @('-v', 'error', '-select_streams', 'a:0', '-show_entries', 'stream=codec_name,channels,sample_rate:format=duration', '-of', 'json', $File)
    $result = Invoke-NativeTool -FilePath $Ffprobe -ArgumentList $arguments
    if ($result.ExitCode -ne 0) { return $null }
    return ConvertFrom-FfprobeJson -Json ($result.Output -join "`n")
}

function Get-FormatProblem {
    # Text lines for a file that is not Vorbis, mono, 44,100 Hz. Empty when it is.
    param([Parameter(Mandatory)][pscustomobject]$Fact)

    $problems = @()
    if ($Fact.Codec -ne 'vorbis') { $problems += "codec is '$($Fact.Codec)', expected vorbis" }
    if ($Fact.Channels -ne 1) { $problems += "channels is $($Fact.Channels), expected 1 (mono)" }
    if ($Fact.SampleRate -ne 44100) { $problems += "sample rate is $($Fact.SampleRate), expected 44100" }
    return $problems
}

function Get-LengthProblem {
    # A text line when the measured length is more than -Tolerance away from -Expected.
    param(
        [Parameter(Mandatory)][double]$Measured,
        [Parameter(Mandatory)][double]$Expected,
        [Parameter(Mandatory)][double]$Tolerance
    )

    $difference = [Math]::Abs($Measured - $Expected)
    if ($difference -le $Tolerance) { return @() }
    return @(("length is {0:0.00} s, trackSeconds[] says {1:0.##} s (difference {2:0.00} s, allowed {3:0.##} s)" -f $Measured, $Expected, $difference, $Tolerance))
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

Import-Module (Join-Path (Join-Path (Join-Path $RepoRoot 'scripts') 'modules') 'NimbyCommon.psm1') -Force

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$logDir = Join-Path $RepoRoot 'logs'
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
$log = Join-Path $logDir ("tapes-assets-{0}.log" -f $stamp)
$failed = 0

function Write-CheckResult {
    # One PASS line, or one FAIL line per problem, for a named check.
    param([Parameter(Mandatory)][string]$Check, [AllowEmptyCollection()][string[]]$Problem = @())
    if ($Problem.Count -eq 0) {
        Write-Manifest -LogPath $log -Line "PASS $Check"
        Write-Information -MessageData "PASS $Check" -InformationAction Continue
        return
    }
    foreach ($line in $Problem) {
        Write-Manifest -LogPath $log -Line "FAIL $Check $line"
        Write-Information -MessageData "FAIL $Check $line" -InformationAction Continue
    }
    $script:failed += $Problem.Count
}

try {
    Write-Manifest -LogPath $log -Line ("START {0} AddonPath={1} AllowMissingConfig={2} PS={3}" -f (Get-Date -Format o), $AddonPath, [bool]$AllowMissingConfig, $PSVersionTable.PSVersion)
    $configPath = Join-Path $AddonPath 'config.cpp'
    if (-not (Test-Path -LiteralPath $configPath -PathType Leaf)) { throw "AddonPath '$AddonPath' has no config.cpp; it is not the NimbyTapes addon folder." }
    $ffprobe = Resolve-FfprobePath -Path $FfprobePath
    Write-Manifest -LogPath $log -Line "TOOL ffprobe $ffprobe"

    $soundDir = Join-Path $AddonPath 'sound'
    $oggFiles = @()
    if (Test-Path -LiteralPath $soundDir -PathType Container) { $oggFiles = @(Get-ChildItem -LiteralPath $soundDir -File -Filter '*.ogg' | Sort-Object Name) }

    # 1. config
    $plan = Get-TapeTrackPlan -ConfigText ([System.IO.File]::ReadAllText($configPath)) -AddonPath $AddonPath -AddonPrefix (Split-Path -Leaf $AddonPath)
    $configProblems = @($plan.Problems)
    $haveTapes = ($plan.Tracks.Count -gt 0) -or ($configProblems.Count -gt 0)
    if (-not $haveTapes) {
        if ($AllowMissingConfig) {
            Write-Manifest -LogPath $log -Line 'NOTE config defines no tape yet; -AllowMissingConfig given, so only the format check runs'
            Write-Information -MessageData 'NOTE config defines no tape yet; only the format check runs (-AllowMissingConfig)' -InformationAction Continue
        }
        else { $configProblems += 'config.cpp defines no tape class with tracks[] (Phase 0 step 3 adds them; -AllowMissingConfig is for step 2 only)' }
    }
    foreach ($track in $plan.Tracks) {
        if (-not (Test-Path -LiteralPath $track.File -PathType Leaf)) { $configProblems += ("{0} track {1} file is missing: {2} (run New-TimingTrack.ps1)" -f $track.Tape, $track.Index, $track.File) }
    }
    if ($haveTapes -or -not $AllowMissingConfig) { Write-CheckResult -Check 'config' -Problem $configProblems }

    # 2. format
    $facts = @{}
    $formatProblems = @()
    if ($oggFiles.Count -eq 0) { $formatProblems += "no .ogg file under $soundDir (run New-TimingTrack.ps1)" }
    foreach ($file in $oggFiles) {
        $fact = Get-AudioFact -Ffprobe $ffprobe -File $file.FullName
        if ($null -eq $fact) { $formatProblems += "$($file.Name) could not be read by ffprobe"; continue }
        $facts[$file.FullName] = $fact
        Write-Manifest -LogPath $log -Line ("FILE {0} codec={1} channels={2} sampleRate={3} seconds={4:0.000} bytes={5}" -f $file.Name, $fact.Codec, $fact.Channels, $fact.SampleRate, $fact.Seconds, $file.Length)
        foreach ($line in (Get-FormatProblem -Fact $fact)) { $formatProblems += "$($file.Name) $line" }
    }
    Write-CheckResult -Check 'format' -Problem $formatProblems

    if ($haveTapes) {
        # 3. length
        $lengthProblems = @()
        $used = @{}
        foreach ($track in $plan.Tracks) {
            $key = [System.IO.Path]::GetFullPath($track.File)
            $used[$key] = $true
            if (-not $facts.ContainsKey($key)) { continue }
            foreach ($line in (Get-LengthProblem -Measured $facts[$key].Seconds -Expected $track.Seconds -Tolerance $ToleranceSeconds)) {
                $lengthProblems += ("{0} track {1} ({2}) {3}" -f $track.Tape, $track.Index, (Split-Path -Leaf $track.File), $line)
            }
        }
        Write-CheckResult -Check 'length' -Problem $lengthProblems

        # 4. unused
        $unusedProblems = @()
        foreach ($file in $oggFiles) {
            if (-not $used.ContainsKey([System.IO.Path]::GetFullPath($file.FullName))) { $unusedProblems += "$($file.Name) is on no tape" }
        }
        Write-CheckResult -Check 'unused' -Problem $unusedProblems
    }
}
catch {
    Write-Manifest -LogPath $log -Line ("FAIL error {0}" -f $_.Exception.Message)
    Write-Information -MessageData ("FAIL error {0}" -f $_.Exception.Message) -InformationAction Continue
    $failed++
}

$verdict = 'PASS'
if ($failed -gt 0) { $verdict = 'FAIL' }
Write-Manifest -LogPath $log -Line ("{0}: {1} failure(s)" -f $verdict, $failed)
Write-Information -MessageData ("{0}: {1} failure(s). Log: {2}" -f $verdict, $failed, $log) -InformationAction Continue
if ($failed -gt 0) { exit 1 }
exit 0
