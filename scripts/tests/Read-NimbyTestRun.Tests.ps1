# Pester 5 tests for scripts\Read-NimbyTestRun.ps1: the manifest reader, line deduplication, the
# Hands chain builder with its timings, the totals, the failure-pattern scan, the self-test lines,
# the crash-log reader, the verdict, and one end-to-end run of the script against a run folder whose
# profile folder is gone. Fixture lines are synthetic (no real player ids).

BeforeAll {
    . (Join-Path $PSScriptRoot 'TestHelpers.ps1')
    $repo = Get-TestRepoRoot
    $readScript = Join-Path $repo 'scripts\Read-NimbyTestRun.ps1'
    . (Get-ScriptFunctionBlock -Path $readScript -Name 'Get-TestRunManifest', 'Get-UniqueLogLine', 'ConvertTo-LogTime', 'Get-LogDeltaMs', 'Get-HandsTransferReport', 'Format-HandsChain', 'Get-HandsTotal', 'Get-FailurePatternHit', 'Get-SelfTestLine', 'Read-CrashLog', 'Get-CrashReport', 'Get-RunVerdict')
    $script:HandsTerminalStates = Get-ScriptAssignmentValue -Path $readScript -Name 'HandsTerminalStates'

    $giver = '"A" (aaaa=, 76561190000000001)'
    $receiver = '"B" (bbbb=, 76561190000000002)'
    $script:HandsLog = @(
        '22:39:38.800  SCRIPT       : [NIMBY][Hands] loaded; server=true debug=true',
        '22:39:39.100  SCRIPT       : [NIMBY][Hands] selftest complete; cases=163 failures=0',
        '23:00:41.030    SCRIPT       : [NIMBY][Hands][Debug] id=1 wait start',
        '23:00:41.832 SCRIPT       : [NIMBY][Hands][Debug] id=1 wait free waited=802',
        ('23:00:41.832 SCRIPT       : [NIMBY][Hands] id=1 action=NimbyHandsActionTakeItem state=REQUESTED giver={0} receiver={1} item=Wrench #027507 detail=stage=CARGO:TaloonBag_Blue@{0} dst=CARGO:TaloonBag_Blue@{1} waited=802' -f $giver, $receiver),
        ('23:00:42.214    SCRIPT       : [NIMBY][Hands] id=1 action=NimbyHandsActionTakeItem state=STAGED giver={0} receiver={1} item=Wrench #027507 detail=signal=EEItemLocationChanged dst=CARGO:TaloonBag_Blue@{0}' -f $giver, $receiver),
        '23:00:42.264 SCRIPT       : [NIMBY][Hands][Debug] id=1 step 2 facts: location read',
        '23:00:42.265 SCRIPT       : [NIMBY][Hands][Debug] id=1 step 2 facts: at source=true',
        ('23:00:43.000 SCRIPT       : [NIMBY][Hands] id=2 action=NimbyHandsActionGiveItem state=REQUESTED giver={0} receiver={1} item=Pliers #000002 detail=stage=x dst=y waited=0' -f $giver, $receiver),
        ('23:00:48.010 SCRIPT       : [NIMBY][Hands] id=2 action=NimbyHandsActionGiveItem state=UNCONFIRMED giver={0} receiver={1} item=Pliers #000002 detail=no completion signal within 5000 ms; item with giver at HANDS:SurvivorM_Mirek@{0}' -f $giver, $receiver),
        ('23:00:50.000 SCRIPT       : [NIMBY][Hands] id=3 action=NimbyHandsActionTakeItem state=REQUESTED giver={0} receiver={1} item=Wrench #000003 detail=stage=x dst=y waited=10' -f $giver, $receiver),
        ('23:00:50.400 SCRIPT       : [NIMBY][Hands] id=3 action=NimbyHandsActionTakeItem state=STAGED giver={0} receiver={1} item=Wrench #000003 detail=signal=EEItemLocationChanged dst=CARGO:TaloonBag_Blue@{0}' -f $giver, $receiver),
        ('23:00:50.451 SCRIPT       : [NIMBY][Hands] id=3 action=NimbyHandsActionTakeItem state=CONFIRMED giver={0} receiver={1} item=Wrench #000003 detail=signal=EEItemLocationChanged dst=CARGO:TaloonBag_Blue@{1}' -f $giver, $receiver),
        ('23:00:51.000 SCRIPT       : [NIMBY][Hands] id=4 action=NimbyHandsActionTakeItem state=REFUSED giver={0} receiver={1} item=none detail=no item' -f $giver, $receiver),
        ('23:00:52.000 SCRIPT       : [NIMBY][Hands] pockets player={0} open=true reason=toggle' -f $giver)
    )
}

Describe 'Get-TestRunManifest' {
    It 'reads START, BUILD, MOD, LOGS, STOP and the exit code of a crashed run' {
        $path = Join-Path $TestDrive 'testserver.log'
        Set-Content -LiteralPath $path -Value @(
            'START 2026-10-28T22:39:24.6284102-05:00 PS=7.6.9',
            'INSTANCE override=998',
            'BUILD explicit verdict=PASS log=D:\x\logs\build-20261028-223604\build.log',
            'MOD D:\x\build\@NIMBY-20261028-223604',
            'SERVER START -config=...',
            'STOP requested by server-exited',
            'SERVER EXITED before stop code=-1073741819',
            'LOGS D:\x\profiles\patch-1',
            'SUMMARY events=62'
        ) -Encoding ascii
        $m = Get-TestRunManifest -Path $path
        $m.Kind | Should -Be 'testserver'
        $m.Start.Year | Should -Be 2026
        $m.Start.Second | Should -Be 24
        $m.Build | Should -Be 'D:\x\logs\build-20261028-223604\build.log'
        $m.Mod | Should -Be 'D:\x\build\@NIMBY-20261028-223604'
        $m.ProfileDir | Should -Be 'D:\x\profiles\patch-1'
        $m.Stop | Should -Be 'STOP requested by server-exited'
        $m.ExitCode | Should -Be -1073741819
    }

    It 'has no exit code when the server stopped on request, and reads a build.log as kind build' {
        $path = Join-Path $TestDrive 'build.log'
        Set-Content -LiteralPath $path -Value @('START 2026-10-29T00:11:22.0000000-05:00 RepoRoot=D:\x PS=7.6.9 Dev=True', 'STOP requested by key', 'SUMMARY PASS mod=D:\x\build\@NIMBY-1') -Encoding ascii
        $m = Get-TestRunManifest -Path $path
        $m.Kind | Should -Be 'build'
        $m.ExitCode | Should -BeNullOrEmpty
    }

    It 'reads the exit code of a boot check whose server exited' {
        $path = Join-Path $TestDrive 'build2.log'
        Set-Content -LiteralPath $path -Value @('START 2026-10-29T00:11:22.0000000-05:00 PS=7', 'BOOT FAIL server exited after ready with code -1073741819') -Encoding ascii
        (Get-TestRunManifest -Path $path).ExitCode | Should -Be -1073741819
    }
}

Describe 'Get-UniqueLogLine' {
    It 'drops exact repeats and blank lines' {
        @(Get-UniqueLogLine -LogText "a`r`nb`r`n`r`na`nb  `nc") | Should -Be @('a', 'b', 'c')
    }
}

Describe 'ConvertTo-LogTime and Get-LogDeltaMs' {
    It 'parses HH:mm:ss.fff' {
        $t = ConvertTo-LogTime -Text '23:00:42.264 SCRIPT : x'
        $t.Hours | Should -Be 23
        $t.Milliseconds | Should -Be 264
    }

    It 'returns null for a line without a time' {
        ConvertTo-LogTime -Text 'no time here' | Should -BeNullOrEmpty
    }

    It 'measures the delta and wraps past midnight' {
        Get-LogDeltaMs -From (ConvertTo-LogTime -Text '23:00:41.832') -To (ConvertTo-LogTime -Text '23:00:42.214') | Should -Be 382
        Get-LogDeltaMs -From (ConvertTo-LogTime -Text '23:59:59.900') -To (ConvertTo-LogTime -Text '00:00:00.100') | Should -Be 200
    }
}

Describe 'Get-HandsTransferReport' {
    BeforeAll {
        $script:Report = @(Get-HandsTransferReport -Lines $script:HandsLog)
        $script:ById = @{}
        foreach ($entry in $script:Report) { $script:ById[$entry.Id] = $entry }
    }

    It 'finds every id once, in first-seen order, and skips lines without an id' {
        @($script:Report | ForEach-Object { $_.Id }) | Should -Be @(1, 2, 3, 4)
    }

    It 'builds the chain with deltas and leaves a crashed id open with its last debug line' {
        $one = $script:ById[1]
        $one.Action | Should -Be 'TakeItem'
        $one.Item | Should -Be 'Wrench #027507'
        @($one.Steps | ForEach-Object { $_.State }) | Should -Be @('REQUESTED', 'STAGED')
        $one.Steps[1].DeltaMs | Should -Be 382
        $one.Terminal | Should -Be ''
        $one.LastDebug | Should -Be 'step 2 facts: at source=true'
        Format-HandsChain -Entry $one | Should -Be 'REQUESTED -> STAGED (+382 ms)'
    }

    It 'reads the terminal state, its detail, and the total time' {
        $three = $script:ById[3]
        $three.Terminal | Should -Be 'CONFIRMED'
        $three.TotalMs | Should -Be 451
        Format-HandsChain -Entry $three | Should -Be 'REQUESTED -> STAGED (+400 ms) -> CONFIRMED (+51 ms)'
        $script:ById[2].Terminal | Should -Be 'UNCONFIRMED'
        $script:ById[2].TerminalDetail | Should -BeLike 'no completion signal within 5000 ms;*'
        $script:ById[4].Terminal | Should -Be 'REFUSED'
        $script:ById[4].Steps.Count | Should -Be 1
    }

    It 'counts the totals' {
        $total = Get-HandsTotal -Report $script:Report
        $total.Total | Should -Be 4
        $total.Counts['CONFIRMED'] | Should -Be 1
        $total.Counts['REFUSED'] | Should -Be 1
        $total.Counts['UNCONFIRMED'] | Should -Be 1
        $total.Counts['Open'] | Should -Be 1
        $total.OpenIds | Should -Be @(1)
    }
}

Describe 'Get-FailurePatternHit and Get-SelfTestLine' {
    It 'reports only the patterns that hit, with a count and the first line' {
        $lines = @('a SCRIPT (E): bad', 'b SCRIPT    (E): worse', 'c fine')
        $hits = @(Get-FailurePatternHit -Lines $lines -Patterns @('SCRIPT\s*\(E\)', 'Scripted variables corrupted'))
        $hits.Count | Should -Be 1
        $hits[0].Count | Should -Be 2
        $hits[0].First | Should -Be 'a SCRIPT (E): bad'
    }

    It 'reads the selftest complete lines' {
        $selfTests = @(Get-SelfTestLine -Lines $script:HandsLog)
        $selfTests.Count | Should -Be 1
        $selfTests[0].Component | Should -Be 'Hands'
        $selfTests[0].Cases | Should -Be 163
        $selfTests[0].Failures | Should -Be 0
    }
}

Describe 'Read-CrashLog and Get-CrashReport' {
    BeforeAll {
        $script:Profile = Join-Path $TestDrive 'profile'
        New-Item -ItemType Directory -Path $script:Profile -Force | Out-Null
        Set-Content -LiteralPath (Join-Path $script:Profile 'crash_2026-10-28_22-39-24.log') -Value @(
            '---------------------------------------------',
            'Log D:\x\crash_2026-10-28_22-39-24.log started at 2026-10-28 23:00:42',
            '',
            '28.10 2026 23:00:42',
            'Unhandled exception',
            '',
            'Program: D:\DayZServer\DayZServer_x64.exe',
            'Reason: Access violation. Illegal read by 0x7ff6f3107047 at 0xffffffffffffffff',
            '',
            "Function: 'RunStep2'",
            'Stack trace:',
            'NimbyHands/scripts/4_world/systems/inventory/moveitemtoplayer.c:387 Function RunStep2',
            'NimbyHands/scripts/4_world/systems/inventory/nimbyhands_server.c:433 Function OnStep2',
            'scripts/3_Game/dayzgame.c:3080 Function OnUpdate',
            '',
            'SymGetSymFromAddr:487, addr:0x7ff6f3107047'
        ) -Encoding ascii
        Set-Content -LiteralPath (Join-Path $script:Profile 'DayZServer_x64_2026-10-28_23-00-42.mdmp') -Value 'x' -Encoding ascii
        Set-Content -LiteralPath (Join-Path $script:Profile 'error_2026-10-28_22-39-24.log') -Value @('22:39:27.613 SCRIPT    (W): warning only') -Encoding ascii
        Set-Content -LiteralPath (Join-Path $script:Profile 'error_2026-10-28_21-00-00.log') -Value @('21:00:01.000 SCRIPT    (E): JSON ERROR') -Encoding ascii
    }

    It 'reads the reason, the function and the script stack' {
        $crash = Read-CrashLog -Path (Join-Path $script:Profile 'crash_2026-10-28_22-39-24.log')
        $crash.Reason | Should -BeLike 'Access violation*'
        $crash.Function | Should -Be 'RunStep2'
        $crash.Stack.Count | Should -Be 3
        $crash.Stack[0] | Should -Be 'NimbyHands/scripts/4_world/systems/inventory/moveitemtoplayer.c:387 Function RunStep2'
    }

    It 'lists the crash log, the minidump and the error log with an (E) line, not a warnings-only error log' {
        $report = @(Get-CrashReport -ProfilePath $script:Profile)
        @($report | ForEach-Object { $_.Kind } | Sort-Object) | Should -Be @('crash', 'error', 'minidump')
        @($report | Where-Object { $_.Kind -eq 'error' })[0].Reason | Should -BeLike '1 (E) line(s); first:*JSON ERROR'
    }

    It 'honours Since' {
        @(Get-CrashReport -ProfilePath $script:Profile -Since (Get-Date).AddDays(1)).Count | Should -Be 0
    }
}

Describe 'Get-RunVerdict' {
    It 'passes a clean run' {
        $total = Get-HandsTotal -Report @([PSCustomObject]@{ Id = 1; Terminal = 'CONFIRMED' })
        $v = Get-RunVerdict -HandsTotal $total -Hits @() -Crashes @() -ExitCode $null
        $v.Verdict | Should -Be 'PASS'
        $v.Reasons.Count | Should -Be 0
    }

    It 'fails on an exit code, a pattern hit, a crash file, an UNCONFIRMED id, or an open id, each with a reason' {
        $total = Get-HandsTotal -Report @([PSCustomObject]@{ Id = 1; Terminal = '' }, [PSCustomObject]@{ Id = 2; Terminal = 'UNCONFIRMED' })
        $hits = @([PSCustomObject]@{ Pattern = 'SCRIPT\s*\(E\)'; Count = 2; First = 'x' })
        $crashes = @([PSCustomObject]@{ Kind = 'crash'; Name = 'c.log' }, [PSCustomObject]@{ Kind = 'error'; Name = 'e.log' })
        $v = Get-RunVerdict -HandsTotal $total -Hits $hits -Crashes $crashes -ExitCode -1073741819
        $v.Verdict | Should -Be 'FAIL'
        $v.Reasons.Count | Should -Be 5
        $v.Reasons[0] | Should -BeLike 'server exited*-1073741819'
        $v.Reasons[2] | Should -Be '1 crash file(s)'
        $v.Reasons[4] | Should -Be 'Hands id(s) with no terminal state: 1'
    }
}

Describe 'Read-NimbyTestRun.ps1 end to end' {
    It 'evaluates a crashed run folder from nimby-events.log when the profile folder is gone, writes evaluation.log, exits 1' {
        $run = Join-Path $TestDrive 'testserver-20261028-223924'
        New-Item -ItemType Directory -Path $run -Force | Out-Null
        Set-Content -LiteralPath (Join-Path $run 'testserver.log') -Value @(
            'START 2026-10-28T22:39:24.6284102-05:00 PS=7.6.9',
            'STOP requested by server-exited',
            'SERVER EXITED before stop code=-1073741819',
            ('LOGS {0}' -f (Join-Path $TestDrive 'no-such-profile')),
            'SUMMARY events=2'
        ) -Encoding ascii
        Set-Content -LiteralPath (Join-Path $run 'nimby-events.log') -Value ($script:HandsLog + $script:HandsLog) -Encoding utf8
        $shell = (Get-Process -Id $PID).Path
        $output = & $shell -NoProfile -ExecutionPolicy Bypass -File $readScript -RunPath $run -RepoRoot $repo 2>&1
        $LASTEXITCODE | Should -Be 1
        $text = Get-Content -LiteralPath (Join-Path $run 'evaluation.log') -Raw
        $text | Should -BeLike '*HANDS id=1 TakeItem Wrench #027507: REQUESTED -> STAGED (+382 ms) OPEN last debug: step 2 facts: at source=true*'
        $text | Should -BeLike '*HANDS TOTAL ids=4 confirmed=1 refused=1 unconfirmed=1 open=1*'
        $text | Should -BeLike '*REASON server exited on its own with code -1073741819*'
        $text | Should -BeLike '*VERDICT FAIL*'
        ($output -join "`n") | Should -BeLike '*VERDICT FAIL*'
    }
}
