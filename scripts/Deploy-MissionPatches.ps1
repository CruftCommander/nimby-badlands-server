#Requires -Version 5.1

<#
.SYNOPSIS
    Applies the patch files under server\mission-overrides to the live HostHavoc mission over SFTP:
    pull the live file, prove it still matches the reference it was drafted from, patch a copy,
    stage it, and (only with -Upload) send it back and read it back.

.DESCRIPTION
    The October 28, 2026 helicopter change set the pattern this script makes reusable: a row is
    never written to the live server unless the live file still reads exactly what the patch was
    drafted against. Each patch file carries whole entries (a <type>, <event> or <group>) and says
    nothing about mode; the mode comes from the reference copy in server\live\:

      A patch file whose header comment says APPLIED is refused: it is the record of a past change.
      REPLACE  the entry exists in the reference. The live entry must be byte-for-byte the
               reference entry (comments and whitespace ignored), or the run FAILS before any
               file is written.
      ADD      the entry is absent from the reference. It must be absent from the live file too,
               or the run FAILS.
      ALREADY  the live entry already equals the patch entry (a re-run after a successful upload).
               Nothing to do for that entry.

    Steps:
      1. Resolve each -Patch file to the mission file it targets by its root element:
         <types> -> db\types.xml, <events> -> db\events.xml, <spawnabletypes> -> cfgspawnabletypes.xml,
         <eventposdef> -> cfgeventspawns.xml, <prototype> -> mapgroupproto.xml,
         <economycore> -> cfgeconomycore.xml (its <default> rows live under <defaults>).
      2. Pull each targeted live file into <repo>\logs\deploy-<stamp>\live\ (Posh-SSH), or copy it
         from -LiveDir when given (a dry run against a local copy; no network).
      3. Plan: every entry of every patch gets ADD, REPLACE, ALREADY or FAIL. One FAIL stops the run
         with exit 1 and nothing patched.
      4. Patch a copy of each live file into <repo>\logs\deploy-<stamp>\patched\<relative path>,
         re-parse it, record entry counts and SHA-256 hashes in manifest.log.
      5. Only with -Upload, and after the ShouldProcess confirmation (ConfirmImpact High; -WhatIf
         lists): upload each patched file to the remote mission folder with Set-SFTPItem -Force,
         read it back into readback\, and compare hashes. A mismatch is FAIL.

    Reads   : the -Patch files; <repo>\server\live\<file> (or -ReferenceDir) as the reference;
              the live mission files over SFTP (or -LiveDir); <repo>\scripts\testserver.local.json
              (git-ignored) for "sftpHost", "sftpPort", "sftpUser", "sftpProfileDir" or
              "sftpMissionDir", "sftpCredentialFile"; the saved DPAPI credential file.
    Writes  : <repo>\logs\deploy-<stamp>\ (git-ignored): live\, patched\, readback\, manifest.log.
              With -Upload: the targeted files inside the remote mission folder, nothing else.
    Never   : starts, stops or restarts the server (that is the HostHavoc panel, inside the restart
              chain, Jeff's hand); writes server\live\, the replica, storage_*, serverDZ*.cfg,
              the profile folder or any file it did not pull; writes or prints the password.

    Order at the restart (docs/operations/restart-schedule.md; release 1 pattern): stop the server
    in the panel, take the panel backup, run this script with -Upload, start the server, watch the
    first CE log for "Type ... will be ignored" naming any patched class, mark the LOOT-CHANGELOG
    section APPLIED, run Sync-LiveReplica.ps1 so server\live\ moves forward.

    Needs the Posh-SSH module once per user (Install-Module Posh-SSH -Scope CurrentUser) unless
    -LiveDir is given.

.PARAMETER Patch
    One or more patch files (server\mission-overrides\...). Several patches of one mission file
    are applied to the same copy in the order given.
.PARAMETER ReferenceDir
    The folder holding the reference copies the patches were drafted against. Default
    <repo>\server\live.
.PARAMETER LiveDir
    A local folder laid out like the mission folder (db\types.xml, cfgeventspawns.xml, ...). When
    given, the live files are copied from here instead of pulled over SFTP, and -Upload is refused.
.PARAMETER Upload
    Send the patched files to the server. Without it the run stops after staging.
.PARAMETER Mission
    The mission folder name on the server. Default dayzOffline.nasdara.
.PARAMETER RemoteMissionDir
    The remote mission folder. Default: "sftpMissionDir" from testserver.local.json, else the
    parent of "sftpProfileDir" plus /mpmissions/<Mission>.

.EXAMPLE
    # Dry run against the local replica: plan and stage only, no network. Run from the repo folder in
    # an open PowerShell; "pwsh -File" joins an array argument into one string, so pass wildcards.
    .\scripts\Deploy-MissionPatches.ps1 -LiveDir D:\DayZServer\mpmissions\dayzOffline.nasdara -Patch .\server\mission-overrides\*.dpv.patch.xml, .\server\mission-overrides\db\*.dpv.patch.xml, .\server\mission-overrides\db\types.release2-loot.patch.xml

.EXAMPLE
    # Release 2: pull live, verify, stage; then the same line with -Upload once the server is stopped
    .\scripts\Deploy-MissionPatches.ps1 -Patch .\server\mission-overrides\*.dpv.patch.xml, .\server\mission-overrides\db\*.dpv.patch.xml, .\server\mission-overrides\db\types.release2-loot.patch.xml, .\server\mission-overrides\db\types.oasis-military.patch.xml
    .\scripts\Deploy-MissionPatches.ps1 -Upload -Patch .\server\mission-overrides\*.dpv.patch.xml, .\server\mission-overrides\db\*.dpv.patch.xml, .\server\mission-overrides\db\types.release2-loot.patch.xml, .\server\mission-overrides\db\types.oasis-military.patch.xml

.EXAMPLE
    # The same dry run through pwsh -File: one wildcard per argument, nothing nested
    pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Deploy-MissionPatches.ps1 -LiveDir D:\DayZServer\mpmissions\dayzOffline.nasdara .\server\mission-overrides\*.dpv.patch.xml .\server\mission-overrides\db\*.dpv.patch.xml .\server\mission-overrides\db\types.release2-loot.patch.xml
#>
[CmdletBinding(SupportsShouldProcess, ConfirmImpact = 'High', PositionalBinding = $false)]
param(
    # Patch files: paths, wildcards (server\mission-overrides\*.dpv.patch.xml) or comma-separated lists;
    # every unnamed argument is a patch too, so "pwsh -File ... a.xml b.xml -LiveDir X" works.
    [Parameter(Position = 0, ValueFromRemainingArguments = $true)][string[]]$Patch,
    [string]$ReferenceDir,
    [string]$LiveDir,
    [switch]$Upload,
    [string]$Mission = 'dayzOffline.nasdara',
    [string]$RemoteMissionDir,
    [string]$ComputerName,
    [int]$Port,
    [string]$UserName,
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }
$ErrorActionPreference = 'Stop'

Import-Module (Join-Path $PSScriptRoot 'modules\NimbyCommon.psm1') -Force

# Root element of a patch file -> the mission file it targets and the entry element inside it.
# Container: the element under the root that holds the entries ('' = the root itself).
$script:PatchTargets = @{
    'types'          = @{ Relative = 'db\types.xml'; Entry = 'type'; Container = '' }
    'events'         = @{ Relative = 'db\events.xml'; Entry = 'event'; Container = '' }
    'spawnabletypes' = @{ Relative = 'cfgspawnabletypes.xml'; Entry = 'type'; Container = '' }
    'eventposdef'    = @{ Relative = 'cfgeventspawns.xml'; Entry = 'event'; Container = '' }
    'prototype'      = @{ Relative = 'mapgroupproto.xml'; Entry = 'group'; Container = '' }
    'economycore'    = @{ Relative = 'cfgeconomycore.xml'; Entry = 'default'; Container = 'defaults' }
}

function Read-XmlFile {
    # Loads an XML file with whitespace preserved, so a patched copy keeps the live file's layout.
    param([Parameter(Mandatory)][string]$Path)
    $doc = New-Object System.Xml.XmlDocument
    $doc.PreserveWhitespace = $true
    $doc.Load((Resolve-Path -LiteralPath $Path).ProviderPath)
    return $doc
}

function Get-PatchDescriptor {
    # The mission file a patch targets, from its root element. Throws on an unknown root.
    param([Parameter(Mandatory)][string]$PatchPath)
    $doc = Read-XmlFile -Path $PatchPath
    $rootName = $doc.DocumentElement.LocalName
    if (-not $script:PatchTargets.ContainsKey($rootName)) {
        throw ("{0}: root element <{1}> is not a mission file this script knows ({2})." -f $PatchPath, $rootName, (($script:PatchTargets.Keys | Sort-Object) -join ', '))
    }
    $target = $script:PatchTargets[$rootName]
    # A patch whose header records APPLIED is history, not an input: release 1's
    # mapgroupproto.cisterns.patch.xml carries the changed lines only (no <point> rows), so replaying it
    # would strip the live groups' points. The LOOT-CHANGELOG rule marks every applied patch this way.
    $header = @($doc.ChildNodes | Where-Object { $_.NodeType -eq 'Comment' } | Select-Object -First 1)
    if ($header.Count -eq 1 -and $header[0].Value -cmatch '\bAPPLIED\b') {   # case-sensitive: DRAFT headers say "not applied"
        throw ("{0}: its header says APPLIED; an applied patch is a record, not a deploy input." -f $PatchPath)
    }
    return [PSCustomObject]@{
        PatchPath = (Resolve-Path -LiteralPath $PatchPath).ProviderPath
        Root      = $rootName
        Relative  = $target.Relative
        Entry     = $target.Entry
        Container = $target.Container
        Document  = $doc
    }
}

function ConvertTo-CanonicalXml {
    # One string per element: name, attributes sorted by name, then child elements and non-blank
    # text, recursively. Comments and whitespace do not count, so a re-indented row still matches.
    param([Parameter(Mandatory)][System.Xml.XmlNode]$Node)
    $builder = New-Object System.Text.StringBuilder
    [void]$builder.Append('<').Append($Node.LocalName)
    foreach ($attribute in @($Node.Attributes | Sort-Object Name)) {
        [void]$builder.Append(' ').Append($attribute.Name).Append('="').Append($attribute.Value).Append('"')
    }
    [void]$builder.Append('>')
    foreach ($child in @($Node.ChildNodes)) {
        switch ($child.NodeType) {
            'Element' { [void]$builder.Append((ConvertTo-CanonicalXml -Node $child)) }
            'Text' { $text = $child.Value.Trim(); if ($text -ne '') { [void]$builder.Append($text) } }
            'CDATA' { $text = $child.Value.Trim(); if ($text -ne '') { [void]$builder.Append($text) } }
            default { }
        }
    }
    [void]$builder.Append('</').Append($Node.LocalName).Append('>')
    return $builder.ToString()
}

function Get-EntryParent {
    # The element that holds the entries: the root, or the single child named -Container
    # (cfgeconomycore.xml keeps its <default> rows under <defaults>). Throws when the container is
    # missing or repeated, so a patch never lands in the wrong place.
    param(
        [Parameter(Mandatory)][System.Xml.XmlDocument]$Document,
        [AllowEmptyString()][string]$Container = ''
    )
    if (-not $Container) { return $Document.DocumentElement }
    $found = @($Document.DocumentElement.SelectNodes(("./*[local-name()='{0}']" -f $Container)))
    if ($found.Count -ne 1) { throw ("Expected one <{0}> under <{1}>, found {2}." -f $Container, $Document.DocumentElement.LocalName, $found.Count) }
    return $found[0]
}

function Get-NamedEntry {
    # The children of the entry parent with the given element name and name="..." attribute.
    param(
        [Parameter(Mandatory)][System.Xml.XmlDocument]$Document,
        [Parameter(Mandatory)][string]$Entry,
        [Parameter(Mandatory)][string]$Name,
        [AllowEmptyString()][string]$Container = ''
    )
    $parent = Get-EntryParent -Document $Document -Container $Container
    # The leading comma keeps a one-element result an array when the function returns it.
    return , @($parent.SelectNodes(("./*[local-name()='{0}'][@name='{1}']" -f $Entry, $Name)))
}

function Get-PatchPlan {
    # One row per patch entry: Name, Mode (ADD, REPLACE, ALREADY, FAIL) and Reason. A row FAILS when
    # the live file has drifted from the reference (REPLACE), already holds an ADD entry, holds the
    # entry more than once, or the patch itself repeats a name.
    param(
        [Parameter(Mandatory)][System.Xml.XmlDocument]$PatchDocument,
        [Parameter(Mandatory)][System.Xml.XmlDocument]$LiveDocument,
        [Parameter(Mandatory)][System.Xml.XmlDocument]$ReferenceDocument,
        [Parameter(Mandatory)][string]$Entry,
        [AllowEmptyString()][string]$Container = ''
    )
    $rows = [System.Collections.Generic.List[object]]::new()
    $seen = @{}
    $patchParent = Get-EntryParent -Document $PatchDocument -Container $Container
    foreach ($node in @($patchParent.ChildNodes | Where-Object { $_.NodeType -eq 'Element' })) {
        $name = $node.GetAttribute('name')
        $mode = 'FAIL'; $reason = ''
        if ($node.LocalName -ne $Entry) {
            $reason = ("element <{0}> where <{1}> was expected" -f $node.LocalName, $Entry)
        }
        elseif (-not $name) {
            $reason = 'entry has no name attribute'
        }
        elseif ($seen.ContainsKey($name)) {
            $reason = 'name repeated inside the patch'
        }
        else {
            $seen[$name] = $true
            $live = Get-NamedEntry -Document $LiveDocument -Entry $Entry -Name $name -Container $Container
            $reference = Get-NamedEntry -Document $ReferenceDocument -Entry $Entry -Name $name -Container $Container
            $patchCanonical = ConvertTo-CanonicalXml -Node $node
            if ($live.Count -gt 1) {
                $reason = ("live holds {0} entries of that name" -f $live.Count)
            }
            elseif ($live.Count -eq 1 -and (ConvertTo-CanonicalXml -Node $live[0]) -eq $patchCanonical) {
                $mode = 'ALREADY'; $reason = 'live already equals the patch'
            }
            elseif ($reference.Count -ge 1) {
                if ($live.Count -eq 0) { $reason = 'in the reference but missing on live' }
                elseif ((ConvertTo-CanonicalXml -Node $live[0]) -ne (ConvertTo-CanonicalXml -Node $reference[0])) { $reason = 'live differs from the reference the patch was drafted against; run Sync-LiveReplica.ps1 and redraft' }
                else { $mode = 'REPLACE' }
            }
            else {
                if ($live.Count -eq 0) { $mode = 'ADD' }
                else { $reason = 'not in the reference, but live already holds an entry of that name' }
            }
        }
        $rows.Add([PSCustomObject]@{ Name = $name; Mode = $mode; Reason = $reason; Node = $node })
    }
    return $rows
}

function Merge-PatchIntoDocument {
    # Applies a plan to the live document in place: REPLACE swaps the node, ADD appends it after the
    # last entry with the same indentation, ALREADY and anything else leaves the document alone.
    param(
        [Parameter(Mandatory)][System.Xml.XmlDocument]$LiveDocument,
        [Parameter(Mandatory)][AllowEmptyCollection()][object[]]$Plan,
        [Parameter(Mandatory)][string]$Entry,
        [AllowEmptyString()][string]$Container = ''
    )
    $root = Get-EntryParent -Document $LiveDocument -Container $Container
    $added = 0; $replaced = 0
    foreach ($row in $Plan) {
        if ($row.Mode -eq 'REPLACE') {
            $existing = (Get-NamedEntry -Document $LiveDocument -Entry $Entry -Name $row.Name -Container $Container)[0]
            [void]$root.ReplaceChild($LiveDocument.ImportNode($row.Node, $true), $existing)
            $replaced++
        }
        elseif ($row.Mode -eq 'ADD') {
            $lastElement = $null
            foreach ($child in @($root.ChildNodes)) { if ($child.NodeType -eq 'Element') { $lastElement = $child } }
            $imported = $LiveDocument.ImportNode($row.Node, $true)
            if ($null -eq $lastElement) {
                [void]$root.AppendChild($imported)
            }
            else {
                $indent = "`r`n    "
                $before = $lastElement.PreviousSibling
                if ($null -ne $before -and ($before.NodeType -eq 'Whitespace' -or $before.NodeType -eq 'SignificantWhitespace')) { $indent = $before.Value }
                $whitespace = $LiveDocument.CreateWhitespace($indent)
                [void]$root.InsertAfter($whitespace, $lastElement)
                [void]$root.InsertAfter($imported, $whitespace)
            }
            $added++
        }
    }
    return [PSCustomObject]@{ Added = $added; Replaced = $replaced }
}

function Get-FileNewLine {
    # "`r`n" when the file's first line break is CRLF, else "`n". The live mission files are not
    # uniform (November 8, 2026 mirror: types.xml, events.xml and cfgspawnabletypes.xml LF,
    # cfgeventspawns.xml CRLF), so a patched copy keeps the line ending of the file it replaces.
    param([Parameter(Mandatory)][string]$Path)
    $raw = [System.IO.File]::ReadAllText($Path)
    $index = $raw.IndexOf("`n")
    if ($index -gt 0 -and $raw[$index - 1] -eq "`r") { return "`r`n" }
    return "`n"
}

function Save-XmlFile {
    # Writes the document as UTF-8 without a byte-order mark (as the live files are stored),
    # whitespace as held, every line break normalised to -NewLine (the live file's own, by default
    # CRLF). Patch entries arrive with CRLF from the repository and are normalised with the rest.
    param(
        [Parameter(Mandatory)][System.Xml.XmlDocument]$Document,
        [Parameter(Mandatory)][string]$Path,
        [string]$NewLine = "`r`n"
    )
    $directory = Split-Path -Parent $Path
    if ($directory -and -not (Test-Path -LiteralPath $directory)) { New-Item -ItemType Directory -Path $directory -Force -WhatIf:$false | Out-Null }
    $settings = New-Object System.Xml.XmlWriterSettings
    $settings.Encoding = New-Object System.Text.UTF8Encoding($false)
    $settings.Indent = $false
    $settings.NewLineHandling = [System.Xml.NewLineHandling]::None
    $writer = [System.Xml.XmlWriter]::Create($Path, $settings)
    try { $Document.Save($writer) } finally { $writer.Dispose() }
    # The XML writer keeps each text node's own line ends, so mixed endings are normalised here.
    $raw = [System.IO.File]::ReadAllText($Path)
    $normalized = [regex]::Replace($raw, "`r?`n", $NewLine)
    if ($normalized -ne $raw) { [System.IO.File]::WriteAllText($Path, $normalized, (New-Object System.Text.UTF8Encoding($false))) }
}

function Resolve-PatchPath {
    # Expands each -Patch value: comma-separated pieces, wildcards and relative paths, in order,
    # without duplicates. Throws when a piece matches nothing.
    param([Parameter(Mandatory)][AllowEmptyCollection()][string[]]$Patch)
    $paths = [System.Collections.Generic.List[string]]::new()
    foreach ($item in $Patch) {
        foreach ($piece in ($item -split ',')) {
            $trimmed = $piece.Trim()
            if (-not $trimmed) { continue }
            $resolved = @(Resolve-Path -Path $trimmed -ErrorAction SilentlyContinue | ForEach-Object { $_.ProviderPath } | Sort-Object)
            if ($resolved.Count -eq 0) { throw ("No patch file matches '{0}'." -f $trimmed) }
            foreach ($path in $resolved) { if (-not $paths.Contains($path)) { $paths.Add($path) } }
        }
    }
    return , $paths.ToArray()
}

function Get-EntryCount {
    param([Parameter(Mandatory)][System.Xml.XmlDocument]$Document, [Parameter(Mandatory)][string]$Entry, [AllowEmptyString()][string]$Container = '')
    $parent = Get-EntryParent -Document $Document -Container $Container
    return @($parent.SelectNodes(("./*[local-name()='{0}']" -f $Entry))).Count
}

function Get-FileSha256 {
    param([Parameter(Mandatory)][string]$Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

function Join-RemotePath {
    param([Parameter(Mandatory)][string]$Parent, [Parameter(Mandatory)][string]$Child)
    if ($Parent.EndsWith('/')) { return "$Parent$Child" }
    return "$Parent/$Child"
}

function Get-RemoteMissionDirDefault {
    # "sftpMissionDir" from the local settings, else the parent of "sftpProfileDir" plus
    # /mpmissions/<Mission> (HostHavoc keeps the profile folder and mpmissions side by side).
    param([Parameter(Mandatory)][string]$RepoRoot, [Parameter(Mandatory)][string]$Mission)
    $setting = [string](Get-NimbyLocalSetting -RepoRoot $RepoRoot -Name 'sftpMissionDir')
    if ($setting) { return $setting }
    $profileDir = [string](Get-NimbyLocalSetting -RepoRoot $RepoRoot -Name 'sftpProfileDir')
    if (-not $profileDir) { return '' }
    $parent = ($profileDir.TrimEnd('/') -replace '/[^/]*$', '')
    if (-not $parent) { $parent = '/' }
    return (Join-RemotePath -Parent (Join-RemotePath -Parent $parent -Child 'mpmissions') -Child $Mission)
}

# ---------------------------------------------------------------- main
if ($MyInvocation.InvocationName -eq '.') { return }   # dot-sourced by tests: functions only

if (-not $Patch -or $Patch.Count -eq 0) { throw 'Pass at least one patch file with -Patch.' }
$Patch = Resolve-PatchPath -Patch $Patch
if ($Upload -and $LiveDir) { throw '-Upload cannot be combined with -LiveDir: a local copy is not the live server.' }
if (-not $ReferenceDir) { $ReferenceDir = Join-Path $RepoRoot 'server\live' }

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$outDir = Join-Path $RepoRoot "logs\deploy-$stamp"
$liveOut = Join-Path $outDir 'live'
$patchedOut = Join-Path $outDir 'patched'
$readbackOut = Join-Path $outDir 'readback'
New-Item -ItemType Directory -Path $outDir -Force -WhatIf:$false | Out-Null
$manifest = Join-Path $outDir 'manifest.log'
$mode = if ($Upload) { 'upload' } else { 'stage' }
# -WhatIf gates only the upload (step 5). Pulling, planning and staging always happen, so the
# preference is read once here and cleared, the way Sync-LiveReplica.ps1 handles its dry run.
$dryRun = [bool]$WhatIfPreference
$WhatIfPreference = $false
if ($dryRun) { $mode = "$mode-whatif" }
Write-Manifest -LogPath $manifest -Line ("Deploy-MissionPatches {0} mode={1} mission={2} reference={3} patches={4}" -f $stamp, $mode, $Mission, $ReferenceDir, $Patch.Count)

# 1. Descriptors, grouped by target file in the order first seen.
$descriptors = @($Patch | ForEach-Object { Get-PatchDescriptor -PatchPath $_ })
$targets = [System.Collections.Generic.List[string]]::new()
foreach ($descriptor in $descriptors) {
    if (-not $targets.Contains($descriptor.Relative)) { $targets.Add($descriptor.Relative) }
    Write-Manifest -LogPath $manifest -Line ("PATCHFILE {0} -> {1}" -f $descriptor.PatchPath, $descriptor.Relative)
}

# 2. Live copies: SFTP pull, or -LiveDir.
$session = $null
$fail = 0
$uploaded = 0
try {
    if ($LiveDir) {
        foreach ($relative in $targets) {
            $source = Join-Path $LiveDir $relative
            if (-not (Test-Path -LiteralPath $source)) { throw ("-LiveDir has no {0}." -f $relative) }
            $destination = Join-Path $liveOut $relative
            New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force -WhatIf:$false | Out-Null
            Copy-Item -LiteralPath $source -Destination $destination -Force -WhatIf:$false
            Write-Manifest -LogPath $manifest -Line ("LIVE {0} from={1} sha256={2}" -f $relative, $source, (Get-FileSha256 -Path $destination))
        }
    }
    else {
        if (-not (Get-Module -ListAvailable -Name Posh-SSH)) { throw 'Posh-SSH is not installed. Run: Install-Module Posh-SSH -Scope CurrentUser' }
        Import-Module Posh-SSH
        if (-not $ComputerName) { $ComputerName = [string](Get-NimbyLocalSetting -RepoRoot $RepoRoot -Name 'sftpHost') }
        if (-not $Port) {
            $settingPort = Get-NimbyLocalSetting -RepoRoot $RepoRoot -Name 'sftpPort'
            if ($null -ne $settingPort) { $Port = [int]$settingPort }
        }
        if (-not $UserName) { $UserName = [string](Get-NimbyLocalSetting -RepoRoot $RepoRoot -Name 'sftpUser') }
        if (-not $RemoteMissionDir) { $RemoteMissionDir = Get-RemoteMissionDirDefault -RepoRoot $RepoRoot -Mission $Mission }
        $missing = @()
        if (-not $ComputerName) { $missing += '-ComputerName (sftpHost)' }
        if (-not $Port) { $missing += '-Port (sftpPort)' }
        if (-not $UserName) { $missing += '-UserName (sftpUser)' }
        if (-not $RemoteMissionDir) { $missing += '-RemoteMissionDir (sftpMissionDir or sftpProfileDir)' }
        if ($missing.Count -gt 0) { throw ("Missing {0}. Pass them, or set them in {1}." -f ($missing -join ', '), (Get-NimbyLocalSettingPath -RepoRoot $RepoRoot)) }
        Write-Manifest -LogPath $manifest -Line ("REMOTE host={0}:{1} user={2} missionDir={3}" -f $ComputerName, $Port, $UserName, $RemoteMissionDir)

        $credential = Read-NimbySftpCredential -Path (Get-NimbySftpCredentialPath -RepoRoot $RepoRoot) -UserName $UserName
        if ($null -eq $credential) {
            Write-Manifest -LogPath $manifest -Line 'CREDENTIAL prompt'
            $credential = Get-Credential -UserName $UserName -Message "HostHavoc panel password for $UserName (SFTP ${ComputerName}:$Port)"
        }
        else { Write-Manifest -LogPath $manifest -Line 'CREDENTIAL saved file' }

        $session = New-SFTPSession -ComputerName $ComputerName -Port $Port -Credential $credential -ConnectionTimeout 20
        $sessionId = $session.SessionId
        # The folder must look like a mission: db/types.xml present.
        $probe = @(Get-SFTPChildItem -SessionId $sessionId -Path (Join-RemotePath -Parent $RemoteMissionDir -Child 'db') -File | Where-Object { $_.FullName -match '/types\.xml$' })
        if ($probe.Count -ne 1) { throw ("{0} does not look like a mission folder (no db/types.xml). Pass -RemoteMissionDir." -f $RemoteMissionDir) }
        foreach ($relative in $targets) {
            $remoteFile = Join-RemotePath -Parent $RemoteMissionDir -Child ($relative -replace '\\', '/')
            $destination = Join-Path $liveOut $relative
            $destinationDir = Split-Path -Parent $destination
            New-Item -ItemType Directory -Path $destinationDir -Force -WhatIf:$false | Out-Null
            Get-SFTPItem -SessionId $sessionId -Path $remoteFile -Destination $destinationDir -Force
            if (-not (Test-Path -LiteralPath $destination)) { throw ("Pull of {0} produced no file." -f $remoteFile) }
            Write-Manifest -LogPath $manifest -Line ("LIVE {0} from={1} sha256={2}" -f $relative, $remoteFile, (Get-FileSha256 -Path $destination))
        }
    }

    # 3. Plan, every patch against its live and reference copy. A FAIL anywhere stops before step 4.
    $plans = @{}
    foreach ($descriptor in $descriptors) {
        $livePath = Join-Path $liveOut $descriptor.Relative
        $referencePath = Join-Path $ReferenceDir $descriptor.Relative
        if (-not (Test-Path -LiteralPath $referencePath)) {
            Write-Manifest -LogPath $manifest -Line ("PLAN {0} FAIL no reference copy at {1}; run Sync-LiveReplica.ps1 (it mirrors this file since November 8, 2026) or pass -ReferenceDir" -f $descriptor.Relative, $referencePath)
            $fail++
            continue
        }
        if (-not $plans.ContainsKey($descriptor.Relative)) { $plans[$descriptor.Relative] = @{ Live = (Read-XmlFile -Path $livePath); Rows = [System.Collections.Generic.List[object]]::new(); Entry = $descriptor.Entry; Container = $descriptor.Container } }
        $reference = Read-XmlFile -Path $referencePath
        $rows = @(Get-PatchPlan -PatchDocument $descriptor.Document -LiveDocument $plans[$descriptor.Relative].Live -ReferenceDocument $reference -Entry $descriptor.Entry -Container $descriptor.Container)
        foreach ($row in $rows) {
            $line = ("PLAN {0} {1} {2}" -f $descriptor.Relative, $row.Mode, $row.Name)
            if ($row.Reason) { $line += (" ({0})" -f $row.Reason) }
            Write-Manifest -LogPath $manifest -Line $line
            Write-Information $line -InformationAction Continue
            if ($row.Mode -eq 'FAIL') { $fail++ }
            $plans[$descriptor.Relative].Rows.Add($row)
        }
    }
    $planFailures = $fail
    if ($planFailures -gt 0) {
        Write-Manifest -LogPath $manifest -Line ("PLAN STOP plan-failures={0}; nothing patched, nothing uploaded" -f $planFailures)
    }

    # 4. Patch copies and stage them (skipped entirely after a plan failure).
    $staged = [System.Collections.Generic.List[object]]::new()
    foreach ($relative in @($targets | Where-Object { $planFailures -eq 0 })) {
        $plan = $plans[$relative]
        $before = Get-EntryCount -Document $plan.Live -Entry $plan.Entry -Container $plan.Container
        $result = Merge-PatchIntoDocument -LiveDocument $plan.Live -Plan $plan.Rows.ToArray() -Entry $plan.Entry -Container $plan.Container
        $outPath = Join-Path $patchedOut $relative
        $newLine = Get-FileNewLine -Path (Join-Path $liveOut $relative)
        Save-XmlFile -Document $plan.Live -Path $outPath -NewLine $newLine
        $check = Read-XmlFile -Path $outPath   # re-parse: a broken file never leaves this step
        $after = Get-EntryCount -Document $check -Entry $plan.Entry -Container $plan.Container
        if ($after -ne ($before + $result.Added)) { throw ("{0}: {1} entries before, {2} added, but {3} after the write." -f $relative, $before, $result.Added, $after) }
        $alreadyCount = @($plan.Rows | Where-Object { $_.Mode -eq 'ALREADY' }).Count
        $eol = if ($newLine -eq "`r`n") { 'CRLF' } else { 'LF' }
        Write-Manifest -LogPath $manifest -Line ("PATCH {0} entries={1}->{2} added={3} replaced={4} already={5} eol={6} sha256={7} staged={8}" -f $relative, $before, $after, $result.Added, $result.Replaced, $alreadyCount, $eol, (Get-FileSha256 -Path $outPath), $outPath)
        $staged.Add([PSCustomObject]@{ Relative = $relative; Path = $outPath; Changed = (($result.Added + $result.Replaced) -gt 0) })
    }

    # 5. Upload and read back.
    if ($Upload -and $planFailures -eq 0) {
        foreach ($item in $staged) {
            if (-not $item.Changed) { Write-Manifest -LogPath $manifest -Line ("UPLOAD {0} SKIP nothing changed" -f $item.Relative); continue }
            $remoteDir = $RemoteMissionDir
            $parent = Split-Path -Parent $item.Relative
            if ($parent) { $remoteDir = Join-RemotePath -Parent $RemoteMissionDir -Child ($parent -replace '\\', '/') }
            $remoteFile = Join-RemotePath -Parent $remoteDir -Child (Split-Path -Leaf $item.Relative)
            if ($dryRun -or -not $PSCmdlet.ShouldProcess($remoteFile, ("Overwrite the live {0} with the patched copy" -f $item.Relative))) {
                Write-Manifest -LogPath $manifest -Line ("UPLOAD {0} WHATIF -> {1}" -f $item.Relative, $remoteFile)
                continue
            }
            Set-SFTPItem -SessionId $sessionId -Path $item.Path -Destination $remoteDir -Force
            $readbackDir = Split-Path -Parent (Join-Path $readbackOut $item.Relative)
            New-Item -ItemType Directory -Path $readbackDir -Force -WhatIf:$false | Out-Null
            Get-SFTPItem -SessionId $sessionId -Path $remoteFile -Destination $readbackDir -Force
            $localHash = Get-FileSha256 -Path $item.Path
            $remoteHash = Get-FileSha256 -Path (Join-Path $readbackOut $item.Relative)
            if ($localHash -eq $remoteHash) {
                Write-Manifest -LogPath $manifest -Line ("UPLOAD {0} OK remote={1} sha256={2}" -f $item.Relative, $remoteFile, $remoteHash)
                $uploaded++
            }
            else {
                Write-Manifest -LogPath $manifest -Line ("UPLOAD {0} FAIL readback sha256 {1} differs from staged {2}" -f $item.Relative, $remoteHash, $localHash)
                $fail++
            }
        }
    }
}
finally {
    if ($null -ne $session) { Remove-SFTPSession -SessionId $session.SessionId | Out-Null }
}

$verdict = if ($fail -gt 0) { 'FAIL' } else { 'PASS' }
$summary = "SUMMARY $verdict mode=$mode files=$($targets.Count) plan-failures=$fail uploaded=$uploaded staged=$patchedOut manifest=$manifest"
Write-Manifest -LogPath $manifest -Line $summary
Write-Information $summary -InformationAction Continue
if ($fail -gt 0) { exit 1 }
exit 0
