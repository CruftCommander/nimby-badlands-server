# Pester 5 tests for scripts\Deploy-MissionPatches.ps1: the patch-to-mission-file mapping, the
# canonical comparison, the ADD / REPLACE / ALREADY / FAIL plan against fixtures, the patched
# write (well-formed, CRLF, no BOM, layout kept), and the repository's own patch files against
# server\live. No network, no server, no Posh-SSH.

BeforeAll {
    . (Join-Path $PSScriptRoot 'TestHelpers.ps1')
    $repo = Get-TestRepoRoot
    $deployScript = Join-Path $repo 'scripts\Deploy-MissionPatches.ps1'
    $overrides = Join-Path $repo 'server\mission-overrides'
    $liveDir = Join-Path $repo 'server\live'
    Import-Module (Join-Path $repo 'scripts\modules\NimbyCommon.psm1') -Force
    . (Get-ScriptFunctionBlock -Path $deployScript -Name 'Read-XmlFile', 'Get-PatchDescriptor', 'ConvertTo-CanonicalXml', 'Get-NamedEntry', 'Get-PatchPlan', 'Merge-PatchIntoDocument', 'Get-FileNewLine', 'Save-XmlFile', 'Get-EntryCount', 'Get-EntryParent', 'Join-RemotePath', 'Resolve-PatchPath')
    $script:PatchTargets = (Get-ScriptAssignmentValue -Path $deployScript -Name 'PatchTargets')[0]

    function Write-Fixture {
        # Writes an XML fixture the way the live files are stored: UTF-8, no BOM, CRLF.
        param([string]$Path, [string]$Content)
        $directory = Split-Path -Parent $Path
        if (-not (Test-Path -LiteralPath $directory)) { New-Item -ItemType Directory -Path $directory -Force | Out-Null }
        $text = [regex]::Replace($Content.Trim(), "`r?`n", "`r`n") + "`r`n"
        [System.IO.File]::WriteAllText($Path, $text, (New-Object System.Text.UTF8Encoding($false)))
        return $Path
    }

    $liveXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<types>
    <type name="A">
        <nominal>1</nominal>
        <min>1</min>
        <flags count_in_map="1" count_in_cargo="0"/>
    </type>
    <type name="B">
        <nominal>2</nominal>
        <min>2</min>
    </type>
</types>
'@
    $patchXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<!-- header comment -->
<types>
  <!-- A: replaced -->
  <type name="A">
    <nominal>10</nominal>
    <min>5</min>
    <flags count_in_cargo="0" count_in_map="1"/>
  </type>
  <type name="C">
    <nominal>3</nominal>
    <min>3</min>
  </type>
</types>
'@
}

Describe 'Get-PatchDescriptor' {
    It 'maps a <Root> patch to <Relative>' -ForEach @(
        @{ Root = 'types'; Relative = 'db\types.xml'; Entry = 'type'; Container = '' }
        @{ Root = 'events'; Relative = 'db\events.xml'; Entry = 'event'; Container = '' }
        @{ Root = 'spawnabletypes'; Relative = 'cfgspawnabletypes.xml'; Entry = 'type'; Container = '' }
        @{ Root = 'eventposdef'; Relative = 'cfgeventspawns.xml'; Entry = 'event'; Container = '' }
        @{ Root = 'prototype'; Relative = 'mapgroupproto.xml'; Entry = 'group'; Container = '' }
        @{ Root = 'economycore'; Relative = 'cfgeconomycore.xml'; Entry = 'default'; Container = 'defaults' }
    ) {
        # A DRAFT fixture per root element, so this test does not depend on which repository patch files
        # are still DRAFT (every release moves its patches to APPLIED, which this function refuses).
        $content = "<?xml version=`"1.0`" encoding=`"UTF-8`" standalone=`"yes`"?>`r`n<!-- DRAFT - not applied to the live server. -->`r`n<$Root></$Root>"
        $path = Write-Fixture -Path (Join-Path $TestDrive ('map\' + $Root + '.patch.xml')) -Content $content
        $descriptor = Get-PatchDescriptor -PatchPath $path
        $descriptor.Root | Should -Be $Root
        $descriptor.Relative | Should -Be $Relative
        $descriptor.Entry | Should -Be $Entry
        $descriptor.Container | Should -Be $Container
    }

    It 'refuses an applied patch (header says APPLIED): <PatchFile>' -ForEach @(
        @{ PatchFile = 'db\events.helicrash.patch.xml' }
        @{ PatchFile = 'db\types.release1-loot.patch.xml' }
        @{ PatchFile = 'mapgroupproto.cisterns.patch.xml' }
        @{ PatchFile = 'db\types.dpv.patch.xml' }
        @{ PatchFile = 'db\types.release2-loot.patch.xml' }
        @{ PatchFile = 'db\types.oasis-military.patch.xml' }
        @{ PatchFile = 'db\events.dpv.patch.xml' }
        @{ PatchFile = 'cfgspawnabletypes.dpv.patch.xml' }
        @{ PatchFile = 'cfgeventspawns.dpv.patch.xml' }
        @{ PatchFile = 'cfgeconomycore.logging.patch.xml' }
    ) {
        { Get-PatchDescriptor -PatchPath (Join-Path $overrides $PatchFile) } | Should -Throw '*APPLIED*'
    }

    It 'accepts a DRAFT header, whose text says "not applied" in lower case' {
        $path = Write-Fixture -Path (Join-Path $TestDrive 'map\draft.xml') -Content '<?xml version="1.0"?><!-- DRAFT - not applied to the live server. --><types/>'
        (Get-PatchDescriptor -PatchPath $path).Relative | Should -Be 'db\types.xml'
    }

    It 'refuses a root element it does not know' {
        $path = Write-Fixture -Path (Join-Path $TestDrive 'odd.xml') -Content '<?xml version="1.0"?><messages><message>x</message></messages>'
        { Get-PatchDescriptor -PatchPath $path } | Should -Throw '*<messages>*'
    }
}

Describe 'ConvertTo-CanonicalXml' {
    It 'ignores comments, whitespace and attribute order' {
        $a = Read-XmlFile -Path (Write-Fixture -Path (Join-Path $TestDrive 'a.xml') -Content '<t><type name="X"><!-- c --><nominal> 1 </nominal><flags a="1" b="2"/></type></t>')
        $b = Read-XmlFile -Path (Write-Fixture -Path (Join-Path $TestDrive 'b.xml') -Content "<t>`r`n  <type name=`"X`">`r`n    <nominal>1</nominal>`r`n    <flags b=`"2`" a=`"1`"/>`r`n  </type>`r`n</t>")
        (ConvertTo-CanonicalXml -Node $a.DocumentElement.FirstChild) | Should -Be (ConvertTo-CanonicalXml -Node (Get-NamedEntry -Document $b -Entry 'type' -Name 'X')[0])
    }

    It 'sees a changed value' {
        $a = Read-XmlFile -Path (Write-Fixture -Path (Join-Path $TestDrive 'a2.xml') -Content '<t><type name="X"><nominal>1</nominal></type></t>')
        $b = Read-XmlFile -Path (Write-Fixture -Path (Join-Path $TestDrive 'b2.xml') -Content '<t><type name="X"><nominal>2</nominal></type></t>')
        (ConvertTo-CanonicalXml -Node $a.DocumentElement.FirstChild) | Should -Not -Be (ConvertTo-CanonicalXml -Node $b.DocumentElement.FirstChild)
    }
}

Describe 'Get-PatchPlan' {
    BeforeEach {
        $live = Read-XmlFile -Path (Write-Fixture -Path (Join-Path $TestDrive 'plan\live.xml') -Content $liveXml)
        $reference = Read-XmlFile -Path (Write-Fixture -Path (Join-Path $TestDrive 'plan\reference.xml') -Content $liveXml)
        $patch = Read-XmlFile -Path (Write-Fixture -Path (Join-Path $TestDrive 'plan\patch.xml') -Content $patchXml)
    }

    It 'plans REPLACE for a reference row and ADD for a new one when live matches the reference' {
        $plan = @(Get-PatchPlan -PatchDocument $patch -LiveDocument $live -ReferenceDocument $reference -Entry 'type')
        $plan.Count | Should -Be 2
        ($plan | Where-Object Name -eq 'A').Mode | Should -Be 'REPLACE'
        ($plan | Where-Object Name -eq 'C').Mode | Should -Be 'ADD'
    }

    It 'FAILS a REPLACE when live has drifted from the reference' {
        $drifted = Read-XmlFile -Path (Write-Fixture -Path (Join-Path $TestDrive 'plan\drift.xml') -Content ($liveXml -replace '<nominal>1</nominal>', '<nominal>7</nominal>'))
        $plan = @(Get-PatchPlan -PatchDocument $patch -LiveDocument $drifted -ReferenceDocument $reference -Entry 'type')
        ($plan | Where-Object Name -eq 'A').Mode | Should -Be 'FAIL'
        ($plan | Where-Object Name -eq 'A').Reason | Should -Match 'differs from the reference'
        ($plan | Where-Object Name -eq 'C').Mode | Should -Be 'ADD'
    }

    It 'FAILS an ADD when live already holds the name' {
        $withC = Read-XmlFile -Path (Write-Fixture -Path (Join-Path $TestDrive 'plan\withc.xml') -Content ($liveXml -replace '</types>', '<type name="C"><nominal>9</nominal></type></types>'))
        $plan = @(Get-PatchPlan -PatchDocument $patch -LiveDocument $withC -ReferenceDocument $reference -Entry 'type')
        ($plan | Where-Object Name -eq 'C').Mode | Should -Be 'FAIL'
        ($plan | Where-Object Name -eq 'C').Reason | Should -Match 'already holds'
    }

    It 'reports ALREADY when live already equals the patch (a re-run)' {
        $applied = Read-XmlFile -Path (Join-Path $TestDrive 'plan\patch.xml')
        $plan = @(Get-PatchPlan -PatchDocument $patch -LiveDocument $applied -ReferenceDocument $reference -Entry 'type')
        @($plan | Where-Object Mode -eq 'ALREADY').Count | Should -Be 2
    }

    It 'FAILS a name repeated inside the patch and an element of the wrong kind' {
        $bad = Read-XmlFile -Path (Write-Fixture -Path (Join-Path $TestDrive 'plan\bad.xml') -Content '<types><type name="A"><nominal>1</nominal></type><type name="A"><nominal>2</nominal></type><event name="E"/></types>')
        $plan = @(Get-PatchPlan -PatchDocument $bad -LiveDocument $live -ReferenceDocument $reference -Entry 'type')
        $plan[1].Mode | Should -Be 'FAIL'
        $plan[1].Reason | Should -Match 'repeated'
        $plan[2].Mode | Should -Be 'FAIL'
        $plan[2].Reason | Should -Match '<event>'
    }
}

Describe 'Merge-PatchIntoDocument and Save-XmlFile' {
    It 'replaces A, keeps B, appends C, and writes CRLF without a BOM' {
        $live = Read-XmlFile -Path (Write-Fixture -Path (Join-Path $TestDrive 'merge\live.xml') -Content $liveXml)
        $reference = Read-XmlFile -Path (Write-Fixture -Path (Join-Path $TestDrive 'merge\reference.xml') -Content $liveXml)
        $patch = Read-XmlFile -Path (Write-Fixture -Path (Join-Path $TestDrive 'merge\patch.xml') -Content $patchXml)
        $plan = @(Get-PatchPlan -PatchDocument $patch -LiveDocument $live -ReferenceDocument $reference -Entry 'type')
        $result = Merge-PatchIntoDocument -LiveDocument $live -Plan $plan -Entry 'type'
        $result.Added | Should -Be 1
        $result.Replaced | Should -Be 1
        $out = Join-Path $TestDrive 'merge\out\db\types.xml'
        Save-XmlFile -Document $live -Path $out

        $bytes = [System.IO.File]::ReadAllBytes($out)
        ($bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB) | Should -BeFalse
        $text = [System.IO.File]::ReadAllText($out)
        $text | Should -Match '^<\?xml version="1.0" encoding="UTF-8" standalone="yes"\?>\r\n'
        ($text -split "`r`n").Count | Should -BeGreaterThan 10
        ([regex]::Matches($text, "(?<!`r)`n")).Count | Should -Be 0

        $check = Read-XmlFile -Path $out
        Get-EntryCount -Document $check -Entry 'type' | Should -Be 3
        $names = @($check.DocumentElement.SelectNodes("./type") | ForEach-Object { $_.GetAttribute('name') })
        $names | Should -Be @('A', 'B', 'C')
        (Get-NamedEntry -Document $check -Entry 'type' -Name 'A')[0].SelectSingleNode('nominal').InnerText | Should -Be '10'
        (Get-NamedEntry -Document $check -Entry 'type' -Name 'B')[0].SelectSingleNode('nominal').InnerText | Should -Be '2'
    }
}

Describe 'Get-FileNewLine and Save-XmlFile -NewLine' {
    It 'keeps an LF live file LF, as the mirrored live types.xml is' {
        $lf = Join-Path $TestDrive 'eol\live-lf.xml'
        New-Item -ItemType Directory -Path (Split-Path -Parent $lf) -Force | Out-Null
        [System.IO.File]::WriteAllText($lf, ($liveXml.Trim() -replace "`r`n", "`n") + "`n", (New-Object System.Text.UTF8Encoding($false)))
        Get-FileNewLine -Path $lf | Should -Be "`n"
        $live = Read-XmlFile -Path $lf
        $patch = Read-XmlFile -Path (Write-Fixture -Path (Join-Path $TestDrive 'eol\patch.xml') -Content $patchXml)
        $plan = @(Get-PatchPlan -PatchDocument $patch -LiveDocument $live -ReferenceDocument (Read-XmlFile -Path $lf) -Entry 'type')
        Merge-PatchIntoDocument -LiveDocument $live -Plan $plan -Entry 'type' | Out-Null
        $out = Join-Path $TestDrive 'eol\out.xml'
        Save-XmlFile -Document $live -Path $out -NewLine (Get-FileNewLine -Path $lf)
        $text = [System.IO.File]::ReadAllText($out)
        $text.Contains("`r") | Should -BeFalse
        Get-EntryCount -Document (Read-XmlFile -Path $out) -Entry 'type' | Should -Be 3
    }

    It 'reads CRLF from a CRLF file' {
        Get-FileNewLine -Path (Write-Fixture -Path (Join-Path $TestDrive 'eol\crlf.xml') -Content '<t/>') | Should -Be "`r`n"
    }
}

Describe 'The repository patch files against server\live' {
    # Every APPLIED patch must be reflected in server\live: planning it against the mirrored live file
    # (live as its own reference) gives ALREADY on every row. A DRAFT row here means Sync-LiveReplica.ps1
    # has not run since the apply, or the mirror was never refreshed; a FAIL row means live has moved on
    # top of the applied values. Get-PatchDescriptor refuses APPLIED headers, so the documents are read
    # directly. The release 2 set was applied November 8, 2026 (logs\deploy-20261108-131721).
    It '<PatchFile> (APPLIED) is already in server\live: every row ALREADY' -ForEach @(
        @{ PatchFile = 'db\types.dpv.patch.xml'; Relative = 'db\types.xml'; Entry = 'type'; Container = '' }
        @{ PatchFile = 'db\types.release2-loot.patch.xml'; Relative = 'db\types.xml'; Entry = 'type'; Container = '' }
        @{ PatchFile = 'db\types.oasis-military.patch.xml'; Relative = 'db\types.xml'; Entry = 'type'; Container = '' }
        @{ PatchFile = 'db\events.dpv.patch.xml'; Relative = 'db\events.xml'; Entry = 'event'; Container = '' }
        @{ PatchFile = 'cfgspawnabletypes.dpv.patch.xml'; Relative = 'cfgspawnabletypes.xml'; Entry = 'type'; Container = '' }
        @{ PatchFile = 'cfgeventspawns.dpv.patch.xml'; Relative = 'cfgeventspawns.xml'; Entry = 'event'; Container = '' }
        @{ PatchFile = 'cfgeconomycore.logging.patch.xml'; Relative = 'cfgeconomycore.xml'; Entry = 'default'; Container = 'defaults' }
    ) {
        $livePath = Join-Path $liveDir $Relative
        $live = Read-XmlFile -Path $livePath
        $patch = Read-XmlFile -Path (Join-Path $overrides $PatchFile)
        $plan = @(Get-PatchPlan -PatchDocument $patch -LiveDocument $live -ReferenceDocument (Read-XmlFile -Path $livePath) -Entry $Entry -Container $Container)
        $plan.Count | Should -BeGreaterThan 0
        @($plan | Where-Object Mode -ne 'ALREADY' | ForEach-Object { '{0}={1}' -f $_.Name, $_.Mode }) | Should -BeNullOrEmpty
    }

    It 'the release 2 row counts match the deploy manifest (3 + 4 + 16 type rows, 1 event, 1 spawnable type, 2 positions, 2 defaults)' {
        $count = {
            param($PatchFile, $Entry, $Container)
            $doc = Read-XmlFile -Path (Join-Path $overrides $PatchFile)
            @((Get-EntryParent -Document $doc -Container $Container).ChildNodes | Where-Object { $_.NodeType -eq 'Element' -and $_.LocalName -eq $Entry }).Count
        }
        (& $count 'db\types.dpv.patch.xml' 'type' '') | Should -Be 3
        (& $count 'db\types.release2-loot.patch.xml' 'type' '') | Should -Be 4
        (& $count 'db\types.oasis-military.patch.xml' 'type' '') | Should -Be 16
        (& $count 'db\events.dpv.patch.xml' 'event' '') | Should -Be 1
        (& $count 'cfgspawnabletypes.dpv.patch.xml' 'type' '') | Should -Be 1
        (& $count 'cfgeventspawns.dpv.patch.xml' 'event' '') | Should -Be 2
        (& $count 'cfgeconomycore.logging.patch.xml' 'default' 'defaults') | Should -Be 2
    }
}

Describe 'Nested container (cfgeconomycore defaults)' {
    It 'plans a defaults patch as REPLACE under the defaults container and applies it to a copy of server\live' {
        # A fixture rather than the repository's logging patch, which is APPLIED and already in server\live.
        $livePath = Join-Path $liveDir 'cfgeconomycore.xml'
        $live = Read-XmlFile -Path $livePath
        $current = (Get-NamedEntry -Document $live -Entry 'default' -Name 'log_ce_loop' -Container 'defaults')[0].GetAttribute('value')
        $flipped = if ($current -eq 'true') { 'false' } else { 'true' }
        $patchPath = Write-Fixture -Path (Join-Path $TestDrive 'eco\patch.xml') -Content ("<?xml version=`"1.0`" encoding=`"UTF-8`" standalone=`"yes`"?>`r`n<!-- DRAFT - not applied to the live server. -->`r`n<economycore>`r`n  <defaults>`r`n    <default name=`"log_ce_loop`" value=`"{0}`"/>`r`n  </defaults>`r`n</economycore>" -f $flipped)
        $patch = Get-PatchDescriptor -PatchPath $patchPath
        $patch.Container | Should -Be 'defaults'
        $plan = @(Get-PatchPlan -PatchDocument $patch.Document -LiveDocument $live -ReferenceDocument (Read-XmlFile -Path $livePath) -Entry 'default' -Container 'defaults')
        $plan.Count | Should -Be 1
        $plan[0].Mode | Should -Be 'REPLACE'
        $before = Get-EntryCount -Document $live -Entry 'default' -Container 'defaults'
        $result = Merge-PatchIntoDocument -LiveDocument $live -Plan $plan -Entry 'default' -Container 'defaults'
        $result.Replaced | Should -Be 1
        $out = Join-Path $TestDrive 'eco\cfgeconomycore.xml'
        Save-XmlFile -Document $live -Path $out
        $check = Read-XmlFile -Path $out
        Get-EntryCount -Document $check -Entry 'default' -Container 'defaults' | Should -Be $before
        (Get-NamedEntry -Document $check -Entry 'default' -Name 'log_ce_loop' -Container 'defaults')[0].GetAttribute('value') | Should -Be $flipped
        (Get-NamedEntry -Document $check -Entry 'default' -Name 'log_ce_vehicle' -Container 'defaults')[0].GetAttribute('value') | Should -Be 'true'
        @($check.DocumentElement.SelectNodes('./classes/rootclass')).Count | Should -Be 10
    }

    It 'refuses a missing container' {
        $doc = Read-XmlFile -Path (Write-Fixture -Path (Join-Path $TestDrive 'eco\nodefaults.xml') -Content '<economycore><classes/></economycore>')
        { Get-EntryParent -Document $doc -Container 'defaults' } | Should -Throw '*Expected one <defaults>*'
    }
}

Describe 'Get-NamedEntry' {
    It 'returns an array for one match and an empty array for none' {
        $doc = Read-XmlFile -Path (Write-Fixture -Path (Join-Path $TestDrive 'named.xml') -Content '<types><type name="A"/></types>')
        $one = Get-NamedEntry -Document $doc -Entry 'type' -Name 'A'
        $one.Count | Should -Be 1
        $one[0].GetAttribute('name') | Should -Be 'A'
        $none = Get-NamedEntry -Document $doc -Entry 'type' -Name 'Z'
        $none.Count | Should -Be 0
    }
}

Describe 'Resolve-PatchPath' {
    It 'expands wildcards and comma-separated lists in order, without duplicates' {
        $all = Resolve-PatchPath -Patch @((Join-Path $overrides '*.dpv.patch.xml'), ((Join-Path $overrides 'db\*.dpv.patch.xml') + ',' + (Join-Path $overrides 'db\types.dpv.patch.xml')))
        $all.Count | Should -Be 4
        @($all | Where-Object { $_ -like '*cfgeventspawns.dpv.patch.xml' }).Count | Should -Be 1
        @($all | Where-Object { $_ -like '*\db\types.dpv.patch.xml' }).Count | Should -Be 1   # listed twice (wildcard and explicit), kept once
    }

    It 'throws when a piece matches nothing' {
        { Resolve-PatchPath -Patch @((Join-Path $overrides 'nothing-here-*.xml')) } | Should -Throw '*matches*'
    }
}

Describe 'Join-RemotePath' {
    It 'joins with one slash' {
        Join-RemotePath -Parent '/a/' -Child 'b' | Should -Be '/a/b'
        Join-RemotePath -Parent '/a' -Child 'b/c' | Should -Be '/a/b/c'
    }
}
