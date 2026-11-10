#Requires -Version 5.1

<#
.SYNOPSIS
    Generates the two Phase 0 timing tracks for NimbyTapes (Music, Plan 13) with ffmpeg.

.DESCRIPTION
    Writes timing_a.ogg (240 seconds) and timing_b.ogg (60 seconds) into the addon's sound\
    folder: Ogg Vorbis, mono, 44,100 Hz (ruling T-4). Each track carries:
      - a beep (100 ms) at second 0 and every 5 seconds after it;
      - a position mark every 30 seconds: 0.5 seconds after the beep, a run of short pips
        (80 ms on, 120 ms off) at a second pitch. The mark at second 0 has 1 pip, the mark at
        second 30 has 2, and so on, so a recording shows the absolute position and not only
        the beep alignment (test rows T0-14 and T0-15).
    Track A beeps at 880 Hz with pips at 1760 Hz. Track B beeps at 440 Hz with pips at 660 Hz,
    so the two are told apart by ear.

    The files are test fixtures: git-ignored (mods/own/nimby-pack/NimbyTapes/sound/*.ogg) and
    never committed (plan P4). An existing track is left alone unless -Force is given. The
    track lengths here are the values the config's trackSeconds[] must carry;
    Test-TapesAssets.ps1 compares the two.

    Reads   : ffmpeg on PATH (or -FfmpegPath), scripts\modules\NimbyCommon.psm1.
    Writes  : <addon>\sound\timing_a.ogg, <addon>\sound\timing_b.ogg,
              <repo>\logs\tapes-timingtrack-<stamp>.log, and while encoding
              <addon>\sound\<name>.tmp.ogg, which stays behind only when ffmpeg fails.
    Never   : touches any other file, contacts a remote, or replaces an existing track
              without -Force.

    Exit codes: 0 = both tracks are in place, 1 = failure.

    Runs in Windows PowerShell 5.1 and PowerShell 7. No elevation needed.

.PARAMETER AddonPath
    The NimbyTapes addon folder. Defaults to the parent of this script's folder.

.PARAMETER RepoRoot
    Repository root. Defaults to four levels above the addon folder.

.PARAMETER FfmpegPath
    Full path of ffmpeg. Found on PATH when omitted.

.PARAMETER Force
    Replace tracks that already exist.

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server-wt\nimby-music\mods\own\nimby-pack\NimbyTapes\tools\New-TimingTrack.ps1
#>
[CmdletBinding()]
param(
    [string]$AddonPath,
    [string]$RepoRoot,
    [string]$FfmpegPath,
    [switch]$Force
)

Set-StrictMode -Version Latest
# Windows PowerShell 5.1 leaves $PSScriptRoot empty in a param() default when an advanced
# script is started with -File, so the defaults are resolved here.
if (-not $AddonPath) { $AddonPath = Split-Path -Parent $PSScriptRoot }
# Full paths from here on. A relative -AddonPath would otherwise break the RepoRoot default, and
# .NET file calls resolve a relative path against the process directory, which is not always the
# PowerShell location.
$AddonPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($AddonPath)
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $AddonPath))) }
$RepoRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($RepoRoot)
$ErrorActionPreference = 'Stop'

$FfmpegInstallHint = 'Install it with: winget install --id Gyan.FFmpeg -e   (the full build, which has libvorbis), then open a new shell.'

function Get-TimingTrackDefinition {
    # The Phase 0 fixtures (plan step 2, ruling T-4). Seconds is what trackSeconds[] must say.
    return @(
        [pscustomobject]@{ Name = 'timing_a'; Seconds = 240; BeepHz = 880; PipHz = 1760 }
        [pscustomobject]@{ Name = 'timing_b'; Seconds = 60; BeepHz = 440; PipHz = 660 }
    )
}

function Get-TimingTrackExpression {
    # The aevalsrc expression for one track. No spaces and no double quotes: the value travels
    # as one native argument in both Windows PowerShell 5.1 and PowerShell 7.
    param(
        [Parameter(Mandatory)][int]$BeepHz,
        [Parameter(Mandatory)][int]$PipHz
    )

    # A 100 ms beep at every multiple of 5 seconds.
    $beep = '0.5*sin(2*PI*{0}*t)*lt(mod(t,5),0.1)' -f $BeepHz
    # Every 30 seconds, from 0.5 s after the beep: floor(t/30)+1 pips, 80 ms on and 120 ms off.
    $pips = '0.5*sin(2*PI*{0}*t)*gte(mod(t,30),0.5)*lt(mod(t,30)-0.5,(floor(t/30)+1)*0.2)*lt(mod(mod(t,30)-0.5,0.2),0.08)' -f $PipHz
    return ($beep + '+' + $pips)
}

function Get-TimingTrackArgument {
    # The ffmpeg argument list for one track, written to -OutFile in Ogg Vorbis.
    param(
        [Parameter(Mandatory)][pscustomobject]$Track,
        [Parameter(Mandatory)][string]$OutFile
    )

    $expression = Get-TimingTrackExpression -BeepHz $Track.BeepHz -PipHz $Track.PipHz
    $source = "aevalsrc='{0}':s=44100:d={1}" -f $expression, $Track.Seconds
    $arguments = New-Object 'System.Collections.Generic.List[string]'
    foreach ($item in @('-hide_banner', '-nostdin', '-loglevel', 'error', '-y', '-f', 'lavfi', '-i', $source)) { $arguments.Add($item) }
    foreach ($item in @('-ac', '1', '-ar', '44100', '-c:a', 'libvorbis', '-q:a', '4')) { $arguments.Add($item) }
    # No metadata and fixed stream serials, so the same ffmpeg gives the same file every run.
    foreach ($item in @('-map_metadata', '-1', '-fflags', '+bitexact', '-flags:a', '+bitexact', '-f', 'ogg')) { $arguments.Add($item) }
    $arguments.Add($OutFile)
    return $arguments.ToArray()
}

function Resolve-FfmpegPath {
    # The ffmpeg to run: -FfmpegPath when given, otherwise the first ffmpeg on PATH. Throws with
    # the install command when none is found.
    param([string]$Path)

    if ($Path) {
        if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "ffmpeg not found at '$Path'. $FfmpegInstallHint" }
        return $Path
    }
    $command = Get-Command -Name 'ffmpeg' -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $command) { throw "ffmpeg is not on PATH. $FfmpegInstallHint" }
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

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

Import-Module (Join-Path (Join-Path (Join-Path $RepoRoot 'scripts') 'modules') 'NimbyCommon.psm1') -Force

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$logDir = Join-Path $RepoRoot 'logs'
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
$log = Join-Path $logDir ("tapes-timingtrack-{0}.log" -f $stamp)
$failed = 0

function Write-RunLine {
    # One line to the log and to the console.
    param([Parameter(Mandatory)][string]$Line)
    Write-Manifest -LogPath $log -Line $Line
    Write-Information -MessageData $Line -InformationAction Continue
}

try {
    Write-RunLine -Line ("START {0} AddonPath={1} Force={2} PS={3}" -f (Get-Date -Format o), $AddonPath, [bool]$Force, $PSVersionTable.PSVersion)
    if (-not (Test-Path -LiteralPath (Join-Path $AddonPath 'config.cpp') -PathType Leaf)) { throw "AddonPath '$AddonPath' has no config.cpp; it is not the NimbyTapes addon folder." }

    $ffmpeg = Resolve-FfmpegPath -Path $FfmpegPath
    $version = Invoke-NativeTool -FilePath $ffmpeg -ArgumentList @('-hide_banner', '-version')
    $versionLine = '(no output)'
    if ($version.Output.Count -gt 0) { $versionLine = $version.Output[0] }
    Write-RunLine -Line ("TOOL ffmpeg {0} | {1}" -f $ffmpeg, $versionLine)
    $encoders = Invoke-NativeTool -FilePath $ffmpeg -ArgumentList @('-hide_banner', '-encoders')
    if (@($encoders.Output | Where-Object { $_ -match '\blibvorbis\b' }).Count -eq 0) { throw "This ffmpeg has no libvorbis encoder. $FfmpegInstallHint" }

    $soundDir = Join-Path $AddonPath 'sound'
    New-Item -ItemType Directory -Path $soundDir -Force | Out-Null

    foreach ($track in (Get-TimingTrackDefinition)) {
        $outFile = Join-Path $soundDir ("{0}.ogg" -f $track.Name)
        if ((Test-Path -LiteralPath $outFile -PathType Leaf) -and -not $Force) {
            $existing = Get-Item -LiteralPath $outFile
            Write-RunLine -Line ("SKIP {0} exists ({1} bytes); pass -Force to regenerate" -f $outFile, $existing.Length)
            continue
        }

        # Encode beside the target and rename it into place, so a failed run never leaves a
        # half-written track under the real name. The temporary name ends in .ogg, so a
        # leftover is git-ignored and Test-TapesAssets.ps1 reports it as unused.
        $tempFile = Join-Path $soundDir ("{0}.tmp.ogg" -f $track.Name)
        $result = Invoke-NativeTool -FilePath $ffmpeg -ArgumentList (Get-TimingTrackArgument -Track $track -OutFile $tempFile)
        $tempOk = (Test-Path -LiteralPath $tempFile -PathType Leaf) -and ((Get-Item -LiteralPath $tempFile).Length -gt 0)
        if ($result.ExitCode -ne 0 -or -not $tempOk) {
            Write-RunLine -Line ("FAIL {0} ffmpeg exit={1} output={2}" -f $track.Name, $result.ExitCode, ($result.Output -join ' | '))
            if (Test-Path -LiteralPath $tempFile -PathType Leaf) { Write-RunLine -Line ("NOTE partial file left at {0}" -f $tempFile) }
            $failed++
            continue
        }
        # Paths are full here, so the .NET calls see the same files PowerShell does. The old
        # track goes only now, after the new one encoded; the rename is one step on one volume.
        if (Test-Path -LiteralPath $outFile -PathType Leaf) { [System.IO.File]::Delete($outFile) }
        [System.IO.File]::Move($tempFile, $outFile)
        $written = Get-Item -LiteralPath $outFile
        $hash = (Get-FileHash -LiteralPath $outFile -Algorithm SHA256).Hash
        Write-RunLine -Line ("OK {0} seconds={1} beepHz={2} pipHz={3} bytes={4} sha256={5}" -f $outFile, $track.Seconds, $track.BeepHz, $track.PipHz, $written.Length, $hash)
    }
}
catch {
    Write-RunLine -Line ("FAIL error {0}" -f $_.Exception.Message)
    $failed++
}

$verdict = 'PASS'
if ($failed -gt 0) { $verdict = 'FAIL' }
Write-RunLine -Line ("{0}: {1} failure(s). Log: {2}" -f $verdict, $failed, $log)
if ($failed -gt 0) { exit 1 }
exit 0
