#Requires -Version 5.1

<#
.SYNOPSIS
    Builds or refreshes the local replica of the NIMBY Badlands HostHavoc server at D:\DayZServer
    from the WinSCP mirror at D:\HostHavoc\NIMBY Badlands\104.192.227.46_2302.

.DESCRIPTION
    Reads (never modifies):
      - The mirror: serverDZ_By_HostHavoc.cfg (one line, "template="), mpmissions\<mission>\ including
        storage_1, and when -BinarySource Mirror, the server binaries (dta, addons, nasdara, sakhal, keys,
        root exe and dll files).
      - The Steam DayZServer install (default binary source), found through the Steam library list or
        -SteamServerPath.

    Writes:
      - <ReplicaRoot>\ (default D:\DayZServer): server binaries (only when the replica has no
        DayZServer_x64.exe yet, or -RefreshBinaries is given), mpmissions\<mission>\ (mission files,
        never storage_*), mpmissions\<mission>\storage_998 (a copy of the live storage_1; the previous
        storage_998 is zipped to <ReplicaRoot>\_backups\ first), and an empty
        mpmissions\<mission>\storage_999 (the fresh world; created only if absent, or recreated with
        -ResetFreshWorld after a zip).
      - <RepoRoot>\server\live\: the mission's loot and gameplay files (types.xml, globals.xml,
        economy.xml, events.xml, messages.xml, cfgspawnabletypes.xml, cfggameplay.json,
        cfgeconomycore.xml, cfglimitsdefinition.xml) and SOURCE.txt (when and where they were copied).
      - <RepoRoot>\logs\replica-<stamp>\: a manifest and one robocopy log per step. -WhatIf still writes
        the manifest and runs every robocopy step with /L (list only); nothing else is written.

    Never touches: serverDZ.cfg, serverDZ_By_HostHavoc.cfg, battleye\, HostHavocDayZServer\ (the live
    profile folder: RPTs, COT state, BE config with the RCON password), ban.txt, whitelist.txt, logs\,
    Backups\, steamapps\, appcache\, server_manager\, and the live storage_1 itself. Nothing in the
    mirror is ever written.

    storage_998 is the live world. storage_999 is the fresh world. Start-NimbyTestServer.ps1 -InstanceId
    picks one per run.

.PARAMETER MirrorRoot
    The WinSCP sync target that holds the server root (the folder with serverDZ_By_HostHavoc.cfg).

.PARAMETER ReplicaRoot
    The replica server root. Start-NimbyTestServer.ps1 already looks here after the Steam library.

.PARAMETER BinarySource
    Steam (default): copy binaries from the Steam DayZServer install. Mirror: copy them from the
    mirror, so the replica runs the exact live build. Mirror requires DayZServer_x64.exe to be present
    in the mirror. Observed October 15, 2026: the HostHavoc FTP view omits DayZServer_x64.exe and the
    keys folder (every other binary matched the Steam install file for file, build 25460313), so
    Steam is the only working source until the host exposes the exe.

.PARAMETER SteamServerPath
    The Steam DayZServer folder when discovery through the Steam library list fails.

.PARAMETER RefreshBinaries
    Copy the binaries again even though the replica already has DayZServer_x64.exe.

.PARAMETER ResetFreshWorld
    Zip the current storage_999 to _backups and recreate it empty.

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Sync-LiveReplica.ps1

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Sync-LiveReplica.ps1 -BinarySource Mirror -RefreshBinaries
#>
[CmdletBinding(SupportsShouldProcess)]
param(
    [string]$RepoRoot,
    [string]$MirrorRoot = 'D:\HostHavoc\NIMBY Badlands\104.192.227.46_2302',
    [string]$ReplicaRoot = 'D:\DayZServer',
    [ValidateSet('Steam', 'Mirror')][string]$BinarySource = 'Steam',
    [string]$SteamServerPath,
    [switch]$RefreshBinaries,
    [switch]$ResetFreshWorld
)

Set-StrictMode -Version Latest
# Windows PowerShell 5.1 leaves $PSScriptRoot empty in a param() default when an advanced
# script is started with -File, so the default is resolved here.
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }
$ErrorActionPreference = 'Stop'

# Write-Manifest, Test-PathInside, and Get-RunningServerProcess come from NimbyCommon.psm1.
Import-Module (Join-Path $PSScriptRoot 'modules\NimbyCommon.psm1') -Force

# Shared rules, section 8: test storage is storage_998 (live world) and storage_999 (fresh world).
$script:LiveInstanceId = 998
$script:FreshInstanceId = 999

# Folders and files that never leave the mirror or the Steam install when binaries are copied.
# mpmissions is copied by its own step; the rest is runtime state, secrets, or Steam bookkeeping.
$script:BinaryExcludeDirs = @('mpmissions', 'HostHavocDayZServer', 'profiles', 'logs', 'Backups', 'steamapps', 'appcache', 'server_manager', 'config', 'docs', '_CommonRedist')
# battleye\ stays in: the server needs <root>\battleye\BEServer_x64.dll or it fails with
# "BattlEye initialization failed" (first replica boot, October 16, 2026). Only the BE config files,
# which carry the RCON password on the live server, are excluded; the test script writes its own.
# serverDZ*.cfg as a wildcard: shared rules 8.2 promise that no serverDZ variant is ever copied.
$script:BinaryExcludeFiles = @('serverDZ*.cfg', '*.cfg.bak', 'ban.txt', 'bans.txt', 'whitelist.txt', 'BEServer*.cfg', 'beserver*.cfg', '*.log', '*.RPT', '*.mdmp', '*.ADM')

# Mission files copied into the repo for the plans' Phase 0 sections. Relative to the mission folder.
$script:LiveReferenceFiles = @('db\types.xml', 'db\globals.xml', 'db\economy.xml', 'db\events.xml', 'db\messages.xml', 'cfgspawnabletypes.xml', 'cfggameplay.json', 'cfgeconomycore.xml', 'cfglimitsdefinition.xml')

function Get-MissionTemplate {
    <#
    .SYNOPSIS
        Returns the template value from a serverDZ config, reading only the template line.
    #>
    [OutputType([string])]
    param([Parameter(Mandatory)][string]$ConfigPath)

    if (-not (Test-Path -LiteralPath $ConfigPath -PathType Leaf)) { throw "Config not found: $ConfigPath" }
    foreach ($line in (Get-Content -LiteralPath $ConfigPath -Encoding utf8)) {
        if ($line -match '^\s*template\s*=\s*"([^"]+)"') { return $Matches[1] }
    }
    throw "No template line in $ConfigPath"
}

function Get-RobocopyVerdict {
    <#
    .SYNOPSIS
        Maps a robocopy exit code to OK, WARN, PARTIAL, or FAIL.
    #>
    [OutputType([string])]
    param([Parameter(Mandatory)][int]$ExitCode)

    if ($ExitCode -lt 0) { return 'FAIL' }
    if ($ExitCode -le 3) { return 'OK' }
    if ($ExitCode -le 7) { return 'WARN' }
    if ($ExitCode -le 15) { return 'PARTIAL' }
    return 'FAIL'
}

function Get-RobocopyArgumentList {
    <#
    .SYNOPSIS
        Builds the robocopy argument array. Paths never end in a backslash; no quoting is added.
    #>
    [OutputType([string[]])]
    param(
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$Destination,
        [Parameter(Mandatory)][string]$LogPath,
        [string[]]$ExcludeDirs = @(),
        [string[]]$ExcludeFiles = @(),
        [switch]$Mirror,
        [switch]$ListOnly
    )

    $argv = [System.Collections.Generic.List[string]]::new()
    $argv.Add($Source.TrimEnd('\'))
    $argv.Add($Destination.TrimEnd('\'))
    if ($Mirror) { $argv.Add('/MIR') } else { $argv.Add('/E') }
    $argv.Add('/R:2'); $argv.Add('/W:2'); $argv.Add('/NP'); $argv.Add('/NDL'); $argv.Add('/NFL')
    $argv.Add(('/UNILOG:{0}' -f $LogPath))
    if ($ExcludeDirs.Count -gt 0) { $argv.Add('/XD'); foreach ($d in $ExcludeDirs) { $argv.Add($d) } }
    if ($ExcludeFiles.Count -gt 0) { $argv.Add('/XF'); foreach ($f in $ExcludeFiles) { $argv.Add($f) } }
    if ($ListOnly) { $argv.Add('/L') }
    return $argv.ToArray()
}

function Invoke-RobocopyStep {
    <#
    .SYNOPSIS
        Runs one robocopy step and writes its manifest line. Returns the verdict.
    #>
    [OutputType([string])]
    param(
        [Parameter(Mandatory)][string]$Step,
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$Destination,
        [Parameter(Mandatory)][string]$LogDir,
        [Parameter(Mandatory)][string]$ManifestPath,
        [string[]]$ExcludeDirs = @(),
        [string[]]$ExcludeFiles = @(),
        [switch]$Mirror,
        [switch]$ListOnly
    )

    $logPath = Join-Path $LogDir ('robocopy-{0}.log' -f $Step)
    # Full paths for /XD: a bare name would match a folder of that name at every depth.
    $excludePaths = @(foreach ($d in $ExcludeDirs) { '{0}\{1}' -f $Source.TrimEnd('\'), $d })
    $argv = Get-RobocopyArgumentList -Source $Source -Destination $Destination -LogPath $logPath -ExcludeDirs $excludePaths -ExcludeFiles $ExcludeFiles -Mirror:$Mirror -ListOnly:$ListOnly
    $global:LASTEXITCODE = $null
    & robocopy @argv | Out-Null
    $code = $LASTEXITCODE
    if ($null -eq $code) { $code = 99 }   # robocopy did not run; never read as success
    $verdict = Get-RobocopyVerdict -ExitCode $code
    $mode = if ($Mirror) { 'mirror' } else { 'copy' }
    if ($ListOnly) { $mode = 'list-only' }
    Write-Manifest -LogPath $ManifestPath -Line ("{0} {1} exit={2} mode={3} src={4} dst={5} xd={6} xf={7} log={8}" -f $verdict, $Step, $code, $mode, $Source, $Destination, ($ExcludeDirs -join ','), ($ExcludeFiles -join ','), $logPath)
    return $verdict
}

function Copy-ReferenceFile {
    <#
    .SYNOPSIS
        Copies one named mission file into server\live, testing it first. Returns OK or SKIP.
    #>
    [OutputType([string])]
    param(
        [Parameter(Mandatory)][string]$MissionRoot,
        [Parameter(Mandatory)][string]$RelativePath,
        [Parameter(Mandatory)][string]$LiveDir,
        [Parameter(Mandatory)][string]$ManifestPath
    )

    $source = Join-Path $MissionRoot $RelativePath
    $destination = Join-Path $LiveDir $RelativePath
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        Write-Manifest -LogPath $ManifestPath -Line ("SKIP reference missing src={0}" -f $source)
        return 'SKIP'
    }
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination -Force
    Write-Manifest -LogPath $ManifestPath -Line ("OK reference src={0} dst={1}" -f $source, $destination)
    return 'OK'
}

function Backup-StorageFolder {
    <#
    .SYNOPSIS
        Zips a storage folder to the backup folder. Returns the zip path, or $null when the folder is absent.
    #>
    [OutputType([string])]
    param(
        [Parameter(Mandatory)][string]$StoragePath,
        [Parameter(Mandatory)][string]$BackupDir,
        [Parameter(Mandatory)][string]$Stamp
    )

    if (-not (Test-Path -LiteralPath $StoragePath -PathType Container)) { return $null }
    New-Item -ItemType Directory -Path $BackupDir -Force | Out-Null
    $zip = Join-Path $BackupDir ('{0}-{1}.zip' -f (Split-Path -Leaf $StoragePath), $Stamp)
    Compress-Archive -LiteralPath $StoragePath -DestinationPath $zip -CompressionLevel Optimal
    return $zip
}

function Resolve-BinarySourcePath {
    <#
    .SYNOPSIS
        Returns the folder that holds DayZServer_x64.exe for the chosen binary source, or throws.
    #>
    [OutputType([string])]
    param(
        [Parameter(Mandatory)][string]$BinarySource,
        [Parameter(Mandatory)][string]$MirrorRoot,
        [string]$SteamServerPath,
        [string[]]$SteamLibraryPaths = @()
    )

    if ($BinarySource -eq 'Mirror') {
        if (-not (Test-Path -LiteralPath (Join-Path $MirrorRoot 'DayZServer_x64.exe') -PathType Leaf)) { throw "DayZServer_x64.exe is not in the mirror yet ($MirrorRoot); let the WinSCP sync finish or use -BinarySource Steam." }
        return $MirrorRoot
    }
    $candidates = [System.Collections.Generic.List[string]]::new()
    if ($SteamServerPath) { $candidates.Add($SteamServerPath) }
    foreach ($library in $SteamLibraryPaths) { $candidates.Add((Join-Path $library 'steamapps\common\DayZServer')) }
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath (Join-Path $candidate 'DayZServer_x64.exe') -PathType Leaf) { return $candidate }
    }
    throw 'No Steam DayZServer install with DayZServer_x64.exe found; pass -SteamServerPath.'
}

# ---------------------------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------------------------
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$runDir = Join-Path $RepoRoot ('logs\replica-{0}' -f $stamp)
New-Item -ItemType Directory -Path $runDir -Force -WhatIf:$false | Out-Null
$manifest = Join-Path $runDir 'manifest.txt'
$counts = @{ OK = 0; WARN = 0; PARTIAL = 0; FAIL = 0; SKIP = 0 }
# -WhatIf: every robocopy step runs with /L and writes its log; nothing is created, zipped, removed, or copied.
$dryRun = [bool]$WhatIfPreference

function Add-Verdict { param([string]$Verdict) if ($counts.ContainsKey($Verdict)) { $counts[$Verdict]++ } }

try {
    Write-Manifest -LogPath $manifest -Line ("START {0} PS={1} whatif={2}" -f (Get-Date -Format o), $PSVersionTable.PSVersion, $dryRun)

    # Preconditions ----------------------------------------------------------------------------
    if (-not (Test-Path -LiteralPath $MirrorRoot -PathType Container)) { throw "Mirror root not found: $MirrorRoot" }
    $replicaDrive = [System.IO.Path]::GetPathRoot($ReplicaRoot)
    if (-not (Test-Path -LiteralPath $replicaDrive)) { throw "Replica drive not found: $replicaDrive" }
    if ((Test-PathInside -Candidate $ReplicaRoot -Root $MirrorRoot) -or (Test-PathInside -Candidate $MirrorRoot -Root $ReplicaRoot)) { throw "Replica root '$ReplicaRoot' and mirror root '$MirrorRoot' overlap; the mirror is never written." }
    if (Test-PathInside -Candidate $ReplicaRoot -Root $RepoRoot) { throw "Replica root '$ReplicaRoot' is inside the repo '$RepoRoot'; keep server data out of the repository." }
    $running = @(Get-RunningServerProcess -Root $ReplicaRoot)   # @() because a one-element return unwraps
    if ($running.Count -gt 0) { throw ("A DayZServer_x64 process is running from the replica (pid {0}); stop it before syncing storage." -f (($running | ForEach-Object { $_.Id }) -join ',')) }
    $mission = Get-MissionTemplate -ConfigPath (Join-Path $MirrorRoot 'serverDZ_By_HostHavoc.cfg')
    $mirrorMission = Join-Path $MirrorRoot ('mpmissions\{0}' -f $mission)
    if (-not (Test-Path -LiteralPath $mirrorMission -PathType Container)) { throw "Mission folder not in the mirror: $mirrorMission" }
    $mirrorStorage = Join-Path $mirrorMission 'storage_1'
    if (-not (Test-Path -LiteralPath $mirrorStorage -PathType Container)) { throw "Live storage_1 not in the mirror: $mirrorStorage (sync still running?)" }
    Write-Manifest -LogPath $manifest -Line ("MISSION {0} mirror={1}" -f $mission, $mirrorMission)

    $steamLibraries = @()
    if ($BinarySource -eq 'Steam') {
        Import-Module (Join-Path $PSScriptRoot 'modules\NimbySteam.psm1') -Force
        $steamRoot = Get-SteamRootPath
        if ($null -ne $steamRoot) { $steamLibraries = @(Get-SteamLibraryPath -SteamRoot $steamRoot) }
    }
    $binarySourcePath = Resolve-BinarySourcePath -BinarySource $BinarySource -MirrorRoot $MirrorRoot -SteamServerPath $SteamServerPath -SteamLibraryPaths $steamLibraries
    if (Test-PathInside -Candidate $ReplicaRoot -Root $binarySourcePath) { throw "Replica root '$ReplicaRoot' is inside the binary source '$binarySourcePath'." }
    Write-Manifest -LogPath $manifest -Line ("BINARIES source={0} path={1}" -f $BinarySource, $binarySourcePath)

    # Step 1: binaries ----------------------------------------------------------------------------
    $replicaExe = Join-Path $ReplicaRoot 'DayZServer_x64.exe'
    if ($RefreshBinaries -or -not (Test-Path -LiteralPath $replicaExe -PathType Leaf)) {
        if ($dryRun -or $PSCmdlet.ShouldProcess($ReplicaRoot, "Copy server binaries from $binarySourcePath")) {
            Add-Verdict (Invoke-RobocopyStep -Step 'binaries' -Source $binarySourcePath -Destination $ReplicaRoot -LogDir $runDir -ManifestPath $manifest -ExcludeDirs $script:BinaryExcludeDirs -ExcludeFiles $script:BinaryExcludeFiles -ListOnly:$dryRun)
        }
    }
    else {
        Write-Manifest -LogPath $manifest -Line ("SKIP binaries present={0} (use -RefreshBinaries to copy again)" -f $replicaExe)
        Add-Verdict 'SKIP'
    }

    # Step 2: mission files, never storage ----------------------------------------------------
    $replicaMission = Join-Path $ReplicaRoot ('mpmissions\{0}' -f $mission)
    if ($dryRun -or $PSCmdlet.ShouldProcess($replicaMission, 'Copy mission files (storage excluded)')) {
        Add-Verdict (Invoke-RobocopyStep -Step 'mission' -Source $mirrorMission -Destination $replicaMission -LogDir $runDir -ManifestPath $manifest -ExcludeDirs @('storage_1', 'storage_998', 'storage_999') -ListOnly:$dryRun)
    }

    # Step 3: live world -> storage_998 (backup first, then mirror) ---------------------------
    $liveStorage = Join-Path $replicaMission ('storage_{0}' -f $script:LiveInstanceId)
    $backupDir = Join-Path $ReplicaRoot '_backups'
    if ($dryRun -or $PSCmdlet.ShouldProcess($liveStorage, "Replace with a copy of the live storage_1 (previous copy zipped to $backupDir)")) {
        if ($dryRun) {
            Write-Manifest -LogPath $manifest -Line ("WHATIF backup src={0} dir={1}" -f $liveStorage, $backupDir)
        }
        else {
            $zip = Backup-StorageFolder -StoragePath $liveStorage -BackupDir $backupDir -Stamp $stamp
            if ($zip) { Write-Manifest -LogPath $manifest -Line ("OK backup src={0} zip={1}" -f $liveStorage, $zip); Add-Verdict 'OK' }
        }
        Add-Verdict (Invoke-RobocopyStep -Step 'storage-live' -Source $mirrorStorage -Destination $liveStorage -LogDir $runDir -ManifestPath $manifest -Mirror -ListOnly:$dryRun)
    }

    # Step 4: fresh world -> storage_999 ------------------------------------------------------
    $freshStorage = Join-Path $replicaMission ('storage_{0}' -f $script:FreshInstanceId)
    if ($ResetFreshWorld -and (Test-Path -LiteralPath $freshStorage -PathType Container)) {
        if ($dryRun) { Write-Manifest -LogPath $manifest -Line ("WHATIF reset fresh-world src={0}" -f $freshStorage) }
        elseif ($PSCmdlet.ShouldProcess($freshStorage, "Zip to $backupDir and recreate empty")) {
            $zip = Backup-StorageFolder -StoragePath $freshStorage -BackupDir $backupDir -Stamp $stamp
            Write-Manifest -LogPath $manifest -Line ("OK backup src={0} zip={1}" -f $freshStorage, $zip)
            Remove-Item -LiteralPath $freshStorage -Recurse -Force
            Add-Verdict 'OK'
        }
    }
    if (-not (Test-Path -LiteralPath $freshStorage -PathType Container)) {
        if ($dryRun) { Write-Manifest -LogPath $manifest -Line ("WHATIF create fresh-world={0}" -f $freshStorage) }
        elseif ($PSCmdlet.ShouldProcess($freshStorage, 'Create empty fresh-world storage')) {
            New-Item -ItemType Directory -Path $freshStorage -Force | Out-Null
            Write-Manifest -LogPath $manifest -Line ("OK fresh-world created={0}" -f $freshStorage)
            Add-Verdict 'OK'
        }
    }
    else {
        Write-Manifest -LogPath $manifest -Line ("SKIP fresh-world exists={0} (use -ResetFreshWorld to empty it)" -f $freshStorage)
        Add-Verdict 'SKIP'
    }

    # Step 5: reference files into the repo ---------------------------------------------------
    $liveDir = Join-Path $RepoRoot 'server\live'
    if ($dryRun) { Write-Manifest -LogPath $manifest -Line ("WHATIF reference files -> {0}" -f $liveDir) }
    elseif ($PSCmdlet.ShouldProcess($liveDir, 'Copy mission loot and gameplay files')) {
        foreach ($relative in $script:LiveReferenceFiles) {
            Add-Verdict (Copy-ReferenceFile -MissionRoot $mirrorMission -RelativePath $relative -LiveDir $liveDir -ManifestPath $manifest)
        }
        Set-Content -LiteralPath (Join-Path $liveDir 'SOURCE.txt') -Value ("Copied {0} from {1} (mission {2}) by Sync-LiveReplica.ps1. Live loot reference; edit types.xml in the HostHavoc panel, not here." -f (Get-Date -Format o), $MirrorRoot, $mission) -Encoding utf8
    }

    $summary = "SUMMARY OK={0} WARN={1} PARTIAL={2} FAIL={3} SKIP={4} replica={5} live=storage_{6} fresh=storage_{7}" -f $counts.OK, $counts.WARN, $counts.PARTIAL, $counts.FAIL, $counts.SKIP, $ReplicaRoot, $script:LiveInstanceId, $script:FreshInstanceId
    Write-Manifest -LogPath $manifest -Line $summary
    Write-Information $summary -InformationAction Continue
    Write-Information "Manifest: $manifest" -InformationAction Continue
    if ($counts.FAIL -gt 0 -or $counts.PARTIAL -gt 0) { exit 1 }
    exit 0
}
catch {
    Write-Manifest -LogPath $manifest -Line ("FAIL {0}" -f $_.Exception.Message)
    Write-Information ("FAIL {0}" -f $_.Exception.Message) -InformationAction Continue
    Write-Information "Manifest: $manifest" -InformationAction Continue
    exit 2
}
