#Requires -Version 5.1

<#
.SYNOPSIS
    Copies the live HostHavoc server's logs into the repo over SFTP so they can be analyzed
    locally (baselines, incidents, mod readiness checks).

.DESCRIPTION
    1. Opens one SFTP session to the HostHavoc service (Posh-SSH). The credential comes from
       the DPAPI-encrypted file scripts\Save-NimbySftpCredential.ps1 writes (only your Windows
       account on this machine can decrypt it); without that file you type the panel password
       once, and it is not written anywhere.
    2. Uses -RemoteProfileDir (default: the path the first run discovered). If that folder
       is gone or holds no DayZServer_x64_*.RPT, walks the SFTP tree to find it again and
       prints the new path. Pass -RemoteProfileDir '' to force the search.
    3. Downloads every top-level *.RPT, *.ADM and *.log file in that folder (crash dumps only
       with -IncludeCrashDumps) into logs\server-<stamp>\, then writes manifest.log: one line
       per file with a verdict, remote size, local size and path, and a summary.

    Reads   : the remote profile folder (read only; nothing on the server is changed), and
              <repo>\scripts\testserver.local.json (git-ignored by *.local.json) for the
              connection: "sftpHost", "sftpPort", "sftpUser", and optionally "sftpProfileDir"
              and "sftpCredentialFile"; the saved credential file when it exists.
    Writes  : <repo>\logs\server-<stamp>\ (git-ignored). Each run gets its own folder, so a
              re-run never overwrites an earlier pull.
    Never   : uploads, renames or deletes anything on the server; writes the password.

    Needs the Posh-SSH module once per user:
        Install-Module Posh-SSH -Scope CurrentUser
    The first connection asks you to trust the server's host key; check the fingerprint.

    The host, port, account name, and profile folder are not in this script (the repository is
    to be made public). Pass -ComputerName, -Port, -UserName, and -RemoteProfileDir, or put them
    in scripts\testserver.local.json once, for example:
        { "sftpHost": "<address>", "sftpPort": <port>, "sftpUser": "<account>",
          "sftpProfileDir": "/<folder>/HostHavocDayZServer" }
    Without sftpProfileDir the script searches the SFTP tree for the profile folder.

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Get-ServerLogs.ps1
#>
[CmdletBinding()]
param(
    # No literal defaults: each comes from the parameter or scripts\testserver.local.json.
    [string]$ComputerName,
    [int]$Port,
    [string]$UserName,
    # If the folder is gone or empty, or this is '', the script searches the SFTP tree.
    [string]$RemoteProfileDir,
    [int]$SearchDepth = 4,
    [switch]$IncludeCrashDumps,
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
# Windows PowerShell 5.1 leaves $PSScriptRoot empty in a param() default when an advanced
# script is started with -File, so the default is resolved here.
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }
$ErrorActionPreference = 'Stop'

# Write-Manifest and Get-NimbyLocalSetting come from NimbyCommon.psm1.
Import-Module (Join-Path $PSScriptRoot 'modules\NimbyCommon.psm1') -Force

function Get-RemoteLeafName {
    # Last segment of a remote (forward-slash) path.
    param([Parameter(Mandatory)][string]$RemotePath)
    $trimmed = $RemotePath.TrimEnd('/')
    if ($trimmed -eq '') { return '' }
    return ($trimmed -split '/')[-1]
}

function Join-RemotePath {
    param(
        [Parameter(Mandatory)][string]$Parent,
        [Parameter(Mandatory)][string]$Child
    )
    if ($Parent.EndsWith('/')) { return "$Parent$Child" }
    return "$Parent/$Child"
}

function Test-LogFileName {
    # True for the file names this script pulls.
    param(
        [Parameter(Mandatory)][string]$Name,
        [switch]$IncludeCrashDumps
    )
    if ($Name -match '\.(RPT|ADM|log)$') { return $true }
    if ($IncludeCrashDumps -and $Name -match '\.mdmp$') { return $true }
    return $false
}

function Get-RemoteEntry {
    # Lists one remote directory. Returns objects with FullName, Name, IsDirectory, Length.
    # Uses -Directory / -File because Posh-SSH does not document an IsDirectory property.
    param(
        [Parameter(Mandatory)][int]$SessionId,
        [Parameter(Mandatory)][string]$Path
    )
    $entries = [System.Collections.Generic.List[object]]::new()
    foreach ($dir in @(Get-SFTPChildItem -SessionId $SessionId -Path $Path -Directory)) {
        $name = Get-RemoteLeafName -RemotePath $dir.FullName
        if ($name -in @('', '.', '..')) { continue }
        $entries.Add([PSCustomObject]@{ FullName = $dir.FullName; Name = $name; IsDirectory = $true; Length = 0 })
    }
    foreach ($file in @(Get-SFTPChildItem -SessionId $SessionId -Path $Path -File)) {
        $name = Get-RemoteLeafName -RemotePath $file.FullName
        $entries.Add([PSCustomObject]@{ FullName = $file.FullName; Name = $name; IsDirectory = $false; Length = [long]$file.Length })
    }
    return $entries
}

function Find-RemoteProfileDir {
    # Breadth-first walk from '/' for the folder that holds DayZServer_x64_*.RPT files.
    # Returns the remote path, or $null when none is found within $MaxDepth levels.
    param(
        [Parameter(Mandatory)][int]$SessionId,
        [int]$MaxDepth = 4
    )
    $queue = [System.Collections.Generic.Queue[object]]::new()
    $queue.Enqueue([PSCustomObject]@{ Path = '/'; Depth = 0 })
    while ($queue.Count -gt 0) {
        $item = $queue.Dequeue()
        $entries = @(Get-RemoteEntry -SessionId $SessionId -Path $item.Path)
        $hasRpt = @($entries | Where-Object { -not $_.IsDirectory -and $_.Name -like 'DayZServer_x64_*.RPT' }).Count -gt 0
        if ($hasRpt) { return $item.Path }
        if ($item.Depth -ge $MaxDepth) { continue }
        # Search the folder HostHavoc names the profile first, then everything else.
        $dirs = @($entries | Where-Object { $_.IsDirectory } | Sort-Object { if ($_.Name -eq 'HostHavocDayZServer') { 0 } else { 1 } }, Name)
        foreach ($dir in $dirs) {
            $queue.Enqueue([PSCustomObject]@{ Path = $dir.FullName; Depth = $item.Depth + 1 })
        }
    }
    return $null
}

function Test-RemoteProfileDir {
    # True when the remote folder exists and holds at least one DayZServer_x64_*.RPT.
    param(
        [Parameter(Mandatory)][int]$SessionId,
        [Parameter(Mandatory)][string]$Path
    )
    try {
        $entries = @(Get-RemoteEntry -SessionId $SessionId -Path $Path)
    } catch {
        return $false
    }
    return @($entries | Where-Object { -not $_.IsDirectory -and $_.Name -like 'DayZServer_x64_*.RPT' }).Count -gt 0
}

function Get-PullVerdict {
    # OK when the local copy exists and matches the remote size. The live RPT keeps growing
    # while the server runs, so a larger local file is still OK; smaller is SHORT.
    param(
        [Parameter(Mandatory)][long]$RemoteLength,
        [AllowNull()][System.IO.FileInfo]$LocalFile
    )
    if ($null -eq $LocalFile -or -not $LocalFile.Exists) { return 'FAIL' }
    if ($LocalFile.Length -ge $RemoteLength) { return 'OK' }
    return 'SHORT'
}

# ---------------------------------------------------------------- main
if ($MyInvocation.InvocationName -eq '.') { return }   # dot-sourced by tests: functions only

if (-not (Get-Module -ListAvailable -Name Posh-SSH)) {
    throw 'Posh-SSH is not installed. Run: Install-Module Posh-SSH -Scope CurrentUser'
}
Import-Module Posh-SSH

# Connection settings: parameter, else scripts\testserver.local.json. The manifest records host
# and user as before (it stays under the git-ignored logs\); nothing here is printed.
if (-not $ComputerName) { $ComputerName = [string](Get-NimbyLocalSetting -RepoRoot $RepoRoot -Name 'sftpHost') }
if (-not $Port) {
    $settingPort = Get-NimbyLocalSetting -RepoRoot $RepoRoot -Name 'sftpPort'
    if ($null -ne $settingPort) { $Port = [int]$settingPort }
}
if (-not $UserName) { $UserName = [string](Get-NimbyLocalSetting -RepoRoot $RepoRoot -Name 'sftpUser') }
if (-not $PSBoundParameters.ContainsKey('RemoteProfileDir')) { $RemoteProfileDir = [string](Get-NimbyLocalSetting -RepoRoot $RepoRoot -Name 'sftpProfileDir') }
$missing = @()
if (-not $ComputerName) { $missing += '-ComputerName (sftpHost)' }
if (-not $Port) { $missing += '-Port (sftpPort)' }
if (-not $UserName) { $missing += '-UserName (sftpUser)' }
if ($missing.Count -gt 0) {
    throw ("Missing {0}. Pass them, or set them in {1}." -f ($missing -join ', '), (Get-NimbyLocalSettingPath -RepoRoot $RepoRoot))
}

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$outDir = Join-Path $RepoRoot "logs\server-$stamp"
New-Item -ItemType Directory -Path $outDir -Force | Out-Null
$manifest = Join-Path $outDir 'manifest.log'
Write-Manifest -LogPath $manifest -Line "Get-ServerLogs $stamp host=${ComputerName}:$Port user=$UserName"

$credential = Read-NimbySftpCredential -Path (Get-NimbySftpCredentialPath -RepoRoot $RepoRoot) -UserName $UserName
if ($null -eq $credential) {
    Write-Manifest -LogPath $manifest -Line 'CREDENTIAL prompt'
    $credential = Get-Credential -UserName $UserName -Message "HostHavoc panel password for $UserName (SFTP ${ComputerName}:$Port)"
}
else {
    Write-Manifest -LogPath $manifest -Line 'CREDENTIAL saved file'
}
$session = $null
$results = [System.Collections.Generic.List[string]]::new()
try {
    $session = New-SFTPSession -ComputerName $ComputerName -Port $Port -Credential $credential -ConnectionTimeout 20
    $sessionId = $session.SessionId

    if ($RemoteProfileDir -and -not (Test-RemoteProfileDir -SessionId $sessionId -Path $RemoteProfileDir)) {
        Write-Manifest -LogPath $manifest -Line "WARN remoteProfileDir=$RemoteProfileDir has no DayZServer_x64_*.RPT; searching"
        $RemoteProfileDir = ''
    }
    if (-not $RemoteProfileDir) {
        Write-Information "Searching the SFTP tree for the server profile folder (depth $SearchDepth)..." -InformationAction Continue
        $RemoteProfileDir = Find-RemoteProfileDir -SessionId $sessionId -MaxDepth $SearchDepth
        if (-not $RemoteProfileDir) {
            Write-Manifest -LogPath $manifest -Line "FAIL discovery: no folder with DayZServer_x64_*.RPT within depth $SearchDepth"
            throw "No folder with DayZServer_x64_*.RPT found within depth $SearchDepth. Pass -RemoteProfileDir."
        }
    }
    Write-Manifest -LogPath $manifest -Line "remoteProfileDir=$RemoteProfileDir"
    Write-Information "Profile folder: $RemoteProfileDir (pass -RemoteProfileDir '$RemoteProfileDir' to skip the search)" -InformationAction Continue

    $files = @(Get-RemoteEntry -SessionId $sessionId -Path $RemoteProfileDir |
        Where-Object { -not $_.IsDirectory -and (Test-LogFileName -Name $_.Name -IncludeCrashDumps:$IncludeCrashDumps) } |
        Sort-Object Name)
    if ($files.Count -eq 0) {
        Write-Manifest -LogPath $manifest -Line "FAIL list: no log files in $RemoteProfileDir"
        throw "No log files found in $RemoteProfileDir."
    }

    foreach ($file in $files) {
        $verdict = 'FAIL'
        $localPath = Join-Path $outDir $file.Name
        try {
            Get-SFTPItem -SessionId $sessionId -Path $file.FullName -Destination $outDir -Force
            $local = Get-Item -LiteralPath $localPath -ErrorAction SilentlyContinue
            $verdict = Get-PullVerdict -RemoteLength $file.Length -LocalFile $local
            $localLength = if ($null -ne $local) { $local.Length } else { 0 }
            Write-Manifest -LogPath $manifest -Line "$verdict $($file.Name) remote=$($file.Length) local=$localLength"
        } catch {
            Write-Manifest -LogPath $manifest -Line "FAIL $($file.Name) error=$($_.Exception.Message)"
        }
        $results.Add($verdict)
    }
} finally {
    if ($null -ne $session) { Remove-SFTPSession -SessionId $session.SessionId | Out-Null }
}

$ok = @($results | Where-Object { $_ -eq 'OK' }).Count
$short = @($results | Where-Object { $_ -eq 'SHORT' }).Count
$fail = @($results | Where-Object { $_ -eq 'FAIL' }).Count
Write-Manifest -LogPath $manifest -Line "SUMMARY files=$($results.Count) ok=$ok short=$short fail=$fail"
Write-Information "Done: $($results.Count) files ($ok OK, $short SHORT, $fail FAIL). Manifest: $manifest" -InformationAction Continue
if ($fail -gt 0) { exit 1 }
