#Requires -Version 5.1

<#
.SYNOPSIS
    Validates a Greet welcome text (greet.json) with the same rules the client applies, before
    the file goes to the host. Mandatory before every upload (plan v0.4, G12, NIMBY-T-042).

.DESCRIPTION
    A malformed greet.json on the host is an engine "SCRIPT (E): JSON ERROR" in the RPT on every
    boot until it is fixed, and a release-gate failure, even though the server keeps serving the
    last good copy. This script is what keeps that file off the host. It checks:
      - the file parses as JSON and has the plan's shape (version, defaultLanguage, languages[]);
      - every language key is one of the 14 vanilla stringtable columns, not CJK (no font yet),
        not duplicated, and the default language is present;
      - the caps of plan G9 (title 60, tagline 80, 16 sections, heading 40, id 32, body 4,000,
        40,000 per language), which the client truncates and this script reports;
      - section ids are [a-z0-9-] and unique within a language;
      - every markdown link is https:// and no section has more than 8;
      - every media reference ![alt](id) names an id in the addon's Media\manifest.json and no
        section has more than 4;
      - raw < and > (the client prints them as angle quotes), tables wider than two columns, and
        unknown or unterminated constructs, as warnings;
      - the timing fields are numbers inside the ranges the server clamps to.
    Errors fail the run; warnings do not. Every finding is one line, the way the server logs it.

    Reads   : the greet.json under test and the addon manifest (both read-only).
    Writes  : <repo>\logs\greettext-<stamp>\greettext.log.
    Never   : changes the file under test, contacts a host, starts a server, or reads a secret.

    Exit codes: 0 = no error (warnings allowed), 1 = at least one error, 2 = the file could not
    be read or parsed.

    Runs in Windows PowerShell 5.1 and PowerShell 7.

.PARAMETER Path
    The greet.json to validate. Defaults to the repository draft
    server\profile-overrides\NimbyGreet\greet.json.

.PARAMETER ManifestPath
    The addon media manifest. Defaults to mods\own\nimby-pack\NimbyGreet\Media\manifest.json.
    A missing manifest means every media reference is an error.

.PARAMETER RepoRoot
    Repository root. Defaults to the parent of this script's folder.

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Test-GreetText.ps1

.EXAMPLE
    pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Test-GreetText.ps1 -Path D:\tmp\greet.json
#>
[CmdletBinding()]
param(
    [string]$Path,
    [string]$ManifestPath,
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }
if (-not $Path) { $Path = Join-Path $RepoRoot 'server\profile-overrides\NimbyGreet\greet.json' }
if (-not $ManifestPath) { $ManifestPath = Join-Path $RepoRoot 'mods\own\nimby-pack\NimbyGreet\Media\manifest.json' }

# The caps and lists of plan v0.4 (G9, G14, G10, G15). A change here is a plan change first and a
# nimbygreet_rules.c change second; the two must agree.
$script:GreetLanguages = @('original', 'english', 'czech', 'german', 'russian', 'polish', 'hungarian', 'italian', 'spanish', 'french', 'chinese', 'japanese', 'portuguese', 'chinesesimp')
$script:GreetCjkLanguages = @('chinese', 'japanese', 'chinesesimp')
$script:GreetCaps = @{
    MaxSections = 16; MaxBodyChars = 4000; MaxLinksPerSection = 8; MaxMediaPerSection = 4; MaxTablesPerSection = 2
    MaxTotalChars = 40000; MaxTitleChars = 60; MaxTaglineChars = 80; MaxHeadingChars = 40; MaxIdChars = 32
    MaxOpenDelaySeconds = 30; MaxOpenRetrySeconds = 300; MaxCooldownSeconds = 60; MaxRereadSeconds = 3600
}

function New-GreetFinding {
    param([Parameter(Mandatory)][ValidateSet('ERROR', 'WARN', 'INFO')][string]$Level, [Parameter(Mandatory)][string]$Message)
    return [pscustomobject]@{ Level = $Level; Message = $Message }
}

function Test-GreetId {
    # [a-z0-9-], 1 to 32 characters, not starting or ending with a dash (nimbygreet_rules.c IsValidId).
    param([AllowNull()][AllowEmptyString()][string]$Id)
    if ([string]::IsNullOrEmpty($Id)) { return $false }
    if ($Id.Length -gt $script:GreetCaps.MaxIdChars) { return $false }
    return ($Id -cmatch '^[a-z0-9]([a-z0-9-]*[a-z0-9])?$')
}

function Test-GreetUrl {
    # https:// with a host, no whitespace, quotes or brackets (nimbygreet_rules.c IsAcceptedUrl).
    param([AllowNull()][AllowEmptyString()][string]$Url)
    if ([string]::IsNullOrEmpty($Url)) { return $false }
    if (-not $Url.StartsWith('https://')) { return $false }
    if ($Url.Length -le 8 -or $Url.Length -gt 2048) { return $false }
    return ($Url -notmatch '[\s<>"'']')
}

function Get-GreetBodyReport {
    # Mirrors nimbygreet_markdown.c at the level the validator needs: counts links, media
    # references and tables, and flags the constructs the client degrades.
    param([Parameter(Mandatory)][AllowEmptyString()][string]$Body, [AllowNull()][string[]]$MediaIds)

    $report = [pscustomobject]@{ Links = 0; RejectedLinks = @(); Media = 0; UnknownMedia = @(); Tables = 0; WideRows = 0; RawBrackets = 0; Unterminated = 0 }
    $report.RawBrackets = ([regex]::Matches($Body, '[<>]')).Count
    $lines = $Body -split "`n"
    $inTable = $false
    foreach ($rawLine in $lines) {
        $line = $rawLine.TrimEnd("`r").Trim()
        if ($line.StartsWith('|')) {
            if (-not $inTable) { $report.Tables++; $inTable = $true }
            $inner = $line.TrimStart('|').TrimEnd('|')
            $cells = @($inner -split '\|' | ForEach-Object { $_.Trim() })
            $isSeparator = ($cells.Count -gt 0) -and (@($cells | Where-Object { $_ -notmatch '^:?-+:?$' }).Count -eq 0)
            if (-not $isSeparator -and $cells.Count -ne 2) { $report.WideRows++ }
            continue
        }
        $inTable = $false
        if ($line -match '^!\[(.*)\]\((.+)\)$') {
            $report.Media++
            $id = $Matches[2]
            if ($null -eq $MediaIds -or ($MediaIds -notcontains $id)) { $report.UnknownMedia += $id }
            continue
        }
        # Unescaped single-star and double-star counts per paragraph line: an odd count is unterminated
        # emphasis, which the client closes at the paragraph end.
        $stripped = [regex]::Replace($line, '\\[\\*\[\]()!#|_-]', '')
        $doubles = ([regex]::Matches($stripped, '\*\*')).Count
        $singles = ([regex]::Matches($stripped, '(?<!\*)\*(?!\*)')).Count
        if (($doubles % 2) -ne 0 -or ($singles % 2) -ne 0) { $report.Unterminated++ }
    }
    foreach ($match in [regex]::Matches($Body, '(?<!\\)\[([^\[\]]*)\]\(([^)]*)\)')) {
        if ($match.Index -gt 0 -and $Body[$match.Index - 1] -eq '!') { continue }
        $url = $match.Groups[2].Value
        if (Test-GreetUrl -Url $url) { $report.Links++ } else { $report.RejectedLinks += $url }
    }
    return $report
}

function Test-GreetDocument {
    # Returns the findings for a parsed greet.json (the object ConvertFrom-Json gives) and the
    # manifest's media ids (null when there is no manifest).
    param([Parameter(Mandatory)]$Document, [AllowNull()][string[]]$MediaIds)

    $findings = New-Object System.Collections.Generic.List[object]
    function Add-Finding { param([string]$Level, [string]$Message) $findings.Add((New-GreetFinding -Level $Level -Message $Message)) }
    function Test-HasProperty { param($Object, [string]$Name) return ($null -ne $Object) -and ($Object.PSObject.Properties.Name -contains $Name) }

    if (-not (Test-HasProperty $Document 'version')) { Add-Finding 'ERROR' 'version is missing' }
    else {
        $version = $Document.version
        if (($version -isnot [int]) -and ($version -isnot [long])) { Add-Finding 'ERROR' ("version must be an integer, got {0}" -f $version) }
        elseif ($version -lt 1) { Add-Finding 'ERROR' ("version must be 1 or higher, got {0}" -f $version) }
    }
    if (Test-HasProperty $Document 'forceShow') {
        # The engine writes booleans as 0 and 1 (the default file has "forceShow": 0) and reads true and false; both pass.
        $force = $Document.forceShow
        $forceOk = ($force -is [bool]) -or ((($force -is [int]) -or ($force -is [long])) -and ($force -eq 0 -or $force -eq 1))
        if (-not $forceOk) { Add-Finding 'ERROR' 'forceShow must be true or false (or 0 or 1)' }
    }
    foreach ($timing in @(
            @{ Name = 'openDelaySeconds'; Max = $script:GreetCaps.MaxOpenDelaySeconds },
            @{ Name = 'openRetrySeconds'; Max = $script:GreetCaps.MaxOpenRetrySeconds },
            @{ Name = 'requestCooldownSeconds'; Max = $script:GreetCaps.MaxCooldownSeconds },
            @{ Name = 'rereadSeconds'; Max = $script:GreetCaps.MaxRereadSeconds })) {
        if (-not (Test-HasProperty $Document $timing.Name)) { continue }
        $value = $Document.($timing.Name)
        if (($value -isnot [int]) -and ($value -isnot [long]) -and ($value -isnot [double]) -and ($value -isnot [decimal])) {
            Add-Finding 'ERROR' ("{0} must be a number" -f $timing.Name); continue
        }
        if ($value -lt 0 -or $value -gt $timing.Max) { Add-Finding 'WARN' ("{0}={1} is outside 0..{2}; the server clamps it" -f $timing.Name, $value, $timing.Max) }
    }

    $defaultLanguage = $null
    if (-not (Test-HasProperty $Document 'defaultLanguage')) { Add-Finding 'ERROR' 'defaultLanguage is missing' }
    else { $defaultLanguage = [string]$Document.defaultLanguage }

    if (-not (Test-HasProperty $Document 'languages')) { Add-Finding 'ERROR' 'languages is missing'; return $findings.ToArray() }
    $languages = @($Document.languages)
    if ($languages.Count -eq 0) { Add-Finding 'ERROR' 'languages is empty'; return $findings.ToArray() }

    $codes = @()
    foreach ($language in $languages) {
        if ($null -eq $language) { Add-Finding 'ERROR' 'an empty language entry'; continue }
        $code = ''
        if (Test-HasProperty $language 'language') { $code = [string]$language.language }
        $label = if ($code) { $code } else { '(no language key)' }
        if (-not $code) { Add-Finding 'ERROR' 'a language entry has no "language" key'; continue }
        if ($script:GreetLanguages -cnotcontains $code) { Add-Finding 'ERROR' ("unknown language key: {0} (vanilla column names only)" -f $code); continue }
        if ($script:GreetCjkLanguages -contains $code) { Add-Finding 'ERROR' ("{0}: CJK languages cannot ship until a font is proven (plan G14); the server skips this entry" -f $code); continue }
        if ($codes -contains $code) { Add-Finding 'ERROR' ("duplicate language key: {0}; the server skips the second" -f $code); continue }
        $codes += $code

        $title = ''
        if (Test-HasProperty $language 'title') { $title = [string]$language.title }
        if (-not $title) { Add-Finding 'ERROR' ("{0}: title is missing or empty" -f $label) }
        elseif ($title.Length -gt $script:GreetCaps.MaxTitleChars) { Add-Finding 'WARN' ("{0}: title is {1} characters, cut to {2}" -f $label, $title.Length, $script:GreetCaps.MaxTitleChars) }
        if (Test-HasProperty $language 'tagline') {
            $tagline = [string]$language.tagline
            if ($tagline.Length -gt $script:GreetCaps.MaxTaglineChars) { Add-Finding 'WARN' ("{0}: tagline is {1} characters, cut to {2}" -f $label, $tagline.Length, $script:GreetCaps.MaxTaglineChars) }
        }
        if (-not (Test-HasProperty $language 'sections')) { Add-Finding 'ERROR' ("{0}: sections is missing" -f $label); continue }
        $sections = @($language.sections)
        if ($sections.Count -eq 0) { Add-Finding 'ERROR' ("{0}: sections is empty" -f $label); continue }
        if ($sections.Count -gt $script:GreetCaps.MaxSections) { Add-Finding 'WARN' ("{0}: {1} sections, the client keeps the first {2}" -f $label, $sections.Count, $script:GreetCaps.MaxSections) }

        $ids = @()
        $total = 0
        $index = 0
        foreach ($section in $sections) {
            $index++
            if ($null -eq $section) { Add-Finding 'ERROR' ("{0}: section {1} is empty" -f $label, $index); continue }
            $id = ''
            if (Test-HasProperty $section 'id') { $id = [string]$section.id }
            $where = "{0} section {1} ({2})" -f $label, $index, $(if ($id) { $id } else { 'no id' })
            if (-not (Test-GreetId -Id $id)) { Add-Finding 'ERROR' ("{0}: id must be [a-z0-9-], 1 to {1} characters, not starting or ending with -" -f $where, $script:GreetCaps.MaxIdChars) }
            elseif ($ids -contains $id) { Add-Finding 'ERROR' ("{0}: duplicate id" -f $where) }
            else { $ids += $id }
            $heading = ''
            if (Test-HasProperty $section 'heading') { $heading = [string]$section.heading }
            if (-not $heading) { Add-Finding 'ERROR' ("{0}: heading is missing or empty" -f $where) }
            elseif ($heading.Length -gt $script:GreetCaps.MaxHeadingChars) { Add-Finding 'WARN' ("{0}: heading is {1} characters, cut to {2}" -f $where, $heading.Length, $script:GreetCaps.MaxHeadingChars) }
            $body = ''
            if (Test-HasProperty $section 'body') { $body = [string]$section.body }
            if (-not $body) { Add-Finding 'ERROR' ("{0}: body is missing or empty" -f $where); continue }
            if ($body.Length -gt $script:GreetCaps.MaxBodyChars) { Add-Finding 'WARN' ("{0}: body is {1} characters, cut to {2}" -f $where, $body.Length, $script:GreetCaps.MaxBodyChars) }
            $total += [Math]::Min($body.Length, $script:GreetCaps.MaxBodyChars)
            foreach ($bad in @('url', 'links', 'media')) {
                if (Test-HasProperty $section $bad) { Add-Finding 'WARN' ("{0}: field '{1}' is not part of the format (v0.2 leftover) and is ignored" -f $where, $bad) }
            }
            $report = Get-GreetBodyReport -Body $body -MediaIds $MediaIds
            foreach ($url in $report.RejectedLinks) { Add-Finding 'ERROR' ("{0}: link rejected (https:// only, no spaces or quotes): {1}" -f $where, $url) }
            if ($report.Links -gt $script:GreetCaps.MaxLinksPerSection) { Add-Finding 'WARN' ("{0}: {1} links, the client keeps the first {2} as buttons" -f $where, $report.Links, $script:GreetCaps.MaxLinksPerSection) }
            foreach ($mediaId in $report.UnknownMedia) { Add-Finding 'ERROR' ("{0}: media id not in the manifest: {1}" -f $where, $mediaId) }
            if ($report.Media -gt $script:GreetCaps.MaxMediaPerSection) { Add-Finding 'WARN' ("{0}: {1} media blocks, the client keeps the first {2}" -f $where, $report.Media, $script:GreetCaps.MaxMediaPerSection) }
            if ($report.Tables -gt $script:GreetCaps.MaxTablesPerSection) { Add-Finding 'WARN' ("{0}: {1} tables, the client renders the first {2}" -f $where, $report.Tables, $script:GreetCaps.MaxTablesPerSection) }
            if ($report.WideRows -gt 0) { Add-Finding 'WARN' ("{0}: {1} table row(s) not two cells wide print as written" -f $where, $report.WideRows) }
            if ($report.RawBrackets -gt 0) { Add-Finding 'WARN' ("{0}: {1} raw < or > character(s) render as angle quotes" -f $where, $report.RawBrackets) }
            if ($report.Unterminated -gt 0) { Add-Finding 'WARN' ("{0}: {1} line(s) with unterminated * or ** emphasis (closed at the paragraph end)" -f $where, $report.Unterminated) }
        }
        if ($total -gt $script:GreetCaps.MaxTotalChars) { Add-Finding 'WARN' ("{0}: {1} body characters in total, cut to {2}" -f $label, $total, $script:GreetCaps.MaxTotalChars) }
        Add-Finding 'INFO' ("{0}: {1} section(s), {2} body characters" -f $label, $sections.Count, $total)
    }
    if ($codes.Count -eq 0) { Add-Finding 'ERROR' 'no servable language; the server will have no text' }
    elseif ($defaultLanguage -and ($codes -cnotcontains $defaultLanguage)) { Add-Finding 'ERROR' ("defaultLanguage {0} is not among the servable languages ({1})" -f $defaultLanguage, ($codes -join ', ')) }
    return $findings.ToArray()
}

function Get-GreetManifestIds {
    # The media ids in Media\manifest.json, or $null when it cannot be read.
    param([Parameter(Mandatory)][string]$ManifestFile)
    if (-not (Test-Path -LiteralPath $ManifestFile)) { return $null }
    try {
        $manifest = Get-Content -LiteralPath $ManifestFile -Raw -Encoding UTF8 | ConvertFrom-Json
    }
    catch { return $null }
    # An empty array must come back as an empty array, not as $null (which means "no manifest"),
    # so the result is wrapped with the comma operator.
    if ($null -eq $manifest -or -not ($manifest.PSObject.Properties.Name -contains 'media')) { return , [string[]]@() }
    $ids = [string[]]@(@($manifest.media) | Where-Object { $null -ne $_ -and ($_.PSObject.Properties.Name -contains 'id') } | ForEach-Object { [string]$_.id })
    return , $ids
}

# ---- main --------------------------------------------------------------------------------------

if ($MyInvocation.InvocationName -ne '.') {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $logDir = Join-Path $RepoRoot ("logs\greettext-{0}" -f $stamp)
    New-Item -ItemType Directory -Path $logDir -Force | Out-Null
    $logFile = Join-Path $logDir 'greettext.log'
    function Write-Line { param([string]$Text) Write-Host $Text; Add-Content -LiteralPath $logFile -Value $Text -Encoding UTF8 }

    Write-Line ("Test-GreetText {0} file={1} manifest={2}" -f $stamp, $Path, $ManifestPath)
    if (-not (Test-Path -LiteralPath $Path)) { Write-Line ("ERROR file not found: {0}" -f $Path); exit 2 }
    $raw = Get-Content -LiteralPath $Path -Raw -Encoding UTF8
    if ($raw.Length -gt 0 -and [int][char]$raw[0] -eq 0xFEFF) { Write-Line 'WARN the file starts with a UTF-8 byte order mark; the engine may refuse it. Save without BOM'; $raw = $raw.Substring(1) }
    try {
        $document = $raw | ConvertFrom-Json
    }
    catch {
        Write-Line ("ERROR JSON does not parse: {0}" -f $_.Exception.Message)
        Write-Line ("FAIL: 1 error. Log: {0}" -f $logFile)
        exit 2
    }
    $mediaIds = Get-GreetManifestIds -ManifestFile $ManifestPath
    if ($null -eq $mediaIds) { Write-Line ("WARN manifest not readable: {0}; every media reference is an error" -f $ManifestPath) }
    $results = @(Test-GreetDocument -Document $document -MediaIds $mediaIds)
    foreach ($finding in $results) { Write-Line ("{0} {1}" -f $finding.Level, $finding.Message) }
    $errors = @($results | Where-Object { $_.Level -eq 'ERROR' }).Count
    $warnings = @($results | Where-Object { $_.Level -eq 'WARN' }).Count
    if ($errors -gt 0) {
        Write-Line ("FAIL: {0} error(s), {1} warning(s). Do not upload. Log: {2}" -f $errors, $warnings, $logFile)
        exit 1
    }
    Write-Line ("PASS: 0 errors, {0} warning(s). Log: {1}" -f $warnings, $logFile)
    exit 0
}
