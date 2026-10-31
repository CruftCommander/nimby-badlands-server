#Requires -Version 5.1

<#
.SYNOPSIS
    Reads one test-server run (a logs\testserver-<stamp> folder) or one boot check and reports what
    the server logs say: every Hands transfer as a state chain with timings, the UNCONFIRMED count,
    script errors, corrupted-variable lines, crash files, and the self-test lines.

.DESCRIPTION
    The October 28, 2026 Hands run was read by hand across the RPT, the script log, three crash
    logs, and nimby-events.log (which carries every line twice, once from each log). This script is
    that reading, done the same way every time (Jeff, October 28, 2026). For the run it:
      1. Reads testserver.log (START time, BUILD, MOD, how the server stopped, LOGS profile folder),
         or build.log plus server-profile for a boot check.
      2. Reads the script log(s) written at or after START from the profile folder (the RPT only
         when no script log exists; the RPT repeats every script line). An identical line counts
         once. When the profile folder is gone, nimby-events.log is read instead, deduplicated.
      3. Builds one chain per Hands correlation id: action, item, each lifecycle state with its time
         and the milliseconds since the previous state, the terminal detail, and for an id with no
         terminal state the last debug line it wrote (which is where a crash left it).
      4. Counts CONFIRMED, REFUSED, UNCONFIRMED and open ids; scans for the boot check's failure
         patterns (pack.json bootCheck.failurePatterns: SCRIPT (E), Scripted variables corrupted,
         SelfTest FAIL, and the rest); lists the "selftest complete" lines; lists every crash_*.log,
         error_*.log with an (E) line, and *.mdmp written since START, with the crash log's reason,
         function, and script stack.
      5. Writes the verdict: PASS when the server stopped on request, no failure pattern hit, no
         crash file, and no Hands id ended UNCONFIRMED or open; FAIL otherwise, with the reasons.

    Reads   : the run folder's testserver.log (or build.log), nimby-events.log, the profile folder
              named by its LOGS line (script_*.log, *.RPT, crash_*.log, error_*.log, *.mdmp names),
              and mods\own\nimby-pack\pack.json for the failure patterns. All read-only.
    Writes  : <run folder>\evaluation.log (overwritten on each run).
    Never   : starts or stops a server, changes a log, or contacts a remote host.

    Exit codes: 0 = PASS, 1 = FAIL, 2 = UNKNOWN (no run folder, no manifest, or no server log to read).

    Runs in Windows PowerShell 5.1 and PowerShell 7.

.PARAMETER RunPath
    A logs\testserver-<stamp> folder (Start-NimbyTestServer.ps1) or a logs\build-<stamp> folder
    (Build-NimbyPack.ps1, read through its server-profile). Defaults to the newest testserver-*
    folder under <repo>\logs.

.PARAMETER ProfilePath
    Read this server profile folder instead of the one the manifest names. With -Since, only logs
    written at or after that time.

.PARAMETER Since
    Only log files written at or after this time. Defaults to the manifest's START time for a
    test-server run; a boot check's own profile folder needs no cut-off.

.PARAMETER RepoRoot
    Repository root. Defaults to the parent of this script's folder.

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Read-NimbyTestRun.ps1

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Read-NimbyTestRun.ps1 -RunPath .\logs\testserver-20261028-223924

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Read-NimbyTestRun.ps1 -RunPath .\logs\build-20261029-001122
#>
[CmdletBinding()]
param(
    [string]$RunPath,
    [string]$ProfilePath,
    [AllowNull()][Nullable[datetime]]$Since,
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }

# The terminal states of the registry (nimbyhands_state.c); PENDING is never logged.
$script:HandsTerminalStates = @('CONFIRMED', 'REFUSED', 'UNCONFIRMED')

function Get-TestRunManifest {
    # Reads a Start-NimbyTestServer.ps1 testserver.log or a Build-NimbyPack.ps1 build.log into one
    # object: Kind (testserver|build), Start, Build (the build log named), Mod, ProfileDir, Stop (the
    # STOP line), ExitCode (from "SERVER EXITED ... code=N" or "BOOT FAIL server exited ... code N";
    # $null when the server stopped on request), Summary and Lines. A missing value is $null or ''.
    param([Parameter(Mandatory)][string]$Path)

    $lines = @(Get-Content -LiteralPath $Path -Encoding UTF8)
    $kind = 'testserver'
    if ((Split-Path -Leaf $Path) -ieq 'build.log') { $kind = 'build' }
    $first = { param($pattern) $m = @($lines | Where-Object { $_ -match $pattern } | Select-Object -First 1); if ($m.Count -gt 0) { return $m[0] } return '' }
    $start = $null
    $startLine = & $first '^START\s+\S+'
    if ($startLine -match '^START\s+(?<t>\S+)') {
        try { $start = [datetime]::Parse($Matches['t'], [System.Globalization.CultureInfo]::InvariantCulture, [System.Globalization.DateTimeStyles]::RoundtripKind) } catch { $start = $null }
    }
    $build = ''
    if ((& $first '^BUILD\s') -match 'log=(?<p>.+)$') { $build = $Matches['p'].Trim() }
    $mod = ''
    if ((& $first '^MOD\s') -match '^MOD\s+(?<p>.+)$') { $mod = $Matches['p'].Trim() }
    $profileDir = ''
    if ((& $first '^LOGS\s') -match '^LOGS\s+(?<p>.+)$') { $profileDir = $Matches['p'].Trim() }
    $stop = & $first '^STOP\s'
    $exitCode = $null
    if ((& $first '^SERVER EXITED') -match 'code=(?<c>-?\d+)') { $exitCode = [long]$Matches['c'] }
    if ($null -eq $exitCode -and (& $first '^BOOT FAIL') -match 'exited.*code (?<c>-?\d+)') { $exitCode = [long]$Matches['c'] }
    $summary = & $first '^SUMMARY\s'
    return [PSCustomObject]@{
        Kind = $kind; Start = $start; Build = $build; Mod = $mod; ProfileDir = $profileDir
        Stop = $stop; ExitCode = $exitCode; Summary = $summary; Lines = $lines
    }
}

function Get-UniqueLogLine {
    # Splits log text into lines and drops blank lines and exact repeats (nimby-events.log holds the
    # RPT copy of every script line; the boot check counts an identical line once for that reason).
    param([Parameter(Mandatory)][AllowEmptyString()][string]$LogText)

    $seen = @{}
    $out = New-Object System.Collections.Generic.List[string]
    foreach ($line in ($LogText -split "\r?\n")) {
        $trimmed = $line.TrimEnd()
        if ($trimmed -eq '') { continue }
        if ($seen.ContainsKey($trimmed)) { continue }
        $seen[$trimmed] = $true
        $out.Add($trimmed)
    }
    return @($out)
}

function ConvertTo-LogTime {
    # The HH:mm:ss.fff at the start of a server log line as a TimeSpan, or $null.
    param([Parameter(Mandatory)][AllowEmptyString()][string]$Text)

    if ($Text -match '^(?<h>\d{1,2}):(?<m>\d{2}):(?<s>\d{2})\.(?<f>\d{1,3})') {
        $ms = [int]($Matches['f'].PadRight(3, '0'))
        return New-Object TimeSpan(0, [int]$Matches['h'], [int]$Matches['m'], [int]$Matches['s'], $ms)
    }
    return $null
}

function Get-LogDeltaMs {
    # Milliseconds from one log time to the next; a wrap past midnight adds a day.
    param([Parameter(Mandatory)][TimeSpan]$From, [Parameter(Mandatory)][TimeSpan]$To)

    $delta = $To - $From
    if ($delta.TotalMilliseconds -lt 0) { $delta = $delta + (New-Object TimeSpan(1, 0, 0, 0)) }
    return [int][Math]::Round($delta.TotalMilliseconds)
}

function Get-HandsTransferReport {
    # One object per Hands correlation id, in first-seen order: Id, Action (without the
    # NimbyHandsAction prefix), Item, Giver, Receiver, Steps (State, Time, DeltaMs since the previous
    # state, Detail), Terminal (the terminal state or ''), TerminalDetail, TotalMs (first lifecycle
    # line to the terminal line), and LastDebug (the id's last debug line; where a crash left it).
    param([Parameter(Mandatory)][AllowEmptyCollection()][string[]]$Lines)

    $byId = [ordered]@{}
    foreach ($line in $Lines) {
        $m = [regex]::Match($line, '^(?<t>\d{1,2}:\d{2}:\d{2}\.\d{1,3})\s+SCRIPT\s*:\s*\[NIMBY\]\[Hands\](?<dbg>\[Debug\])?\s+(?<rest>.*)$')
        if (-not $m.Success) { continue }
        $idMatch = [regex]::Match($m.Groups['rest'].Value, '^id=(?<id>\d+)\s+(?<text>.*)$')
        if (-not $idMatch.Success) { continue }
        $id = [int]$idMatch.Groups['id'].Value
        # String keys: an ordered dictionary indexed with an int reads by position, not by key.
        $key = $idMatch.Groups['id'].Value
        $time = ConvertTo-LogTime -Text $m.Groups['t'].Value
        if (-not $byId.Contains($key)) {
            $byId[$key] = [PSCustomObject]@{
                Id = $id; Action = ''; Item = ''; Giver = ''; Receiver = ''
                Steps = New-Object System.Collections.Generic.List[object]
                Terminal = ''; TerminalDetail = ''; TotalMs = $null; LastDebug = ''
            }
        }
        $entry = $byId[$key]
        if ($m.Groups['dbg'].Success) {
            $entry.LastDebug = $idMatch.Groups['text'].Value
            continue
        }
        $life = [regex]::Match($idMatch.Groups['text'].Value, '^action=(?<a>\S+)\s+state=(?<s>[A-Z]+)\s+giver=(?<g>.*?)\s+receiver=(?<r>.*?)\s+item=(?<i>.*?)\s+detail=(?<d>.*)$')
        if (-not $life.Success) { continue }
        $entry.Action = $life.Groups['a'].Value -replace '^NimbyHandsAction', ''
        $entry.Giver = $life.Groups['g'].Value
        $entry.Receiver = $life.Groups['r'].Value
        $entry.Item = $life.Groups['i'].Value
        $state = $life.Groups['s'].Value
        $delta = $null
        if ($entry.Steps.Count -gt 0 -and $null -ne $time -and $null -ne $entry.Steps[$entry.Steps.Count - 1].Time) {
            $delta = Get-LogDeltaMs -From $entry.Steps[$entry.Steps.Count - 1].Time -To $time
        }
        $entry.Steps.Add([PSCustomObject]@{ State = $state; Time = $time; DeltaMs = $delta; Detail = $life.Groups['d'].Value })
        if ($script:HandsTerminalStates -contains $state) {
            $entry.Terminal = $state
            $entry.TerminalDetail = $life.Groups['d'].Value
            if ($null -ne $time -and $null -ne $entry.Steps[0].Time) { $entry.TotalMs = Get-LogDeltaMs -From $entry.Steps[0].Time -To $time }
        }
    }
    return @($byId.Values)
}

function Format-HandsChain {
    # "REQUESTED -> STAGED (+382 ms) -> CONFIRMED (+51 ms)" for one report entry.
    param([Parameter(Mandatory)][object]$Entry)

    $parts = @()
    foreach ($step in $Entry.Steps) {
        if ($null -eq $step.DeltaMs) { $parts += $step.State } else { $parts += ('{0} (+{1} ms)' -f $step.State, $step.DeltaMs) }
    }
    if ($parts.Count -eq 0) { return '(no lifecycle line)' }
    return ($parts -join ' -> ')
}

function Get-HandsTotal {
    # Counts per terminal state plus the ids with no terminal state (Open), and those ids.
    param([Parameter(Mandatory)][AllowEmptyCollection()][object[]]$Report)

    $counts = [ordered]@{ CONFIRMED = 0; REFUSED = 0; UNCONFIRMED = 0; Open = 0 }
    $openIds = @()
    foreach ($entry in $Report) {
        if ($entry.Terminal -eq '') { $counts['Open']++; $openIds += $entry.Id }
        else { $counts[$entry.Terminal]++ }
    }
    return [PSCustomObject]@{ Counts = $counts; OpenIds = $openIds; Total = $Report.Count }
}

function Get-FailurePatternHit {
    # For each pattern (a regex, as pack.json bootCheck.failurePatterns writes them): Pattern,
    # Count, First (the first matching line). Only patterns with at least one hit are returned.
    param(
        [Parameter(Mandatory)][AllowEmptyCollection()][string[]]$Lines,
        [Parameter(Mandatory)][AllowEmptyCollection()][string[]]$Patterns
    )

    $hits = @()
    foreach ($pattern in $Patterns) {
        $matching = @($Lines | Where-Object { $_ -match $pattern })
        if ($matching.Count -gt 0) { $hits += [PSCustomObject]@{ Pattern = $pattern; Count = $matching.Count; First = $matching[0] } }
    }
    return $hits
}

function Get-SelfTestLine {
    # Component, Cases, Failures for each "[NIMBY][<c>] selftest complete; cases=N failures=M" line.
    param([Parameter(Mandatory)][AllowEmptyCollection()][string[]]$Lines)

    $out = @()
    foreach ($line in $Lines) {
        $m = [regex]::Match($line, '\[NIMBY\]\[(?<c>[^\]]+)\]\s+selftest complete;\s*cases=(?<n>\d+)\s+failures=(?<f>\d+)')
        if ($m.Success) { $out += [PSCustomObject]@{ Component = $m.Groups['c'].Value; Cases = [int]$m.Groups['n'].Value; Failures = [int]$m.Groups['f'].Value } }
    }
    return $out
}

function Read-CrashLog {
    # Reason, Function and the script stack frames of one DayZ crash_*.log.
    param([Parameter(Mandatory)][string]$Path)

    $text = [string](Get-Content -LiteralPath $Path -Raw -Encoding UTF8)
    $reason = ''
    if ($text -match '(?m)^Reason:\s*(?<r>.+)$') { $reason = $Matches['r'].Trim() }
    $function = ''
    if ($text -match "(?m)^Function:\s*'(?<f>[^']*)'") { $function = $Matches['f'] }
    $stack = @([regex]::Matches($text, '(?m)^(?<frame>\S+\.c:\d+\s+Function\s+\S+)\s*$') | ForEach-Object { $_.Groups['frame'].Value })
    return [PSCustomObject]@{ Path = $Path; Reason = $reason; Function = $function; Stack = $stack }
}

function Get-CrashReport {
    # Every crash_*.log (read), *.mdmp (named), and error_*.log holding an (E) line, written at or
    # after Since, in a profile folder, oldest first.
    param(
        [Parameter(Mandatory)][string]$ProfilePath,
        [AllowNull()][Nullable[datetime]]$Since = $null
    )

    $out = @()
    if (-not (Test-Path -LiteralPath $ProfilePath)) { return $out }
    $files = @(Get-ChildItem -LiteralPath $ProfilePath -File | Where-Object { $null -eq $Since -or $_.LastWriteTime -ge $Since })
    foreach ($file in ($files | Sort-Object LastWriteTime)) {
        if ($file.Name -like 'crash_*.log') {
            $crash = Read-CrashLog -Path $file.FullName
            $out += [PSCustomObject]@{ Kind = 'crash'; Name = $file.Name; Time = $file.LastWriteTime; Reason = $crash.Reason; Function = $crash.Function; Stack = $crash.Stack }
        }
        elseif ($file.Extension -ieq '.mdmp') {
            $out += [PSCustomObject]@{ Kind = 'minidump'; Name = $file.Name; Time = $file.LastWriteTime; Reason = ''; Function = ''; Stack = @() }
        }
        elseif ($file.Name -like 'error_*.log') {
            $errors = @(Get-Content -LiteralPath $file.FullName -Encoding UTF8 | Where-Object { $_ -match 'SCRIPT\s*\(E\)' })
            if ($errors.Count -gt 0) { $out += [PSCustomObject]@{ Kind = 'error'; Name = $file.Name; Time = $file.LastWriteTime; Reason = ('{0} (E) line(s); first: {1}' -f $errors.Count, $errors[0]); Function = ''; Stack = @() } }
        }
    }
    return $out
}

function Get-RunVerdict {
    # PASS or FAIL with the reasons: a server that exited on its own, a failure pattern hit, a crash
    # file (not an error log alone), or a Hands id that ended UNCONFIRMED or never ended.
    param(
        [Parameter(Mandatory)][object]$HandsTotal,
        [Parameter(Mandatory)][AllowEmptyCollection()][object[]]$Hits,
        [Parameter(Mandatory)][AllowEmptyCollection()][object[]]$Crashes,
        [AllowNull()][Nullable[long]]$ExitCode
    )

    $reasons = @()
    if ($null -ne $ExitCode) { $reasons += ('server exited on its own with code {0}' -f $ExitCode) }
    foreach ($hit in $Hits) { $reasons += ('{0} line(s) match "{1}"' -f $hit.Count, $hit.Pattern) }
    $crashCount = @($Crashes | Where-Object { $_.Kind -ne 'error' }).Count
    if ($crashCount -gt 0) { $reasons += ('{0} crash file(s)' -f $crashCount) }
    if ($HandsTotal.Counts['UNCONFIRMED'] -gt 0) { $reasons += ('{0} Hands transfer(s) UNCONFIRMED' -f $HandsTotal.Counts['UNCONFIRMED']) }
    if ($HandsTotal.Counts['Open'] -gt 0) { $reasons += ('Hands id(s) with no terminal state: {0}' -f ($HandsTotal.OpenIds -join ',')) }
    $verdict = 'PASS'
    if ($reasons.Count -gt 0) { $verdict = 'FAIL' }
    return [PSCustomObject]@{ Verdict = $verdict; Reasons = $reasons }
}

if ($MyInvocation.InvocationName -ne '.') {
    Import-Module (Join-Path $PSScriptRoot 'modules\NimbyCommon.psm1') -Force

    if (-not $RunPath) {
        $newest = @(Get-ChildItem -LiteralPath (Join-Path $RepoRoot 'logs') -Directory -Filter 'testserver-*' -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1)
        if ($newest.Count -eq 0) { Write-Host 'UNKNOWN: no logs\testserver-* folder; pass -RunPath'; exit 2 }
        $RunPath = $newest[0].FullName
    }
    $RunPath = (Resolve-Path -LiteralPath $RunPath).Path
    $manifestPath = Join-Path $RunPath 'testserver.log'
    if (-not (Test-Path -LiteralPath $manifestPath)) { $manifestPath = Join-Path $RunPath 'build.log' }
    if (-not (Test-Path -LiteralPath $manifestPath)) { Write-Host ("UNKNOWN: no testserver.log or build.log in {0}" -f $RunPath); exit 2 }

    $logFile = Join-Path $RunPath 'evaluation.log'
    Set-Content -LiteralPath $logFile -Value '' -Encoding UTF8
    function Write-Line { param([string]$Text) Write-Host $Text; Add-Content -LiteralPath $logFile -Value $Text -Encoding UTF8 }

    $manifest = Get-TestRunManifest -Path $manifestPath
    $startText = 'unknown'
    if ($null -ne $manifest.Start) { $startText = $manifest.Start.ToString('o') }
    Write-Line ("RUN {0} kind={1} start={2}" -f $RunPath, $manifest.Kind, $startText)
    if ($manifest.Build) { Write-Line ("BUILD {0}" -f $manifest.Build) }
    if ($manifest.Mod) { Write-Line ("MOD {0}" -f $manifest.Mod) }

    if (-not $ProfilePath) {
        if ($manifest.Kind -eq 'build') { $ProfilePath = Join-Path $RunPath 'server-profile' }
        else { $ProfilePath = $manifest.ProfileDir }
    }
    if (-not $PSBoundParameters.ContainsKey('Since')) {
        $Since = $null
        if ($manifest.Kind -eq 'testserver') { $Since = $manifest.Start }
    }

    $source = ''
    $text = ''
    if ($ProfilePath -and (Test-Path -LiteralPath $ProfilePath)) {
        $text = Read-ServerLogText -ProfilePath $ProfilePath -Kind Script -Since $Since
        $source = 'script log'
        if (-not $text.Trim()) { $text = Read-ServerLogText -ProfilePath $ProfilePath -Kind Rpt -Since $Since; $source = 'rpt' }
    }
    if (-not $text.Trim()) {
        $events = Join-Path $RunPath 'nimby-events.log'
        if (Test-Path -LiteralPath $events) { $text = [string](Get-Content -LiteralPath $events -Raw -Encoding UTF8); $source = 'nimby-events.log (profile folder not read)' }
    }
    if (-not $text.Trim()) { Write-Line ("UNKNOWN no server log to read; profile={0}" -f $ProfilePath); exit 2 }
    $lines = @(Get-UniqueLogLine -LogText $text)
    $sinceText = 'all'
    if ($null -ne $Since) { $sinceText = $Since.ToString('o') }
    Write-Line ("LOGS {0} source={1} since={2} lines={3}" -f $ProfilePath, $source, $sinceText, $lines.Count)

    if ($manifest.Stop) { Write-Line ("SERVER {0}" -f $manifest.Stop) }
    if ($null -ne $manifest.ExitCode) { Write-Line ("SERVER EXITED code={0}" -f $manifest.ExitCode) }

    $patterns = @('SCRIPT\s*\(E\)', 'Scripted variables corrupted')
    $packPath = Join-Path $RepoRoot 'mods\own\nimby-pack\pack.json'
    if (Test-Path -LiteralPath $packPath) {
        $pack = Get-Content -LiteralPath $packPath -Raw -Encoding UTF8 | ConvertFrom-Json
        if ($pack.bootCheck -and $pack.bootCheck.failurePatterns) { $patterns = @($pack.bootCheck.failurePatterns) }
    }
    $hits = @(Get-FailurePatternHit -Lines $lines -Patterns $patterns)
    foreach ($hit in $hits) { Write-Line ("ERRORS {0} x {1}; first: {2}" -f $hit.Count, $hit.Pattern, $hit.First) }
    if ($hits.Count -eq 0) { Write-Line 'ERRORS none' }

    foreach ($selfTest in @(Get-SelfTestLine -Lines $lines)) { Write-Line ("SELFTEST {0} cases={1} failures={2}" -f $selfTest.Component, $selfTest.Cases, $selfTest.Failures) }

    $report = @(Get-HandsTransferReport -Lines $lines)
    foreach ($entry in $report) {
        $tail = ' OPEN (no debug line)'
        if ($entry.Terminal -ne '') { $tail = ' detail=' + $entry.TerminalDetail }
        elseif ($entry.LastDebug -ne '') { $tail = ' OPEN last debug: ' + $entry.LastDebug }
        $total = ''
        if ($null -ne $entry.TotalMs) { $total = ' total=' + $entry.TotalMs + ' ms' }
        Write-Line ("HANDS id={0} {1} {2}: {3}{4}{5}" -f $entry.Id, $entry.Action, $entry.Item, (Format-HandsChain -Entry $entry), $total, $tail)
    }
    $handsTotal = Get-HandsTotal -Report $report
    Write-Line ("HANDS TOTAL ids={0} confirmed={1} refused={2} unconfirmed={3} open={4}" -f $handsTotal.Total, $handsTotal.Counts['CONFIRMED'], $handsTotal.Counts['REFUSED'], $handsTotal.Counts['UNCONFIRMED'], $handsTotal.Counts['Open'])

    $crashes = @()
    if ($ProfilePath -and (Test-Path -LiteralPath $ProfilePath)) { $crashes = @(Get-CrashReport -ProfilePath $ProfilePath -Since $Since) }
    foreach ($crash in $crashes) {
        $line = "CRASH {0} {1} {2}" -f $crash.Kind, $crash.Name, $crash.Time.ToString('HH:mm:ss')
        if ($crash.Reason) { $line += ' reason=' + $crash.Reason }
        if ($crash.Function) { $line += " function='" + $crash.Function + "'" }
        Write-Line $line
        foreach ($frame in $crash.Stack) { Write-Line ("CRASH   {0}" -f $frame) }
    }
    if ($crashes.Count -eq 0) { Write-Line 'CRASH none' }

    $verdict = Get-RunVerdict -HandsTotal $handsTotal -Hits $hits -Crashes $crashes -ExitCode $manifest.ExitCode
    foreach ($reason in $verdict.Reasons) { Write-Line ("REASON {0}" -f $reason) }
    Write-Line ("VERDICT {0} run={1} log={2}" -f $verdict.Verdict, $RunPath, $logFile)
    if ($verdict.Verdict -eq 'PASS') { exit 0 }
    exit 1
}
