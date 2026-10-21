# NimbySteam.psm1 - locates Steam, its library folders, installed apps, Workshop items, and the
# DayZ Server install (Steam first, then D:\DayZServer).
# Reads: HKCU:\Software\Valve\Steam, libraryfolders.vdf, appmanifest_*.acf, and whether the
# candidate folders exist. Read-only: never writes anywhere. Runs in Windows PowerShell 5.1 and
# PowerShell 7.

Set-StrictMode -Version Latest

function Get-SteamRootPath {
    <#
    .SYNOPSIS
        Returns the Steam install folder, or $null when none can be found.
    .PARAMETER Candidates
        Extra folders to try after the registry value.
    #>
    [CmdletBinding()]
    [OutputType([string])]
    param([string[]]$Candidates = @())

    $found = @()
    $regKey = 'HKCU:\Software\Valve\Steam'
    if (Test-Path -LiteralPath $regKey) {
        $value = Get-ItemProperty -LiteralPath $regKey -Name 'SteamPath' -ErrorAction SilentlyContinue
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
    <#
    .SYNOPSIS
        Returns every Steam library folder listed in libraryfolders.vdf plus the Steam root,
        with duplicates removed ignoring case and trailing separators.
    #>
    [CmdletBinding()]
    [OutputType([string[]])]
    param([Parameter(Mandatory)][string]$SteamRoot)

    $libraries = New-Object System.Collections.Generic.List[string]
    $seen = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)

    $all = @($SteamRoot)
    $vdf = Join-Path $SteamRoot 'steamapps\libraryfolders.vdf'
    if (Test-Path -LiteralPath $vdf) {
        $content = Get-Content -LiteralPath $vdf -Raw -Encoding utf8
        foreach ($match in [regex]::Matches($content, '"path"\s+"([^"]+)"')) {
            # VDF escapes backslashes as \\.
            $all += ($match.Groups[1].Value -replace '\\\\', '\')
        }
    }

    foreach ($path in $all) {
        $key = $path.TrimEnd('\', '/')
        if ($key -and $seen.Add($key)) {
            $libraries.Add($path)
        }
    }
    return $libraries.ToArray()
}

function Test-SteamAppFolder {
    <#
    .SYNOPSIS
        True when the folder exists and, if -RequiredFile is given, that file exists inside it.
    .DESCRIPTION
        A folder name alone is not proof of an install: Steam leaves a folder behind after an
        uninstall or a move to another library (observed October 16, 2026: an empty
        "DayZ Tools\Bin\Logs" on C: while the real install was on F:).
    #>
    [CmdletBinding()]
    [OutputType([bool])]
    param(
        [Parameter(Mandatory)][string]$Path,
        [string]$RequiredFile
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Container)) { return $false }
    if (-not $RequiredFile) { return $true }
    return (Test-Path -LiteralPath (Join-Path $Path $RequiredFile) -PathType Leaf)
}

function Find-SteamAppPath {
    <#
    .SYNOPSIS
        Returns the first steamapps\common\<FolderName> that exists in any library, or $null.
    .PARAMETER RequiredFile
        A path relative to the app folder that must exist for the folder to count, for example
        'Bin\DsUtils\DSSignFile.exe'. Without it, a leftover empty folder matches.
    #>
    [CmdletBinding()]
    [OutputType([string])]
    param(
        [Parameter(Mandatory)][string[]]$LibraryPaths,
        [Parameter(Mandatory)][string]$FolderName,
        [string]$RequiredFile
    )

    foreach ($library in $LibraryPaths) {
        $candidate = Join-Path $library ("steamapps\common\{0}" -f $FolderName)
        if (Test-SteamAppFolder -Path $candidate -RequiredFile $RequiredFile) {
            return $candidate
        }
    }
    return $null
}

function Find-SteamInstalledAppPath {
    <#
    .SYNOPSIS
        Returns the install folder of a Steam app by its app id, read from the library that holds
        its appmanifest_<AppId>.acf, or $null when no library has the manifest.
    .DESCRIPTION
        Steam writes steamapps\appmanifest_<AppId>.acf only in the library where the app is
        installed, and "installdir" inside it names the folder under steamapps\common. This is
        the authoritative lookup; Find-SteamAppPath by folder name is the fallback.
    .PARAMETER RequiredFile
        A path relative to the install folder that must exist, so a manifest left mid-download or
        mid-move does not count.
    #>
    [CmdletBinding()]
    [OutputType([string])]
    param(
        [Parameter(Mandatory)][string[]]$LibraryPaths,
        [Parameter(Mandatory)][ValidatePattern('^\d+$')][string]$AppId,
        [string]$RequiredFile
    )

    foreach ($library in $LibraryPaths) {
        $manifest = Join-Path $library ("steamapps\appmanifest_{0}.acf" -f $AppId)
        if (-not (Test-Path -LiteralPath $manifest -PathType Leaf)) { continue }
        $content = Get-Content -LiteralPath $manifest -Raw -Encoding utf8
        $match = [regex]::Match($content, '"installdir"\s+"([^"]+)"')
        if (-not $match.Success) { continue }
        $candidate = Join-Path $library ("steamapps\common\{0}" -f $match.Groups[1].Value)
        if (Test-SteamAppFolder -Path $candidate -RequiredFile $RequiredFile) {
            return $candidate
        }
    }
    return $null
}

function Find-WorkshopItemPath {
    <#
    .SYNOPSIS
        Returns the first library path that holds the Workshop item, or $null.
    #>
    [CmdletBinding()]
    [OutputType([string])]
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

function Find-DayZServerPath {
    <#
    .SYNOPSIS
        Returns the DayZ Server install folder, or $null: the Steam app manifest (app 223350) in
        any library, then a steamapps\common\DayZServer folder, then each Fallback folder. Every
        candidate must hold DayZServer_x64.exe.
    .PARAMETER Fallback
        Folders tried after Steam. D:\DayZServer is the local replica built by Sync-LiveReplica.ps1.
    #>
    [CmdletBinding()]
    [OutputType([string])]
    param(
        [AllowEmptyCollection()][string[]]$LibraryPaths = @(),
        [AllowEmptyCollection()][string[]]$Fallback = @('D:\DayZServer')
    )

    $candidates = @()
    if ($LibraryPaths.Count -gt 0) {
        $candidates += Find-SteamInstalledAppPath -LibraryPaths $LibraryPaths -AppId '223350' -RequiredFile 'DayZServer_x64.exe'
        $candidates += Find-SteamAppPath -LibraryPaths $LibraryPaths -FolderName 'DayZServer' -RequiredFile 'DayZServer_x64.exe'
    }
    $candidates += $Fallback
    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path -LiteralPath (Join-Path $candidate 'DayZServer_x64.exe') -PathType Leaf)) { return $candidate }
    }
    return $null
}

Export-ModuleMember -Function Get-SteamRootPath, Get-SteamLibraryPath, Test-SteamAppFolder, Find-SteamAppPath, Find-SteamInstalledAppPath, Find-WorkshopItemPath, Find-DayZServerPath
