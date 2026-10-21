#Requires -Version 5.1

<#
.SYNOPSIS
    Runs the local DayZ server with a qualified @NIMBY build for an in-game test, stops it
    gracefully over RCON, then summarizes every [NIMBY] log line the test produced.

.DESCRIPTION
    1. Picks the build: -BuildPath, or the newest build\@NIMBY-* whose logs\build-<stamp>\build.log
       ends in "SUMMARY PASS". A build that recorded FAIL or UNKNOWN, or has no build log, is
       never picked up by itself and is refused as -BuildPath unless -Force is given. The build
       log also names the -Server and -Dev companion outputs, loaded when present. Every PBO is
       hashed and compared with the sha256 the build log recorded for it ("ADDON OK"); a PBO
       that changed, went missing, or was added since the build is a HASH MISMATCH and is
       refused unless -Force is given.
    2. Copies nimby.bikey (and the dev key when a -Dev build is loaded) into the server's keys
       folder so clients that load the pack pass signature verification (verifySignatures = 2).
    3. Writes a throwaway server config from pack.json "testServer" with the join password from
       -JoinPassword, else the one saved in the reusable profile, else the standing password from
       NIMBY_JOIN_PASSWORD or scripts\testserver.local.json, else a random one; BattlEye = 1 and
       storageAutoFix = 0 (a qualification run must show corruption, not hide it), writes
       <profile>\BattlEye\BEServer_x64.cfg with a random RCON password, starts the server, and
       waits for Enter or Ctrl+C (both stop it the same way) or for the server to
       exit by itself.
    4. Sends "#shutdown" over RCON, waits up to -ShutdownTimeoutSeconds for the server to exit,
       and only then stops the process by force. Redacts both passwords from the config files.
    5. Writes nimby-events.log (every [NIMBY] line from this run) and a per-result count to
       testserver.log, which also records every PBO it loaded with its SHA-256.

    Profile: by default each run gets a fresh <run>\server-profile. With -ProfileName the run
    reuses <repo>\profiles\<name>\ (git-ignored), so $profile:<Prefix>\ state and the server's
    storage_<instanceId> carry over between runs.

    Reads   : pack.json, build\@NIMBY-*, logs\build-*\build.log, the local DayZ Server install,
              the NIMBY_JOIN_PASSWORD environment variable, scripts\testserver.local.json
              (git-ignored), <profile>\join-password.txt with -ProfileName, the Steam registry
              key and library files (to find the server), and the process list.
    Writes  : <repo>\logs\testserver-<stamp>\ (config, summary, and the server profile unless
              -ProfileName), <repo>\profiles\<ProfileName>\ with -ProfileName, and
              <server>\keys\<authority>.bikey (added, never overwritten if different). The server
              writes its test persistence to mpmissions\<mission>\storage_<instanceId>; pack.json
              must name 998 or 999.
    Secrets : the RCON password is a random throwaway value, never shown or logged. The join
              password comes from -JoinPassword, else <profile>\join-password.txt, else the
              standing value in the NIMBY_JOIN_PASSWORD environment variable or
              <repo>\scripts\testserver.local.json ("joinPassword"; git-ignored by *.local.json),
              else a random value. It is shown once on screen. With -ProfileName an explicit or
              random value is saved to <profile>\join-password.txt (git-ignored) and reused by
              later runs of that profile; a standing value is never saved there. The manifest
              records only the source, never the value. Both passwords sit in config files under
              the git-ignored logs or profiles folder while the server runs and are redacted once
              it stops.
    Never   : opens a firewall port, touches storage_1, deletes a file, or starts a server while
              another DayZServer_x64 process runs.

    Runs in Windows PowerShell 5.1 and PowerShell 7. No elevation needed. Windows Firewall may
    ask once to allow DayZServer_x64.exe; allow private networks only.

.PARAMETER BuildPath
    The build\@NIMBY-<stamp> folder to run. Alias -ModPath. When omitted, the newest build whose
    build.log records SUMMARY PASS is used.

.PARAMETER ServerModPath
    A build\@NIMBY-Server-<stamp> folder for -servermod=. Defaults from the chosen build's log.

.PARAMETER DevPath
    A build\@NIMBY-Dev-<stamp> folder loaded beside the pack. Defaults from the chosen build's log.

.PARAMETER ProfileName
    Reuse <repo>\profiles\<ProfileName> as the server profile instead of a per-run folder.

.PARAMETER JoinPassword
    A fixed join password (8 to 32 printable characters, no space, double quote, or semicolon)
    instead of a random one, for a tester who
    connects across sessions. With -ProfileName it is saved to <profile>\join-password.txt and
    reused by later runs of that profile that omit -JoinPassword. When omitted, the standing
    password (environment variable NIMBY_JOIN_PASSWORD, else scripts\testserver.local.json) is
    used before falling back to a random value. Delete join-password.txt to let a profile pick up
    the standing value.

.PARAMETER ShutdownTimeoutSeconds
    How long to wait for the server to exit after the RCON #shutdown before stopping the process.

.PARAMETER Vanilla
    Boot with no -mod= at all: the same-build vanilla control the release gate compares against
    (shared rules 9.2). No build is selected, no key is installed, and -BuildPath, -ServerModPath,
    and -DevPath are refused.

.PARAMETER Force
    Run a -BuildPath build that did not record SUMMARY PASS, or whose PBOs no longer match the
    sha256 values in its build log. The manifest records BUILD FORCED and each HASH MISMATCH.

.PARAMETER InstanceId
    Override pack.json testServer.instanceId for this run. On the local replica built by
    Sync-LiveReplica.ps1, 998 is the copy of the live world and 999 is the fresh world. Only 998
    and 999 are accepted (shared rules, section 8).

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Start-NimbyTestServer.ps1

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Start-NimbyTestServer.ps1 -ProfileName legs-phase1
#>
[CmdletBinding()]
param(
    [string]$RepoRoot,
    [Alias('ModPath')][string]$BuildPath,
    [string]$ServerModPath,
    [string]$DevPath,
    [string]$ServerPath,
    [ValidatePattern('^[A-Za-z0-9][A-Za-z0-9_.-]*$')][string]$ProfileName,
    [ValidatePattern('^[\x21\x23-\x3a\x3c-\x7e]{8,32}$')][string]$JoinPassword,
    [ValidateRange(5, 600)][int]$ShutdownTimeoutSeconds = 60,
    [ValidateSet(998, 999)][int]$InstanceId,
    [switch]$Vanilla,
    [switch]$Force
)

Set-StrictMode -Version Latest
# Windows PowerShell 5.1 leaves $PSScriptRoot empty in a param() default when an advanced
# script is started with -File, so the default is resolved here.
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }
$ErrorActionPreference = 'Stop'

# Write-Manifest, Test-TestInstanceId, Get-ServerArgument, Read-ServerLogText, the running-server
# guard, the random-string generator, and the local settings file come from NimbyCommon.psm1.
Import-Module (Join-Path $PSScriptRoot 'modules\NimbyCommon.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'modules\NimbySteam.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'modules\NimbyRcon.psm1') -Force

function Read-BuildLog {
    # Parses one build.log written by Build-NimbyPack.ps1. Returns the verdict, the outputs it
    # names (a line reading "SERVERMOD none ..." or "DEV none ..." yields $null for that output),
    # and Hashes: "<label>|<name>.pbo" to the sha256 of each "ADDON OK" line.
    param([Parameter(Mandatory)][string]$Path)

    $result = [PSCustomObject]@{ Path = $Path; Verdict = 'MISSING'; ModPath = $null; ServerModPath = $null; DevPath = $null; Hashes = @{} }
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $result }
    $result.Verdict = 'INCOMPLETE'
    foreach ($line in (Get-Content -LiteralPath $Path -Encoding utf8)) {
        $addon = [regex]::Match($line, '^ADDON OK (?<pbo>\S+) \[(?<label>[^\]]+)\] .*\bsha256=(?<h>[0-9A-Fa-f]{64})\s*$')
        if ($addon.Success) { $result.Hashes[("{0}|{1}.pbo" -f $addon.Groups['label'].Value, $addon.Groups['pbo'].Value)] = $addon.Groups['h'].Value.ToUpperInvariant(); continue }
        $summary = [regex]::Match($line, '^SUMMARY (?<v>PASS|FAIL|UNKNOWN) mod=(?<p>.+)$')
        if ($summary.Success) { $result.Verdict = $summary.Groups['v'].Value; $result.ModPath = $summary.Groups['p'].Value.Trim(); continue }
        $server = [regex]::Match($line, '^SERVERMOD (?<p>.+)$')
        if ($server.Success -and $server.Groups['p'].Value -notmatch '^none\b') { $result.ServerModPath = $server.Groups['p'].Value.Trim(); continue }
        $dev = [regex]::Match($line, '^DEV (?<p>.+)$')
        if ($dev.Success -and $dev.Groups['p'].Value -notmatch '^none\b') { $result.DevPath = $dev.Groups['p'].Value.Trim(); continue }
    }
    return $result
}

function Test-ModFolder {
    # True when the folder holds at least one addons\*.pbo.
    param([AllowNull()][AllowEmptyString()][string]$Path)

    if (-not $Path) { return $false }
    $addons = Join-Path $Path 'addons'
    return ((Test-Path -LiteralPath $addons) -and @(Get-ChildItem -LiteralPath $addons -Filter '*.pbo').Count -gt 0)
}

function Get-BuildLogForBuild {
    # Maps build\<modFolderName>-<stamp> to logs\build-<stamp>\build.log, or $null when the
    # folder name has no stamp (for example a hand-made folder passed with -BuildPath).
    param(
        [Parameter(Mandatory)][string]$BuildPath,
        [Parameter(Mandatory)][string]$LogRoot,
        [Parameter(Mandatory)][string]$ModFolderName
    )

    $leaf = Split-Path -Leaf $BuildPath
    $match = [regex]::Match($leaf, ('^{0}-(?<stamp>\d{{8}}-\d{{6}})$' -f [regex]::Escape($ModFolderName)))
    if (-not $match.Success) { return $null }
    return (Join-Path $LogRoot ("build-{0}\build.log" -f $match.Groups['stamp'].Value))
}

function Get-QualifiedModBuild {
    # Returns the newest build whose build.log records SUMMARY PASS and whose mod folder still
    # holds PBOs, with its companions; or $null. Never selects on folder name alone.
    param(
        [Parameter(Mandatory)][string]$LogRoot,
        [Parameter(Mandatory)][string]$ModFolderName
    )

    if (-not (Test-Path -LiteralPath $LogRoot)) { return $null }
    $logs = Get-ChildItem -LiteralPath $LogRoot -Directory | Where-Object { $_.Name -match '^build-\d{8}-\d{6}$' } | Sort-Object Name -Descending
    foreach ($dir in $logs) {
        $build = Read-BuildLog -Path (Join-Path $dir.FullName 'build.log')
        if ($build.Verdict -ne 'PASS') { continue }
        if ((Split-Path -Leaf $build.ModPath) -notlike "$ModFolderName-*") { continue }
        if (-not (Test-ModFolder -Path $build.ModPath)) { continue }
        return $build
    }
    return $null
}

function Get-RandomJoinPassword {
    # Random 12-character join password from an unambiguous alphabet.
    param([int]$Length = 12)
    return (Get-NimbyRandomString -Alphabet 'abcdefghjkmnpqrstuvwxyz23456789' -Length $Length)
}

function Get-StandingJoinPassword {
    # The standing test-server join password, never a literal in this file: NIMBY_JOIN_PASSWORD,
    # else "joinPassword" in <repo>\scripts\testserver.local.json (git-ignored by *.local.json).
    # Returns Password and Source, or $null when neither is set. An invalid value or an unreadable
    # file is reported (never the value) and skipped so the next source is tried.
    param([Parameter(Mandatory)][string]$RepoRoot)

    $pattern = '^[\x21\x23-\x3a\x3c-\x7e]{8,32}$'
    $fromEnv = [System.Environment]::GetEnvironmentVariable('NIMBY_JOIN_PASSWORD')
    if ($fromEnv) {
        if ($fromEnv -match $pattern) { return [pscustomobject]@{ Password = $fromEnv; Source = 'env' } }
        Write-Warning 'Ignoring NIMBY_JOIN_PASSWORD: not 8 to 32 printable characters without space, double quote, or semicolon.'
    }
    $fromFile = Get-NimbyLocalSetting -RepoRoot $RepoRoot -Name 'joinPassword'
    if ($null -ne $fromFile) {
        $value = [string]$fromFile
        if ($value -match $pattern) { return [pscustomobject]@{ Password = $value; Source = 'local-file' } }
        Write-Warning ("Ignoring joinPassword in {0}: not 8 to 32 printable characters without space, double quote, or semicolon." -f (Get-NimbyLocalSettingPath -RepoRoot $RepoRoot))
    }
    return $null
}

function Resolve-JoinPassword {
    # The join password for this run and where it came from: -JoinPassword (saved into a reusable
    # profile), else the value saved by an earlier run of that profile, else the standing value
    # (environment variable, then scripts\testserver.local.json; never saved), else random (saved).
    [Diagnostics.CodeAnalysis.SuppressMessageAttribute('PSAvoidUsingPlainTextForPassword', 'Requested', Justification = 'serverDZ.cfg requires plaintext; local test value.')]
    param(
        [string]$Requested,
        [Parameter(Mandatory)][string]$ProfilePath,
        [bool]$UseProfile,
        [AllowNull()][object]$Standing
    )

    $pattern = '^[\x21\x23-\x3a\x3c-\x7e]{8,32}$'
    $file = Join-Path $ProfilePath 'join-password.txt'
    if ($Requested) {
        if ($UseProfile) { Set-Content -LiteralPath $file -Value $Requested -Encoding ascii -NoNewline }
        return [pscustomobject]@{ Password = $Requested; Source = 'parameter' }
    }
    if ($UseProfile -and (Test-Path -LiteralPath $file)) {
        # Get-Content -Raw returns $null for an empty file; .Trim() on $null fails under strict mode.
        $raw = Get-Content -LiteralPath $file -Raw
        $saved = ''
        if ($null -ne $raw) { $saved = $raw.Trim() }
        if ($saved -match $pattern) { return [pscustomobject]@{ Password = $saved; Source = 'profile' } }
        Write-Warning ("Ignoring {0}: not 8 to 32 printable characters without space, double quote, or semicolon." -f $file)
    }
    if ($null -ne $Standing) { return $Standing }
    $random = Get-RandomJoinPassword
    if ($UseProfile) { Set-Content -LiteralPath $file -Value $random -Encoding ascii -NoNewline }
    return [pscustomobject]@{ Password = $random; Source = 'random' }
}

function Get-TestServerConfigLine {
    # Returns the lines of the throwaway server config. serverDZ.cfg only accepts a plaintext
    # password, so the analyzer's SecureString rule cannot apply here.
    # BattlEye = 1 so RCON #shutdown works; storageAutoFix = 0 so a qualification run shows
    # persistence corruption instead of repairing it (shared rules, section 8).
    [Diagnostics.CodeAnalysis.SuppressMessageAttribute('PSAvoidUsingPlainTextForPassword', 'Password', Justification = 'serverDZ.cfg requires plaintext; throwaway local test value.')]
    param(
        [Parameter(Mandatory)][string]$Hostname,
        [Parameter(Mandatory)][string]$Password,
        [Parameter(Mandatory)][int]$MaxPlayers,
        [Parameter(Mandatory)][int]$InstanceId,
        [Parameter(Mandatory)][string]$Mission
    )

    if (-not (Test-TestInstanceId -InstanceId $InstanceId)) { throw "Refusing to write a test config for instanceId $InstanceId; only 998 and 999 are test instances." }
    return @(
        '// Generated by Start-NimbyTestServer.ps1 for a local in-game test. Not a real server config.',
        "hostname = `"$Hostname`";",
        "password = `"$Password`";",
        'passwordAdmin = "";',
        "maxPlayers = $MaxPlayers;",
        'verifySignatures = 2;',
        'forceSameBuild = 1;',
        'disable3rdPerson = 1;',
        'BattlEye = 1;',
        "instanceId = $InstanceId;",
        'storageAutoFix = 0;',
        'class Missions',
        '{',
        '    class DayZ',
        '    {',
        "        template = `"$Mission`";",
        '    };',
        '};'
    )
}

function Get-AddonHashLine {
    # One "ADDON <label> <pbo> sha256=<hash>" line per PBO under <path>\addons, for the manifest.
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Label
    )

    $lines = @()
    $addons = Join-Path $Path 'addons'
    if (-not (Test-Path -LiteralPath $addons)) { return $lines }
    foreach ($pbo in Get-ChildItem -LiteralPath $addons -File -Filter '*.pbo' | Sort-Object Name) {
        $lines += ("ADDON {0} {1} sha256={2}" -f $Label, $pbo.Name, (Get-FileHash -LiteralPath $pbo.FullName -Algorithm SHA256).Hash)
    }
    return $lines
}

function Get-BuildHashMismatch {
    # Compares the PBOs now under <Path>\addons with the sha256 values the build log recorded for
    # Label (from Read-BuildLog). Returns one line per PBO that changed, went missing, or was
    # added since the build; empty when every PBO matches.
    param(
        [Parameter(Mandatory)][object]$Build,
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Label
    )

    $expected = @{}
    foreach ($key in $Build.Hashes.Keys) {
        if ($key.StartsWith("$Label|", [System.StringComparison]::OrdinalIgnoreCase)) { $expected[$key.Substring($Label.Length + 1)] = $Build.Hashes[$key] }
    }
    $actual = @{}
    foreach ($line in (Get-AddonHashLine -Path $Path -Label $Label)) {
        $match = [regex]::Match($line, '^ADDON \S+ (?<name>\S+) sha256=(?<h>[0-9A-F]{64})$')
        if ($match.Success) { $actual[$match.Groups['name'].Value] = $match.Groups['h'].Value }
    }

    $problems = @()
    foreach ($name in @($expected.Keys | Sort-Object)) {
        if (-not $actual.ContainsKey($name)) { $problems += "$Label $name missing (build log sha256=$($expected[$name]))" }
        elseif ($actual[$name] -ne $expected[$name]) { $problems += "$Label $name sha256=$($actual[$name]) build log sha256=$($expected[$name])" }
    }
    foreach ($name in @($actual.Keys | Sort-Object)) {
        if (-not $expected.ContainsKey($name)) { $problems += "$Label $name not in the build log" }
    }
    return $problems
}

function Install-ServerKey {
    # Copies every .bikey from <modPath>\keys into <server>\keys. Refuses to overwrite a different
    # key of the same name. Returns one manifest line per key.
    param(
        [Parameter(Mandatory)][string]$ModPath,
        [Parameter(Mandatory)][string]$ServerKeys
    )

    $lines = @()
    $keysDir = Join-Path $ModPath 'keys'
    if (-not (Test-Path -LiteralPath $keysDir)) { throw "No keys folder in $ModPath." }
    $keys = @(Get-ChildItem -LiteralPath $keysDir -File -Filter '*.bikey')
    if ($keys.Count -eq 0) { throw "No .bikey in $keysDir." }
    foreach ($key in $keys) {
        $installed = Join-Path $ServerKeys $key.Name
        if (Test-Path -LiteralPath $installed) {
            if ((Get-FileHash -LiteralPath $installed).Hash -ne (Get-FileHash -LiteralPath $key.FullName).Hash) {
                throw "A different $($key.Name) is already in $ServerKeys. Not overwriting; resolve it by hand."
            }
            $lines += "KEY present $installed"
        }
        else {
            Copy-Item -LiteralPath $key.FullName -Destination $ServerKeys
            $lines += "KEY installed $installed"
        }
    }
    return $lines
}

function Get-NimbyEventSummary {
    # Extracts [NIMBY] lines from log text and counts them by component and result.
    param([Parameter(Mandatory)][AllowEmptyString()][string]$LogText)

    $lines = @(($LogText -split "\r?\n") | Where-Object { $_ -match '\[NIMBY\]' })
    $counts = [ordered]@{}
    foreach ($line in $lines) {
        $component = 'other'
        $result = 'n/a'
        $componentMatch = [regex]::Match($line, '\[NIMBY\]\[(?<c>[^\]]+)\]')
        if ($componentMatch.Success) { $component = $componentMatch.Groups['c'].Value }
        $resultMatch = [regex]::Match($line, '\bresult=(?<r>\S+)')
        if ($resultMatch.Success) { $result = $resultMatch.Groups['r'].Value }
        elseif ($line -match 'Rejected') { $result = 'REJECTED' }
        $key = "$component $result"
        if ($counts.Contains($key)) { $counts[$key]++ } else { $counts[$key] = 1 }
    }
    return [PSCustomObject]@{ Lines = $lines; Counts = $counts }
}

function Invoke-TestServerShutdown {
    # Graceful stop: RCON #shutdown, wait, then and only then a process stop. Returns manifest lines.
    [Diagnostics.CodeAnalysis.SuppressMessageAttribute('PSAvoidUsingPlainTextForPassword', 'RconPassword', Justification = 'BERCon transports the password as ASCII; throwaway local test value.')]
    param(
        [Parameter(Mandatory)][System.Diagnostics.Process]$Process,
        [Parameter(Mandatory)][int]$RconPort,
        [Parameter(Mandatory)][string]$RconPassword,
        [Parameter(Mandatory)][int]$TimeoutSeconds
    )

    $lines = @()
    if ($Process.HasExited) { return @("SERVER EXITED before stop code=$($Process.ExitCode)") }

    $rcon = Send-NimbyRconCommand -Port $RconPort -Password $RconPassword -Command '#shutdown' -TimeoutSeconds 5
    $lines += ("RCON shutdown loggedIn={0} sent={1} reason='{2}'" -f $rcon.LoggedIn, $rcon.Sent, $rcon.Reason)
    if ($rcon.Sent) {
        if ($Process.WaitForExit($TimeoutSeconds * 1000)) {
            $lines += "SERVER STOPPED gracefully pid=$($Process.Id) code=$($Process.ExitCode)"
            return $lines
        }
        $lines += "SERVER still running ${TimeoutSeconds}s after #shutdown"
    }
    Stop-Process -Id $Process.Id -Force
    # BattlEye holds BEServer_x64_active_*.cfg until the process is gone; the redaction follows.
    [void]$Process.WaitForExit(5000)
    $lines += "SERVER KILLED pid=$($Process.Id) (graceful stop did not complete; treat this run's storage as suspect)"
    return $lines
}

function Wait-TestServerStopKey {
    # Waits for Enter or Ctrl+C, or for the server to exit by itself, and returns which one. Ctrl+C
    # is read as a key, so PowerShell does not stop the pipeline and the caller's finally block
    # still runs every cmdlet: the RCON shutdown, Stop-Process, the redaction, and the log lines.
    # Without a console (redirected input or a non-console host) it falls back to Read-Host.
    param([Parameter(Mandatory)][System.Diagnostics.Process]$Process)

    if ([Console]::IsInputRedirected -or $Host.Name -ne 'ConsoleHost') {
        [void](Read-Host)
        return 'input'
    }
    $previous = [Console]::TreatControlCAsInput
    [Console]::TreatControlCAsInput = $true
    try {
        while ($true) {
            if ($Process.HasExited) { return 'server-exited' }
            while ([Console]::KeyAvailable) {
                $key = [Console]::ReadKey($true)
                if ($key.Key -eq [ConsoleKey]::Enter) { return 'enter' }
                if ($key.Key -eq [ConsoleKey]::C -and ($key.Modifiers -band [ConsoleModifiers]::Control)) { return 'ctrl-c' }
            }
            Start-Sleep -Milliseconds 250
        }
    }
    finally {
        [Console]::TreatControlCAsInput = $previous
    }
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$runDir = Join-Path $RepoRoot ("logs\testserver-{0}" -f $stamp)
New-Item -ItemType Directory -Path $runDir -Force | Out-Null
$manifest = Join-Path $runDir 'testserver.log'
$process = $null
$config = $null
$profileDir = $null
$rconPassword = $null
$rconPort = 0
$runStart = Get-Date

try {
    Write-Manifest -LogPath $manifest -Line ("START {0} PS={1}" -f (Get-Date -Format o), $PSVersionTable.PSVersion)
    $pack = Get-Content -LiteralPath (Join-Path $RepoRoot 'mods\own\nimby-pack\pack.json') -Raw -Encoding utf8 | ConvertFrom-Json
    $test = $pack.testServer
    if ($null -eq $test) { throw 'pack.json has no testServer section.' }
    if ($PSBoundParameters.ContainsKey('InstanceId')) {
        # Per-run world selection on the replica: 998 live copy, 999 fresh (Sync-LiveReplica.ps1).
        $test.instanceId = $InstanceId
        Write-Manifest -LogPath $manifest -Line ("INSTANCE override={0}" -f $InstanceId)
    }
    if (-not (Test-TestInstanceId -InstanceId ([int]$test.instanceId))) { throw "testServer.instanceId $($test.instanceId) is not a test instance; only 998 and 999 are allowed so storage_1 is never written." }

    # Build selection ---------------------------------------------------------
    $logRoot = Join-Path $RepoRoot 'logs'
    $build = $null
    if ($Vanilla) {
        if ($BuildPath -or $ServerModPath -or $DevPath) { throw '-Vanilla boots with no mod; drop -BuildPath, -ServerModPath, and -DevPath.' }
        Write-Manifest -LogPath $manifest -Line 'BUILD none (vanilla control boot)'
    }
    elseif ($BuildPath) {
        if (-not (Test-ModFolder -Path $BuildPath)) { throw "No addons\*.pbo under -BuildPath '$BuildPath'." }
        $buildLogPath = Get-BuildLogForBuild -BuildPath $BuildPath -LogRoot $logRoot -ModFolderName $pack.modFolderName
        if ($buildLogPath) { $build = Read-BuildLog -Path $buildLogPath }
        $explicitVerdict = 'UNKNOWN'
        if ($null -ne $build) {
            $explicitVerdict = $build.Verdict
            Write-Manifest -LogPath $manifest -Line ("BUILD explicit verdict={0} log={1}" -f $build.Verdict, $build.Path)
        }
        else { Write-Manifest -LogPath $manifest -Line 'BUILD explicit verdict=UNKNOWN (no build.log for this folder)' }
        if ($explicitVerdict -ne 'PASS') {
            if (-not $Force) { throw "-BuildPath '$BuildPath' recorded $explicitVerdict, not SUMMARY PASS. Pass -Force to run it anyway." }
            Write-Manifest -LogPath $manifest -Line "BUILD FORCED verdict=$explicitVerdict by -Force"
        }
        if (-not $ServerModPath -and $null -ne $build -and $build.ServerModPath) { $ServerModPath = $build.ServerModPath }
        if (-not $DevPath -and $null -ne $build -and $build.DevPath) { $DevPath = $build.DevPath }
    }
    else {
        $build = Get-QualifiedModBuild -LogRoot $logRoot -ModFolderName $pack.modFolderName
        if ($null -eq $build) { throw 'No build with SUMMARY PASS in logs\build-*\build.log. Run Build-NimbyPack.ps1 to a PASS, or pass -BuildPath explicitly.' }
        $BuildPath = $build.ModPath
        if (-not $ServerModPath -and $build.ServerModPath) { $ServerModPath = $build.ServerModPath }
        if (-not $DevPath -and $build.DevPath) { $DevPath = $build.DevPath }
        Write-Manifest -LogPath $manifest -Line ("BUILD selected verdict=PASS log={0}" -f $build.Path)
    }
    if ($ServerModPath -and -not (Test-ModFolder -Path $ServerModPath)) { throw "No addons\*.pbo under -ServerModPath '$ServerModPath'." }
    if ($DevPath -and -not (Test-ModFolder -Path $DevPath)) { throw "No addons\*.pbo under -DevPath '$DevPath'." }
    if ($Vanilla) { Write-Manifest -LogPath $manifest -Line 'MOD none' } else { Write-Manifest -LogPath $manifest -Line "MOD $BuildPath" }
    if ($ServerModPath) { Write-Manifest -LogPath $manifest -Line "SERVERMOD $ServerModPath" } else { Write-Manifest -LogPath $manifest -Line 'SERVERMOD none' }
    if ($DevPath) { Write-Manifest -LogPath $manifest -Line "DEV $DevPath" } else { Write-Manifest -LogPath $manifest -Line 'DEV none' }
    if (-not $Vanilla) { foreach ($line in (Get-AddonHashLine -Path $BuildPath -Label 'client')) { Write-Manifest -LogPath $manifest -Line $line } }
    if ($ServerModPath) { foreach ($line in (Get-AddonHashLine -Path $ServerModPath -Label 'server')) { Write-Manifest -LogPath $manifest -Line $line } }
    if ($DevPath) { foreach ($line in (Get-AddonHashLine -Path $DevPath -Label 'dev')) { Write-Manifest -LogPath $manifest -Line $line } }

    # The PBOs must still be the ones the build log verified.
    if ($null -ne $build -and $build.Verdict -ne 'MISSING') {
        $mismatch = @(Get-BuildHashMismatch -Build $build -Path $BuildPath -Label 'client')
        if ($ServerModPath) { $mismatch += @(Get-BuildHashMismatch -Build $build -Path $ServerModPath -Label 'server') }
        if ($DevPath) { $mismatch += @(Get-BuildHashMismatch -Build $build -Path $DevPath -Label 'dev') }
        foreach ($line in $mismatch) { Write-Manifest -LogPath $manifest -Line "HASH MISMATCH $line" }
        if ($mismatch.Count -eq 0) { Write-Manifest -LogPath $manifest -Line 'HASH OK every PBO matches its build log sha256' }
        elseif (-not $Force) { throw ("HASH MISMATCH: {0} PBO(s) differ from {1} (first: {2}). Rebuild, or pass -Force to run it anyway." -f $mismatch.Count, $build.Path, $mismatch[0]) }
        else { Write-Manifest -LogPath $manifest -Line 'HASH MISMATCH accepted by -Force' }
    }
    elseif (-not $Vanilla) { Write-Manifest -LogPath $manifest -Line 'HASH unchecked (no build log for this folder)' }

    # Server install ----------------------------------------------------------
    if (-not $ServerPath) {
        $steamCandidates = @()
        if (${env:ProgramFiles(x86)}) { $steamCandidates += (Join-Path ${env:ProgramFiles(x86)} 'Steam') }
        $steamRoot = Get-SteamRootPath -Candidates $steamCandidates
        $libraries = @()
        if ($null -ne $steamRoot) { $libraries = @(Get-SteamLibraryPath -SteamRoot $steamRoot) }
        $ServerPath = Find-DayZServerPath -LibraryPaths $libraries
    }
    if (-not $ServerPath) { throw 'No DayZ Server install found; pass -ServerPath.' }
    Write-Manifest -LogPath $manifest -Line "SERVER $ServerPath"
    # Checked before any key is installed or any password is written.
    $runningServer = Get-RunningServerMessage
    if ($runningServer) { throw $runningServer }
    if (-not (Test-Path -LiteralPath (Join-Path $ServerPath ("mpmissions\{0}" -f $test.mission)))) { throw "Mission '$($test.mission)' not found in $ServerPath\mpmissions." }
    Write-Manifest -LogPath $manifest -Line ("STORAGE instanceId={0} path={1}" -f $test.instanceId, (Join-Path $ServerPath ("mpmissions\{0}\storage_{1}" -f $test.mission, $test.instanceId)))

    # Keys so clients loading the pack pass signature verification.
    $serverKeys = Join-Path $ServerPath 'keys'
    if (-not $Vanilla) { foreach ($line in (Install-ServerKey -ModPath $BuildPath -ServerKeys $serverKeys)) { Write-Manifest -LogPath $manifest -Line $line } }
    if ($DevPath) { foreach ($line in (Install-ServerKey -ModPath $DevPath -ServerKeys $serverKeys)) { Write-Manifest -LogPath $manifest -Line $line } }

    # Profile -----------------------------------------------------------------
    if ($ProfileName) {
        $profileDir = Join-Path $RepoRoot ("profiles\{0}" -f $ProfileName)
        Write-Manifest -LogPath $manifest -Line "PROFILE reused $profileDir"
    }
    else {
        $profileDir = Join-Path $runDir 'server-profile'
        Write-Manifest -LogPath $manifest -Line "PROFILE fresh $profileDir"
    }
    New-Item -ItemType Directory -Path $profileDir -Force | Out-Null

    # Configs -----------------------------------------------------------------
    $join = Resolve-JoinPassword -Requested $JoinPassword -ProfilePath $profileDir -UseProfile ([bool]$ProfileName) -Standing (Get-StandingJoinPassword -RepoRoot $RepoRoot)
    $password = $join.Password
    Write-Manifest -LogPath $manifest -Line ("JOIN PASSWORD source={0}" -f $join.Source)
    $config = Join-Path $runDir 'serverDZ.test.cfg'
    Set-Content -LiteralPath $config -Value (Get-TestServerConfigLine -Hostname $test.hostname -Password $password -MaxPlayers ([int]$test.maxPlayers) -InstanceId ([int]$test.instanceId) -Mission $test.mission) -Encoding ascii

    $rconPassword = Get-NimbyRconPassword
    $rconPort = [int]$test.port + 3
    foreach ($moved in (Move-NimbyStaleBeServerConfig -ProfilePath $profileDir -Stamp $stamp)) { Write-Manifest -LogPath $manifest -Line "BE STALE renamed $moved" }
    $beConfig = Write-NimbyBeServerConfig -ProfilePath $profileDir -Password $rconPassword -Port $rconPort
    Write-Manifest -LogPath $manifest -Line "BE CONFIG $beConfig rconPort=$rconPort"

    # Start -------------------------------------------------------------------
    $modPaths = @()
    if (-not $Vanilla) { $modPaths += $BuildPath }
    if ($DevPath) { $modPaths += $DevPath }
    $serverArgs = Get-ServerArgument -ConfigPath $config -ProfilePath $profileDir -ModPath $modPaths -ServerModPath $ServerModPath -Port ([int]$test.port)
    Write-Manifest -LogPath $manifest -Line ("SERVER START {0}" -f ($serverArgs -join ' '))
    $quoted = $serverArgs | ForEach-Object { '"{0}"' -f $_ }
    $runStart = Get-Date
    $process = Start-Process -FilePath (Join-Path $ServerPath 'DayZServer_x64.exe') -ArgumentList $quoted -WorkingDirectory $ServerPath -PassThru -WindowStyle Minimized

    Write-Information -MessageData ("Server starting on port {0}. Join password: {1}" -f $test.port, $password) -InformationAction Continue
    if ($Vanilla) { Write-Information -MessageData 'Client mod folder: none (vanilla)' -InformationAction Continue } else { Write-Information -MessageData ("Client mod folder: {0}" -f $BuildPath) -InformationAction Continue }
    if ($DevPath) { Write-Information -MessageData ("Dev harness folder (load beside the pack): {0}" -f $DevPath) -InformationAction Continue }
    Write-Information -MessageData 'Run the test, then press Enter (or Ctrl+C) here to stop the server (RCON #shutdown) and summarize.' -InformationAction Continue
    $stopReason = Wait-TestServerStopKey -Process $process
    Write-Manifest -LogPath $manifest -Line "STOP requested by $stopReason"
}
catch {
    Write-Manifest -LogPath $manifest -Line ("FAIL {0}" -f $_.Exception.Message)
    Write-Information -MessageData ("FAIL: {0} Log: {1}" -f $_.Exception.Message, $manifest) -InformationAction Continue
}
finally {
    if ($null -ne $process) {
        foreach ($line in (Invoke-TestServerShutdown -Process $process -RconPort $rconPort -RconPassword $rconPassword -TimeoutSeconds $ShutdownTimeoutSeconds)) {
            Write-Manifest -LogPath $manifest -Line $line
        }
    }
    if ($null -ne $config -and (Test-Path -LiteralPath $config)) {
        $redacted = (Get-Content -LiteralPath $config -Encoding utf8) -replace '^password = ".*";$', 'password = "REDACTED after test";'
        Set-Content -LiteralPath $config -Value $redacted -Encoding ascii
        Write-Manifest -LogPath $manifest -Line 'CONFIG password redacted'
    }
    if ($null -ne $profileDir) {
        foreach ($cleared in (Clear-NimbyBeServerConfig -ProfilePath $profileDir)) { Write-Manifest -LogPath $manifest -Line "BE CONFIG redacted $cleared" }
    }
}

if ($null -ne $process) {
    Start-Sleep -Seconds 2
    $summary = Get-NimbyEventSummary -LogText (Read-ServerLogText -ProfilePath $profileDir -Since $runStart)
    Set-Content -LiteralPath (Join-Path $runDir 'nimby-events.log') -Value $summary.Lines -Encoding utf8
    foreach ($key in $summary.Counts.Keys) {
        Write-Manifest -LogPath $manifest -Line ("COUNT {0} = {1}" -f $key, $summary.Counts[$key])
    }
    Write-Manifest -LogPath $manifest -Line ("LOGS {0}" -f $profileDir)
    Write-Manifest -LogPath $manifest -Line ("SUMMARY events={0}" -f $summary.Lines.Count)
    Write-Information -MessageData ("DONE: {0} [NIMBY] events. Log: {1}" -f $summary.Lines.Count, $manifest) -InformationAction Continue
}
