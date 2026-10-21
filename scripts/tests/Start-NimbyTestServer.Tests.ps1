# Pester 5 tests for the pure functions in scripts\Start-NimbyTestServer.ps1: build-log parsing,
# PASS-only build selection, the join-password sources, and the build hash cross-check.
# Everything runs in $TestDrive with dummy values; no real password is read or written.

BeforeAll {
    . (Join-Path $PSScriptRoot 'TestHelpers.ps1')
    $repo = Get-TestRepoRoot
    Import-Module (Join-Path $repo 'scripts\modules\NimbyCommon.psm1') -Force
    . (Get-ScriptFunctionBlock -Path (Join-Path $repo 'scripts\Start-NimbyTestServer.ps1') -Name 'Read-BuildLog', 'Test-ModFolder', 'Get-BuildLogForBuild', 'Get-QualifiedModBuild', 'Get-RandomJoinPassword', 'Get-StandingJoinPassword', 'Resolve-JoinPassword', 'Get-AddonHashLine', 'Get-BuildHashMismatch')

    function New-FakeBuild {
        # build\@NIMBY-<stamp>\addons\<pbo> plus logs\build-<stamp>\build.log ending in Verdict.
        param([string]$Root, [string]$Stamp, [string]$Verdict, [string[]]$Pbo = @('NimbyAutorun'), [switch]$NoSummary, [switch]$NoPbo)
        $mod = Join-Path $Root ("build\@NIMBY-{0}" -f $Stamp)
        $addons = Join-Path $mod 'addons'
        New-Item -ItemType Directory -Path $addons -Force | Out-Null
        $log = Join-Path $Root ("logs\build-{0}" -f $Stamp)
        New-Item -ItemType Directory -Path $log -Force | Out-Null
        $lines = @("START x")
        foreach ($name in $Pbo) {
            $file = Join-Path $addons "$name.pbo"
            Set-Content -LiteralPath $file -Value "$name $Stamp" -Encoding ascii
            $hash = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash
            $lines += "ADDON OK $name [client] entries=1 sources=1 signexit=0 sha256=$hash"
            $lines += "  ENTRY $name\config.cpp"
            if ($NoPbo) { Remove-Item -LiteralPath $file }
        }
        $lines += "MOD $mod", 'SERVERMOD none (no serverOnly addon in pack.json)', 'DEV none (build without -Dev)'
        if (-not $NoSummary) { $lines += "SUMMARY $Verdict mod=$mod" }
        Set-Content -LiteralPath (Join-Path $log 'build.log') -Value $lines -Encoding utf8
        return $mod
    }
}

Describe 'Read-BuildLog' {
    It 'reads the verdict, the outputs, and one sha256 per ADDON OK line' {
        $mod = New-FakeBuild -Root $TestDrive -Stamp '20261021-100000' -Verdict 'PASS' -Pbo 'NimbyAutorun', 'NimbyTack'
        $build = Read-BuildLog -Path (Join-Path $TestDrive 'logs\build-20261021-100000\build.log')
        $build.Verdict | Should -Be 'PASS'
        $build.ModPath | Should -Be $mod
        $build.ServerModPath | Should -BeNullOrEmpty
        $build.DevPath | Should -BeNullOrEmpty
        $build.Hashes.Count | Should -Be 2
        $build.Hashes['client|NimbyTack.pbo'] | Should -Match '^[0-9A-F]{64}$'
    }

    It 'is INCOMPLETE when the log has no SUMMARY line, MISSING when there is no log' {
        [void](New-FakeBuild -Root $TestDrive -Stamp '20261021-100100' -Verdict 'PASS' -NoSummary)
        (Read-BuildLog -Path (Join-Path $TestDrive 'logs\build-20261021-100100\build.log')).Verdict | Should -Be 'INCOMPLETE'
        (Read-BuildLog -Path (Join-Path $TestDrive 'nowhere\build.log')).Verdict | Should -Be 'MISSING'
    }
}

Describe 'Get-QualifiedModBuild' {
    It 'picks the newest PASS build and skips a newer FAIL, UNKNOWN, unfinished, or emptied one' {
        $root = Join-Path $TestDrive 'select'
        $older = New-FakeBuild -Root $root -Stamp '20261020-090000' -Verdict 'PASS'
        $wanted = New-FakeBuild -Root $root -Stamp '20261020-100000' -Verdict 'PASS'
        [void](New-FakeBuild -Root $root -Stamp '20261020-110000' -Verdict 'FAIL')
        [void](New-FakeBuild -Root $root -Stamp '20261020-120000' -Verdict 'UNKNOWN')
        [void](New-FakeBuild -Root $root -Stamp '20261020-130000' -Verdict 'PASS' -NoSummary)
        [void](New-FakeBuild -Root $root -Stamp '20261020-140000' -Verdict 'PASS' -NoPbo)
        $build = Get-QualifiedModBuild -LogRoot (Join-Path $root 'logs') -ModFolderName '@NIMBY'
        $build.ModPath | Should -Be $wanted
        $build.ModPath | Should -Not -Be $older
    }

    It 'returns nothing when no build passed' {
        $root = Join-Path $TestDrive 'nopass'
        [void](New-FakeBuild -Root $root -Stamp '20261020-110000' -Verdict 'FAIL')
        Get-QualifiedModBuild -LogRoot (Join-Path $root 'logs') -ModFolderName '@NIMBY' | Should -BeNullOrEmpty
    }

    It 'maps a build folder to its build log by stamp, and a hand-made folder to nothing' {
        $logs = Join-Path $TestDrive 'logs'
        Get-BuildLogForBuild -BuildPath (Join-Path $TestDrive 'build\@NIMBY-20261020-100000') -LogRoot $logs -ModFolderName '@NIMBY' | Should -Be (Join-Path $logs 'build-20261020-100000\build.log')
        Get-BuildLogForBuild -BuildPath (Join-Path $TestDrive 'build\my-test-copy') -LogRoot $logs -ModFolderName '@NIMBY' | Should -BeNullOrEmpty
    }
}

Describe 'Get-BuildHashMismatch' {
    BeforeAll {
        $mod = New-FakeBuild -Root (Join-Path $TestDrive 'hash') -Stamp '20261021-110000' -Verdict 'PASS' -Pbo 'NimbyAutorun', 'NimbyHands', 'NimbyTack'
        $build = Read-BuildLog -Path (Join-Path $TestDrive 'hash\logs\build-20261021-110000\build.log')
    }

    It 'reports nothing when every PBO matches its build log sha256' {
        @(Get-BuildHashMismatch -Build $build -Path $mod -Label 'client').Count | Should -Be 0
    }

    It 'reports a changed, a missing, and an added PBO' {
        Add-Content -LiteralPath (Join-Path $mod 'addons\NimbyTack.pbo') -Value 'changed' -Encoding ascii
        Remove-Item -LiteralPath (Join-Path $mod 'addons\NimbyHands.pbo')
        Set-Content -LiteralPath (Join-Path $mod 'addons\Extra.pbo') -Value 'extra' -Encoding ascii
        $problems = @(Get-BuildHashMismatch -Build $build -Path $mod -Label 'client')
        $problems.Count | Should -Be 3
        ($problems -join "`n") | Should -Match 'NimbyTack\.pbo sha256='
        ($problems -join "`n") | Should -Match 'NimbyHands\.pbo missing'
        ($problems -join "`n") | Should -Match 'Extra\.pbo not in the build log'
    }
}

Describe 'Resolve-JoinPassword and Get-StandingJoinPassword' {
    BeforeAll {
        $savedEnv = [System.Environment]::GetEnvironmentVariable('NIMBY_JOIN_PASSWORD', 'Process')
    }
    AfterAll {
        [System.Environment]::SetEnvironmentVariable('NIMBY_JOIN_PASSWORD', $savedEnv, 'Process')
    }
    BeforeEach {
        # A fresh fake repo and profile for every case; the real environment value never applies.
        [System.Environment]::SetEnvironmentVariable('NIMBY_JOIN_PASSWORD', $null, 'Process')
        $root = Join-Path $TestDrive ([guid]::NewGuid().ToString('N'))
        $profileDir = Join-Path $root 'profiles\p'
        New-Item -ItemType Directory -Path $profileDir, (Join-Path $root 'scripts') -Force | Out-Null
        $savedFile = Join-Path $profileDir 'join-password.txt'
        $localFile = Join-Path $root 'scripts\testserver.local.json'
    }

    It 'saves a random password to a reusable profile and reuses it on the next run (the October 19 bug)' {
        $first = Resolve-JoinPassword -ProfilePath $profileDir -UseProfile $true -Standing (Get-StandingJoinPassword -RepoRoot $root)
        $first.Source | Should -Be 'random'
        Test-Path -LiteralPath $savedFile | Should -BeTrue
        $second = Resolve-JoinPassword -ProfilePath $profileDir -UseProfile $true -Standing (Get-StandingJoinPassword -RepoRoot $root)
        $second.Source | Should -Be 'profile'
        $second.Password | Should -Be $first.Password
    }

    It 'writes nothing without a reusable profile' {
        $result = Resolve-JoinPassword -ProfilePath $profileDir -UseProfile $false -Standing $null
        $result.Source | Should -Be 'random'
        $result.Password | Should -Match '^[a-z2-9]{12}$'
        Test-Path -LiteralPath $savedFile | Should -BeFalse
    }

    It 'lets -JoinPassword win over a saved value and saves it' {
        Set-Content -LiteralPath $savedFile -Value 'savedDummy123' -Encoding ascii -NoNewline
        $result = Resolve-JoinPassword -Requested 'paramDummy99' -ProfilePath $profileDir -UseProfile $true -Standing $null
        $result.Source | Should -Be 'parameter'
        Get-Content -LiteralPath $savedFile -Raw | Should -Be 'paramDummy99'
    }

    It 'ignores a malformed saved value with a warning' {
        Set-Content -LiteralPath $savedFile -Value 'has space' -Encoding ascii -NoNewline
        $result = Resolve-JoinPassword -ProfilePath $profileDir -UseProfile $true -Standing $null -WarningVariable warned -WarningAction SilentlyContinue
        $result.Source | Should -Be 'random'
        @($warned).Count | Should -Be 1
    }

    It 'treats an empty saved file as absent under strict mode' {
        Set-Content -LiteralPath $savedFile -Value '' -Encoding ascii -NoNewline
        (Resolve-JoinPassword -ProfilePath $profileDir -UseProfile $true -Standing $null -WarningAction SilentlyContinue).Source | Should -Be 'random'
    }

    It 'uses the standing value from the local file and never saves it to the profile' {
        Set-Content -LiteralPath $localFile -Value '{ "joinPassword": "localDummy12" }' -Encoding ascii
        $result = Resolve-JoinPassword -ProfilePath $profileDir -UseProfile $true -Standing (Get-StandingJoinPassword -RepoRoot $root)
        $result.Source | Should -Be 'local-file'
        $result.Password | Should -Be 'localDummy12'
        Test-Path -LiteralPath $savedFile | Should -BeFalse
    }

    It 'prefers the environment variable over the local file' {
        Set-Content -LiteralPath $localFile -Value '{ "joinPassword": "localDummy12" }' -Encoding ascii
        [System.Environment]::SetEnvironmentVariable('NIMBY_JOIN_PASSWORD', 'envDummy4567', 'Process')
        (Get-StandingJoinPassword -RepoRoot $root).Source | Should -Be 'env'
    }

    It 'prefers a value saved in the profile over the standing value' {
        Set-Content -LiteralPath $savedFile -Value 'savedDummy123' -Encoding ascii -NoNewline
        [System.Environment]::SetEnvironmentVariable('NIMBY_JOIN_PASSWORD', 'envDummy4567', 'Process')
        (Resolve-JoinPassword -ProfilePath $profileDir -UseProfile $true -Standing (Get-StandingJoinPassword -RepoRoot $root)).Source | Should -Be 'profile'
    }

    It 'skips an invalid standing value or a broken local file with a warning' {
        [System.Environment]::SetEnvironmentVariable('NIMBY_JOIN_PASSWORD', 'bad;value', 'Process')
        Set-Content -LiteralPath $localFile -Value '{ not json' -Encoding ascii
        $standing = Get-StandingJoinPassword -RepoRoot $root -WarningVariable warned -WarningAction SilentlyContinue 3>$null
        $standing | Should -BeNullOrEmpty
        @($warned).Count | Should -Be 2
        ($warned -join ' ') | Should -Not -Match 'bad;value'
    }

    It 'returns nothing when neither standing source is set' {
        Get-StandingJoinPassword -RepoRoot $root | Should -BeNullOrEmpty
    }
}
