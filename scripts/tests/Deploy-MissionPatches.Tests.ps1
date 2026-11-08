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
    . (Get-ScriptFunctionBlock -Path $deployScript -Name 'Read-XmlFile', 'Get-PatchDescriptor', 'ConvertTo-CanonicalXml', 'Get-NamedEntry', 'Get-PatchPlan', 'Merge-PatchIntoDocument', 'Get-FileNewLine', 'Save-XmlFile', 'Get-EntryCount', 'Join-RemotePath', 'Resolve-PatchPath')
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
    It 'maps <PatchFile> to <Relative>' -ForEach @(
        @{ PatchFile = 'db\types.dpv.patch.xml'; Relative = 'db\types.xml'; Entry = 'type' }
        @{ PatchFile = 'db\types.release2-loot.patch.xml'; Relative = 'db\types.xml'; Entry = 'type' }
        @{ PatchFile = 'db\events.dpv.patch.xml'; Relative = 'db\events.xml'; Entry = 'event' }
        @{ PatchFile = 'db\events.helicrash.patch.xml'; Relative = 'db\events.xml'; Entry = 'event' }
        @{ PatchFile = 'cfgspawnabletypes.dpv.patch.xml'; Relative = 'cfgspawnabletypes.xml'; Entry = 'type' }
        @{ PatchFile = 'cfgeventspawns.dpv.patch.xml'; Relative = 'cfgeventspawns.xml'; Entry = 'event' }
        @{ PatchFile = 'mapgroupproto.cisterns.patch.xml'; Relative = 'mapgroupproto.xml'; Entry = 'group' }
    ) {
        $descriptor = Get-PatchDescriptor -PatchPath (Join-Path $overrides $PatchFile)
        $descriptor.Relative | Should -Be $Relative
        $descriptor.Entry | Should -Be $Entry
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
    It '<PatchFile> plans with no FAIL row and applies to a copy of the live file' -ForEach @(
        @{ PatchFile = 'db\types.dpv.patch.xml'; Relative = 'db\types.xml'; Entry = 'type'; Expect = 'ADD' }
        @{ PatchFile = 'db\types.release2-loot.patch.xml'; Relative = 'db\types.xml'; Entry = 'type'; Expect = 'REPLACE' }
        @{ PatchFile = 'db\events.dpv.patch.xml'; Relative = 'db\events.xml'; Entry = 'event'; Expect = 'ADD' }
        @{ PatchFile = 'cfgspawnabletypes.dpv.patch.xml'; Relative = 'cfgspawnabletypes.xml'; Entry = 'type'; Expect = 'ADD' }
    ) {
        $livePath = Join-Path $liveDir $Relative
        $patch = Get-PatchDescriptor -PatchPath (Join-Path $overrides $PatchFile)
        $live = Read-XmlFile -Path $livePath
        $reference = Read-XmlFile -Path $livePath
        $plan = @(Get-PatchPlan -PatchDocument $patch.Document -LiveDocument $live -ReferenceDocument $reference -Entry $Entry)
        $plan.Count | Should -BeGreaterThan 0
        @($plan | Where-Object Mode -eq 'FAIL') | Should -BeNullOrEmpty
        @($plan | Where-Object Mode -ne $Expect) | Should -BeNullOrEmpty
        $before = Get-EntryCount -Document $live -Entry $Entry
        $result = Merge-PatchIntoDocument -LiveDocument $live -Plan $plan -Entry $Entry
        $out = Join-Path $TestDrive ('real\' + $Relative)
        Save-XmlFile -Document $live -Path $out
        Get-EntryCount -Document (Read-XmlFile -Path $out) -Entry $Entry | Should -Be ($before + $result.Added)
    }

    It 'cfgeventspawns.dpv.patch.xml plans against the mirrored live file once Sync-LiveReplica.ps1 has copied it' {
        $livePath = Join-Path $liveDir 'cfgeventspawns.xml'
        if (-not (Test-Path -LiteralPath $livePath)) { Set-ItResult -Skipped -Because 'server\live\cfgeventspawns.xml is not mirrored yet (run Sync-LiveReplica.ps1)'; return }
        $patch = Get-PatchDescriptor -PatchPath (Join-Path $overrides 'cfgeventspawns.dpv.patch.xml')
        $plan = @(Get-PatchPlan -PatchDocument $patch.Document -LiveDocument (Read-XmlFile -Path $livePath) -ReferenceDocument (Read-XmlFile -Path $livePath) -Entry 'event')
        ($plan | Where-Object Name -eq 'VehicleNimbyDpv').Mode | Should -Be 'ADD'
        ($plan | Where-Object Name -eq 'VehicleOffroad02').Mode | Should -Be 'REPLACE'
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
