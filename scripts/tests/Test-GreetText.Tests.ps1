# Pester 5 tests for scripts\Test-GreetText.ps1: the id and url rules, the body report (links,
# media, tables, brackets, unterminated emphasis), the document findings against the plan v0.4
# shape and caps, the manifest reader, and one end-to-end run of the script with its exit codes.
# The repository draft server\profile-overrides\NimbyGreet\greet.json must pass with no error.

BeforeAll {
    . (Join-Path $PSScriptRoot 'TestHelpers.ps1')
    $repo = Get-TestRepoRoot
    $greetScript = Join-Path $repo 'scripts\Test-GreetText.ps1'
    . (Get-ScriptFunctionBlock -Path $greetScript -Name 'New-GreetFinding', 'Test-GreetId', 'Test-GreetUrl', 'Get-GreetBodyReport', 'Test-GreetDocument', 'Get-GreetManifestIds')
    $script:GreetLanguages = Get-ScriptAssignmentValue -Path $greetScript -Name 'GreetLanguages'
    $script:GreetCjkLanguages = Get-ScriptAssignmentValue -Path $greetScript -Name 'GreetCjkLanguages'
    $script:GreetCaps = Get-ScriptAssignmentValue -Path $greetScript -Name 'GreetCaps'

    function New-GreetDoc {
        # A minimal valid document as ConvertFrom-Json would give it; $Edit mutates it before return.
        param([scriptblock]$Edit)
        $json = @'
{
  "version": 1,
  "forceShow": false,
  "defaultLanguage": "english",
  "openDelaySeconds": 1,
  "languages": [
    {
      "language": "english",
      "title": "NIMBY",
      "tagline": "Badlands",
      "sections": [
        { "id": "welcome", "heading": "Welcome", "body": "Hello **there**.\n\n[Discord](https://discord.gg/x)" },
        { "id": "rules", "heading": "Rules", "body": "Don't be a dick." }
      ]
    }
  ]
}
'@
        $doc = $json | ConvertFrom-Json
        if ($Edit) { & $Edit $doc }
        return $doc
    }

    function Get-Errors { param($Findings) return @($Findings | Where-Object { $_.Level -eq 'ERROR' } | ForEach-Object { $_.Message }) }
    function Get-Warnings { param($Findings) return @($Findings | Where-Object { $_.Level -eq 'WARN' } | ForEach-Object { $_.Message }) }
}

Describe 'Test-GreetId' {
    It 'accepts <Id>' -ForEach @(@{ Id = 'welcome' }, @{ Id = 'how-to-join-2' }, @{ Id = 'a' }, @{ Id = ('a' * 32) }) {
        Test-GreetId -Id $Id | Should -BeTrue
    }
    It 'rejects <Id>' -ForEach @(@{ Id = 'Welcome' }, @{ Id = '-a' }, @{ Id = 'a-' }, @{ Id = 'a b' }, @{ Id = '' }, @{ Id = ('a' * 33) }, @{ Id = 'a_b' }) {
        Test-GreetId -Id $Id | Should -BeFalse
    }
}

Describe 'Test-GreetUrl' {
    It 'accepts an https url' { Test-GreetUrl -Url 'https://discord.gg/kH3eJ5mdEv' | Should -BeTrue }
    It 'rejects http' { Test-GreetUrl -Url 'http://discord.gg/x' | Should -BeFalse }
    It 'rejects a bare scheme' { Test-GreetUrl -Url 'https://' | Should -BeFalse }
    It 'rejects whitespace and quotes' {
        Test-GreetUrl -Url 'https://a.b/c d' | Should -BeFalse
        Test-GreetUrl -Url 'https://a.b/"c' | Should -BeFalse
    }
}

Describe 'Get-GreetBodyReport' {
    It 'counts accepted links and lists rejected ones' {
        $report = Get-GreetBodyReport -Body 'a [one](https://a.b/1) b [two](http://a.b/2) c [three](https://a.b/3)' -MediaIds @()
        $report.Links | Should -Be 2
        $report.RejectedLinks | Should -Be @('http://a.b/2')
    }
    It 'does not count a media reference as a link' {
        $report = Get-GreetBodyReport -Body "![The map](map)`nand [link](https://a.b)" -MediaIds @('map')
        $report.Links | Should -Be 1
        $report.Media | Should -Be 1
        $report.UnknownMedia.Count | Should -Be 0
    }
    It 'reports an unknown media id, and every id when there is no manifest' {
        (Get-GreetBodyReport -Body '![x](missing)' -MediaIds @('map')).UnknownMedia | Should -Be @('missing')
        (Get-GreetBodyReport -Body '![x](map)' -MediaIds $null).UnknownMedia | Should -Be @('map')
        (Get-GreetBodyReport -Body '![x]()' -MediaIds @('map')).UnknownMedia | Should -Be @('')
    }
    It 'counts tables and rows that are not two cells wide' {
        $body = "| a | b |`n| --- | --- |`n| 1 | 2 |`n| 3 | 4 | 5 |`n`ntext`n`n| c | d |`n| --- | --- |"
        $report = Get-GreetBodyReport -Body $body -MediaIds @()
        $report.Tables | Should -Be 2
        $report.WideRows | Should -Be 1
    }
    It 'counts raw brackets and unterminated emphasis' {
        $report = Get-GreetBodyReport -Body "a < b`n**open`n*fine* and \*escaped`n* bullet with 5 * 3 inside" -MediaIds @()
        $report.RawBrackets | Should -Be 1
        $report.Unterminated | Should -Be 1
    }
}

Describe 'Test-GreetDocument' {
    It 'passes a minimal valid document with no error' {
        $findings = Test-GreetDocument -Document (New-GreetDoc) -MediaIds @()
        Get-Errors $findings | Should -BeNullOrEmpty
    }
    It 'requires version, defaultLanguage and languages' {
        $doc = New-GreetDoc { param($d) $d.PSObject.Properties.Remove('version'); $d.PSObject.Properties.Remove('defaultLanguage') }
        $errors = Get-Errors (Test-GreetDocument -Document $doc -MediaIds @())
        ($errors -join "`n") | Should -Match 'version is missing'
        ($errors -join "`n") | Should -Match 'defaultLanguage is missing'
    }
    It 'rejects a non-integer version and a non-boolean forceShow' {
        $doc = New-GreetDoc { param($d) $d.version = 1.5; $d.forceShow = 'yes' }
        $errors = Get-Errors (Test-GreetDocument -Document $doc -MediaIds @())
        ($errors -join "`n") | Should -Match 'version must be an integer'
        ($errors -join "`n") | Should -Match 'forceShow'
    }
    It 'rejects an unknown, a CJK and a duplicate language key, and a default that is not served' {
        $doc = New-GreetDoc {
            param($d)
            $extra = $d.languages[0] | ConvertTo-Json -Depth 10 | ConvertFrom-Json
            $extra.language = 'klingon'
            $cjk = $d.languages[0] | ConvertTo-Json -Depth 10 | ConvertFrom-Json
            $cjk.language = 'japanese'
            $dup = $d.languages[0] | ConvertTo-Json -Depth 10 | ConvertFrom-Json
            $d.languages = @($d.languages[0], $extra, $cjk, $dup)
            $d.defaultLanguage = 'german'
        }
        $errors = Get-Errors (Test-GreetDocument -Document $doc -MediaIds @())
        ($errors -join "`n") | Should -Match 'unknown language key: klingon'
        ($errors -join "`n") | Should -Match 'japanese: CJK'
        ($errors -join "`n") | Should -Match 'duplicate language key: english'
        ($errors -join "`n") | Should -Match 'defaultLanguage german is not among'
    }
    It 'rejects a bad id, a duplicate id, an empty heading and an empty body' {
        $doc = New-GreetDoc { param($d) $d.languages[0].sections[0].id = 'Bad Id'; $d.languages[0].sections[1].id = 'rules'; $d.languages[0].sections[1].heading = ''; $d.languages[0].sections[1].body = '' }
        $second = New-GreetDoc { param($d) $d.languages[0].sections[1].id = 'welcome' }
        $errors = Get-Errors (Test-GreetDocument -Document $doc -MediaIds @())
        ($errors -join "`n") | Should -Match 'id must be'
        ($errors -join "`n") | Should -Match 'heading is missing or empty'
        ($errors -join "`n") | Should -Match 'body is missing or empty'
        ((Get-Errors (Test-GreetDocument -Document $second -MediaIds @())) -join "`n") | Should -Match 'duplicate id'
    }
    It 'warns on every cap the client truncates' {
        $doc = New-GreetDoc {
            param($d)
            $d.languages[0].title = 't' * 61
            $d.languages[0].tagline = 'g' * 81
            $d.languages[0].sections[0].heading = 'h' * 41
            $d.languages[0].sections[0].body = 'b' * 4001
            $sections = @($d.languages[0].sections)
            while ($sections.Count -lt 17) { $sections += ($sections[1] | ConvertTo-Json -Depth 5 | ConvertFrom-Json); $sections[-1].id = ('s{0}' -f $sections.Count) }
            $d.languages[0].sections = $sections
        }
        $warnings = Get-Warnings (Test-GreetDocument -Document $doc -MediaIds @())
        ($warnings -join "`n") | Should -Match 'title is 61 characters'
        ($warnings -join "`n") | Should -Match 'tagline is 81 characters'
        ($warnings -join "`n") | Should -Match 'heading is 41 characters'
        ($warnings -join "`n") | Should -Match 'body is 4001 characters'
        ($warnings -join "`n") | Should -Match '17 sections'
        Get-Errors (Test-GreetDocument -Document $doc -MediaIds @()) | Should -BeNullOrEmpty
    }
    It 'rejects a non-https link and an unknown media id, warns on a v0.2 url field' {
        $doc = New-GreetDoc {
            param($d)
            $d.languages[0].sections[0].body = "[x](http://a.b)`n![alt](nope)"
            $d.languages[0].sections[0] | Add-Member -NotePropertyName url -NotePropertyValue 'https://a.b'
        }
        $findings = Test-GreetDocument -Document $doc -MediaIds @('map')
        ((Get-Errors $findings) -join "`n") | Should -Match 'link rejected'
        ((Get-Errors $findings) -join "`n") | Should -Match 'media id not in the manifest: nope'
        ((Get-Warnings $findings) -join "`n") | Should -Match "field 'url'"
    }
    It 'accepts forceShow written as 0 or 1 by the engine' {
        $doc = New-GreetDoc { param($d) $d.forceShow = 0 }
        Get-Errors (Test-GreetDocument -Document $doc -MediaIds @()) | Should -BeNullOrEmpty
    }
    It 'warns on an out-of-range timing value and rejects a non-numeric one' {
        $doc = New-GreetDoc { param($d) $d.openDelaySeconds = 99; $d | Add-Member -NotePropertyName rereadSeconds -NotePropertyValue 'soon' }
        $findings = Test-GreetDocument -Document $doc -MediaIds @()
        ((Get-Warnings $findings) -join "`n") | Should -Match 'openDelaySeconds=99 is outside'
        ((Get-Errors $findings) -join "`n") | Should -Match 'rereadSeconds must be a number'
    }
}

Describe 'Get-GreetManifestIds' {
    It 'returns null for a missing or unreadable manifest and the ids otherwise' {
        Get-GreetManifestIds -ManifestFile (Join-Path $TestDrive 'none.json') | Should -BeNullOrEmpty
        Set-Content -LiteralPath (Join-Path $TestDrive 'bad.json') -Value '{ nope' -Encoding UTF8
        Get-GreetManifestIds -ManifestFile (Join-Path $TestDrive 'bad.json') | Should -BeNullOrEmpty
        Set-Content -LiteralPath (Join-Path $TestDrive 'ok.json') -Value '{ "media": [ { "id": "map", "kind": "still" }, { "id": "clip", "kind": "clip" } ] }' -Encoding UTF8
        # The function returns the array as one object (comma operator), so it is joined before the comparison.
        ((Get-GreetManifestIds -ManifestFile (Join-Path $TestDrive 'ok.json')) -join ',') | Should -Be 'map,clip'
    }
    It 'reads the addon manifest in the repository (empty list, not null)' {
        $ids = Get-GreetManifestIds -ManifestFile (Join-Path $repo 'mods\own\nimby-pack\NimbyGreet\Media\manifest.json')
        $null -eq $ids | Should -BeFalse
        @($ids).Count | Should -Be 0
    }
}

Describe 'Test-GreetText.ps1 end to end' {
    It 'passes the repository draft with no error' {
        $logRoot = Join-Path $TestDrive 'repo'
        New-Item -ItemType Directory -Path $logRoot -Force | Out-Null
        # Write-Host goes to the information stream, so every stream is merged (*>&1) for the capture.
        $output = & $greetScript -Path (Join-Path $repo 'server\profile-overrides\NimbyGreet\greet.json') -ManifestPath (Join-Path $repo 'mods\own\nimby-pack\NimbyGreet\Media\manifest.json') -RepoRoot $logRoot *>&1 | Out-String
        $LASTEXITCODE | Should -Be 0
        $output | Should -Match 'PASS: 0 errors'
        Get-ChildItem -Path (Join-Path $logRoot 'logs') -Recurse -Filter 'greettext.log' | Should -Not -BeNullOrEmpty
    }
    It 'fails with exit 2 on a file that does not parse and exit 1 on a rule error' {
        $logRoot = Join-Path $TestDrive 'repo2'
        New-Item -ItemType Directory -Path $logRoot -Force | Out-Null
        $broken = Join-Path $TestDrive 'broken.json'
        Set-Content -LiteralPath $broken -Value '{ "version": 1, "text": "unterminated' -Encoding UTF8
        & $greetScript -Path $broken -RepoRoot $logRoot *>&1 | Out-Null
        $LASTEXITCODE | Should -Be 2
        $bad = Join-Path $TestDrive 'bad.json'
        (New-GreetDoc { param($d) $d.languages[0].sections[0].id = 'Bad Id' }) | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $bad -Encoding UTF8
        & $greetScript -Path $bad -RepoRoot $logRoot *>&1 | Out-Null
        $LASTEXITCODE | Should -Be 1
    }
}
