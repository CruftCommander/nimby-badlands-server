#Requires -Version 5.1

<#
.SYNOPSIS
    Saves the HostHavoc panel (SFTP) credential to a file that only your Windows account on this
    machine can decrypt, so Get-ServerLogs.ps1 stops prompting for the password.

.DESCRIPTION
    Prompts for the password of the "sftpUser" account in scripts\testserver.local.json, writes
    it with Export-Clixml (Windows DPAPI encrypts the password for the current user), then reads
    it back to prove it decrypts. Ruled by Jeff, October 27, 2026, in place of a plain-text
    environment variable.

    Reads   : <repo>\scripts\testserver.local.json ("sftpUser", optionally "sftpCredentialFile").
    Writes  : the credential file, by default %LOCALAPPDATA%\Nimby\hosthavoc-sftp.credential.xml.
    Never   : prints the password, writes inside the repository, or contacts the server.

    Runs in Windows PowerShell 5.1 and PowerShell 7, on Windows only (DPAPI). Run it again to
    replace the saved password after a panel password change; delete the file to go back to
    the prompt.

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Save-NimbySftpCredential.ps1
#>
[CmdletBinding()]
param([string]$RepoRoot)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }
Import-Module (Join-Path $PSScriptRoot 'modules\NimbyCommon.psm1') -Force

$userName = [string](Get-NimbyLocalSetting -RepoRoot $RepoRoot -Name 'sftpUser')
if (-not $userName) { throw ("No sftpUser in {0}." -f (Get-NimbyLocalSettingPath -RepoRoot $RepoRoot)) }
$path = Get-NimbySftpCredentialPath -RepoRoot $RepoRoot
if (-not $path) { throw 'LOCALAPPDATA is not set; set sftpCredentialFile in testserver.local.json.' }
if (Test-PathInside -Candidate ([System.IO.Path]::GetFullPath($path)) -Root ([System.IO.Path]::GetFullPath($RepoRoot))) {
    throw "Refusing to save a credential inside the repository: $path"
}

$credential = Get-Credential -UserName $userName -Message "HostHavoc panel password for $userName"
if ($null -eq $credential) { throw 'Cancelled; nothing saved.' }
New-Item -ItemType Directory -Path (Split-Path -Parent $path) -Force | Out-Null
$credential | Export-Clixml -LiteralPath $path -Force

if ($null -eq (Read-NimbySftpCredential -Path $path -UserName $userName)) { throw "Saved $path but could not read it back." }
Write-Information -MessageData ("Saved the credential for {0} to {1}; only this Windows account on this machine can decrypt it." -f $userName, $path) -InformationAction Continue
