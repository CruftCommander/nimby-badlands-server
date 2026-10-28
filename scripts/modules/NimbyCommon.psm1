#Requires -Version 5.1
<#
.SYNOPSIS
    Helpers shared by the NIMBY scripts: the manifest writer, the test-instance guard, the DayZ
    server argument list, the server log reader, the running-server guard, the random-string
    generator, and the git-ignored local settings file.

.DESCRIPTION
    One copy of each helper that Build-NimbyPack.ps1, Start-NimbyTestServer.ps1,
    Sync-LiveReplica.ps1, Get-ServerLogs.ps1, and Copy-WorkshopItem.ps1 used to define for
    themselves (code review of October 21, 2026, finding 22). Orchestrator-owned (shared rules,
    section 3).

    Reads   : the RPT and script logs in a server profile folder (Read-ServerLogText), the process
              list (Get-RunningServerProcess), and <repo>\scripts\testserver.local.json
              (Get-NimbyLocalSetting; git-ignored by *.local.json).
    Writes  : only the manifest file a caller names to Write-Manifest.
    Never   : writes a setting value, a password, or anything from testserver.local.json to a log
              or to the screen; starts or stops a process; deletes a file.

    Runs in Windows PowerShell 5.1 and PowerShell 7.
#>

Set-StrictMode -Version Latest
# Module functions do not inherit the calling script's preference, so set it here.
$ErrorActionPreference = 'Stop'

function Write-Manifest {
    # Appends one UTF-8 line to a manifest log. Never suppressed by -WhatIf: the manifest is the
    # record of the run, including a dry run.
    param(
        [Parameter(Mandatory)][string]$LogPath,
        [Parameter(Mandatory)][string]$Line
    )
    Add-Content -LiteralPath $LogPath -Value $Line -Encoding utf8 -WhatIf:$false
}

function Test-TestInstanceId {
    # Shared rules, section 8: test storage is storage_998 and storage_999 only.
    param([Parameter(Mandatory)][int]$InstanceId)
    return ($InstanceId -eq 998 -or $InstanceId -eq 999)
}

function Get-ServerArgument {
    # Builds the DayZ server argument list. Several -mod folders join with ';' inside one argument;
    # no -mod= at all when ModPath is empty (the vanilla control boot); -servermod= only when a
    # server-only output is given.
    param(
        [Parameter(Mandatory)][string]$ConfigPath,
        [Parameter(Mandatory)][string]$ProfilePath,
        [AllowEmptyCollection()][string[]]$ModPath = @(),
        [string]$ServerModPath,
        [Parameter(Mandatory)][int]$Port
    )

    $arguments = @(
        "-config=$ConfigPath",
        "-port=$Port",
        "-profiles=$ProfilePath"
    )
    if ($ModPath.Count -gt 0) { $arguments += ("-mod={0}" -f ($ModPath -join ';')) }
    if ($ServerModPath) { $arguments += "-servermod=$ServerModPath" }
    $arguments += '-dologs', '-adminlog', '-freezecheck'
    return $arguments
}

function Read-ServerLogText {
    # Concatenates the RPT and script logs in a server profile folder (Kind All), only the script
    # logs (Script), or only the RPTs (Rpt). With Since, only files written at or after that time,
    # so a reused profile's earlier runs stay out. Shared-read so a running server's open files
    # can be read.
    param(
        [Parameter(Mandatory)][string]$ProfilePath,
        [ValidateSet('All', 'Script', 'Rpt')][string]$Kind = 'All',
        [AllowNull()][Nullable[datetime]]$Since = $null
    )

    $builder = New-Object System.Text.StringBuilder
    if (-not (Test-Path -LiteralPath $ProfilePath)) { return '' }
    $files = Get-ChildItem -LiteralPath $ProfilePath -File | Where-Object {
        $isRpt = $_.Extension -ieq '.rpt'
        $isScript = $_.Name -like 'script*.log'
        $kindOk = ($Kind -eq 'All' -and ($isRpt -or $isScript)) -or ($Kind -eq 'Rpt' -and $isRpt) -or ($Kind -eq 'Script' -and $isScript)
        $kindOk -and ($null -eq $Since -or $_.LastWriteTime -ge $Since)
    }
    foreach ($file in $files) {
        $stream = New-Object System.IO.FileStream($file.FullName, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
        try {
            $reader = New-Object System.IO.StreamReader($stream)
            [void]$builder.AppendLine($reader.ReadToEnd())
        }
        finally {
            $stream.Dispose()
        }
    }
    return $builder.ToString()
}

function Test-PathInside {
    # True when Candidate equals Root or lies under it (case-insensitive, separator-normalized).
    [OutputType([bool])]
    param([Parameter(Mandatory)][string]$Candidate, [Parameter(Mandatory)][string]$Root)

    $c = $Candidate.Replace('/', '\').TrimEnd('\').ToLowerInvariant()
    $r = $Root.Replace('/', '\').TrimEnd('\').ToLowerInvariant()
    return ($c -eq $r) -or $c.StartsWith($r + '\')
}

function Get-RunningServerProcess {
    # Running DayZServer_x64 processes: all of them, or with Root only those whose image lives
    # under that folder.
    param([string]$Root)

    $processes = @(Get-Process -Name 'DayZServer_x64' -ErrorAction SilentlyContinue)
    if (-not $Root) { return $processes }
    return @($processes | Where-Object { $_.Path -and (Test-PathInside -Candidate $_.Path -Root $Root) })
}

function Get-RunningServerMessage {
    # $null when no DayZServer_x64 runs (under Root, when given); otherwise the refusal text with
    # the process ids. Only one DayZ server runs at a time: the boot check, the test server, and
    # the replica share ports and storage_998 and storage_999.
    param([string]$Root)

    $running = @(Get-RunningServerProcess -Root $Root)
    if ($running.Count -eq 0) { return $null }
    $where = 'already running'
    if ($Root) { $where = "running from $Root" }
    return ("A DayZServer_x64 process is {0} (pid {1}); only one DayZ server runs at a time. Stop it first." -f $where, (($running | ForEach-Object { $_.Id }) -join ','))
}

function Get-NimbyRandomString {
    # Random string from Alphabet with a cryptographic generator. Bytes at or above the largest
    # multiple of the alphabet size are drawn again, so every character is equally likely.
    param(
        [Parameter(Mandatory)][ValidateLength(2, 256)][string]$Alphabet,
        [Parameter(Mandatory)][ValidateRange(1, 256)][int]$Length
    )

    $limit = 256 - (256 % $Alphabet.Length)
    $buffer = New-Object byte[] 64
    $builder = New-Object System.Text.StringBuilder
    $rng = [System.Security.Cryptography.RandomNumberGenerator]::Create()
    try {
        while ($builder.Length -lt $Length) {
            $rng.GetBytes($buffer)
            foreach ($b in $buffer) {
                if ($b -ge $limit) { continue }
                [void]$builder.Append($Alphabet[$b % $Alphabet.Length])
                if ($builder.Length -eq $Length) { break }
            }
        }
    }
    finally {
        $rng.Dispose()
    }
    return $builder.ToString()
}

function Get-NimbyLocalSettingPath {
    # <repo>\scripts\testserver.local.json: per-machine values that never enter the repository
    # (git-ignored by *.local.json in .gitignore).
    param([Parameter(Mandatory)][string]$RepoRoot)
    return (Join-Path $RepoRoot 'scripts\testserver.local.json')
}

function Get-NimbyLocalSetting {
    # One value from testserver.local.json, or $null when the file or the key is absent or empty.
    # A file that is not valid JSON is reported by name (never its content) and treated as absent.
    param(
        [Parameter(Mandatory)][string]$RepoRoot,
        [Parameter(Mandatory)][string]$Name
    )

    $file = Get-NimbyLocalSettingPath -RepoRoot $RepoRoot
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { return $null }
    $settings = $null
    try { $settings = Get-Content -LiteralPath $file -Raw -Encoding utf8 | ConvertFrom-Json }
    catch {
        Write-Warning ("Ignoring {0}: not valid JSON." -f $file)
        return $null
    }
    if ($null -eq $settings) { return $null }
    $property = $settings.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value -or [string]$property.Value -eq '') { return $null }
    return $property.Value
}

function Get-NimbySftpCredentialPath {
    # Where the saved HostHavoc SFTP credential lives: testserver.local.json "sftpCredentialFile",
    # else %LOCALAPPDATA%\Nimby\hosthavoc-sftp.credential.xml, outside the repository. The file is
    # an Export-Clixml of a PSCredential, so Windows DPAPI encrypts the password and only the
    # Windows account that saved it, on that machine, can read it (Jeff, October 27, 2026).
    [OutputType([string])]
    param([Parameter(Mandatory)][string]$RepoRoot)

    $configured = Get-NimbyLocalSetting -RepoRoot $RepoRoot -Name 'sftpCredentialFile'
    if ($null -ne $configured) { return [string]$configured }
    if (-not $env:LOCALAPPDATA) { return $null }
    return (Join-Path $env:LOCALAPPDATA 'Nimby\hosthavoc-sftp.credential.xml')
}

function Read-NimbySftpCredential {
    # The saved credential when the file exists, decrypts for this account, and names UserName;
    # otherwise $null, with a warning that names the file (never its content), so the caller
    # falls back to a prompt.
    param(
        [AllowNull()][AllowEmptyString()][string]$Path,
        [Parameter(Mandatory)][string]$UserName
    )

    if (-not $Path -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $null }
    $saved = $null
    try { $saved = Import-Clixml -LiteralPath $Path }
    catch {
        Write-Warning ("Ignoring {0}: this Windows account cannot read or decrypt it." -f $Path)
        return $null
    }
    if ($saved -isnot [System.Management.Automation.PSCredential]) {
        Write-Warning ("Ignoring {0}: it does not hold a saved credential." -f $Path)
        return $null
    }
    if ($saved.UserName -ne $UserName) {
        Write-Warning ("Ignoring {0}: it was saved for a different account." -f $Path)
        return $null
    }
    return $saved
}

Export-ModuleMember -Function Write-Manifest, Test-TestInstanceId, Get-ServerArgument, Read-ServerLogText, Test-PathInside, Get-RunningServerProcess, Get-RunningServerMessage, Get-NimbyRandomString, Get-NimbyLocalSettingPath, Get-NimbyLocalSetting, Get-NimbySftpCredentialPath, Read-NimbySftpCredential
