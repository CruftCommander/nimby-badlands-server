#Requires -Version 5.1

<#
.SYNOPSIS
    Static checks of the repository that need no DayZ, Steam, signing key, or secret: the CI
    gate (.github\workflows\static-gate.yml) and a quick local check before a commit.

.DESCRIPTION
    Each check writes PASS or FAIL lines to the log; any FAIL fails the run.
      1. secrets   : password and passwordAdmin are empty in server\serverDZ.cfg.
      2. em-dash   : no em dash (U+2014) in a tracked text file under docs\ or mods\.
      3. and-and   : no double ampersand in a tracked .ps1, .psm1, or .psd1 file (Windows
                     PowerShell 5.1 has no pipeline chain operator).
      4. cfgmods   : every addon (and the dev harness, when its folder exists) has a NOTICE, every
                     folder named in its config.cpp CfgMods files[] exists, and every .c file sits
                     under one of those folders, so no *_selftest.c can go unloaded.
      5. pack      : every addon packs into an unsigned PBO (NimbyPbo.psm1) whose prefix matches
                     pack.json and which holds every include-pattern source file.
      6. eol       : every tracked file whose .gitattributes rule says eol=crlf is CRLF in the
                     working copy (git ls-files --eol).

    Reads   : the tracked files (git ls-files), mods\own\nimby-pack\pack.json and the addon
              folders, server\serverDZ.cfg, .gitattributes through git.
    Writes  : <repo>\logs\static-<stamp>\static.log and unsigned check PBOs under
              <repo>\logs\static-<stamp>\pbo\ (git-ignored).
    Never   : signs anything, reads a key, starts a server, contacts a remote, or changes a
              tracked file.

    Exit codes: 0 = every check passed, 1 = at least one failed.

    Runs in Windows PowerShell 5.1 and PowerShell 7. Needs git on PATH.

.PARAMETER RepoRoot
    Repository root. Defaults to the parent of this script's folder.

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Test-NimbyRepo.ps1
#>
[CmdletBinding()]
param(
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
# Windows PowerShell 5.1 leaves $PSScriptRoot empty in a param() default when an advanced
# script is started with -File, so the default is resolved here.
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }
$ErrorActionPreference = 'Stop'

Import-Module (Join-Path $PSScriptRoot 'modules\NimbyCommon.psm1') -Force
# Build is an approved verb from PowerShell 6 only; 5.1 would warn about Build-PboArchive.
Import-Module (Join-Path $PSScriptRoot 'modules\NimbyPbo.psm1') -Force -DisableNameChecking

function Invoke-Git {
    # Runs git in the repository and returns its output lines; throws on a nonzero exit.
    param([Parameter(Mandatory)][string[]]$ArgumentList)

    $ErrorActionPreference = 'Continue'
    $output = @(& git -C $RepoRoot @ArgumentList 2>&1 | ForEach-Object {
            if ($_ -is [System.Management.Automation.ErrorRecord]) { $_.Exception.Message } else { [string]$_ }
        })
    if ($LASTEXITCODE -ne 0) { throw ("git {0} exited {1}: {2}" -f ($ArgumentList -join ' '), $LASTEXITCODE, ($output -join ' ')) }
    return $output
}

function Get-TrackedFile {
    # Tracked paths (forward slashes) under the given top-level folders, or all when none given.
    param([string[]]$Under = @())

    $list = (Invoke-Git -ArgumentList (@('ls-files', '-z', '--') + $Under)) -join ''
    return @($list -split "`0" | Where-Object { $_ })
}

function Test-ServerConfigSecret {
    # FAIL lines for a non-empty password or passwordAdmin value. Never prints the value.
    param([Parameter(Mandatory)][string]$Path)

    $problems = @()
    $number = 0
    foreach ($line in (Get-Content -LiteralPath $Path -Encoding utf8)) {
        $number++
        $match = [regex]::Match($line, '^\s*(?<key>password|passwordAdmin)\s*=\s*"(?<value>[^"]*)"')
        if ($match.Success -and $match.Groups['value'].Value -ne '') { $problems += ("{0}:{1} {2} is not empty" -f $Path, $number, $match.Groups['key'].Value) }
    }
    return $problems
}

function Find-TextInFile {
    # "<path>:<line>" for each line of a UTF-8 text file that contains Text.
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Text,
        [Parameter(Mandatory)][string]$Label
    )

    $hits = @()
    $number = 0
    foreach ($line in ([System.IO.File]::ReadAllText($Path, [System.Text.Encoding]::UTF8) -split "\r?\n")) {
        $number++
        if ($line.Contains($Text)) { $hits += ("{0}:{1}" -f $Label, $number) }
    }
    return $hits
}

function Test-AddonScriptModule {
    # FAIL lines for one addon: missing NOTICE, a CfgMods files[] folder that does not exist or is
    # outside the prefix, or a .c file outside every listed folder (it would never load).
    param(
        [Parameter(Mandatory)][string]$Folder,
        [Parameter(Mandatory)][string]$Prefix
    )

    $problems = @()
    $name = Split-Path -Leaf $Folder
    if (-not (Test-Path -LiteralPath (Join-Path $Folder 'NOTICE') -PathType Leaf)) { $problems += "$name has no NOTICE" }
    $config = Join-Path $Folder 'config.cpp'
    if (-not (Test-Path -LiteralPath $config -PathType Leaf)) { return @($problems + "$name has no config.cpp") }

    $text = [System.IO.File]::ReadAllText($config)
    $listed = @()
    foreach ($block in [regex]::Matches($text, 'files\[\]\s*=\s*\{(?<list>[^}]*)\}')) {
        foreach ($entry in [regex]::Matches($block.Groups['list'].Value, '"(?<p>[^"]+)"')) {
            $value = $entry.Groups['p'].Value -replace '/', '\'
            if (-not $value.StartsWith($Prefix + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
                $problems += "$name config.cpp files[] '$($entry.Groups['p'].Value)' is outside the prefix '$Prefix'"
                continue
            }
            $path = Join-Path $Folder $value.Substring($Prefix.Length + 1)
            if (-not (Test-Path -LiteralPath $path -PathType Container)) { $problems += "$name config.cpp files[] '$($entry.Groups['p'].Value)' does not exist" }
            $listed += $path
        }
    }
    if ($listed.Count -eq 0) { $problems += "$name config.cpp names no CfgMods files[] folder" }

    foreach ($file in Get-ChildItem -LiteralPath $Folder -Recurse -File -Filter '*.c') {
        $covered = @($listed | Where-Object { Test-PathInside -Candidate $file.DirectoryName -Root $_ }).Count -gt 0
        if (-not $covered) { $problems += ("{0} {1} is under no CfgMods files[] folder and never loads" -f $name, $file.FullName.Substring($Folder.Length + 1)) }
    }
    return $problems
}

function Test-AddonPack {
    # FAIL lines for one addon packed unsigned: pack error, checksum, prefix, missing sources.
    param(
        [Parameter(Mandatory)][string]$Folder,
        [Parameter(Mandatory)][string]$PboName,
        [Parameter(Mandatory)][string]$Prefix,
        [Parameter(Mandatory)][string[]]$Patterns,
        [Parameter(Mandatory)][string]$OutDir
    )

    $pbo = Join-Path $OutDir ("{0}.pbo" -f $PboName)
    try { [void](Build-PboArchive -SourceFolder $Folder -Destination $pbo -Prefix $Prefix -Patterns $Patterns) }
    catch { return @("$PboName pack error: $($_.Exception.Message)") }
    $problems = @()
    if (-not (Test-PboChecksum -Path $pbo)) { $problems += "$PboName checksum does not match" }
    $content = Get-PboContent -Path $pbo
    if ($content.Prefix -ne $Prefix) { $problems += "$PboName prefix '$($content.Prefix)' expected '$Prefix'" }
    $missing = @(Compare-PboToSource -SourceRelativePaths @(Get-PboSourceFile -Folder $Folder -Patterns $Patterns) -PboEntries $content.Entries)
    if ($missing.Count -gt 0) { $problems += ("{0} missing from PBO: {1}" -f $PboName, ($missing -join ', ')) }
    return $problems
}

function Get-EolProblem {
    # "<path> is <w/...> but .gitattributes says eol=crlf" from git ls-files --eol records.
    param([AllowEmptyCollection()][string[]]$Record = @())

    $problems = @()
    foreach ($entry in $Record) {
        $match = [regex]::Match($entry, '^i/(?<i>\S*)\s+w/(?<w>\S*)\s+attr/(?<attr>[^\t]*)\t(?<path>.+)$')
        if (-not $match.Success) { continue }
        if ($match.Groups['attr'].Value -notmatch '\beol=crlf\b') { continue }
        $working = $match.Groups['w'].Value
        if ($working -eq 'crlf' -or $working -eq 'none' -or $working -eq '') { continue }
        $problems += ("{0} is w/{1} but .gitattributes says eol=crlf" -f $match.Groups['path'].Value, $working)
    }
    return $problems
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$logDir = Join-Path $RepoRoot ("logs\static-{0}" -f $stamp)
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
$log = Join-Path $logDir 'static.log'
$failed = 0

function Write-CheckResult {
    # One PASS line, or one FAIL line per problem, for a named check.
    param([Parameter(Mandatory)][string]$Check, [AllowEmptyCollection()][string[]]$Problem = @())
    if ($Problem.Count -eq 0) {
        Write-Manifest -LogPath $log -Line "PASS $Check"
        Write-Information -MessageData "PASS $Check" -InformationAction Continue
        return
    }
    foreach ($p in $Problem) {
        Write-Manifest -LogPath $log -Line "FAIL $Check $p"
        Write-Information -MessageData "FAIL $Check $p" -InformationAction Continue
    }
    $script:failed += $Problem.Count
}

try {
    Write-Manifest -LogPath $log -Line ("START {0} RepoRoot={1} PS={2}" -f (Get-Date -Format o), $RepoRoot, $PSVersionTable.PSVersion)

    # 1. secrets
    Write-CheckResult -Check 'secrets' -Problem @(Test-ServerConfigSecret -Path (Join-Path $RepoRoot 'server\serverDZ.cfg'))

    # 2. em-dash
    $emDash = [string][char]0x2014
    $hits = @()
    foreach ($relative in (Get-TrackedFile -Under 'docs', 'mods')) {
        if ($relative -match '\.(png|jpg|jpeg|paa|edds|pbo|bikey|p3d|ogg|wav)$') { continue }
        $hits += Find-TextInFile -Path (Join-Path $RepoRoot $relative) -Text $emDash -Label $relative
    }
    Write-CheckResult -Check 'em-dash' -Problem $hits

    # 3. and-and (built from two characters so this file does not match itself)
    $andAnd = '&' + '&'
    $hits = @()
    foreach ($relative in (Get-TrackedFile)) {
        if ($relative -notmatch '\.(ps1|psm1|psd1)$') { continue }
        $hits += Find-TextInFile -Path (Join-Path $RepoRoot $relative) -Text $andAnd -Label $relative
    }
    Write-CheckResult -Check 'and-and' -Problem $hits

    # 4. cfgmods and 5. pack
    $packRoot = Join-Path $RepoRoot 'mods\own\nimby-pack'
    $pack = Get-Content -LiteralPath (Join-Path $packRoot 'pack.json') -Raw -Encoding utf8 | ConvertFrom-Json
    $rows = @($pack.addons | ForEach-Object { [pscustomobject]@{ Folder = (Join-Path $packRoot $_.folder); PboName = $_.pboName; Prefix = $_.prefix } })
    $devProperty = $pack.PSObject.Properties['dev']
    if ($null -ne $devProperty -and $null -ne $devProperty.Value) {
        $devFolder = [System.IO.Path]::GetFullPath((Join-Path $packRoot $devProperty.Value.folder))
        if (Test-Path -LiteralPath $devFolder -PathType Container) { $rows += [pscustomobject]@{ Folder = $devFolder; PboName = $devProperty.Value.pboName; Prefix = $devProperty.Value.prefix } }
        else { Write-Manifest -LogPath $log -Line "NOTE dev folder $devFolder is not on this branch; not checked" }
    }
    $pboDir = Join-Path $logDir 'pbo'
    New-Item -ItemType Directory -Path $pboDir -Force | Out-Null
    $moduleProblems = @()
    $packProblems = @()
    foreach ($row in $rows) {
        $moduleProblems += Test-AddonScriptModule -Folder $row.Folder -Prefix $row.Prefix
        $packProblems += Test-AddonPack -Folder $row.Folder -PboName $row.PboName -Prefix $row.Prefix -Patterns @($pack.includePatterns) -OutDir $pboDir
    }
    Write-CheckResult -Check 'cfgmods' -Problem $moduleProblems
    Write-CheckResult -Check 'pack' -Problem $packProblems

    # 6. eol
    $records = ((Invoke-Git -ArgumentList @('ls-files', '--eol', '-z')) -join '') -split "`0" | Where-Object { $_ }
    Write-CheckResult -Check 'eol' -Problem @(Get-EolProblem -Record $records)
}
catch {
    Write-Manifest -LogPath $log -Line ("FAIL error {0}" -f $_.Exception.Message)
    Write-Information -MessageData ("FAIL error {0}" -f $_.Exception.Message) -InformationAction Continue
    $failed++
}

$verdict = 'PASS'
if ($failed -gt 0) { $verdict = 'FAIL' }
Write-Manifest -LogPath $log -Line ("SUMMARY {0} failures={1}" -f $verdict, $failed)
Write-Information -MessageData ("{0}: {1} failure(s). Log: {2}" -f $verdict, $failed, $log) -InformationAction Continue
if ($failed -gt 0) { exit 1 }
exit 0
