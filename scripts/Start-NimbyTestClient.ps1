#Requires -Version 5.1

<#
.SYNOPSIS
    Starts the DayZ client with an @NIMBY build loaded, optionally connecting straight to the
    local test server.

.DESCRIPTION
    Finds the DayZ client install (Steam app 221100) through the Steam library manifests, then
    starts DayZ_BE.exe with -mod=<BuildPath>. With -Connect the client joins <Connect>:<Port>
    on start and -Password supplies the join password the test server printed.

    Reads   : the Steam library manifests, the build folder.
    Writes  : <repo>\logs\testclient-<stamp>.log (the resolved paths and the argument list with
              the password masked). Nothing else.
    Never   : touches the server, the repository, or any profile folder.

.PARAMETER BuildPath
    The build\@NIMBY-<stamp> folder to load. The same folder the test server loaded, or
    verifySignatures = 2 rejects the client.

.PARAMETER Connect
    Server address to join on start. Omit to land on the main menu.

.PARAMETER Port
    Game port. Defaults to 2302 (pack.json testServer.port).

.PARAMETER Password
    Join password printed by Start-NimbyTestServer.ps1. Plaintext on the DayZ command line is the
    only form the game accepts; it is a throwaway value and is masked in the log.

.PARAMETER DayZPath
    DayZ client install folder. Found through Steam when omitted.

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Start-NimbyTestClient.ps1 -BuildPath D:\GitHub\nimby-badlands-server\build\@NIMBY-20261017-235142 -Connect 127.0.0.1 -Password <join password>
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildPath,
    [string]$Connect,
    [ValidateRange(1, 65535)][int]$Port = 2302,
    [string]$Password,
    [string]$DayZPath,
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
# Windows PowerShell 5.1 leaves $PSScriptRoot empty in a param() default when an advanced
# script is started with -File, so the default is resolved here.
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }
$ErrorActionPreference = 'Stop'

Import-Module (Join-Path $PSScriptRoot 'modules\NimbySteam.psm1') -Force

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$logDir = Join-Path $RepoRoot 'logs'
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
$log = Join-Path $logDir ("testclient-{0}.log" -f $stamp)

if (-not (Test-Path -LiteralPath $BuildPath -PathType Container)) { throw "Build folder not found: $BuildPath" }
if (-not (Test-Path -LiteralPath (Join-Path $BuildPath 'addons') -PathType Container)) { throw "No addons folder under $BuildPath; pass a build\@NIMBY-<stamp> folder." }

if (-not $DayZPath) {
    $libraries = Get-SteamLibraryPath -SteamRoot (Get-SteamRootPath)
    $DayZPath = Find-SteamInstalledAppPath -LibraryPaths $libraries -AppId '221100' -RequiredFile 'DayZ_BE.exe'
    if (-not $DayZPath) { throw 'DayZ client (Steam app 221100) not found in any Steam library; pass -DayZPath.' }
}
$exe = Join-Path $DayZPath 'DayZ_BE.exe'
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw "DayZ_BE.exe not found under $DayZPath" }

$arguments = @("-mod=$BuildPath")
if ($Connect) {
    $arguments += "-connect=$Connect"
    $arguments += "-port=$Port"
    if ($Password) { $arguments += "-password=$Password" }
}

$masked = $arguments | ForEach-Object { if ($_ -like '-password=*') { '-password=***' } else { $_ } }
Add-Content -LiteralPath $log -Value ("CLIENT {0}" -f $exe) -Encoding utf8
Add-Content -LiteralPath $log -Value ("BUILD {0}" -f $BuildPath) -Encoding utf8
Add-Content -LiteralPath $log -Value ("ARGS {0}" -f ($masked -join ' ')) -Encoding utf8

$quoted = $arguments | ForEach-Object { '"{0}"' -f $_ }
$process = Start-Process -FilePath $exe -ArgumentList $quoted -WorkingDirectory $DayZPath -PassThru
Add-Content -LiteralPath $log -Value ("STARTED pid={0}" -f $process.Id) -Encoding utf8
Write-Information -MessageData ("DayZ started (pid {0}) with {1}; log {2}" -f $process.Id, $BuildPath, $log) -InformationAction Continue
