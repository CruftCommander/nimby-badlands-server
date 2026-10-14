#Requires -Version 5.1
<#
.SYNOPSIS
    Copies a subscribed DayZ Steam Workshop item into this repository's vendor folder.

.DESCRIPTION
    Finds the item in any Steam library on this machine and copies it, unchanged, to
    vendor\workshop\<WorkshopId>\<timestamp>\ so Claude can read and extract it. Writes a
    manifest with every file's size and SHA256 to logs\.

    Reads   : HKCU:\Software\Valve\Steam (SteamPath), <Steam>\steamapps\libraryfolders.vdf,
              <library>\steamapps\workshop\content\221100\<WorkshopId>\
    Writes  : <repo>\vendor\workshop\<WorkshopId>\<timestamp>\ and <repo>\logs\
    Never   : modifies, moves, or deletes anything in the Steam folders. vendor\ and logs\
              are git-ignored; nothing here is committed.

    Runs in Windows PowerShell 5.1 and PowerShell 7. No elevation needed.

.PARAMETER WorkshopId
    The numeric Steam Workshop item ID. Subscribe to the item in Steam first.

.PARAMETER RepoRoot
    Repository root. Defaults to the parent of this script's folder.

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Copy-WorkshopItem.ps1 -WorkshopId 2903112334
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidatePattern('^\d+$')]
    [string]$WorkshopId,

    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot)
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$DayZAppId = '221100'

function Get-SteamRootPath {
    # Returns the Steam install folder, or $null when none can be found.
    param([string[]]$Candidates = @())

    $found = @()
    $regKey = 'HKCU:\Software\Valve\Steam'
    if (Test-Path -LiteralPath $regKey) {
        $value = (Get-ItemProperty -LiteralPath $regKey -Name 'SteamPath' -ErrorAction SilentlyContinue)
        if ($null -ne $value -and $value.SteamPath) {
            $found += ($value.SteamPath -replace '/', '\')
        }
    }
    $found += $Candidates

    foreach ($path in $found) {
        if ($path -and (Test-Path -LiteralPath (Join-Path $path 'steamapps'))) {
            return $path
        }
    }
    return $null
}

function Get-SteamLibraryPath {
    # Returns every Steam library folder listed in libraryfolders.vdf, plus the Steam root.
    param([Parameter(Mandatory)][string]$SteamRoot)

    $libraries = New-Object System.Collections.Generic.List[string]
    $libraries.Add($SteamRoot)

    $vdf = Join-Path $SteamRoot 'steamapps\libraryfolders.vdf'
    if (Test-Path -LiteralPath $vdf) {
        $content = Get-Content -LiteralPath $vdf -Raw -Encoding utf8
        foreach ($match in [regex]::Matches($content, '"path"\s+"([^"]+)"')) {
            # VDF escapes backslashes as \\.
            $path = $match.Groups[1].Value -replace '\\\\', '\'
            if (-not $libraries.Contains($path)) {
                $libraries.Add($path)
            }
        }
    }
    return $libraries.ToArray()
}

function Find-WorkshopItemPath {
    # Returns the first library path that holds the item, or $null.
    param(
        [Parameter(Mandatory)][string[]]$LibraryPaths,
        [Parameter(Mandatory)][string]$AppId,
        [Parameter(Mandatory)][string]$ItemId
    )

    foreach ($library in $LibraryPaths) {
        $candidate = Join-Path $library ("steamapps\workshop\content\{0}\{1}" -f $AppId, $ItemId)
        if (Test-Path -LiteralPath $candidate -PathType Container) {
            return $candidate
        }
    }
    return $null
}

function Write-Manifest {
    # Appends one UTF-8 line to the manifest log.
    param(
        [Parameter(Mandatory)][string]$LogPath,
        [Parameter(Mandatory)][string]$Line
    )
    Add-Content -LiteralPath $LogPath -Value $Line -Encoding utf8
}

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$logDir = Join-Path $RepoRoot 'logs'
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
$logPath = Join-Path $logDir ("Copy-WorkshopItem-{0}-{1}.log" -f $WorkshopId, $stamp)

try {
    Write-Manifest -LogPath $logPath -Line ("START {0} WorkshopId={1} RepoRoot={2} PS={3}" -f (Get-Date -Format o), $WorkshopId, $RepoRoot, $PSVersionTable.PSVersion)

    if (-not (Test-Path -LiteralPath (Join-Path $RepoRoot '.git'))) {
        throw "RepoRoot '$RepoRoot' is not a git repository root."
    }

    $candidates = @()
    if (${env:ProgramFiles(x86)}) {
        $candidates += (Join-Path ${env:ProgramFiles(x86)} 'Steam')
    }
    $steamRoot = Get-SteamRootPath -Candidates $candidates
    if ($null -eq $steamRoot) {
        throw 'Steam install not found (registry SteamPath and default location both missing).'
    }
    Write-Manifest -LogPath $logPath -Line "STEAM $steamRoot"

    $libraries = Get-SteamLibraryPath -SteamRoot $steamRoot
    Write-Manifest -LogPath $logPath -Line ("LIBRARIES {0}" -f ($libraries -join ' | '))

    $source = Find-WorkshopItemPath -LibraryPaths $libraries -AppId $DayZAppId -ItemId $WorkshopId
    if ($null -eq $source) {
        throw "Workshop item $WorkshopId not found in any Steam library. Subscribe to it in Steam, start the DayZ Launcher so it downloads, then re-run."
    }

    $files = @(Get-ChildItem -LiteralPath $source -Recurse -File)
    if ($files.Count -eq 0) {
        throw "Workshop item folder '$source' is empty. The download may still be in progress."
    }

    $destination = Join-Path $RepoRoot ("vendor\workshop\{0}\{1}" -f $WorkshopId, $stamp)
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    Write-Manifest -LogPath $logPath -Line "SOURCE $source"
    Write-Manifest -LogPath $logPath -Line "DEST $destination"

    $copied = 0
    $totalBytes = [int64]0
    foreach ($file in $files) {
        $relative = $file.FullName.Substring($source.Length).TrimStart('\')
        $target = Join-Path $destination $relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
        Copy-Item -LiteralPath $file.FullName -Destination $target
        $hash = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash
        Write-Manifest -LogPath $logPath -Line ("FILE OK {0} bytes={1} sha256={2}" -f $relative, $file.Length, $hash)
        $copied++
        $totalBytes += $file.Length
    }

    Write-Manifest -LogPath $logPath -Line ("SUMMARY OK files={0} bytes={1}" -f $copied, $totalBytes)
    Write-Information -MessageData ("OK: copied {0} files from Workshop item {1}. Log: {2}" -f $copied, $WorkshopId, $logPath) -InformationAction Continue
}
catch {
    Write-Manifest -LogPath $logPath -Line ("FAIL {0}" -f $_.Exception.Message)
    Write-Information -MessageData ("FAIL: {0} Log: {1}" -f $_.Exception.Message, $logPath) -InformationAction Continue
    exit 1
}
