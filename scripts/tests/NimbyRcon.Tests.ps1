# Pester 5 tests for scripts\modules\NimbyRcon.psm1: the CRC-32, the BERCon packet format, the
# loopback-only BattlEye config, and the password redaction. No server or network is used.

BeforeAll {
    . (Join-Path $PSScriptRoot 'TestHelpers.ps1')
    $repo = Get-TestRepoRoot
    Import-Module (Join-Path $repo 'scripts\modules\NimbyCommon.psm1') -Force
    Import-Module (Join-Path $repo 'scripts\modules\NimbyRcon.psm1') -Force
}

Describe 'Get-NimbyCrc32' {
    It 'matches the standard check value: "123456789" is 0xCBF43926' {
        Get-NimbyCrc32 -Bytes ([System.Text.Encoding]::ASCII.GetBytes('123456789')) | Should -Be ([uint32]3421780262)
    }

    It 'is 0 for no bytes' {
        Get-NimbyCrc32 -Bytes @() | Should -Be ([uint32]0)
    }
}

Describe 'ConvertTo-NimbyRconPacket and ConvertFrom-NimbyRconPacket' {
    It 'round-trips a command packet' {
        $payload = [byte[]](@(0) + [System.Text.Encoding]::ASCII.GetBytes('#shutdown'))
        $packet = ConvertTo-NimbyRconPacket -Type 1 -Payload $payload
        $packet[0] | Should -Be 0x42
        $packet[1] | Should -Be 0x45
        $packet[6] | Should -Be 0xFF
        $parsed = ConvertFrom-NimbyRconPacket -Bytes $packet
        $parsed.Valid | Should -BeTrue
        $parsed.Type | Should -Be 1
        [System.Text.Encoding]::ASCII.GetString([byte[]]$parsed.Payload[1..($parsed.Payload.Length - 1)]) | Should -Be '#shutdown'
    }

    It 'rejects a bad header' {
        $packet = ConvertTo-NimbyRconPacket -Type 0 -Payload ([System.Text.Encoding]::ASCII.GetBytes('pw'))
        $packet[0] = 0x41
        $parsed = ConvertFrom-NimbyRconPacket -Bytes $packet
        $parsed.Valid | Should -BeFalse
        $parsed.Reason | Should -Be 'bad header'
    }

    It 'rejects a packet whose CRC does not match' {
        $packet = ConvertTo-NimbyRconPacket -Type 0 -Payload ([System.Text.Encoding]::ASCII.GetBytes('pw'))
        $packet[$packet.Length - 1] = $packet[$packet.Length - 1] -bxor 0x01
        (ConvertFrom-NimbyRconPacket -Bytes $packet).Reason | Should -Be 'crc mismatch'
    }

    It 'rejects a packet that is too short' {
        (ConvertFrom-NimbyRconPacket -Bytes ([byte[]]@(0x42, 0x45, 0, 0))).Valid | Should -BeFalse
    }
}

Describe 'BattlEye config' {
    It 'binds RCON to loopback only' {
        $lines = Get-NimbyBeServerConfigLine -Password 'Dummy12345' -Port 2305
        $lines | Should -Contain 'RConIP 127.0.0.1'
        $lines | Should -Contain 'RConPort 2305'
    }

    It 'redacts the password in both BEServer_x64.cfg and the renamed active file' {
        $profileDir = Join-Path $TestDrive 'profile'
        $path = Write-NimbyBeServerConfig -ProfilePath $profileDir -Password 'Dummy12345' -Port 2305
        Copy-Item -LiteralPath $path -Destination (Join-Path $profileDir 'BattlEye\BEServer_x64_active_1a2b.cfg')
        $cleared = @(Clear-NimbyBeServerConfig -ProfilePath $profileDir)
        $cleared.Count | Should -Be 2
        foreach ($file in $cleared) {
            $text = Get-Content -LiteralPath $file -Raw
            $text | Should -Not -Match 'Dummy12345'
            $text | Should -Match 'RConPassword REDACTED'
        }
    }

    It 'renames a stale active file instead of deleting it' {
        $profileDir = Join-Path $TestDrive 'stale'
        New-Item -ItemType Directory -Path (Join-Path $profileDir 'BattlEye') -Force | Out-Null
        Set-Content -LiteralPath (Join-Path $profileDir 'BattlEye\BEServer_x64_active_9f.cfg') -Value 'x' -Encoding ascii
        $moved = @(Move-NimbyStaleBeServerConfig -ProfilePath $profileDir -Stamp '20261021-120000')
        $moved.Count | Should -Be 1
        $moved[0] | Should -BeLike '*BEServer_x64_active_9f.cfg.20261021-120000.old'
        Test-Path -LiteralPath $moved[0] | Should -BeTrue
    }

    It 'makes RCON passwords of letters and digits only' {
        $password = Get-NimbyRconPassword
        $password | Should -Match '^[A-Za-z0-9]{20}$'
    }
}
