# Pester 5 tests for the verdict functions in scripts\Build-NimbyPack.ps1: the boot verdict, the
# self-test gate, the signature verdict, pack.json validation, and the -Addons selection and
# -BootInstanceId parameters (release gate decisions 1 and 2). No DayZ, Steam, or key needed.

BeforeDiscovery {
    # Discovery needs the lines for -ForEach; BeforeAll loads the same file for run time.
    $lines = Import-PowerShellDataFile -Path (Join-Path $PSScriptRoot 'fixtures\BootLines.psd1')
    $FailureLines = $lines.FailureLines
    $QuietLines = $lines.QuietLines
}

BeforeAll {
    . (Join-Path $PSScriptRoot 'TestHelpers.ps1')
    $repo = Get-TestRepoRoot
    Import-Module (Join-Path $repo 'scripts\modules\NimbyCommon.psm1') -Force
    . (Get-ScriptFunctionBlock -Path (Join-Path $repo 'scripts\Build-NimbyPack.ps1') -Name 'Get-OptionalProperty', 'Get-PackDefinition', 'Select-PackAddon', 'Get-BootVerdict', 'Get-SelfTestVerdict', 'Get-SignatureVerdict')
    $packPath = Join-Path $repo 'mods\own\nimby-pack\pack.json'
    $pack = Get-PackDefinition -Path $packPath
    $failurePatterns = @($pack.bootCheck.failurePatterns)
    $readyPatterns = @($pack.bootCheck.readyPatterns)
    $lines = Import-PowerShellDataFile -Path (Join-Path $PSScriptRoot 'fixtures\BootLines.psd1')
    $FailureLines = $lines.FailureLines
    $readyLine = $lines.ReadyLine   # RPT 2026-10-19 21-52-19:5623

    function New-SelfTestText {
        # Lines in the engine's format: Pass PASS lines, Fail FAIL lines, then the complete line
        # (cases=N unless Cases is 'omit'; no complete line at all when Cases is 'none').
        param([string]$Component, [int]$Pass, [int]$Fail = 0, [object]$Cases = $null)
        $lines = @()
        for ($i = 1; $i -le $Pass; $i++) { $lines += ("21:52:34.668  SCRIPT       : [NIMBY][{0}][SelfTest] PASS case {1}" -f $Component, $i) }
        for ($i = 1; $i -le $Fail; $i++) { $lines += ("21:52:34.668  SCRIPT       : [NIMBY][{0}][SelfTest] FAIL bad case {1}" -f $Component, $i) }
        if ($null -eq $Cases) { $Cases = $Pass + $Fail }
        if ($Cases -eq 'omit') { $lines += ("21:52:34.669  SCRIPT       : [NIMBY][{0}] selftest complete; failures={1}" -f $Component, $Fail) }
        elseif ($Cases -ne 'none') { $lines += ("21:52:34.669  SCRIPT       : [NIMBY][{0}] selftest complete; cases={1} failures={2}" -f $Component, $Cases, $Fail) }
        return ($lines -join "`r`n")
    }
}

Describe 'Get-BootVerdict' {
    It 'matches <Pattern> on a <Kind> line (<Source>) and fails the boot' -ForEach $FailureLines {
        [regex]::IsMatch($Line, $Pattern) | Should -BeTrue
        (Get-BootVerdict -LogText $Line -FailurePatterns $failurePatterns -ReadyPatterns $readyPatterns).Verdict | Should -Be 'FAIL'
    }

    It 'has a test line for every failure pattern in pack.json' {
        foreach ($pattern in $failurePatterns) {
            @($FailureLines | Where-Object { $_.Pattern -eq $pattern }).Count | Should -BeGreaterThan 0 -Because "pattern '$pattern' needs a test line"
        }
    }

    It 'does not fail on a <Name> line (<Source>)' -ForEach $QuietLines {
        (Get-BootVerdict -LogText $Line -FailurePatterns $failurePatterns -ReadyPatterns $readyPatterns).Verdict | Should -Be 'PENDING'
    }

    It 'returns READY on the real ready line' {
        $verdict = Get-BootVerdict -LogText $readyLine -FailurePatterns $failurePatterns -ReadyPatterns $readyPatterns
        $verdict.Verdict | Should -Be 'READY'
        $verdict.Evidence | Should -Match 'Init sequence finished'
    }

    It 'lets FAIL win over READY, whichever comes first' {
        $text = $readyLine + "`r`n" + $FailureLines[0].Line
        $verdict = Get-BootVerdict -LogText $text -FailurePatterns $failurePatterns -ReadyPatterns $readyPatterns
        $verdict.Verdict | Should -Be 'FAIL'
        $verdict.Evidence | Should -Match 'Syntax error'
    }

    It 'returns PENDING on empty text' {
        (Get-BootVerdict -LogText '' -FailurePatterns $failurePatterns -ReadyPatterns $readyPatterns).Verdict | Should -Be 'PENDING'
    }
}

Describe 'Get-SelfTestVerdict' {
    BeforeAll {
        $expected = @('Autorun', 'Hands', 'Earplugs', 'Notes', 'Tack')
        $full = @(
            (New-SelfTestText -Component 'Autorun' -Pass 3),
            (New-SelfTestText -Component 'Hands' -Pass 2),
            (New-SelfTestText -Component 'Earplugs' -Pass 1),
            (New-SelfTestText -Component 'Notes' -Pass 2),
            (New-SelfTestText -Component 'Tack' -Pass 4)
        ) -join "`r`n"
    }

    It 'passes five flagged addons that all ran, with per-component counts' {
        $result = Get-SelfTestVerdict -LogText $full -Expected $expected
        $result.Verdict | Should -Be 'PASS'
        $result.Pass | Should -Be 12
        $result.Components['Autorun'].Pass | Should -Be 3
        $result.Components['Tack'].Cases | Should -Be 4
        $result.Problems.Count | Should -Be 0
    }

    It 'counts the same line once when the RPT and the script log both carry it' {
        $result = Get-SelfTestVerdict -LogText ($full + "`r`n" + $full) -Expected $expected
        $result.Pass | Should -Be 12
        $result.Verdict | Should -Be 'PASS'
    }

    It 'fails and names the silent addon when five are flagged and four ran' {
        $four = @(
            (New-SelfTestText -Component 'Autorun' -Pass 3),
            (New-SelfTestText -Component 'Earplugs' -Pass 1),
            (New-SelfTestText -Component 'Notes' -Pass 2),
            (New-SelfTestText -Component 'Tack' -Pass 4)
        ) -join "`r`n"
        $result = Get-SelfTestVerdict -LogText $four -Expected $expected
        $result.Verdict | Should -Be 'FAIL'
        $result.Problems | Should -Contain 'Hands ran=no'
        $result.Problems.Count | Should -Be 1
    }

    It 'fails a complete line with cases=0' {
        $result = Get-SelfTestVerdict -LogText (New-SelfTestText -Component 'Hands' -Pass 0 -Cases 0) -Expected 'Hands'
        $result.Verdict | Should -Be 'FAIL'
        $result.Problems | Should -Contain 'Hands cases=0'
    }

    It 'fails a complete line with no cases=N' {
        $result = Get-SelfTestVerdict -LogText (New-SelfTestText -Component 'Hands' -Pass 2 -Cases 'omit') -Expected 'Hands'
        $result.Verdict | Should -Be 'FAIL'
        $result.Problems | Should -Contain 'Hands complete line has no cases=N'
    }

    It 'fails PASS lines with no complete line' {
        $result = Get-SelfTestVerdict -LogText (New-SelfTestText -Component 'Hands' -Pass 2 -Cases 'none') -Expected 'Hands'
        $result.Problems | Should -Contain "Hands no 'selftest complete' line"
    }

    It 'fails a complete line that claims cases but printed no PASS line' {
        $result = Get-SelfTestVerdict -LogText (New-SelfTestText -Component 'Hands' -Pass 0 -Cases 3) -Expected 'Hands'
        $result.Problems | Should -Contain 'Hands no PASS line'
    }

    It 'fails when PASS plus FAIL does not equal cases' {
        $result = Get-SelfTestVerdict -LogText (New-SelfTestText -Component 'Hands' -Pass 2 -Cases 5) -Expected 'Hands'
        $result.Problems | Should -Contain 'Hands pass+fail=2 but cases=5'
    }

    It 'fails on a FAIL line and returns its text' {
        $result = Get-SelfTestVerdict -LogText (New-SelfTestText -Component 'Tack' -Pass 2 -Fail 1) -Expected 'Tack'
        $result.Verdict | Should -Be 'FAIL'
        $result.Fail | Should -Be 1
        $result.Failures[0] | Should -Match '\[NIMBY\]\[Tack\]\[SelfTest\] FAIL bad case 1'
    }

    It 'passes empty text when nothing is expected' {
        (Get-SelfTestVerdict -LogText '' -Expected @()).Verdict | Should -Be 'PASS'
    }
}

Describe 'Get-SignatureVerdict' {
    BeforeAll {
        # Real DSCheckSignatures output, logs\build-20261019-215218 (client and control logs).
        $okLines = @('NimbyAutorun', 'NimbyEarplugs', 'NimbyHands', 'NimbyNotes', 'NimbyTack') | ForEach-Object {
            'Signature D:\GitHub\nimby-badlands-server\build\@NIMBY-20261019-215218\addons\{0}.pbo.nimby.bisign is OK' -f $_
        }
        $realOutput = $okLines -join "`r`n"
        $controlOutput = 'No signature found for D:\GitHub\nimby-badlands-server\logs\build-20261019-215218\sigcheck-control-client\NimbyAutorun.pbo'
        $expectedPbo = @('NimbyAutorun.pbo', 'NimbyEarplugs.pbo', 'NimbyHands.pbo', 'NimbyNotes.pbo', 'NimbyTack.pbo')
    }

    It 'passes when every PBO has an OK line and the unsigned control was flagged' {
        $result = Get-SignatureVerdict -RealExit 0 -RealOutput $realOutput -NegativeExit 0 -NegativeOutput $controlOutput -ExpectedPbo $expectedPbo
        $result.Verdict | Should -Be 'PASS'
        $result.Evidence | Should -Match '5 of 5'
    }

    It 'is UNKNOWN when the checker did not flag the unsigned control' {
        $result = Get-SignatureVerdict -RealExit 0 -RealOutput $realOutput -NegativeExit 0 -NegativeOutput $okLines[0] -ExpectedPbo $expectedPbo
        $result.Verdict | Should -Be 'UNKNOWN'
    }

    It 'fails and names a PBO with no OK line' {
        $result = Get-SignatureVerdict -RealExit 0 -RealOutput (($okLines | Select-Object -First 4) -join "`r`n") -NegativeExit 0 -NegativeOutput $controlOutput -ExpectedPbo $expectedPbo
        $result.Verdict | Should -Be 'FAIL'
        $result.Evidence | Should -Match 'NimbyTack\.pbo'
    }

    It 'fails closed on an unrecognized line' {
        $result = Get-SignatureVerdict -RealExit 0 -RealOutput ($realOutput + "`r`nSomething unexpected") -NegativeExit 0 -NegativeOutput $controlOutput -ExpectedPbo $expectedPbo
        $result.Verdict | Should -Be 'FAIL'
        $result.Evidence | Should -Match 'Something unexpected'
    }

    It 'fails on a nonzero exit even when every line is OK' {
        (Get-SignatureVerdict -RealExit 1 -RealOutput $realOutput -NegativeExit 0 -NegativeOutput $controlOutput -ExpectedPbo $expectedPbo).Verdict | Should -Be 'FAIL'
    }
}

Describe 'pack.json' {
    It 'loads through Get-PackDefinition with a test instance (998 or 999)' {
        Test-TestInstanceId -InstanceId ([int]$pack.bootCheck.instanceId) | Should -BeTrue
        Test-TestInstanceId -InstanceId ([int]$pack.testServer.instanceId) | Should -BeTrue
    }

    It 'gives every addon a folder that exists and a logPrefix (NOTICE is checked by Test-NimbyRepo.ps1)' {
        foreach ($addon in $pack.addons) {
            $folder = Join-Path (Split-Path -Parent $packPath) $addon.folder
            Test-Path -LiteralPath $folder -PathType Container | Should -BeTrue -Because "$($addon.folder) is listed in pack.json"
            $addon.logPrefix | Should -Not -BeNullOrEmpty
        }
    }

    It 'points dev.folder at a folder that exists' {
        $devFolder = Join-Path (Split-Path -Parent $packPath) $pack.dev.folder
        if (-not (Test-Path -LiteralPath $devFolder -PathType Container)) {
            Set-ItResult -Skipped -Because "dev.folder '$($pack.dev.folder)' is not on this branch yet; -Dev builds fail until the harness lands"
            return
        }
        Test-Path -LiteralPath $devFolder -PathType Container | Should -BeTrue
    }

    It 'rejects an addon row without logPrefix' {
        $broken = Get-Content -LiteralPath $packPath -Raw | ConvertFrom-Json
        $broken.addons[0].PSObject.Properties.Remove('logPrefix')
        $path = Join-Path $TestDrive 'pack-no-logprefix.json'
        Set-Content -LiteralPath $path -Value ($broken | ConvertTo-Json -Depth 10) -Encoding utf8
        { Get-PackDefinition -Path $path } | Should -Throw -ExpectedMessage "*missing 'logPrefix'*"
    }

    It 'rejects a duplicate logPrefix' {
        $broken = Get-Content -LiteralPath $packPath -Raw | ConvertFrom-Json
        $broken.addons[1].logPrefix = $broken.addons[0].logPrefix
        $path = Join-Path $TestDrive 'pack-dup-logprefix.json'
        Set-Content -LiteralPath $path -Value ($broken | ConvertTo-Json -Depth 10) -Encoding utf8
        { Get-PackDefinition -Path $path } | Should -Throw -ExpectedMessage '*Duplicate logPrefix*'
    }

    It 'refuses a boot-check instance that is not 998 or 999' {
        $broken = Get-Content -LiteralPath $packPath -Raw | ConvertFrom-Json
        $broken.bootCheck.instanceId = 1
        $path = Join-Path $TestDrive 'pack-storage1.json'
        Set-Content -LiteralPath $path -Value ($broken | ConvertTo-Json -Depth 10) -Encoding utf8
        { Get-PackDefinition -Path $path } | Should -Throw -ExpectedMessage '*not a test instance*'
    }
}

Describe 'Select-PackAddon (release gate decision 1: the artifact subset is a parameter)' {
    BeforeAll {
        $all = @($pack.addons | ForEach-Object { $_.pboName })
    }

    It 'returns every pack.json row, in pack.json order, when -Addons is omitted or empty' {
        @(Select-PackAddon -Pack $pack -Names $null | ForEach-Object { $_.pboName }) | Should -Be $all
        @(Select-PackAddon -Pack $pack -Names @() | ForEach-Object { $_.pboName }) | Should -Be $all
    }

    It 'returns the named rows in pack.json order, whatever order they were given in' {
        $picked = @(Select-PackAddon -Pack $pack -Names @($all[2], $all[0]) | ForEach-Object { $_.pboName })
        $picked | Should -Be @($all[0], $all[2])
    }

    It 'accepts one comma-separated string with spaces, as -File passes an array parameter' {
        $picked = @(Select-PackAddon -Pack $pack -Names ("{0}, {1}" -f $all[1], $all[3]) | ForEach-Object { $_.pboName })
        $picked | Should -Be @($all[1], $all[3])
    }

    It 'matches a name without regard to case and returns the pack.json spelling' {
        $picked = @(Select-PackAddon -Pack $pack -Names $all[0].ToUpperInvariant() | ForEach-Object { $_.pboName })
        $picked | Should -Be @($all[0])
    }

    It 'selects the release 1 artifact (Legs, Ears, Scrawl, Greet) and leaves Hands and Tack out' {
        $picked = @(Select-PackAddon -Pack $pack -Names 'NimbyAutorun,NimbyEarplugs,NimbyNotes,NimbyGreet' | ForEach-Object { $_.pboName })
        $picked | Should -Be @('NimbyAutorun', 'NimbyEarplugs', 'NimbyNotes', 'NimbyGreet')
        $picked | Should -Not -Contain 'NimbyHands'
        $picked | Should -Not -Contain 'NimbyTack'
    }

    It 'throws on a name pack.json does not list, naming it and the known names' {
        { Select-PackAddon -Pack $pack -Names @($all[0], 'NimbyNothing') } | Should -Throw -ExpectedMessage "*'NimbyNothing'*Known: *"
    }

    It 'throws on a name given twice' {
        { Select-PackAddon -Pack $pack -Names @($all[0], $all[0]) } | Should -Throw -ExpectedMessage '*twice*'
    }

    It 'throws on a list that is empty after trimming instead of silently building everything' {
        { Select-PackAddon -Pack $pack -Names @(' ', ',') } | Should -Throw -ExpectedMessage '*-Addons is empty*'
    }
}

Describe 'Build-NimbyPack.ps1 parameters (release gate decision 2: the boot instance is a parameter)' {
    BeforeAll {
        $command = Get-Command (Join-Path $repo 'scripts\Build-NimbyPack.ps1')
    }

    It 'declares -Addons as a string array' {
        $command.Parameters['Addons'].ParameterType | Should -Be ([string[]])
    }

    It 'declares -BootInstanceId as an int limited to 998 and 999 (shared rules, section 8)' {
        $parameter = $command.Parameters['BootInstanceId']
        $parameter.ParameterType | Should -Be ([int])
        $set = @($parameter.Attributes | Where-Object { $_ -is [System.Management.Automation.ValidateSetAttribute] })
        $set.Count | Should -Be 1
        @($set[0].ValidValues | Sort-Object) | Should -Be @('998', '999')
    }
}
