# NimbyPbo.psm1 - packs and inspects Bohemia PBO archives (uncompressed).
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

function Get-PboSourceFile {
    <#
    .SYNOPSIS
        Returns paths relative to $Folder, with backslash separators and sorted ordinal
        case-insensitive, for files whose name matches any include pattern.
    #>
    [CmdletBinding()]
    [OutputType([string[]])]
    param(
        [Parameter(Mandatory)][string]$Folder,
        [Parameter(Mandatory)][string[]]$Patterns
    )

    $root = (Resolve-Path -LiteralPath $Folder).ProviderPath.TrimEnd('\', '/')
    $result = New-Object System.Collections.Generic.List[string]
    foreach ($file in Get-ChildItem -LiteralPath $root -Recurse -File) {
        foreach ($pattern in $Patterns) {
            if ($file.Name -like $pattern) {
                $result.Add(($file.FullName.Substring($root.Length).TrimStart('\', '/') -replace '/', '\'))
                break
            }
        }
    }
    $sorted = $result.ToArray()
    [System.Array]::Sort($sorted, [System.StringComparer]::OrdinalIgnoreCase)
    return $sorted
}

function Write-PboCString {
    param(
        [Parameter(Mandatory)][System.IO.BinaryWriter]$Writer,
        [Parameter(Mandatory)][AllowEmptyString()][string]$Value
    )
    if ($Value.Length -gt 0) {
        $Writer.Write([System.Text.Encoding]::UTF8.GetBytes($Value))
    }
    $Writer.Write([byte]0)
}

function Build-PboArchive {
    <#
    .SYNOPSIS
        Packs files from a folder into an uncompressed PBO with a prefix property and the
        trailing SHA1 checksum, matching the layout of Mikero-built PBOs.
    .DESCRIPTION
        Layout: a 'Vers' header-extension entry, the properties, one entry per file
        (method 0, original size = data size, reserved 0, timestamp, data size), an all-zero
        terminator entry, the file data in entry order, then 0x00 and the SHA1 of every
        preceding byte. Overwrites nothing: fails if Destination exists.
        Each entry's timestamp is its source file's last-write time (as Addon Builder and Mikero
        write it), so unchanged sources give a byte-identical PBO and the same SHA-256 from one
        build to the next. -Timestamp stamps every entry with one fixed time instead. The engine
        ignores the field.
    .OUTPUTS
        string[] of the entry names written.
    #>
    [CmdletBinding()]
    [OutputType([string[]])]
    param(
        [Parameter(Mandatory)][string]$SourceFolder,
        [Parameter(Mandatory)][string]$Destination,
        [Parameter(Mandatory)][string]$Prefix,
        [Parameter(Mandatory)][string[]]$Patterns,
        [Nullable[datetime]]$Timestamp = $null
    )

    if (Test-Path -LiteralPath $Destination) { throw "Destination '$Destination' already exists." }
    $entries = @(Get-PboSourceFile -Folder $SourceFolder -Patterns $Patterns)
    if ($entries.Count -eq 0) { throw "No files in '$SourceFolder' match the include patterns." }

    $root = (Resolve-Path -LiteralPath $SourceFolder).ProviderPath
    $fixedEpoch = $null
    if ($null -ne $Timestamp) { $fixedEpoch = [uint32]([System.DateTimeOffset]$Timestamp.ToUniversalTime()).ToUnixTimeSeconds() }

    $buffer = New-Object System.IO.MemoryStream
    $writer = New-Object System.IO.BinaryWriter($buffer)
    try {
        # Header extension entry with properties.
        Write-PboCString -Writer $writer -Value ''
        $writer.Write([uint32]0x56657273)
        foreach ($i in 1..4) { $writer.Write([uint32]0) }
        Write-PboCString -Writer $writer -Value 'prefix'
        Write-PboCString -Writer $writer -Value $Prefix
        Write-PboCString -Writer $writer -Value ''

        $data = New-Object System.Collections.Generic.List[byte[]]
        foreach ($entry in $entries) {
            $sourcePath = Join-Path $root $entry
            $bytes = [System.IO.File]::ReadAllBytes($sourcePath)
            $data.Add($bytes)
            $epoch = $fixedEpoch
            if ($null -eq $epoch) {
                $lastWrite = [System.DateTime]::SpecifyKind([System.IO.File]::GetLastWriteTimeUtc($sourcePath), [System.DateTimeKind]::Utc)
                $epoch = [uint32]([System.DateTimeOffset]$lastWrite).ToUnixTimeSeconds()
            }
            Write-PboCString -Writer $writer -Value $entry
            $writer.Write([uint32]0)                  # packing method: uncompressed
            $writer.Write([uint32]$bytes.Length)      # original size
            $writer.Write([uint32]0)                  # reserved
            $writer.Write($epoch)                     # timestamp
            $writer.Write([uint32]$bytes.Length)      # data size
        }

        # Terminator entry.
        Write-PboCString -Writer $writer -Value ''
        foreach ($i in 1..5) { $writer.Write([uint32]0) }

        foreach ($bytes in $data) { $writer.Write($bytes) }
        $writer.Flush()

        $sha1 = [System.Security.Cryptography.SHA1]::Create()
        try {
            $hash = $sha1.ComputeHash($buffer.ToArray())
        }
        finally {
            $sha1.Dispose()
        }
        $writer.Write([byte]0)
        $writer.Write($hash)
        $writer.Flush()

        [System.IO.File]::WriteAllBytes($Destination, $buffer.ToArray())
    }
    finally {
        $writer.Dispose()
    }
    return $entries
}

function Test-PboChecksum {
    <#
    .SYNOPSIS
        True when the PBO ends with 0x00 plus the SHA1 of every preceding byte.
    #>
    [CmdletBinding()]
    [OutputType([bool])]
    param([Parameter(Mandatory)][string]$Path)

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 22 -or $bytes[$bytes.Length - 21] -ne 0) { return $false }
    $sha1 = [System.Security.Cryptography.SHA1]::Create()
    try {
        $hash = $sha1.ComputeHash($bytes, 0, $bytes.Length - 21)
    }
    finally {
        $sha1.Dispose()
    }
    for ($i = 0; $i -lt 20; $i++) {
        if ($hash[$i] -ne $bytes[$bytes.Length - 20 + $i]) { return $false }
    }
    return $true
}

Export-ModuleMember -Function Get-PboContent, Compare-PboToSource, Get-PboSourceFile, Build-PboArchive, Test-PboChecksum
