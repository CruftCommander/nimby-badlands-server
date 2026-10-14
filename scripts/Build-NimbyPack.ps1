#Requires -Version 5.1
<#
.SYNOPSIS
    Builds, signs, verifies, and boot-tests the @NIMBY client-and-server mod.

.DESCRIPTION
    For each addon listed in mods\own\nimby-pack\pack.json:
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

    Reads   : pack.json, the addon folders, DayZ Tools (DsUtils), a local DayZ Server install,
              the private key in -KeyDirectory.
    Writes  : <repo>\build\@NIMBY-<stamp>\ (the built mod), <repo>\logs\build-<stamp>\ (manifest,
              tool logs, server profile and logs), and the key pair in -KeyDirectory on first run.
              The boot check writes the server's persistence to
              <server>\mpmissions\<mission>\storage_<instanceId> (instance 999 by default, so it
              never touches a real server's storage_1).
    Never   : copies the private key anywhere, deletes anything, or contacts a remote server.

    Exit codes: 0 = PASS, 1 = FAIL, 2 = UNKNOWN (built and verified, but the boot check was
    skipped or could not reach a verdict).

    Runs in Windows PowerShell 5.1 and PowerShell 7. No elevation needed.

.PARAMETER KeyDirectory
    Folder holding <authority>.biprivatekey and <authority>.bikey. Created on first run.

.PARAMETER DayZToolsPath
    DayZ Tools install folder. Found through Steam when omitted.

.PARAMETER ServerPath
    DayZ Server install folder (contains DayZServer_x64.exe). Found through Steam, then
    D:\DayZServer, when omitted.

.PARAMETER SkipBootCheck
    Build and verify only.

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Build-NimbyPack.ps1
#>
[CmdletBinding()]
param(
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$KeyDirectory = 'D:\Keys\nimby',
    [string]$DayZToolsPath,
    [string]$ServerPath,
    [switch]$SkipBootCheck
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

Import-Module (Join-Path $PSScriptRoot 'modules\NimbySteam.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'modules\NimbyPbo.psm1') -Force

# ---------------------------------------------------------------------------
# Logic kept in functions so it can be smoke-tested with the tools stubbed.
# ---------------------------------------------------------------------------

function Write-Manifest {
    param(
        [Parameter(Mandatory)][string]$LogPath,
        [Parameter(Mandatory)][string]$Line
    )
    Add-Content -LiteralPath $LogPath -Value $Line -Encoding utf8
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

    $names = @{}
    foreach ($addon in $pack.addons) {
        foreach ($field in 'folder', 'pboName', 'prefix') {
            if (-not $addon.$field) { throw "An addon in pack.json is missing '$field'." }
        }
        if ($names.ContainsKey($addon.pboName)) { throw "Duplicate pboName '$($addon.pboName)' in pack.json." }
        $names[$addon.pboName] = $true
    }
    return $pack
}

function Resolve-ToolPath {
    # Returns the first existing candidate path, or $null.
    param([Parameter(Mandatory)][AllowEmptyCollection()][string[]]$Candidates)

    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) { return $candidate }
    }
    return $null
}

function Get-ServerArgument {
    # Builds the DayZ server argument list for the boot check.
    param(
        [Parameter(Mandatory)][string]$ConfigPath,
        [Parameter(Mandatory)][string]$ProfilePath,
        [Parameter(Mandatory)][string]$ModPath,
        [Parameter(Mandatory)][int]$Port
    )

    return @(
        "-config=$ConfigPath",
        "-port=$Port",
        "-profiles=$ProfilePath",
        "-mod=$ModPath",
        '-dologs',
        '-adminlog',
        '-freezecheck'
    )
}

function Write-BootConfig {
    # Writes a throwaway serverDZ config for the boot check. No passwords, BattlEye off.
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Mission,
        [Parameter(Mandatory)][int]$InstanceId
    )

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
        'storageAutoFix = 1;',
        'class Missions',
        '{',
        '    class DayZ',
        '    {',
        "        template = `"$Mission`";",
        '    };',
        '};'
    )
    Set-Content -LiteralPath $Path -Value $lines -Encoding utf8
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

function Read-ServerLogText {
    # Concatenates every RPT and script log in the profile folder. Shared-read so a running
    # server's open files can be read.
    param([Parameter(Mandatory)][string]$ProfilePath)

    $builder = New-Object System.Text.StringBuilder
    if (-not (Test-Path -LiteralPath $ProfilePath)) { return '' }
    foreach ($file in Get-ChildItem -LiteralPath $ProfilePath -File | Where-Object { $_.Extension -ieq '.rpt' -or $_.Name -like 'script*.log' }) {
        $stream = New-Object System.IO.FileStream($file.FullName, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
        try {
            $reader = New-Object System.IO.StreamReader($stream)
            [void]$builder.AppendLine($reader.ReadToEnd())
        }
        finally {
            $stream.Dispose()
        }
    }
    return $builder.ToString()
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
        & $FilePath @ArgumentList *>&1 | Out-File -LiteralPath $LogPath -Encoding utf8
        return $LASTEXITCODE
    }
    finally {
        Pop-Location
    }
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
    Write-Manifest -LogPath $manifest -Line ("START {0} RepoRoot={1} PS={2}" -f (Get-Date -Format o), $RepoRoot, $PSVersionTable.PSVersion)

    $packRoot = Join-Path $RepoRoot 'mods\own\nimby-pack'
    $pack = Get-PackDefinition -Path (Join-Path $packRoot 'pack.json')
    Write-Manifest -LogPath $manifest -Line ("PACK {0} addons={1}" -f $pack.modFolderName, (@($pack.addons | ForEach-Object { $_.pboName }) -join ','))

    # Tools ---------------------------------------------------------------
    $steamCandidates = @()
    if (${env:ProgramFiles(x86)}) { $steamCandidates += (Join-Path ${env:ProgramFiles(x86)} 'Steam') }
    $steamRoot = Get-SteamRootPath -Candidates $steamCandidates
    $libraries = @()
    if ($null -ne $steamRoot) { $libraries = Get-SteamLibraryPath -SteamRoot $steamRoot }

    if (-not $DayZToolsPath -and $libraries.Count -gt 0) {
        $DayZToolsPath = Find-SteamAppPath -LibraryPaths $libraries -FolderName 'DayZ Tools'
    }
    if (-not $DayZToolsPath) { throw 'DayZ Tools not found. Install it from Steam (app 830720) or pass -DayZToolsPath.' }

    $createKey = Resolve-ToolPath -Candidates @(Join-Path $DayZToolsPath 'Bin\DsUtils\DSCreateKey.exe')
    $signFile = Resolve-ToolPath -Candidates @(Join-Path $DayZToolsPath 'Bin\DsUtils\DSSignFile.exe')
    $checkSignatures = Resolve-ToolPath -Candidates @(Join-Path $DayZToolsPath 'Bin\DsUtils\DSCheckSignatures.exe')
    if ($null -eq $signFile) { throw "DSSignFile.exe not found under '$DayZToolsPath\Bin\DsUtils'." }
    Write-Manifest -LogPath $manifest -Line "TOOL DSSignFile $signFile"

    # Key -------------------------------------------------------------------
    $privateKey = Join-Path $KeyDirectory ("{0}.biprivatekey" -f $pack.keyAuthority)
    $publicKey = Join-Path $KeyDirectory ("{0}.bikey" -f $pack.keyAuthority)
    if (-not (Test-Path -LiteralPath $privateKey)) {
        if ($null -eq $createKey) { throw "No private key at '$privateKey' and DSCreateKey.exe not found to create one." }
        New-Item -ItemType Directory -Path $KeyDirectory -Force | Out-Null
        $code = Invoke-Native -FilePath $createKey -ArgumentList @($pack.keyAuthority) -LogPath (Join-Path $logDir 'DSCreateKey.log') -WorkingDirectory $KeyDirectory
        if (-not (Test-Path -LiteralPath $privateKey) -or -not (Test-Path -LiteralPath $publicKey)) {
            throw "DSCreateKey (exit $code) did not produce both key files in '$KeyDirectory'."
        }
        Write-Manifest -LogPath $manifest -Line "KEY CREATED authority=$($pack.keyAuthority) dir=$KeyDirectory"
    }
    else {
        if (-not (Test-Path -LiteralPath $publicKey)) { throw "Private key exists but public key '$publicKey' is missing." }
        Write-Manifest -LogPath $manifest -Line "KEY EXISTING authority=$($pack.keyAuthority) dir=$KeyDirectory"
    }

    # Build ---------------------------------------------------------------
    $modRoot = Join-Path $RepoRoot ("build\{0}-{1}" -f $pack.modFolderName, $stamp)
    $addonsOut = Join-Path $modRoot 'addons'
    $keysOut = Join-Path $modRoot 'keys'
    New-Item -ItemType Directory -Path $addonsOut, $keysOut -Force | Out-Null

    $failures = 0
    foreach ($addon in $pack.addons) {
        $source = Join-Path $packRoot $addon.folder
        if (-not (Test-Path -LiteralPath $source -PathType Container)) {
            Write-Manifest -LogPath $manifest -Line "ADDON FAIL $($addon.pboName) source folder missing: $source"
            $failures++
            continue
        }

        $pbo = Join-Path $addonsOut ("{0}.pbo" -f $addon.pboName)
        try {
            $written = @(Build-PboArchive -SourceFolder $source -Destination $pbo -Prefix $addon.prefix -Patterns $pack.includePatterns)
        }
        catch {
            Write-Manifest -LogPath $manifest -Line "ADDON FAIL $($addon.pboName) pack error: $($_.Exception.Message)"
            $failures++
            continue
        }

        $code = Invoke-Native -FilePath $signFile -ArgumentList @($privateKey, $pbo) -LogPath (Join-Path $logDir ("DSSignFile-{0}.log" -f $addon.pboName)) -WorkingDirectory $addonsOut
        $signature = Join-Path $addonsOut ("{0}.pbo.{1}.bisign" -f $addon.pboName, $pack.keyAuthority)
        if (-not (Test-Path -LiteralPath $signature)) {
            Write-Manifest -LogPath $manifest -Line "ADDON FAIL $($addon.pboName) DSSignFile exit=$code no signature at $signature"
            $failures++
            continue
        }

        $content = Get-PboContent -Path $pbo
        $expected = @(Get-PboSourceFile -Folder $source -Patterns $pack.includePatterns)
        $missing = @(Compare-PboToSource -SourceRelativePaths $expected -PboEntries $content.Entries)
        if (-not (Test-PboChecksum -Path $pbo)) {
            Write-Manifest -LogPath $manifest -Line "ADDON FAIL $($addon.pboName) checksum does not match"
            $failures++
            continue
        }
        if ($content.Prefix -ne $addon.prefix) {
            Write-Manifest -LogPath $manifest -Line ("ADDON FAIL {0} prefix '{1}' expected '{2}'" -f $addon.pboName, $content.Prefix, $addon.prefix)
            $failures++
            continue
        }
        if ($missing.Count -gt 0) {
            Write-Manifest -LogPath $manifest -Line ("ADDON FAIL {0} missing from PBO: {1}" -f $addon.pboName, ($missing -join ', '))
            $failures++
            continue
        }
        $hash = (Get-FileHash -LiteralPath $pbo -Algorithm SHA256).Hash
        Write-Manifest -LogPath $manifest -Line ("ADDON OK {0} entries={1} sources={2} signexit={3} sha256={4}" -f $addon.pboName, $content.Entries.Count, $written.Count, $code, $hash)
        foreach ($entry in $written) {
            Write-Manifest -LogPath $manifest -Line ("  ENTRY {0}\{1}" -f $addon.pboName, $entry)
        }
    }

    Copy-Item -LiteralPath $publicKey -Destination $keysOut
    $modCpp = @(
        "name = `"$($pack.modName)`";",
        "author = `"$($pack.author)`";",
        "tooltip = `"$($pack.modName)`";",
        "overview = `"$($pack.modName) server pack`";"
    )
    Set-Content -LiteralPath (Join-Path $modRoot 'mod.cpp') -Value $modCpp -Encoding utf8
    Write-Manifest -LogPath $manifest -Line "MOD $modRoot"

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
        $realLog = Join-Path $logDir 'DSCheckSignatures-build.log'
        $realExit = Invoke-Native -FilePath $checkSignatures -ArgumentList @($addonsOut, $keysOut) -LogPath $realLog

        # Folder name avoids failure-sounding words so it cannot leak into a verdict.
        $negativeDir = Join-Path $logDir 'sigcheck-control'
        New-Item -ItemType Directory -Path $negativeDir -Force | Out-Null
        $firstPbo = Get-ChildItem -LiteralPath $addonsOut -Filter '*.pbo' | Select-Object -First 1
        Copy-Item -LiteralPath $firstPbo.FullName -Destination $negativeDir
        $negativeLog = Join-Path $logDir 'DSCheckSignatures-control.log'
        $negativeExit = Invoke-Native -FilePath $checkSignatures -ArgumentList @($negativeDir, $keysOut) -LogPath $negativeLog

        # [string] turns an empty log (Get-Content returns $null) into an empty string.
        $realOutput = [string](Get-Content -LiteralPath $realLog -Raw -Encoding utf8)
        $negativeOutput = [string](Get-Content -LiteralPath $negativeLog -Raw -Encoding utf8)
        $expectedPbo = @(Get-ChildItem -LiteralPath $addonsOut -Filter '*.pbo' | ForEach-Object { $_.Name })
        $signatureResult = Get-SignatureVerdict -RealExit ([int]$realExit) -RealOutput $realOutput -NegativeExit ([int]$negativeExit) -NegativeOutput $negativeOutput -ExpectedPbo $expectedPbo
        Write-Manifest -LogPath $manifest -Line ("SIGN {0} {1}" -f $signatureResult.Verdict, $signatureResult.Evidence)
        if ($signatureResult.Verdict -eq 'FAIL') { throw "Signature check failed: $($signatureResult.Evidence)" }
        if ($signatureResult.Verdict -eq 'UNKNOWN') { $signatureUnknown = $true }
    }

    # Boot check ------------------------------------------------------------
    if ($SkipBootCheck) {
        Write-Manifest -LogPath $manifest -Line 'BOOT SKIPPED by -SkipBootCheck'
        $exitCode = 2
    }
    else {
        if (-not $ServerPath) {
            $candidates = @()
            if ($libraries.Count -gt 0) { $candidates += Find-SteamAppPath -LibraryPaths $libraries -FolderName 'DayZServer' }
            $candidates += 'D:\DayZServer'
            foreach ($candidate in $candidates) {
                if ($candidate -and (Test-Path -LiteralPath (Join-Path $candidate 'DayZServer_x64.exe'))) { $ServerPath = $candidate; break }
            }
        }

        $boot = $pack.bootCheck
        if (-not $ServerPath) {
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
            $bootConfig = Join-Path $logDir 'serverDZ.bootcheck.cfg'
            Write-BootConfig -Path $bootConfig -Mission $boot.mission -InstanceId ([int]$boot.instanceId)

            $serverExe = Join-Path $ServerPath 'DayZServer_x64.exe'
            $serverArgs = Get-ServerArgument -ConfigPath $bootConfig -ProfilePath $profileDir -ModPath $modRoot -Port ([int]$boot.port)
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

                switch ($verdict.Verdict) {
                    'READY' { Write-Manifest -LogPath $manifest -Line "BOOT PASS $($verdict.Evidence)"; $exitCode = 0 }
                    'FAIL' { Write-Manifest -LogPath $manifest -Line "BOOT FAIL $($verdict.Evidence)"; $exitCode = 1 }
                    default { Write-Manifest -LogPath $manifest -Line "BOOT UNKNOWN no ready or failure line within $($boot.timeoutSeconds)s; read $profileDir"; $exitCode = 2 }
                }
            }
            finally {
                # Stops only the server process this script started.
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
