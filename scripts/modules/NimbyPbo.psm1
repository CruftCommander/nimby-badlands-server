# NimbyPbo.psm1 - read-only inspection of Bohemia PBO archives.
# Runs in Windows PowerShell 5.1 and PowerShell 7.

Set-StrictMode -Version Latest

function Read-PboCString {
    param([Parameter(Mandatory)][System.IO.BinaryReader]$Reader)

    $bytes = New-Object System.Collections.Generic.List[byte]
    while ($true) {
        $b = $Reader.ReadByte()
        if ($b -eq 0) { break }
        $bytes.Add($b)
        if ($bytes.Count -gt 1024) {
            throw 'PBO header string longer than 1024 bytes; file is not a valid PBO.'
        }
    }
    return [System.Text.Encoding]::UTF8.GetString($bytes.ToArray())
}

function Get-PboContent {
    <#
    .SYNOPSIS
        Returns the PBO's prefix property and its entry names, read from the header only.
    .OUTPUTS
        PSCustomObject with Prefix (string or $null) and Entries (string[]).
    #>
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Path)

    $stream = [System.IO.File]::OpenRead($Path)
    try {
        $reader = New-Object System.IO.BinaryReader($stream)
        $entries = New-Object System.Collections.Generic.List[string]
        $prefix = $null

        while ($true) {
            $name = Read-PboCString -Reader $reader
            $method = $reader.ReadUInt32()
            [void]$reader.ReadUInt32()   # original size
            [void]$reader.ReadUInt32()   # reserved
            [void]$reader.ReadUInt32()   # timestamp
            [void]$reader.ReadUInt32()   # data size

            if ($name -eq '' -and $method -eq 0x56657273) {
                # 'Vers' header extension: key/value pairs until an empty key.
                while ($true) {
                    $key = Read-PboCString -Reader $reader
                    if ($key -eq '') { break }
                    $value = Read-PboCString -Reader $reader
                    if ($key -eq 'prefix') { $prefix = $value }
                }
                continue
            }
            if ($name -eq '') { break }
            $entries.Add($name)
        }

        return [PSCustomObject]@{
            Prefix  = $prefix
            Entries = $entries.ToArray()
        }
    }
    finally {
        $stream.Dispose()
    }
}

function Compare-PboToSource {
    <#
    .SYNOPSIS
        Lists source files that are missing from a PBO. config.cpp is satisfied by config.bin.
    .OUTPUTS
        string[] of missing relative paths (empty when complete).
    #>
    [CmdletBinding()]
    [OutputType([string[]])]
    param(
        [Parameter(Mandatory)][AllowEmptyCollection()][string[]]$SourceRelativePaths,
        [Parameter(Mandatory)][AllowEmptyCollection()][string[]]$PboEntries
    )

    $present = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    foreach ($entry in $PboEntries) {
        [void]$present.Add(($entry -replace '/', '\'))
    }

    $missing = @()
    foreach ($relative in $SourceRelativePaths) {
        $normalized = $relative -replace '/', '\'
        if ($present.Contains($normalized)) { continue }
        if ($normalized -ieq 'config.cpp' -and $present.Contains('config.bin')) { continue }
        $missing += $normalized
    }
    return $missing
}

Export-ModuleMember -Function Get-PboContent, Compare-PboToSource
