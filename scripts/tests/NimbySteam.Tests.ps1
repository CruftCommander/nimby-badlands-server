# Pester 5 tests for scripts\modules\NimbySteam.psm1 against a fake Steam layout in $TestDrive.
# Never reads the real Steam install.

BeforeAll {
    . (Join-Path $PSScriptRoot 'TestHelpers.ps1')
    $repo = Get-TestRepoRoot
    Import-Module (Join-Path $repo 'scripts\modules\NimbySteam.psm1') -Force

    $steam = Join-Path $TestDrive 'Steam'
    $libraryB = Join-Path $TestDrive 'Library B'
    New-Item -ItemType Directory -Path (Join-Path $steam 'steamapps'), (Join-Path $libraryB 'steamapps') -Force | Out-Null
    # libraryfolders.vdf escapes backslashes; the Steam root is listed again with a trailing one.
    $vdf = @(
        '"libraryfolders"',
        '{',
        ('    "0" {{ "path" "{0}\\" }}' -f ($steam -replace '\\', '\\')),
        ('    "1" {{ "path" "{0}" }}' -f ($libraryB -replace '\\', '\\')),
        '}'
    )
    Set-Content -LiteralPath (Join-Path $steam 'steamapps\libraryfolders.vdf') -Value $vdf -Encoding utf8
}

Describe 'Get-SteamLibraryPath' {
    It 'returns the root and each library once, unescaped' {
        $libraries = @(Get-SteamLibraryPath -SteamRoot $steam)
        $libraries.Count | Should -Be 2
        $libraries[0] | Should -Be $steam
        $libraries[1] | Should -Be $libraryB
    }
}

Describe 'Find-SteamInstalledAppPath and Find-DayZServerPath' {
    BeforeAll {
        # DayZ Server installed under a non-default folder name in library B (app 223350).
        $serverDir = Join-Path $libraryB 'steamapps\common\DayZ Server Renamed'
        New-Item -ItemType Directory -Path $serverDir -Force | Out-Null
        Set-Content -LiteralPath (Join-Path $libraryB 'steamapps\appmanifest_223350.acf') -Value '"AppState" { "appid" "223350" "installdir" "DayZ Server Renamed" }' -Encoding utf8
    }

    It 'ignores a manifest whose install folder lacks the required file' {
        Find-SteamInstalledAppPath -LibraryPaths @($steam, $libraryB) -AppId '223350' -RequiredFile 'DayZServer_x64.exe' | Should -BeNullOrEmpty
    }

    It 'finds the install named by the manifest once the required file exists' {
        Set-Content -LiteralPath (Join-Path $serverDir 'DayZServer_x64.exe') -Value 'x' -Encoding ascii
        Find-SteamInstalledAppPath -LibraryPaths @($steam, $libraryB) -AppId '223350' -RequiredFile 'DayZServer_x64.exe' | Should -Be $serverDir
        Find-DayZServerPath -LibraryPaths @($steam, $libraryB) -Fallback @() | Should -Be $serverDir
    }

    It 'falls back to the replica folder when Steam has no server' {
        $replica = Join-Path $TestDrive 'DayZServer'
        New-Item -ItemType Directory -Path $replica -Force | Out-Null
        Set-Content -LiteralPath (Join-Path $replica 'DayZServer_x64.exe') -Value 'x' -Encoding ascii
        Find-DayZServerPath -LibraryPaths @() -Fallback @($replica) | Should -Be $replica
        Find-DayZServerPath -LibraryPaths @() -Fallback @(Join-Path $TestDrive 'missing') | Should -BeNullOrEmpty
    }
}
