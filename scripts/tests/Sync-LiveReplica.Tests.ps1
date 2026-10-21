# Pester 5 tests for scripts\Sync-LiveReplica.ps1: robocopy arguments and exit-code bands, the
# exclusion lists by name, and real robocopy runs inside $TestDrive that prove storage_1 and the
# BattlEye and serverDZ configs never reach the replica and that -WhatIf changes nothing.

BeforeAll {
    . (Join-Path $PSScriptRoot 'TestHelpers.ps1')
    $repo = Get-TestRepoRoot
    $syncScript = Join-Path $repo 'scripts\Sync-LiveReplica.ps1'
    Import-Module (Join-Path $repo 'scripts\modules\NimbyCommon.psm1') -Force
    . (Get-ScriptFunctionBlock -Path $syncScript -Name 'Get-RobocopyVerdict', 'Get-RobocopyArgumentList', 'Invoke-RobocopyStep', 'Get-MissionTemplate')
    $binaryExcludeDirs = Get-ScriptAssignmentValue -Path $syncScript -Name 'BinaryExcludeDirs'
    $binaryExcludeFiles = Get-ScriptAssignmentValue -Path $syncScript -Name 'BinaryExcludeFiles'

    function Get-TreeSnapshot {
        # Relative path and SHA-256 of every file, plus every folder, sorted: equal snapshots mean
        # an identical tree.
        param([string]$Root)
        if (-not (Test-Path -LiteralPath $Root)) { return @() }
        $items = Get-ChildItem -LiteralPath $Root -Recurse -Force | Sort-Object FullName
        return @($items | ForEach-Object {
                $relative = $_.FullName.Substring($Root.Length)
                if ($_.PSIsContainer) { "D $relative" } else { "F $relative $((Get-FileHash -LiteralPath $_.FullName).Hash)" }
            })
    }
}

Describe 'Get-RobocopyVerdict' {
    It 'maps exit <Code> to <Verdict>' -ForEach @(
        @{ Code = 0; Verdict = 'OK' }, @{ Code = 3; Verdict = 'OK' }, @{ Code = 4; Verdict = 'WARN' }, @{ Code = 7; Verdict = 'WARN' },
        @{ Code = 8; Verdict = 'PARTIAL' }, @{ Code = 15; Verdict = 'PARTIAL' }, @{ Code = 16; Verdict = 'FAIL' }, @{ Code = 99; Verdict = 'FAIL' }, @{ Code = -1; Verdict = 'FAIL' }
    ) {
        Get-RobocopyVerdict -ExitCode $Code | Should -Be $Verdict
    }
}

Describe 'Get-RobocopyArgumentList' {
    It 'uses /E by default and /MIR only with -Mirror' {
        $plain = Get-RobocopyArgumentList -Source 'S' -Destination 'D' -LogPath 'L'
        $plain | Should -Contain '/E'
        $plain | Should -Not -Contain '/MIR'
        Get-RobocopyArgumentList -Source 'S' -Destination 'D' -LogPath 'L' -Mirror | Should -Contain '/MIR'
    }

    It 'adds /L only with -ListOnly' {
        Get-RobocopyArgumentList -Source 'S' -Destination 'D' -LogPath 'L' | Should -Not -Contain '/L'
        Get-RobocopyArgumentList -Source 'S' -Destination 'D' -LogPath 'L' -ListOnly | Should -Contain '/L'
    }

    It 'trims a trailing backslash so robocopy does not read it as an escaped quote' {
        $arguments = Get-RobocopyArgumentList -Source 'C:\S\' -Destination 'C:\D\' -LogPath 'L'
        $arguments[0] | Should -Be 'C:\S'
        $arguments[1] | Should -Be 'C:\D'
    }
}

Describe 'Exclusion lists' {
    It 'keeps runtime state, secrets, and Steam bookkeeping out of the binary copy' {
        foreach ($dir in 'mpmissions', 'HostHavocDayZServer', 'profiles', 'logs', 'Backups', 'steamapps') {
            $binaryExcludeDirs | Should -Contain $dir
        }
    }

    It 'never copies a serverDZ*.cfg, a BattlEye config, or a ban or whitelist file' {
        foreach ($file in 'serverDZ*.cfg', '*.cfg.bak', 'BEServer*.cfg', 'beserver*.cfg', 'ban.txt', 'bans.txt', 'whitelist.txt') {
            $binaryExcludeFiles | Should -Contain $file
        }
    }

    It 'excludes every storage folder from the mission copy' {
        (Get-Content -LiteralPath $syncScript -Raw) | Should -Match ([regex]::Escape("-Step 'mission'") + '.*' + [regex]::Escape("-ExcludeDirs @('storage_1', 'storage_998', 'storage_999')"))
    }
}

Describe 'Robocopy runs in TestDrive' {
    BeforeAll {
        $logDir = Join-Path $TestDrive 'logs'
        New-Item -ItemType Directory -Path $logDir -Force | Out-Null
        $manifest = Join-Path $logDir 'manifest.txt'

        $mirror = Join-Path $TestDrive 'mirror'
        $mirrorMission = Join-Path $mirror 'mpmissions\dayzOffline.nasdara'
        New-Item -ItemType Directory -Path (Join-Path $mirrorMission 'db'), (Join-Path $mirrorMission 'storage_1\data'), (Join-Path $mirror 'battleye'), (Join-Path $mirror 'HostHavocDayZServer') -Force | Out-Null
        Set-Content -LiteralPath (Join-Path $mirrorMission 'init.c') -Value 'void main() {}' -Encoding ascii
        Set-Content -LiteralPath (Join-Path $mirrorMission 'db\types.xml') -Value '<types/>' -Encoding ascii
        Set-Content -LiteralPath (Join-Path $mirrorMission 'storage_1\data\players.db') -Value 'live' -Encoding ascii
        Set-Content -LiteralPath (Join-Path $mirror 'DayZServer_x64.exe') -Value 'exe' -Encoding ascii
        Set-Content -LiteralPath (Join-Path $mirror 'battleye\BEServer_x64.dll') -Value 'dll' -Encoding ascii
        Set-Content -LiteralPath (Join-Path $mirror 'battleye\BEServer_x64.cfg') -Value 'RConPassword live' -Encoding ascii
        Set-Content -LiteralPath (Join-Path $mirror 'serverDZ.cfg') -Value 'passwordAdmin = "live";' -Encoding ascii
        Set-Content -LiteralPath (Join-Path $mirror 'serverDZ_By_HostHavoc.cfg') -Value 'template="dayzOffline.nasdara";' -Encoding ascii
        Set-Content -LiteralPath (Join-Path $mirror 'serverDZ_old.cfg') -Value 'passwordAdmin = "old";' -Encoding ascii
        Set-Content -LiteralPath (Join-Path $mirror 'HostHavocDayZServer\server.RPT') -Value 'rpt' -Encoding ascii
    }

    It 'reads the mission template from the HostHavoc config' {
        Get-MissionTemplate -ConfigPath (Join-Path $mirror 'serverDZ_By_HostHavoc.cfg') | Should -Be 'dayzOffline.nasdara'
    }

    It 'copies mission files but no storage folder, and keeps the replica''s own storage_998' {
        $replicaMission = Join-Path $TestDrive 'replica\mpmissions\dayzOffline.nasdara'
        New-Item -ItemType Directory -Path (Join-Path $replicaMission 'storage_998') -Force | Out-Null
        Set-Content -LiteralPath (Join-Path $replicaMission 'storage_998\keep.bin') -Value 'test world' -Encoding ascii
        $verdict = Invoke-RobocopyStep -Step 'mission' -Source $mirrorMission -Destination $replicaMission -LogDir $logDir -ManifestPath $manifest -ExcludeDirs @('storage_1', 'storage_998', 'storage_999')
        $verdict | Should -Be 'OK'
        Test-Path -LiteralPath (Join-Path $replicaMission 'init.c') | Should -BeTrue
        Test-Path -LiteralPath (Join-Path $replicaMission 'db\types.xml') | Should -BeTrue
        Test-Path -LiteralPath (Join-Path $replicaMission 'storage_1') | Should -BeFalse
        Test-Path -LiteralPath (Join-Path $replicaMission 'storage_998\keep.bin') | Should -BeTrue
    }

    It 'copies binaries without any serverDZ*.cfg, BattlEye config, profile folder, or mission' {
        $replica = Join-Path $TestDrive 'replica-binaries'
        $verdict = Invoke-RobocopyStep -Step 'binaries' -Source $mirror -Destination $replica -LogDir $logDir -ManifestPath $manifest -ExcludeDirs $binaryExcludeDirs -ExcludeFiles $binaryExcludeFiles
        $verdict | Should -Be 'OK'
        Test-Path -LiteralPath (Join-Path $replica 'DayZServer_x64.exe') | Should -BeTrue
        Test-Path -LiteralPath (Join-Path $replica 'battleye\BEServer_x64.dll') | Should -BeTrue
        Test-Path -LiteralPath (Join-Path $replica 'battleye\BEServer_x64.cfg') | Should -BeFalse
        Test-Path -LiteralPath (Join-Path $replica 'serverDZ.cfg') | Should -BeFalse
        Test-Path -LiteralPath (Join-Path $replica 'serverDZ_By_HostHavoc.cfg') | Should -BeFalse
        Test-Path -LiteralPath (Join-Path $replica 'serverDZ_old.cfg') | Should -BeFalse
        Test-Path -LiteralPath (Join-Path $replica 'HostHavocDayZServer') | Should -BeFalse
        Test-Path -LiteralPath (Join-Path $replica 'mpmissions') | Should -BeFalse
    }

    It 'changes nothing in a list-only (-WhatIf) run, even with /MIR' {
        $replica = Join-Path $TestDrive 'replica-whatif'
        New-Item -ItemType Directory -Path (Join-Path $replica 'storage_998') -Force | Out-Null
        Set-Content -LiteralPath (Join-Path $replica 'storage_998\keep.bin') -Value 'would be removed by /MIR' -Encoding ascii
        $before = Get-TreeSnapshot -Root $replica
        $verdict = Invoke-RobocopyStep -Step 'whatif' -Source (Join-Path $mirrorMission 'storage_1') -Destination $replica -LogDir $logDir -ManifestPath $manifest -Mirror -ListOnly
        $verdict | Should -Not -Be 'FAIL'
        Get-TreeSnapshot -Root $replica | Should -Be $before
        (Get-Content -LiteralPath $manifest -Raw) | Should -Match 'whatif exit=\d+ mode=list-only'
    }
}
