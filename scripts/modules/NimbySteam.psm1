# NimbySteam.psm1 - locates Steam, its library folders, installed apps, and Workshop items.
# Read-only: never writes to Steam folders. Runs in Windows PowerShell 5.1 and PowerShell 7.

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

function Find-SteamAppPath {
    <#
    .SYNOPSIS
        Returns the first steamapps\common\<FolderName> that exists in any library, or $null.
    #>
    [CmdletBinding()]
    [OutputType([string])]
    param(
        [Parameter(Mandatory)][string[]]$LibraryPaths,
        [Parameter(Mandatory)][string]$FolderName
    )

    foreach ($library in $LibraryPaths) {
        $candidate = Join-Path $library ("steamapps\common\{0}" -f $FolderName)
        if (Test-Path -LiteralPath $candidate -PathType Container) {
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

Export-ModuleMember -Function Get-SteamRootPath, Get-SteamLibraryPath, Find-SteamAppPath, Find-WorkshopItemPath
