# Pester 5 tests for scripts\modules\NimbyCommon.psm1: the instance guard, the server argument
# list, the log reader, the path guard, the running-server guard, the random generator, and the
# local settings file.

BeforeAll {
    . (Join-Path $PSScriptRoot 'TestHelpers.ps1')
    $repo = Get-TestRepoRoot
    Import-Module (Join-Path $repo 'scripts\modules\NimbyCommon.psm1') -Force
}

Describe 'Test-TestInstanceId' {
    It 'accepts only 998 and 999' {
        Test-TestInstanceId -InstanceId 998 | Should -BeTrue
        Test-TestInstanceId -InstanceId 999 | Should -BeTrue
        Test-TestInstanceId -InstanceId 1 | Should -BeFalse
        Test-TestInstanceId -InstanceId 997 | Should -BeFalse
    }
}

Describe 'Get-ServerArgument' {
    It 'joins several mod folders into one -mod= and adds -servermod= only when given' {
        $arguments = Get-ServerArgument -ConfigPath 'c.cfg' -ProfilePath 'p' -ModPath 'A B', 'C' -ServerModPath 'S' -Port 2402
        $arguments | Should -Contain '-mod=A B;C'
        $arguments | Should -Contain '-servermod=S'
        $arguments | Should -Contain '-port=2402'
    }

    It 'passes no -mod= at all for the vanilla control boot' {
        $arguments = Get-ServerArgument -ConfigPath 'c.cfg' -ProfilePath 'p' -Port 2302
        @($arguments | Where-Object { $_ -like '-mod=*' -or $_ -like '-servermod=*' }).Count | Should -Be 0
    }
}

Describe 'Read-ServerLogText' {
    BeforeAll {
        $profileDir = Join-Path $TestDrive 'profile'
        New-Item -ItemType Directory -Path $profileDir -Force | Out-Null
        Set-Content -LiteralPath (Join-Path $profileDir 'DayZServer_x64_1.RPT') -Value 'rpt line' -Encoding ascii
        Set-Content -LiteralPath (Join-Path $profileDir 'script_1.log') -Value 'script line' -Encoding ascii
        Set-Content -LiteralPath (Join-Path $profileDir 'other.log') -Value 'other line' -Encoding ascii
        $old = Join-Path $profileDir 'script_0.log'
        Set-Content -LiteralPath $old -Value 'old run' -Encoding ascii
        (Get-Item -LiteralPath $old).LastWriteTime = (Get-Date).AddDays(-1)
    }

    It 'reads the RPT and the script logs, never other logs' {
        $text = Read-ServerLogText -ProfilePath $profileDir
        $text | Should -Match 'rpt line'
        $text | Should -Match 'script line'
        $text | Should -Not -Match 'other line'
    }

    It 'reads one kind when asked' {
        $script = Read-ServerLogText -ProfilePath $profileDir -Kind Script
        $script | Should -Match 'script line'
        $script | Should -Not -Match 'rpt line'
        Read-ServerLogText -ProfilePath $profileDir -Kind Rpt | Should -Not -Match 'script line'
    }

    It 'skips files older than -Since' {
        Read-ServerLogText -ProfilePath $profileDir -Since (Get-Date).AddHours(-1) | Should -Not -Match 'old run'
    }

    It 'returns empty text for a missing folder' {
        Read-ServerLogText -ProfilePath (Join-Path $TestDrive 'nope') | Should -Be ''
    }
}

Describe 'Test-PathInside' {
    It 'is true for the folder itself and for a child, ignoring case, separators, and a trailing slash' {
        Test-PathInside -Candidate 'D:\DayZServer' -Root 'D:\DayZServer' | Should -BeTrue
        Test-PathInside -Candidate 'd:/dayzserver/mpmissions' -Root 'D:\DayZServer\' | Should -BeTrue
    }

    It 'is false for a sibling that shares the prefix' {
        Test-PathInside -Candidate 'D:\DayZServer2\x' -Root 'D:\DayZServer' | Should -BeFalse
        Test-PathInside -Candidate 'D:\Other' -Root 'D:\DayZServer' | Should -BeFalse
    }
}

Describe 'Get-RunningServerMessage' {
    It 'is empty when no DayZServer_x64 runs' {
        Mock -ModuleName NimbyCommon Get-Process { @() }
        Get-RunningServerMessage | Should -BeNullOrEmpty
    }

    It 'names the pid of a running server, and honors -Root' {
        Mock -ModuleName NimbyCommon Get-Process { [pscustomobject]@{ Id = 4242; Path = 'D:\DayZServer\DayZServer_x64.exe' } }
        Get-RunningServerMessage | Should -Match 'pid 4242'
        Get-RunningServerMessage -Root 'D:\DayZServer' | Should -Match 'running from D:\\DayZServer'
        Get-RunningServerMessage -Root 'F:\Elsewhere' | Should -BeNullOrEmpty
    }
}

Describe 'Get-NimbyRandomString' {
    It 'returns the requested length from the alphabet only' {
        $value = Get-NimbyRandomString -Alphabet 'abc' -Length 200
        $value.Length | Should -Be 200
        $value | Should -Match '^[abc]+$'
    }

    It 'uses every character of a 55-character alphabet (no byte is mapped out of range)' {
        $alphabet = 'ABCDEFGHJKLMNPQRSTUVWXYZabcdefghjkmnpqrstuvwxyz23456789'
        $value = Get-NimbyRandomString -Alphabet $alphabet -Length 256
        foreach ($char in $value.ToCharArray()) { $alphabet.Contains([string]$char) | Should -BeTrue }
    }
}

Describe 'Get-NimbyLocalSetting' {
    BeforeEach {
        $root = Join-Path $TestDrive ([guid]::NewGuid().ToString('N'))
        New-Item -ItemType Directory -Path (Join-Path $root 'scripts') -Force | Out-Null
        $file = Join-Path $root 'scripts\testserver.local.json'
    }

    It 'returns nothing when the file is absent' {
        Get-NimbyLocalSetting -RepoRoot $root -Name 'sftpHost' | Should -BeNullOrEmpty
    }

    It 'returns a value, and nothing for an absent or empty key' {
        Set-Content -LiteralPath $file -Value '{ "sftpHost": "host.example", "sftpPort": 22, "sftpUser": "" }' -Encoding ascii
        Get-NimbyLocalSetting -RepoRoot $root -Name 'sftpHost' | Should -Be 'host.example'
        Get-NimbyLocalSetting -RepoRoot $root -Name 'sftpPort' | Should -Be 22
        Get-NimbyLocalSetting -RepoRoot $root -Name 'sftpUser' | Should -BeNullOrEmpty
        Get-NimbyLocalSetting -RepoRoot $root -Name 'missing' | Should -BeNullOrEmpty
    }

    It 'treats a file that is not JSON as absent and warns by file name only' {
        Set-Content -LiteralPath $file -Value '{ "joinPassword": "secretDummy1"' -Encoding ascii
        $value = Get-NimbyLocalSetting -RepoRoot $root -Name 'joinPassword' -WarningVariable warned -WarningAction SilentlyContinue 3>$null
        $value | Should -BeNullOrEmpty
        ($warned -join ' ') | Should -Match 'not valid JSON'
        ($warned -join ' ') | Should -Not -Match 'secretDummy1'
    }
}

Describe 'testserver.local.json stays out of git' {
    It 'is matched by a .gitignore rule' {
        $rules = Get-Content -LiteralPath (Join-Path $repo '.gitignore')
        $rules | Should -Contain '*.local.json'
    }
}
