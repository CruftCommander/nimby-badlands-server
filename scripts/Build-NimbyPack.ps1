#Requires -Version 5.1

<#
.SYNOPSIS
    Builds, signs, verifies, and boot-tests the @NIMBY client-and-server mod.

.DESCRIPTION
    For each addon listed in mods\own\nimby-pack\pack.json (every row, or only the rows named
    by -Addons):
      1. Packs the files matching includePatterns into an uncompressed PBO with the pack's
         own packer (NimbyPbo.psm1), then signs it with DayZ Tools DSSignFile.
      2. Confirms the PBO checksum, the .bisign, the PBO prefix, and that every source file
         is inside the PBO.
    DayZ Tools Addon Builder is not used: its DayZ build ignores command-line arguments and
    opens its window (observed October 14, 2026).
    Then copies the public key into @NIMBY\keys, writes mod.cpp, and verifies every signature with
    DayZ Tools DSCheckSignatures. The checker is calibrated on each run against a copy of a PBO
    with no signature: if it does not flag that copy, its verdict on the real build is not trusted
    and the result is UNKNOWN. Then (unless -SkipBootCheck)
    starts a local DayZ server with @NIMBY, scans its logs for script errors, and stops it.

    Three outputs can exist for one build stamp (shared rules, section 8):
      build\@NIMBY-<stamp>\         client PBOs, signed with the release key. The Workshop item.
      build\@NIMBY-Server-<stamp>\  addons marked "serverOnly" in pack.json, unsigned, loaded with
                                    -servermod=. Written only when such an addon exists.
      build\@NIMBY-Dev-<stamp>\     the dev harness from pack.json "dev", signed with the separate
                                    dev key. Written only with -Dev. Never listed in "addons".
    The client output is byte-identical with or without -Dev.

    Self-test: before the boot check the script creates <profile>\<Prefix>\selftest for every
    addon it built. Each addon runs its engine-free validators at mission start when that flag
    exists, prints "[NIMBY][<logPrefix>][SelfTest] PASS|FAIL <case>" per case, and ends with
    "[NIMBY][<logPrefix>] selftest complete; cases=N failures=M". pack.json maps each addon's
    code prefix to its logPrefix. Lines are counted from the script log (the RPT carries the
    same lines, so reading both would count each twice), and an identical line counts once.
    The boot check fails on any FAIL line, and also when a flagged addon stayed silent: no
    complete line, cases=0 or no cases=N, no PASS line, or PASS plus FAIL not equal to cases.
    build.log gets one "SELFTEST <logPrefix> ran=yes|no cases=N pass=N fail=M" line per addon.

    Reads   : pack.json, the addon folders, DayZ Tools (DsUtils), a local DayZ Server install,
              the private key(s) in -KeyDirectory, HKCU:\Software\Valve\Steam,
              libraryfolders.vdf and appmanifest_*.acf (to find DayZ Tools and the server), and
              the process list (no second DayZ server is started while one runs).
    Writes  : <repo>\build\@NIMBY-<stamp>\ (and the -Server and -Dev siblings when produced),
              <repo>\logs\build-<stamp>\ (manifest, tool logs, server profile and logs), and the
              key pair(s) in -KeyDirectory on first run. The boot check writes the server's
              persistence to <server>\mpmissions\<mission>\storage_<instanceId>; pack.json, or
              -BootInstanceId, must name instance 998 or 999 so storage_1 is never written.
    Never   : copies a private key anywhere, deletes anything, or contacts a remote server.

    Exit codes: 0 = PASS, 1 = FAIL, 2 = UNKNOWN (built and verified, but the boot check was
    skipped, could not start because a DayZServer_x64 process was already running, or could not
    reach a verdict).

    Runs in Windows PowerShell 5.1 and PowerShell 7. No elevation needed.

.PARAMETER KeyDirectory
    Folder holding <authority>.biprivatekey and <authority>.bikey. Created on first run. With
    -Dev the dev key pair (pack.json dev.keyAuthority) lives in the same folder.

.PARAMETER DayZToolsPath
    DayZ Tools install folder. Found through Steam when omitted.

.PARAMETER ServerPath
    DayZ Server install folder (contains DayZServer_x64.exe). Found through Steam, then
    D:\DayZServer, when omitted.

.PARAMETER SkipBootCheck
    Build and verify only.

.PARAMETER Dev
    Also build the dev harness (pack.json "dev") to build\@NIMBY-Dev-<stamp>\ with the dev key,
    and load it beside the pack in the boot check.

.PARAMETER Qualification
    Boot with storageAutoFix = 0 so persistence corruption fails the check instead of being
    repaired. The default quick check boots with storageAutoFix = 1.

.PARAMETER Addons
    The pboName values from pack.json to pack, sign, verify, and boot, as one comma-separated
    string or an array, in any order. Every other addon row stays out of this build and its
    self-test is not expected; build.log records the selection as "ADDONS <names> source=parameter".
    A name pack.json does not list, a name given twice, or an empty list stops the script before
    any packing. Omitted, every addon row builds. Release gate decision 1 (October 26, 2026): a
    release artifact carries only the qualified addons, named here; pack.json is not edited for
    a release.

.PARAMETER BootInstanceId
    Override pack.json bootCheck.instanceId for this run's boot check. On the local replica built
    by Sync-LiveReplica.ps1, 998 is the copy of the live world and 999 is the fresh world. Only 998
    and 999 are accepted (shared rules, section 8); build.log records "BOOT INSTANCE <id> source=".
    Release gate decision 2 (October 26, 2026): the release boot is -BootInstanceId 998 with
    -Qualification, against the copy of live storage_1.

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Build-NimbyPack.ps1

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Build-NimbyPack.ps1 -Addons NimbyAutorun,NimbyEarplugs,NimbyNotes,NimbyGreet -BootInstanceId 998 -Qualification

    Release build: the four qualified addons only, booted against storage_998 with storageAutoFix = 0.

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Build-NimbyPack.ps1 -Dev -Qualification
#>
[CmdletBinding()]
param(
    [string]$RepoRoot,
    [string]$KeyDirectory = 'D:\Keys\nimby',
    [string]$DayZToolsPath,
    [string]$ServerPath,
    [string[]]$Addons,
    [ValidateSet(998, 999)][int]$BootInstanceId,
    [switch]$SkipBootCheck,
    [switch]$Dev,
    [switch]$Qualification
)

Set-StrictMode -Version Latest
# Windows PowerShell 5.1 leaves $PSScriptRoot empty in a param() default when an advanced
# script is started with -File, so the default is resolved here.
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }
$ErrorActionPreference = 'Stop'

Import-Module (Join-Path $PSScriptRoot 'modules\NimbyCommon.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'modules\NimbySteam.psm1') -Force
# Build is an approved verb from PowerShell 6 only; 5.1 would warn about Build-PboArchive.
Import-Module (Join-Path $PSScriptRoot 'modules\NimbyPbo.psm1') -Force -DisableNameChecking

# ---------------------------------------------------------------------------
# Logic kept in functions so it can be smoke-tested with the tools stubbed.
# Write-Manifest, Test-TestInstanceId, Get-ServerArgument, Read-ServerLogText, and the
# running-server guard come from NimbyCommon.psm1.
# ---------------------------------------------------------------------------

function Get-OptionalProperty {
    # Reads a property that pack.json may omit. Strict mode throws on a missing member otherwise.
    param(
        [Parameter(Mandatory)][object]$Object,
        [Parameter(Mandatory)][string]$Name,
        [AllowNull()][object]$Default = $null
    )

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) { return $Default }
    return $property.Value
}

function Test-AddonServerOnly {
    # True when the addon row carries "serverOnly": true.
    param([Parameter(Mandatory)][object]$Addon)
    return [bool](Get-OptionalProperty -Object $Addon -Name 'serverOnly' -Default $false)
}

function Get-PackDefinition {
    # Loads and validates pack.json. Throws with a precise message on any problem.
    param([Parameter(Mandatory)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) { throw "pack.json not found at '$Path'." }
    $pack = Get-Content -LiteralPath $Path -Raw -Encoding utf8 | ConvertFrom-Json

    foreach ($field in 'modFolderName', 'modName', 'author', 'keyAuthority', 'includePatterns', 'addons', 'bootCheck') {
        if ($null -eq $pack.PSObject.Properties[$field]) { throw "pack.json is missing '$field'." }
    }
    if (@($pack.addons).Count -eq 0) { throw 'pack.json lists no addons.' }
    if ($pack.keyAuthority -notmatch '^[A-Za-z0-9_]+$') { throw "keyAuthority '$($pack.keyAuthority)' must be letters, digits, or underscore." }

    # logPrefix is the <name> in the addon's "[NIMBY][<name>]" log lines; the self-test gate maps
    # each flagged code prefix to it, so it must be present and unique.
    $logPrefixPattern = '^[A-Za-z0-9_-]+$'
    $names = @{}
    $logPrefixes = @{}
    foreach ($addon in $pack.addons) {
        foreach ($field in 'folder', 'pboName', 'prefix', 'logPrefix') {
            if (-not (Get-OptionalProperty -Object $addon -Name $field)) { throw "An addon in pack.json is missing '$field'." }
        }
        if ($names.ContainsKey($addon.pboName)) { throw "Duplicate pboName '$($addon.pboName)' in pack.json." }
        $names[$addon.pboName] = $true
        if ($addon.logPrefix -notmatch $logPrefixPattern) { throw "logPrefix '$($addon.logPrefix)' must be letters, digits, underscore, or hyphen." }
        if ($logPrefixes.ContainsKey($addon.logPrefix)) { throw "Duplicate logPrefix '$($addon.logPrefix)' in pack.json." }
        $logPrefixes[$addon.logPrefix] = $true
    }

    $dev = Get-OptionalProperty -Object $pack -Name 'dev'
    if ($null -ne $dev) {
        foreach ($field in 'folder', 'pboName', 'prefix', 'logPrefix', 'keyAuthority') {
            if (-not (Get-OptionalProperty -Object $dev -Name $field)) { throw "pack.json 'dev' is missing '$field'." }
        }
        if ($dev.keyAuthority -notmatch '^[A-Za-z0-9_]+$') { throw "dev.keyAuthority '$($dev.keyAuthority)' must be letters, digits, or underscore." }
        if ($dev.keyAuthority -eq $pack.keyAuthority) { throw 'dev.keyAuthority must differ from keyAuthority: the harness is signed with a separate key.' }
        if ($names.ContainsKey($dev.pboName)) { throw "dev.pboName '$($dev.pboName)' collides with an addon; the harness is never an addon row." }
        if ($dev.logPrefix -notmatch $logPrefixPattern) { throw "dev.logPrefix '$($dev.logPrefix)' must be letters, digits, underscore, or hyphen." }
        if ($logPrefixes.ContainsKey($dev.logPrefix)) { throw "dev.logPrefix '$($dev.logPrefix)' collides with an addon." }
    }

    foreach ($field in 'mission', 'port', 'instanceId', 'timeoutSeconds', 'failurePatterns', 'readyPatterns') {
        if ($null -eq $pack.bootCheck.PSObject.Properties[$field]) { throw "pack.json bootCheck is missing '$field'." }
    }
    if (-not (Test-TestInstanceId -InstanceId ([int]$pack.bootCheck.instanceId))) {
        throw "bootCheck.instanceId $($pack.bootCheck.instanceId) is not a test instance; only 998 and 999 are allowed so storage_1 is never written."
    }
    return $pack
}

function Select-PackAddon {
    # Returns the pack.json addon rows named in Names, in pack.json order, or every row when Names
    # is omitted. Names may arrive as one comma-separated string (how -File passes an array
    # parameter) or as an array; matching ignores case and returns the row's own pboName. A name
    # pack.json does not list, a name given twice, or a list that is empty after trimming throws,
    # so a wrong -Addons stops the build before any packing (release gate decision 1, October 26,
    # 2026: the artifact subset is a parameter and pack.json is never edited for a release).
    param(
        [Parameter(Mandatory)][object]$Pack,
        [AllowNull()][AllowEmptyCollection()][string[]]$Names
    )

    $rows = @($Pack.addons)
    if ($null -eq $Names -or $Names.Count -eq 0) { return $rows }
    $wanted = @($Names | ForEach-Object { $_ -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ })
    if ($wanted.Count -eq 0) { throw '-Addons is empty; name at least one pboName from pack.json, or omit -Addons to build every addon.' }
    $known = @($rows | ForEach-Object { $_.pboName })
    $seen = @{}
    foreach ($name in $wanted) {
        if ($known -notcontains $name) { throw ("-Addons names '{0}', which pack.json does not list. Known: {1}." -f $name, ($known -join ', ')) }
        if ($seen.ContainsKey($name)) { throw "-Addons names '$name' twice." }
        $seen[$name] = $true
    }
    return @($rows | Where-Object { $seen.ContainsKey($_.pboName) })
}

function Resolve-ToolPath {
    # Returns the first existing candidate path, or $null.
    param([Parameter(Mandatory)][AllowEmptyCollection()][string[]]$Candidates)

    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) { return $candidate }
    }
    return $null
}

function Write-BootConfig {
    # Writes a throwaway serverDZ config for the boot check. No passwords, BattlEye off: the boot
    # check's storage is disposable, so a process stop is acceptable here (the graceful-stop rule
    # applies to Start-NimbyTestServer.ps1).
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Mission,
        [Parameter(Mandatory)][int]$InstanceId,
        [Parameter(Mandatory)][ValidateSet(0, 1)][int]$StorageAutoFix
    )

    if (-not (Test-TestInstanceId -InstanceId $InstanceId)) { throw "Refusing to write a boot config for instanceId $InstanceId; only 998 and 999 are test instances." }
    $lines = @(
        '// Generated by Build-NimbyPack.ps1 for a local boot check. Not a real server config.',
        'hostname = "NIMBY build check - local only";',
        'password = "";',
        'passwordAdmin = "";',
        'maxPlayers = 2;',
        'verifySignatures = 2;',
        'forceSameBuild = 1;',
        'BattlEye = 0;',
        "instanceId = $InstanceId;",
        "storageAutoFix = $StorageAutoFix;",
        'class Missions',
        '{',
        '    class DayZ',
        '    {',
        "        template = `"$Mission`";",
        '    };',
        '};'
    )
    # ascii, not utf8: 5.1 writes utf8 with a BOM, and the server reads this file.
    Set-Content -LiteralPath $Path -Value $lines -Encoding ascii
}

function Initialize-SelfTestFlag {
    # Creates <profile>\<Prefix>\selftest for each prefix so every addon runs its self-test at
    # mission start (shared rules, section 8). Returns the flag paths.
    param(
        [Parameter(Mandatory)][string]$ProfilePath,
        [Parameter(Mandatory)][string[]]$Prefix
    )

    $flags = @()
    foreach ($p in $Prefix) {
        $dir = Join-Path $ProfilePath $p
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
        $flag = Join-Path $dir 'selftest'
        Set-Content -LiteralPath $flag -Value '' -Encoding ascii
        $flags += $flag
    }
    return $flags
}

function Get-BootVerdict {
    # Classifies log text. FAIL wins over READY; neither means PENDING.
    param(
        [Parameter(Mandatory)][AllowEmptyString()][string]$LogText,
        [Parameter(Mandatory)][string[]]$FailurePatterns,
        [Parameter(Mandatory)][string[]]$ReadyPatterns
    )

    foreach ($pattern in $FailurePatterns) {
        $match = [regex]::Match($LogText, "(?im)^.*$pattern.*$")
        if ($match.Success) {
            return [PSCustomObject]@{ Verdict = 'FAIL'; Evidence = $match.Value.Trim() }
        }
    }
    foreach ($pattern in $ReadyPatterns) {
        $match = [regex]::Match($LogText, "(?im)^.*$pattern.*$")
        if ($match.Success) {
            return [PSCustomObject]@{ Verdict = 'READY'; Evidence = $match.Value.Trim() }
        }
    }
    return [PSCustomObject]@{ Verdict = 'PENDING'; Evidence = '' }
}

function Get-SelfTestVerdict {
    # Reads "[NIMBY][<component>][SelfTest] PASS|FAIL <case>" lines and each component's
    # "[NIMBY][<component>] selftest complete; cases=N failures=M" line. An identical line seen
    # twice counts once. Every component in Expected, and every component that printed anything,
    # must keep the contract: a complete line, cases=N above zero, at least one PASS line, and
    # PASS plus FAIL equal to cases. Problems names each component that broke it; Verdict is FAIL
    # when there is a FAIL line or a problem, else PASS.
    param(
        [Parameter(Mandatory)][AllowEmptyString()][string]$LogText,
        [AllowEmptyCollection()][string[]]$Expected = @()
    )

    $casePattern = '\[NIMBY\]\[(?<c>[^\]]+)\]\[SelfTest\]\s+(?<r>PASS|FAIL)\b'
    $completePattern = '\[NIMBY\]\[(?<c>[^\]]+)\]\s+selftest complete;(?<rest>.*)$'
    $seen = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::Ordinal)
    $counts = [ordered]@{}
    foreach ($name in $Expected) {
        if (-not $counts.Contains($name)) { $counts[$name] = [PSCustomObject]@{ Pass = 0; Fail = 0; Complete = $false; Cases = $null } }
    }
    $failures = @()
    foreach ($raw in ($LogText -split "\r?\n")) {
        $line = $raw.Trim()
        if (-not $line) { continue }
        $case = [regex]::Match($line, $casePattern)
        $complete = [regex]::Match($line, $completePattern)
        if (-not $case.Success -and -not $complete.Success) { continue }
        if (-not $seen.Add($line)) { continue }

        $match = $complete
        if ($case.Success) { $match = $case }
        $component = $match.Groups['c'].Value
        if (-not $counts.Contains($component)) { $counts[$component] = [PSCustomObject]@{ Pass = 0; Fail = 0; Complete = $false; Cases = $null } }
        $entry = $counts[$component]
        if ($case.Success) {
            if ($case.Groups['r'].Value -eq 'PASS') { $entry.Pass++ } else { $entry.Fail++; $failures += $line }
        }
        else {
            $entry.Complete = $true
            $cases = [regex]::Match($complete.Groups['rest'].Value, '\bcases=(?<n>\d+)')
            if ($cases.Success) { $entry.Cases = [int]$cases.Groups['n'].Value }
        }
    }

    $pass = 0; $fail = 0
    $problems = @()
    foreach ($key in $counts.Keys) {
        $entry = $counts[$key]
        $pass += $entry.Pass; $fail += $entry.Fail
        $ran = ($entry.Complete -or ($entry.Pass + $entry.Fail) -gt 0)
        if (-not $ran) { $problems += "$key ran=no" }
        elseif (-not $entry.Complete) { $problems += "$key no 'selftest complete' line" }
        elseif ($null -eq $entry.Cases) { $problems += "$key complete line has no cases=N" }
        elseif ($entry.Cases -eq 0) { $problems += "$key cases=0" }
        elseif ($entry.Pass -eq 0) { $problems += "$key no PASS line" }
        elseif (($entry.Pass + $entry.Fail) -ne $entry.Cases) { $problems += ("{0} pass+fail={1} but cases={2}" -f $key, ($entry.Pass + $entry.Fail), $entry.Cases) }
    }

    $verdict = 'PASS'
    if ($fail -gt 0 -or $problems.Count -gt 0) { $verdict = 'FAIL' }
    return [PSCustomObject]@{ Verdict = $verdict; Pass = $pass; Fail = $fail; Components = $counts; Failures = $failures; Problems = $problems }
}

function Get-SignatureVerdict {
    # Judges DSCheckSignatures output by its observed format (DayZ Tools, October 14, 2026):
    #   good:    "Signature <path>\<name>.pbo.<authority>.bisign is OK"
    #   missing: "No signature found for <path>\<name>.pbo"
    # PASS needs an OK line for every expected PBO, no other non-empty line, and exit 0 on the
    # real build; and the unsigned control must produce at least one non-OK line or a nonzero
    # exit, proving the checker can fail. Anything unrecognized fails closed.
    param(
        [Parameter(Mandatory)][int]$RealExit,
        [Parameter(Mandatory)][AllowEmptyString()][string]$RealOutput,
        [Parameter(Mandatory)][int]$NegativeExit,
        [Parameter(Mandatory)][AllowEmptyString()][string]$NegativeOutput,
        [Parameter(Mandatory)][string[]]$ExpectedPbo
    )

    $okPattern = '^Signature\s+(?<path>.+?)\s+is OK\s*$'

    $negativeBad = @(($NegativeOutput -split "\r?\n") | Where-Object { $_.Trim() -and $_ -notmatch $okPattern })
    if ($NegativeExit -eq 0 -and $negativeBad.Count -eq 0) {
        return [PSCustomObject]@{ Verdict = 'UNKNOWN'; Evidence = "checker did not flag the unsigned control (exit $NegativeExit)" }
    }

    $lines = @(($RealOutput -split "\r?\n") | Where-Object { $_.Trim() })
    $bad = @($lines | Where-Object { $_ -notmatch $okPattern })
    if ($RealExit -ne 0 -or $bad.Count -gt 0) {
        $evidence = "exit $RealExit"
        if ($bad.Count -gt 0) { $evidence = "exit $RealExit; $($bad[0].Trim())" }
        return [PSCustomObject]@{ Verdict = 'FAIL'; Evidence = $evidence }
    }

    $signedPbo = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    foreach ($line in $lines) {
        $leaf = ($line -replace $okPattern, '${path}') -split '[\\/]' | Select-Object -Last 1
        [void]$signedPbo.Add(($leaf -replace '\.pbo\..*$', '.pbo'))
    }
    $unsigned = @($ExpectedPbo | Where-Object { -not $signedPbo.Contains($_) })
    if ($unsigned.Count -gt 0) {
        return [PSCustomObject]@{ Verdict = 'FAIL'; Evidence = "no OK line for: $($unsigned -join ', ')" }
    }
    $control = "exit $NegativeExit"
    if ($negativeBad.Count -gt 0) { $control = $negativeBad[0].Trim() }
    return [PSCustomObject]@{ Verdict = 'PASS'; Evidence = ("{0} of {0} signatures OK; unsigned control flagged: {1}" -f $ExpectedPbo.Count, $control) }
}

function Invoke-Native {
    # Runs a native tool, sends its output to a log file, and returns the exit code.
    param(
        [Parameter(Mandatory)][string]$FilePath,
        [Parameter(Mandatory)][string[]]$ArgumentList,
        [Parameter(Mandatory)][string]$LogPath,
        [string]$WorkingDirectory = (Get-Location).ProviderPath
    )

    $global:LASTEXITCODE = $null
    Push-Location -LiteralPath $WorkingDirectory
    try {
        # Local to this function: in 5.1, 7.0, and 7.1 each stderr line of a redirected native
        # command is an ErrorRecord, and the script's 'Stop' would make the first one terminating.
        # Flatten ErrorRecords to their text so the log reads the same on both shells.
        $ErrorActionPreference = 'Continue'
        $lines = @(& $FilePath @ArgumentList 2>&1 | ForEach-Object {
                if ($_ -is [System.Management.Automation.ErrorRecord]) { $_.Exception.Message } else { [string]$_ }
            })
        Set-Content -LiteralPath $LogPath -Value $lines -Encoding utf8
        return $LASTEXITCODE
    }
    finally {
        Pop-Location
    }
}

function Initialize-SigningKey {
    # Ensures <authority>.biprivatekey and .bikey exist in KeyDirectory, creating them with
    # DSCreateKey on first use. Returns the two paths. Never copies the private key.
    param(
        [Parameter(Mandatory)][string]$KeyDirectory,
        [Parameter(Mandatory)][string]$Authority,
        [AllowNull()][string]$CreateKeyPath,
        [Parameter(Mandatory)][string]$LogDir,
        [Parameter(Mandatory)][string]$Manifest
    )

    $privateKey = Join-Path $KeyDirectory ("{0}.biprivatekey" -f $Authority)
    $publicKey = Join-Path $KeyDirectory ("{0}.bikey" -f $Authority)
    if (-not (Test-Path -LiteralPath $privateKey)) {
        if (-not $CreateKeyPath) { throw "No private key at '$privateKey' and DSCreateKey.exe not found to create one." }
        New-Item -ItemType Directory -Path $KeyDirectory -Force | Out-Null
        $code = Invoke-Native -FilePath $CreateKeyPath -ArgumentList @($Authority) -LogPath (Join-Path $LogDir ("DSCreateKey-{0}.log" -f $Authority)) -WorkingDirectory $KeyDirectory
        if (-not (Test-Path -LiteralPath $privateKey) -or -not (Test-Path -LiteralPath $publicKey)) {
            throw "DSCreateKey (exit $code) did not produce both key files for '$Authority' in '$KeyDirectory'."
        }
        Write-Manifest -LogPath $Manifest -Line "KEY CREATED authority=$Authority dir=$KeyDirectory"
    }
    else {
        if (-not (Test-Path -LiteralPath $publicKey)) { throw "Private key exists but public key '$publicKey' is missing." }
        Write-Manifest -LogPath $Manifest -Line "KEY EXISTING authority=$Authority dir=$KeyDirectory"
    }
    return [PSCustomObject]@{ PrivateKey = $privateKey; PublicKey = $publicKey }
}

function Build-NimbyAddon {
    # Packs one addon into AddonsOut, signs it when PrivateKey is given, and verifies checksum,
    # prefix, and completeness. Writes ADDON OK or ADDON FAIL to the manifest. Returns $true on OK.
    param(
        [Parameter(Mandatory)][string]$SourceFolder,
        [Parameter(Mandatory)][string]$PboName,
        [Parameter(Mandatory)][string]$Prefix,
        [Parameter(Mandatory)][string[]]$Patterns,
        [Parameter(Mandatory)][string]$AddonsOut,
        [AllowNull()][string]$SignFile,
        [AllowNull()][string]$PrivateKey,
        [AllowNull()][string]$KeyAuthority,
        [Parameter(Mandatory)][string]$Label,
        [Parameter(Mandatory)][string]$LogDir,
        [Parameter(Mandatory)][string]$Manifest
    )

    if (-not (Test-Path -LiteralPath $SourceFolder -PathType Container)) {
        Write-Manifest -LogPath $Manifest -Line "ADDON FAIL $PboName [$Label] source folder missing: $SourceFolder"
        return $false
    }

    $pbo = Join-Path $AddonsOut ("{0}.pbo" -f $PboName)
    try {
        $written = @(Build-PboArchive -SourceFolder $SourceFolder -Destination $pbo -Prefix $Prefix -Patterns $Patterns)
    }
    catch {
        Write-Manifest -LogPath $Manifest -Line "ADDON FAIL $PboName [$Label] pack error: $($_.Exception.Message)"
        return $false
    }

    $code = 'unsigned'
    if ($PrivateKey) {
        $code = Invoke-Native -FilePath $SignFile -ArgumentList @($PrivateKey, $pbo) -LogPath (Join-Path $LogDir ("DSSignFile-{0}.log" -f $PboName)) -WorkingDirectory $AddonsOut
        $signature = Join-Path $AddonsOut ("{0}.pbo.{1}.bisign" -f $PboName, $KeyAuthority)
        if (-not (Test-Path -LiteralPath $signature)) {
            Write-Manifest -LogPath $Manifest -Line "ADDON FAIL $PboName [$Label] DSSignFile exit=$code no signature at $signature"
            return $false
        }
    }

    $content = Get-PboContent -Path $pbo
    $expected = @(Get-PboSourceFile -Folder $SourceFolder -Patterns $Patterns)
    $missing = @(Compare-PboToSource -SourceRelativePaths $expected -PboEntries $content.Entries)
    if (-not (Test-PboChecksum -Path $pbo)) {
        Write-Manifest -LogPath $Manifest -Line "ADDON FAIL $PboName [$Label] checksum does not match"
        return $false
    }
    if ($content.Prefix -ne $Prefix) {
        Write-Manifest -LogPath $Manifest -Line ("ADDON FAIL {0} [{1}] prefix '{2}' expected '{3}'" -f $PboName, $Label, $content.Prefix, $Prefix)
        return $false
    }
    if ($missing.Count -gt 0) {
        Write-Manifest -LogPath $Manifest -Line ("ADDON FAIL {0} [{1}] missing from PBO: {2}" -f $PboName, $Label, ($missing -join ', '))
        return $false
    }
    $hash = (Get-FileHash -LiteralPath $pbo -Algorithm SHA256).Hash
    Write-Manifest -LogPath $Manifest -Line ("ADDON OK {0} [{1}] entries={2} sources={3} signexit={4} sha256={5}" -f $PboName, $Label, $content.Entries.Count, $written.Count, $code, $hash)
    foreach ($entry in $written) {
        Write-Manifest -LogPath $Manifest -Line ("  ENTRY {0}\{1}" -f $PboName, $entry)
    }
    return $true
}

function Write-ModCpp {
    param(
        [Parameter(Mandatory)][string]$ModRoot,
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Author,
        [Parameter(Mandatory)][string]$Overview
    )

    $modCpp = @(
        "name = `"$Name`";",
        "author = `"$Author`";",
        "tooltip = `"$Name`";",
        "overview = `"$Overview`";"
    )
    # ascii, not utf8: 5.1 writes utf8 with a BOM, and the game reads mod.cpp.
    Set-Content -LiteralPath (Join-Path $ModRoot 'mod.cpp') -Value $modCpp -Encoding ascii
}

function Invoke-SignatureCheck {
    # Runs DSCheckSignatures on one output and the unsigned control, and returns the verdict.
    param(
        [Parameter(Mandatory)][string]$CheckSignatures,
        [Parameter(Mandatory)][string]$AddonsOut,
        [Parameter(Mandatory)][string]$KeysOut,
        [Parameter(Mandatory)][string]$Label,
        [Parameter(Mandatory)][string]$LogDir
    )

    $realLog = Join-Path $LogDir ("DSCheckSignatures-{0}.log" -f $Label)
    $realExit = Invoke-Native -FilePath $CheckSignatures -ArgumentList @($AddonsOut, $KeysOut) -LogPath $realLog

    # Folder name avoids failure-sounding words so it cannot leak into a verdict.
    $negativeDir = Join-Path $LogDir ("sigcheck-control-{0}" -f $Label)
    New-Item -ItemType Directory -Path $negativeDir -Force | Out-Null
    $firstPbo = Get-ChildItem -LiteralPath $AddonsOut -Filter '*.pbo' | Select-Object -First 1
    Copy-Item -LiteralPath $firstPbo.FullName -Destination $negativeDir
    $negativeLog = Join-Path $LogDir ("DSCheckSignatures-{0}-control.log" -f $Label)
    $negativeExit = Invoke-Native -FilePath $CheckSignatures -ArgumentList @($negativeDir, $KeysOut) -LogPath $negativeLog

    # [string] turns an empty log (Get-Content returns $null) into an empty string.
    $realOutput = [string](Get-Content -LiteralPath $realLog -Raw -Encoding utf8)
    $negativeOutput = [string](Get-Content -LiteralPath $negativeLog -Raw -Encoding utf8)
    $expectedPbo = @(Get-ChildItem -LiteralPath $AddonsOut -Filter '*.pbo' | ForEach-Object { $_.Name })
    return Get-SignatureVerdict -RealExit ([int]$realExit) -RealOutput $realOutput -NegativeExit ([int]$negativeExit) -NegativeOutput $negativeOutput -ExpectedPbo $expectedPbo
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$logDir = Join-Path $RepoRoot ("logs\build-{0}" -f $stamp)
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
$manifest = Join-Path $logDir 'build.log'
$exitCode = 1

try {
    Write-Manifest -LogPath $manifest -Line ("START {0} RepoRoot={1} PS={2} Dev={3} Qualification={4} Addons={5} BootInstanceId={6}" -f (Get-Date -Format o), $RepoRoot, $PSVersionTable.PSVersion, [bool]$Dev, [bool]$Qualification, $(if ($PSBoundParameters.ContainsKey('Addons')) { @($Addons) -join ',' } else { '(all)' }), $(if ($PSBoundParameters.ContainsKey('BootInstanceId')) { $BootInstanceId } else { '(pack.json)' }))

    $packRoot = Join-Path $RepoRoot 'mods\own\nimby-pack'
    $pack = Get-PackDefinition -Path (Join-Path $packRoot 'pack.json')

    # Release gate decisions 1 and 2 (October 26, 2026): the addon subset and the boot instance are
    # parameters, never a pack.json edit. Both are settled and logged before any packing.
    $selectedAddons = @(Select-PackAddon -Pack $pack -Names $Addons)
    $addonSource = 'pack.json'
    if ($PSBoundParameters.ContainsKey('Addons')) { $addonSource = 'parameter' }
    Write-Manifest -LogPath $manifest -Line ("ADDONS {0} source={1} of={2}" -f (@($selectedAddons | ForEach-Object { $_.pboName }) -join ','), $addonSource, @($pack.addons).Count)
    $bootInstanceId = [int]$pack.bootCheck.instanceId
    $instanceSource = 'pack.json'
    if ($PSBoundParameters.ContainsKey('BootInstanceId')) { $bootInstanceId = $BootInstanceId; $instanceSource = 'parameter' }
    if (-not (Test-TestInstanceId -InstanceId $bootInstanceId)) { throw "Refusing boot instance $bootInstanceId; only 998 and 999 are test instances, so storage_1 is never written." }
    Write-Manifest -LogPath $manifest -Line ("BOOT INSTANCE {0} source={1}" -f $bootInstanceId, $instanceSource)

    $clientAddons = @($selectedAddons | Where-Object { -not (Test-AddonServerOnly -Addon $_) })
    $serverAddons = @($selectedAddons | Where-Object { Test-AddonServerOnly -Addon $_ })
    if ($clientAddons.Count -eq 0) { throw 'No client addon in this build (every pack.json row, or every -Addons name, is serverOnly); at least one is needed for the Workshop output.' }
    Write-Manifest -LogPath $manifest -Line ("PACK {0} client={1} server={2}" -f $pack.modFolderName, (@($clientAddons | ForEach-Object { $_.pboName }) -join ','), (@($serverAddons | ForEach-Object { $_.pboName }) -join ','))

    $devDefinition = $null
    if ($Dev) {
        $devDefinition = Get-OptionalProperty -Object $pack -Name 'dev'
        if ($null -eq $devDefinition) { throw '-Dev given but pack.json has no "dev" section.' }
    }

    # Tools ---------------------------------------------------------------
    $steamCandidates = @()
    if (${env:ProgramFiles(x86)}) { $steamCandidates += (Join-Path ${env:ProgramFiles(x86)} 'Steam') }
    $steamRoot = Get-SteamRootPath -Candidates $steamCandidates
    $libraries = @()
    if ($null -ne $steamRoot) { $libraries = Get-SteamLibraryPath -SteamRoot $steamRoot }

    # DayZ Tools: the app manifest (app 830640) names the library that holds the install; the folder
    # name is only a fallback, and both require DSSignFile.exe so a leftover folder never matches.
    $toolsSource = 'parameter'
    if (-not $DayZToolsPath -and $libraries.Count -gt 0) {
        $DayZToolsPath = Find-SteamInstalledAppPath -LibraryPaths $libraries -AppId '830640' -RequiredFile 'Bin\DsUtils\DSSignFile.exe'
        $toolsSource = 'appmanifest'
        if (-not $DayZToolsPath) {
            $DayZToolsPath = Find-SteamAppPath -LibraryPaths $libraries -FolderName 'DayZ Tools' -RequiredFile 'Bin\DsUtils\DSSignFile.exe'
            $toolsSource = 'folder'
        }
    }
    if (-not $DayZToolsPath) { throw 'DayZ Tools with Bin\DsUtils\DSSignFile.exe not found in any Steam library. Install it from Steam (app 830640) or pass -DayZToolsPath.' }
    Write-Manifest -LogPath $manifest -Line "TOOL DayZTools $DayZToolsPath source=$toolsSource"

    $createKey = Resolve-ToolPath -Candidates @(Join-Path $DayZToolsPath 'Bin\DsUtils\DSCreateKey.exe')
    $signFile = Resolve-ToolPath -Candidates @(Join-Path $DayZToolsPath 'Bin\DsUtils\DSSignFile.exe')
    $checkSignatures = Resolve-ToolPath -Candidates @(Join-Path $DayZToolsPath 'Bin\DsUtils\DSCheckSignatures.exe')
    if ($null -eq $signFile) { throw "DSSignFile.exe not found under '$DayZToolsPath\Bin\DsUtils'." }
    Write-Manifest -LogPath $manifest -Line "TOOL DSSignFile $signFile"

    # Keys ------------------------------------------------------------------
    $releaseKey = Initialize-SigningKey -KeyDirectory $KeyDirectory -Authority $pack.keyAuthority -CreateKeyPath $createKey -LogDir $logDir -Manifest $manifest
    $devKey = $null
    if ($Dev) {
        $devKey = Initialize-SigningKey -KeyDirectory $KeyDirectory -Authority $devDefinition.keyAuthority -CreateKeyPath $createKey -LogDir $logDir -Manifest $manifest
    }

    # Build: client output --------------------------------------------------
    $modRoot = Join-Path $RepoRoot ("build\{0}-{1}" -f $pack.modFolderName, $stamp)
    $addonsOut = Join-Path $modRoot 'addons'
    $keysOut = Join-Path $modRoot 'keys'
    New-Item -ItemType Directory -Path $addonsOut, $keysOut -Force | Out-Null

    $failures = 0
    # Code prefix (flag folder) to log prefix (the [NIMBY][<name>] in its lines), per built PBO.
    $prefixes = @()
    $logPrefixOf = @{}
    foreach ($addon in $clientAddons) {
        $ok = Build-NimbyAddon -SourceFolder (Join-Path $packRoot $addon.folder) -PboName $addon.pboName -Prefix $addon.prefix -Patterns $pack.includePatterns -AddonsOut $addonsOut -SignFile $signFile -PrivateKey $releaseKey.PrivateKey -KeyAuthority $pack.keyAuthority -Label 'client' -LogDir $logDir -Manifest $manifest
        if (-not $ok) { $failures++ } else { $prefixes += $addon.prefix; $logPrefixOf[$addon.prefix] = $addon.logPrefix }
    }
    Copy-Item -LiteralPath $releaseKey.PublicKey -Destination $keysOut
    Write-ModCpp -ModRoot $modRoot -Name $pack.modName -Author $pack.author -Overview "$($pack.modName) server pack"
    Write-Manifest -LogPath $manifest -Line "MOD $modRoot"

    # Build: server-only output (shared rules 8.1) ---------------------------
    $serverRoot = $null
    if ($serverAddons.Count -gt 0) {
        $serverRoot = Join-Path $RepoRoot ("build\{0}-Server-{1}" -f $pack.modFolderName, $stamp)
        $serverAddonsOut = Join-Path $serverRoot 'addons'
        New-Item -ItemType Directory -Path $serverAddonsOut -Force | Out-Null
        foreach ($addon in $serverAddons) {
            # Unsigned: verifySignatures checks client PBOs only, and this folder never reaches clients.
            $ok = Build-NimbyAddon -SourceFolder (Join-Path $packRoot $addon.folder) -PboName $addon.pboName -Prefix $addon.prefix -Patterns $pack.includePatterns -AddonsOut $serverAddonsOut -SignFile $null -PrivateKey $null -KeyAuthority $null -Label 'server' -LogDir $logDir -Manifest $manifest
            if (-not $ok) { $failures++ } else { $prefixes += $addon.prefix; $logPrefixOf[$addon.prefix] = $addon.logPrefix }
        }
        Write-ModCpp -ModRoot $serverRoot -Name "$($pack.modName) Server" -Author $pack.author -Overview "$($pack.modName) server-only addons; load with -servermod="
        Write-Manifest -LogPath $manifest -Line "SERVERMOD $serverRoot"
    }
    else {
        Write-Manifest -LogPath $manifest -Line 'SERVERMOD none (no serverOnly addon in this build)'
    }

    # Build: dev harness ----------------------------------------------------
    $devRoot = $null
    if ($Dev) {
        $devRoot = Join-Path $RepoRoot ("build\{0}-Dev-{1}" -f $pack.modFolderName, $stamp)
        $devAddonsOut = Join-Path $devRoot 'addons'
        $devKeysOut = Join-Path $devRoot 'keys'
        New-Item -ItemType Directory -Path $devAddonsOut, $devKeysOut -Force | Out-Null
        $devSource = [System.IO.Path]::GetFullPath((Join-Path $packRoot $devDefinition.folder))
        $ok = Build-NimbyAddon -SourceFolder $devSource -PboName $devDefinition.pboName -Prefix $devDefinition.prefix -Patterns $pack.includePatterns -AddonsOut $devAddonsOut -SignFile $signFile -PrivateKey $devKey.PrivateKey -KeyAuthority $devDefinition.keyAuthority -Label 'dev' -LogDir $logDir -Manifest $manifest
        if (-not $ok) { $failures++ } else { $prefixes += $devDefinition.prefix; $logPrefixOf[$devDefinition.prefix] = $devDefinition.logPrefix }
        Copy-Item -LiteralPath $devKey.PublicKey -Destination $devKeysOut
        Write-ModCpp -ModRoot $devRoot -Name "$($pack.modName) Dev" -Author $pack.author -Overview "$($pack.modName) dev harness; never released"
        Write-Manifest -LogPath $manifest -Line "DEV $devRoot"
    }
    else {
        Write-Manifest -LogPath $manifest -Line 'DEV none (build without -Dev)'
    }

    if ($failures -gt 0) {
        throw "$failures addon(s) failed to build or verify."
    }

    # Signature check -------------------------------------------------------
    $signatureUnknown = $false
    if ($null -eq $checkSignatures) {
        Write-Manifest -LogPath $manifest -Line 'SIGN UNKNOWN DSCheckSignatures.exe not found'
        $signatureUnknown = $true
    }
    else {
        $checks = @(@{ Addons = $addonsOut; Keys = $keysOut; Label = 'client' })
        if ($Dev) { $checks += @{ Addons = $devAddonsOut; Keys = $devKeysOut; Label = 'dev' } }
        foreach ($check in $checks) {
            $signatureResult = Invoke-SignatureCheck -CheckSignatures $checkSignatures -AddonsOut $check.Addons -KeysOut $check.Keys -Label $check.Label -LogDir $logDir
            Write-Manifest -LogPath $manifest -Line ("SIGN {0} [{1}] {2}" -f $signatureResult.Verdict, $check.Label, $signatureResult.Evidence)
            if ($signatureResult.Verdict -eq 'FAIL') { throw "Signature check failed [$($check.Label)]: $($signatureResult.Evidence)" }
            if ($signatureResult.Verdict -eq 'UNKNOWN') { $signatureUnknown = $true }
        }
    }

    # Boot check ------------------------------------------------------------
    if ($SkipBootCheck) {
        Write-Manifest -LogPath $manifest -Line 'BOOT SKIPPED by -SkipBootCheck'
        $exitCode = 2
    }
    else {
        if (-not $ServerPath) { $ServerPath = Find-DayZServerPath -LibraryPaths $libraries }

        $boot = $pack.bootCheck
        $runningServer = Get-RunningServerMessage
        if ($runningServer) {
            Write-Manifest -LogPath $manifest -Line "BOOT UNKNOWN $runningServer"
            $exitCode = 2
        }
        elseif (-not $ServerPath) {
            Write-Manifest -LogPath $manifest -Line 'BOOT UNKNOWN no DayZ Server install found; pass -ServerPath'
            $exitCode = 2
        }
        elseif (-not (Test-Path -LiteralPath (Join-Path $ServerPath ("mpmissions\{0}" -f $boot.mission)))) {
            Write-Manifest -LogPath $manifest -Line "BOOT UNKNOWN mission '$($boot.mission)' not found in $ServerPath\mpmissions"
            $exitCode = 2
        }
        else {
            $profileDir = Join-Path $logDir 'server-profile'
            New-Item -ItemType Directory -Path $profileDir -Force | Out-Null
            $storageAutoFix = 1
            $bootMode = 'quick'
            if ($Qualification) { $storageAutoFix = 0; $bootMode = 'qualification' }
            $bootConfig = Join-Path $logDir 'serverDZ.bootcheck.cfg'
            Write-BootConfig -Path $bootConfig -Mission $boot.mission -InstanceId $bootInstanceId -StorageAutoFix $storageAutoFix
            Write-Manifest -LogPath $manifest -Line ("BOOT MODE {0} storageAutoFix={1} storage={2}" -f $bootMode, $storageAutoFix, (Join-Path $ServerPath ("mpmissions\{0}\storage_{1}" -f $boot.mission, $bootInstanceId)))

            # Every flagged addon is expected to report under its logPrefix (shared rules 8.2).
            $flags = @(Initialize-SelfTestFlag -ProfilePath $profileDir -Prefix $prefixes)
            $expectedLogPrefixes = @($prefixes | ForEach-Object { $logPrefixOf[$_] })
            Write-Manifest -LogPath $manifest -Line ("SELFTEST FLAGS {0}" -f ($prefixes -join ','))

            $modPaths = @($modRoot)
            if ($devRoot) { $modPaths += $devRoot }
            $serverExe = Join-Path $ServerPath 'DayZServer_x64.exe'
            $serverArgs = Get-ServerArgument -ConfigPath $bootConfig -ProfilePath $profileDir -ModPath $modPaths -ServerModPath $serverRoot -Port ([int]$boot.port)
            Write-Manifest -LogPath $manifest -Line ("BOOT START {0} {1}" -f $serverExe, ($serverArgs -join ' '))

            # Start-Process joins arguments with spaces; quote each so paths with spaces survive.
            $quoted = $serverArgs | ForEach-Object { '"{0}"' -f $_ }
            $process = Start-Process -FilePath $serverExe -ArgumentList $quoted -WorkingDirectory $ServerPath -PassThru -WindowStyle Minimized
            try {
                $deadline = (Get-Date).AddSeconds([int]$boot.timeoutSeconds)
                $verdict = [PSCustomObject]@{ Verdict = 'PENDING'; Evidence = '' }
                while ((Get-Date) -lt $deadline) {
                    Start-Sleep -Seconds 5
                    $verdict = Get-BootVerdict -LogText (Read-ServerLogText -ProfilePath $profileDir) -FailurePatterns $boot.failurePatterns -ReadyPatterns $boot.readyPatterns
                    if ($verdict.Verdict -ne 'PENDING') { break }
                    if ($process.HasExited) {
                        $verdict = [PSCustomObject]@{ Verdict = 'FAIL'; Evidence = "server exited early with code $($process.ExitCode)" }
                        break
                    }
                }

                if ($verdict.Verdict -eq 'READY') {
                    # Self-tests run at mission start, which can land after the ready line. Keep
                    # scanning for the grace period; a FAIL pattern during it still fails the boot.
                    $grace = [int](Get-OptionalProperty -Object $boot -Name 'selfTestGraceSeconds' -Default 20)
                    $graceDeadline = (Get-Date).AddSeconds($grace)
                    while ((Get-Date) -lt $graceDeadline) {
                        Start-Sleep -Seconds 5
                        $again = Get-BootVerdict -LogText (Read-ServerLogText -ProfilePath $profileDir) -FailurePatterns $boot.failurePatterns -ReadyPatterns $boot.readyPatterns
                        if ($again.Verdict -eq 'FAIL') { $verdict = $again; break }
                        if ($process.HasExited) { $verdict = [PSCustomObject]@{ Verdict = 'FAIL'; Evidence = "server exited after ready with code $($process.ExitCode)" }; break }
                    }
                    # One source: the RPT repeats every script-log line. The RPT is the fallback only
                    # when no script log was written.
                    $selfTestSource = 'script'
                    $selfTestText = Read-ServerLogText -ProfilePath $profileDir -Kind Script
                    if (-not $selfTestText.Trim()) {
                        $selfTestSource = 'rpt'
                        $selfTestText = Read-ServerLogText -ProfilePath $profileDir -Kind Rpt
                    }
                    $selfTest = Get-SelfTestVerdict -LogText $selfTestText -Expected $expectedLogPrefixes
                    foreach ($component in $selfTest.Components.Keys) {
                        $entry = $selfTest.Components[$component]
                        $ran = 'no'
                        if ($entry.Complete -or ($entry.Pass + $entry.Fail) -gt 0) { $ran = 'yes' }
                        $cases = 'none'
                        if ($null -ne $entry.Cases) { $cases = $entry.Cases }
                        Write-Manifest -LogPath $manifest -Line ("SELFTEST {0} ran={1} cases={2} pass={3} fail={4}" -f $component, $ran, $cases, $entry.Pass, $entry.Fail)
                    }
                    Write-Manifest -LogPath $manifest -Line ("SELFTEST TOTAL pass={0} fail={1} flags={2} expected={3} source={4}" -f $selfTest.Pass, $selfTest.Fail, $flags.Count, $expectedLogPrefixes.Count, $selfTestSource)
                    foreach ($problem in $selfTest.Problems) {
                        Write-Manifest -LogPath $manifest -Line "SELFTEST PROBLEM $problem"
                    }
                    if ($selfTest.Verdict -eq 'FAIL' -and $verdict.Verdict -ne 'FAIL') {
                        $evidence = ''
                        if ($selfTest.Failures.Count -gt 0) { $evidence = $selfTest.Failures[0] }
                        else { $evidence = "self-test gate: {0}" -f ($selfTest.Problems -join '; ') }
                        $verdict = [PSCustomObject]@{ Verdict = 'FAIL'; Evidence = $evidence }
                    }
                }

                switch ($verdict.Verdict) {
                    'READY' { Write-Manifest -LogPath $manifest -Line "BOOT PASS $($verdict.Evidence)"; $exitCode = 0 }
                    'FAIL' { Write-Manifest -LogPath $manifest -Line "BOOT FAIL $($verdict.Evidence)"; $exitCode = 1 }
                    default { Write-Manifest -LogPath $manifest -Line "BOOT UNKNOWN no ready or failure line within $($boot.timeoutSeconds)s; read $profileDir"; $exitCode = 2 }
                }
            }
            finally {
                # Stops only the server process this script started. The boot check's storage is
                # disposable (instance 998 or 999), so a process stop is acceptable here.
                if (-not $process.HasExited) {
                    Stop-Process -Id $process.Id -Force
                    Write-Manifest -LogPath $manifest -Line "BOOT STOPPED pid=$($process.Id)"
                }
            }
        }
    }

    if ($exitCode -eq 0 -and $signatureUnknown) {
        Write-Manifest -LogPath $manifest -Line 'SUMMARY downgraded to UNKNOWN: signatures not proven'
        $exitCode = 2
    }

    $verdictWord = @{ 0 = 'PASS'; 1 = 'FAIL'; 2 = 'UNKNOWN' }[$exitCode]
    Write-Manifest -LogPath $manifest -Line ("SUMMARY {0} mod={1}" -f $verdictWord, $modRoot)
    Write-Information -MessageData ("{0}: {1} Log: {2}" -f $verdictWord, $modRoot, $manifest) -InformationAction Continue
}
catch {
    Write-Manifest -LogPath $manifest -Line ("FAIL {0}" -f $_.Exception.Message)
    Write-Information -MessageData ("FAIL: {0} Log: {1}" -f $_.Exception.Message, $manifest) -InformationAction Continue
    $exitCode = 1
}

exit $exitCode
