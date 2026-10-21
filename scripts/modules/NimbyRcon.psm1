#Requires -Version 5.1
<#
.SYNOPSIS
    Minimal BattlEye RCon (BERCon v2) client for stopping a local DayZ server gracefully.

.DESCRIPTION
    Implements enough of the BattlEye RCon protocol to log in and send one command, which is
    what Start-NimbyTestServer.ps1 needs to issue "#shutdown" before it falls back to stopping
    the process. Packet layout (BERConProtocol.txt, BattlEye 2011):

        'B' 'E' | CRC32 of the following bytes, little-endian | 0xFF | type | payload
        type 0x00 login:    password ASCII            -> reply 0x00 then 0x01 (ok) or 0x00 (bad)
        type 0x01 command:  seq byte, command ASCII   -> reply 0x01 seq [text]
        type 0x02 message:  seq byte, text            -> client must acknowledge with 0x02 seq

    Also writes the throwaway BEServer_x64.cfg the server reads from <profiles>\BattlEye.

    Reads   : <profile>\BattlEye\BEServer_x64*.cfg (Clear-NimbyBeServerConfig, to redact).
    Writes  : <profile>\BattlEye\BEServer_x64.cfg (Write-NimbyBeServerConfig); rewrites every
              <profile>\BattlEye\BEServer_x64*.cfg with the RCon password redacted
              (Clear-NimbyBeServerConfig); renames <profile>\BattlEye\BEServer_x64_active_*.cfg to
              <name>.<stamp>.old (Move-NimbyStaleBeServerConfig).
    Never   : deletes a file; logs or returns a password except through the parameter it was given.

    Runs in Windows PowerShell 5.1 and PowerShell 7. UDP only; never contacts anything but the
    address it is given.
#>

Set-StrictMode -Version Latest

# Get-NimbyRandomString. Reuses the copy the calling script already loaded.
Import-Module (Join-Path $PSScriptRoot 'NimbyCommon.psm1')

$script:Crc32Table = $null

function Get-NimbyCrc32 {
    # Standard CRC-32 (IEEE 802.3, reflected polynomial 0xEDB88320), the one zlib and BattlEye use.
    # Test vector: "123456789" -> 0xCBF43926.
    [OutputType([uint32])]
    param([Parameter(Mandatory)][AllowEmptyCollection()][byte[]]$Bytes)

    if ($null -eq $script:Crc32Table) {
        # Hex literals that fill 32 bits parse as negative Int32, so the constants are written in decimal.
        [uint32]$polynomial = 3988292384   # 0xEDB88320
        $table = New-Object 'uint32[]' 256
        for ($i = 0; $i -lt 256; $i++) {
            [uint32]$c = $i
            for ($k = 0; $k -lt 8; $k++) {
                if (($c -band 1) -ne 0) { $c = [uint32]($polynomial -bxor ($c -shr 1)) } else { $c = [uint32]($c -shr 1) }
            }
            $table[$i] = $c
        }
        $script:Crc32Table = $table
    }

    [uint32]$crc = [uint32]::MaxValue
    foreach ($b in $Bytes) {
        $crc = [uint32]($script:Crc32Table[($crc -bxor $b) -band 0xFF] -bxor ($crc -shr 8))
    }
    return [uint32]($crc -bxor [uint32]::MaxValue)
}

function ConvertTo-NimbyRconPacket {
    # Builds one BERCon packet. Payload is everything after the type byte.
    [OutputType([byte[]])]
    param(
        [Parameter(Mandatory)][ValidateSet(0, 1, 2)][int]$Type,
        [Parameter(Mandatory)][AllowEmptyCollection()][byte[]]$Payload
    )

    $body = New-Object 'System.Collections.Generic.List[byte]'
    $body.Add(0xFF)
    $body.Add([byte]$Type)
    foreach ($b in $Payload) { $body.Add($b) }
    $bodyBytes = $body.ToArray()

    $crc = [System.BitConverter]::GetBytes((Get-NimbyCrc32 -Bytes $bodyBytes))
    if (-not [System.BitConverter]::IsLittleEndian) { [array]::Reverse($crc) }

    $packet = New-Object 'System.Collections.Generic.List[byte]'
    $packet.Add(0x42)   # 'B'
    $packet.Add(0x45)   # 'E'
    foreach ($b in $crc) { $packet.Add($b) }
    foreach ($b in $bodyBytes) { $packet.Add($b) }
    return $packet.ToArray()
}

function ConvertFrom-NimbyRconPacket {
    # Splits a received datagram into Type, Payload, and whether its CRC and header are valid.
    param([Parameter(Mandatory)][AllowEmptyCollection()][byte[]]$Bytes)

    $result = [PSCustomObject]@{ Valid = $false; Type = -1; Payload = [byte[]]@(); Reason = '' }
    if ($Bytes.Length -lt 8) { $result.Reason = "too short ($($Bytes.Length) bytes)"; return $result }
    if ($Bytes[0] -ne 0x42 -or $Bytes[1] -ne 0x45 -or $Bytes[6] -ne 0xFF) { $result.Reason = 'bad header'; return $result }

    $crcBytes = $Bytes[2..5]
    if (-not [System.BitConverter]::IsLittleEndian) { [array]::Reverse($crcBytes) }
    $expected = [System.BitConverter]::ToUInt32([byte[]]$crcBytes, 0)
    $actual = Get-NimbyCrc32 -Bytes ([byte[]]$Bytes[6..($Bytes.Length - 1)])
    if ($expected -ne $actual) { $result.Reason = 'crc mismatch'; return $result }

    $result.Valid = $true
    $result.Type = [int]$Bytes[7]
    if ($Bytes.Length -gt 8) { $result.Payload = [byte[]]$Bytes[8..($Bytes.Length - 1)] }
    return $result
}

function Get-NimbyRconPassword {
    # Random RCon password from the alphabet BattlEye accepts (letters and digits, max 32).
    param([ValidateRange(8, 32)][int]$Length = 20)
    return (Get-NimbyRandomString -Alphabet 'ABCDEFGHJKLMNPQRSTUVWXYZabcdefghjkmnpqrstuvwxyz23456789' -Length $Length)
}

function Get-NimbyBeServerConfigLine {
    # Returns the lines of a throwaway BEServer_x64.cfg. BattlEye reads this file in plaintext;
    # the analyzer's SecureString rule cannot apply.
    [Diagnostics.CodeAnalysis.SuppressMessageAttribute('PSAvoidUsingPlainTextForPassword', 'Password', Justification = 'BEServer_x64.cfg requires plaintext; throwaway local test value.')]
    param(
        [Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9]{1,32}$')][string]$Password,
        [Parameter(Mandatory)][ValidateRange(1, 65535)][int]$Port
    )

    # RConIP binds the RCON socket to loopback: Send-NimbyRconCommand only ever talks to
    # 127.0.0.1, and a test server that an outside player reaches over a tunnel or a forwarded
    # game port must never answer RCON on that path (docs/operations/replica-remote-tester.md).
    return @(
        "RConPassword $Password",
        "RConPort $Port",
        'RConIP 127.0.0.1',
        'RestrictRCon 0'
    )
}

function Write-NimbyBeServerConfig {
    # Writes BEServer_x64.cfg into <profiles>\BattlEye, which is where the DayZ server on Windows
    # keeps BEServer_x64.dll and reads its config (observed in the October 14, 2026 boot checks).
    [Diagnostics.CodeAnalysis.SuppressMessageAttribute('PSAvoidUsingPlainTextForPassword', 'Password', Justification = 'BEServer_x64.cfg requires plaintext; throwaway local test value.')]
    param(
        [Parameter(Mandatory)][string]$ProfilePath,
        [Parameter(Mandatory)][string]$Password,
        [Parameter(Mandatory)][int]$Port
    )

    $beDir = Join-Path $ProfilePath 'BattlEye'
    New-Item -ItemType Directory -Path $beDir -Force | Out-Null
    $path = Join-Path $beDir 'BEServer_x64.cfg'
    Set-Content -LiteralPath $path -Value (Get-NimbyBeServerConfigLine -Password $Password -Port $Port) -Encoding ascii
    return $path
}

function Move-NimbyStaleBeServerConfig {
    # Renames BEServer_x64_active_*.cfg left by an earlier run on a reused profile to
    # <name>.<stamp>.old so only the freshly written BEServer_x64.cfg is a candidate for the
    # server. Renames, never deletes. Returns the new paths.
    param(
        [Parameter(Mandatory)][string]$ProfilePath,
        [Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9-]+$')][string]$Stamp
    )

    $beDir = Join-Path $ProfilePath 'BattlEye'
    if (-not (Test-Path -LiteralPath $beDir)) { return @() }
    $moved = @()
    foreach ($file in Get-ChildItem -LiteralPath $beDir -File -Filter 'BEServer_x64_active_*.cfg') {
        $target = "{0}.{1}.old" -f $file.FullName, $Stamp
        Move-Item -LiteralPath $file.FullName -Destination $target
        $moved += $target
    }
    return $moved
}

function Clear-NimbyBeServerConfig {
    # Redacts the RCon password from every BEServer_x64*.cfg in <profiles>\BattlEye. The server
    # renames the file to BEServer_x64_active_<id>.cfg while it runs, so both names are covered.
    param([Parameter(Mandatory)][string]$ProfilePath)

    $beDir = Join-Path $ProfilePath 'BattlEye'
    if (-not (Test-Path -LiteralPath $beDir)) { return @() }
    $cleared = @()
    foreach ($file in Get-ChildItem -LiteralPath $beDir -File -Filter 'BEServer_x64*.cfg') {
        $lines = @(Get-Content -LiteralPath $file.FullName -Encoding ascii) -replace '^(RConPassword)\s+.*$', '$1 REDACTED'
        Set-Content -LiteralPath $file.FullName -Value $lines -Encoding ascii
        $cleared += $file.FullName
    }
    return $cleared
}

function Receive-NimbyRconPacket {
    # Waits for one valid packet of the wanted type, acknowledging server messages on the way.
    param(
        [Parameter(Mandatory)][System.Net.Sockets.UdpClient]$Client,
        [Parameter(Mandatory)][int]$WantedType,
        [Parameter(Mandatory)][int]$TimeoutMilliseconds
    )

    $deadline = (Get-Date).AddMilliseconds($TimeoutMilliseconds)
    while ((Get-Date) -lt $deadline) {
        $remaining = [int][math]::Max(1, ($deadline - (Get-Date)).TotalMilliseconds)
        $Client.Client.ReceiveTimeout = $remaining
        $endpoint = New-Object System.Net.IPEndPoint ([System.Net.IPAddress]::Any, 0)
        try {
            $data = $Client.Receive([ref]$endpoint)
        }
        catch [System.Net.Sockets.SocketException] {
            return $null
        }
        $packet = ConvertFrom-NimbyRconPacket -Bytes $data
        if (-not $packet.Valid) { continue }
        if ($packet.Type -eq 2 -and $packet.Payload.Length -ge 1) {
            # Server message: acknowledge so the server keeps us authenticated.
            [void]$Client.Send((ConvertTo-NimbyRconPacket -Type 2 -Payload @($packet.Payload[0])), 9)
            continue
        }
        if ($packet.Type -eq $WantedType) { return $packet }
    }
    return $null
}

function Send-NimbyRconCommand {
    # Logs in and sends one command. Returns LoggedIn, Sent, Response (text or empty), Reason.
    [Diagnostics.CodeAnalysis.SuppressMessageAttribute('PSAvoidUsingPlainTextForPassword', 'Password', Justification = 'BERCon sends the password as ASCII bytes; no SecureString transport exists.')]
    param(
        [string]$Address = '127.0.0.1',
        [Parameter(Mandatory)][int]$Port,
        [Parameter(Mandatory)][string]$Password,
        [Parameter(Mandatory)][string]$Command,
        [int]$TimeoutSeconds = 5,
        [int]$LoginAttempts = 3
    )

    $result = [PSCustomObject]@{ LoggedIn = $false; Sent = $false; Response = ''; Reason = '' }
    $client = New-Object System.Net.Sockets.UdpClient
    try {
        $client.Connect($Address, $Port)
        $loginPacket = ConvertTo-NimbyRconPacket -Type 0 -Payload ([System.Text.Encoding]::ASCII.GetBytes($Password))
        for ($attempt = 1; $attempt -le $LoginAttempts -and -not $result.LoggedIn; $attempt++) {
            [void]$client.Send($loginPacket, $loginPacket.Length)
            $reply = Receive-NimbyRconPacket -Client $client -WantedType 0 -TimeoutMilliseconds ($TimeoutSeconds * 1000)
            if ($null -eq $reply) { $result.Reason = "no login reply after $attempt attempt(s)"; continue }
            if ($reply.Payload.Length -ge 1 -and $reply.Payload[0] -eq 1) { $result.LoggedIn = $true; $result.Reason = '' }
            else { $result.Reason = 'login rejected'; break }
        }
        if (-not $result.LoggedIn) { return $result }

        $commandBytes = New-Object 'System.Collections.Generic.List[byte]'
        $commandBytes.Add(0)   # sequence number 0: first and only command on this login
        foreach ($b in [System.Text.Encoding]::ASCII.GetBytes($Command)) { $commandBytes.Add($b) }
        $commandPacket = ConvertTo-NimbyRconPacket -Type 1 -Payload $commandBytes.ToArray()
        [void]$client.Send($commandPacket, $commandPacket.Length)
        $result.Sent = $true

        $reply = Receive-NimbyRconPacket -Client $client -WantedType 1 -TimeoutMilliseconds ($TimeoutSeconds * 1000)
        if ($null -ne $reply -and $reply.Payload.Length -gt 1) {
            $result.Response = [System.Text.Encoding]::ASCII.GetString([byte[]]$reply.Payload[1..($reply.Payload.Length - 1)])
        }
        elseif ($null -eq $reply) {
            # A server that shuts down on the command may never answer. Not an error by itself.
            $result.Reason = 'no command reply'
        }
        return $result
    }
    catch {
        $result.Reason = $_.Exception.Message
        return $result
    }
    finally {
        # Close() is public on every .NET the two shells run on; Dispose() is public only from
        # .NET Framework 4.6, and a throw here would skip the caller's Stop-Process and redaction.
        $client.Close()
    }
}

Export-ModuleMember -Function Get-NimbyCrc32, ConvertTo-NimbyRconPacket, ConvertFrom-NimbyRconPacket, Get-NimbyRconPassword, Get-NimbyBeServerConfigLine, Write-NimbyBeServerConfig, Move-NimbyStaleBeServerConfig, Clear-NimbyBeServerConfig, Send-NimbyRconCommand
