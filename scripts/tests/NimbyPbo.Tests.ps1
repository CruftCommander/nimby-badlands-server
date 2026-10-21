# Pester 5 tests for scripts\modules\NimbyPbo.psm1: the hand-written PBO packer, its header
# reader, the trailing SHA1 checksum, include patterns, and reproducible output.

BeforeAll {
    . (Join-Path $PSScriptRoot 'TestHelpers.ps1')
    $repo = Get-TestRepoRoot
    Import-Module (Join-Path $repo 'scripts\modules\NimbyPbo.psm1') -Force -DisableNameChecking
    $patterns = @('*.c', '*.cpp', '*.xml', 'NOTICE')

    $source = Join-Path $TestDrive 'Addon'
    New-Item -ItemType Directory -Path (Join-Path $source 'Scripts\4_World'), (Join-Path $source 'Data') -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $source 'config.cpp') -Value "class CfgPatches {};`r`n" -Encoding ascii -NoNewline
    Set-Content -LiteralPath (Join-Path $source 'Scripts\4_World\b.c') -Value "// b`r`n" -Encoding ascii -NoNewline
    Set-Content -LiteralPath (Join-Path $source 'Data\Inputs.xml') -Value '<x/>' -Encoding ascii -NoNewline
    Set-Content -LiteralPath (Join-Path $source 'NOTICE') -Value 'notice' -Encoding ascii -NoNewline
    Set-Content -LiteralPath (Join-Path $source 'notes.md') -Value 'not packed' -Encoding ascii -NoNewline
}

Describe 'Build-PboArchive and Get-PboContent' {
    It 'packs only the include-pattern files, sorted, with the prefix, and reads them back' {
        $pbo = Join-Path $TestDrive 'roundtrip.pbo'
        $written = @(Build-PboArchive -SourceFolder $source -Destination $pbo -Prefix 'NimbyTest' -Patterns $patterns)
        $written | Should -Be @('config.cpp', 'Data\Inputs.xml', 'NOTICE', 'Scripts\4_World\b.c')
        $content = Get-PboContent -Path $pbo
        $content.Prefix | Should -Be 'NimbyTest'
        $content.Entries | Should -Be $written
        Test-PboChecksum -Path $pbo | Should -BeTrue
    }

    It 'stores file bytes unchanged (CRLF stays CRLF)' {
        $pbo = Join-Path $TestDrive 'bytes.pbo'
        [void](Build-PboArchive -SourceFolder $source -Destination $pbo -Prefix 'NimbyTest' -Patterns @('*.cpp'))
        $bytes = [System.IO.File]::ReadAllBytes($pbo)
        $text = [System.Text.Encoding]::ASCII.GetString($bytes)
        $text | Should -Match ([regex]::Escape("class CfgPatches {};`r`n"))
    }

    It 'fails the checksum when one byte changes' {
        $pbo = Join-Path $TestDrive 'flip.pbo'
        [void](Build-PboArchive -SourceFolder $source -Destination $pbo -Prefix 'NimbyTest' -Patterns $patterns)
        $bytes = [System.IO.File]::ReadAllBytes($pbo)
        $bytes[40] = $bytes[40] -bxor 0x01
        [System.IO.File]::WriteAllBytes($pbo, $bytes)
        Test-PboChecksum -Path $pbo | Should -BeFalse
    }

    It 'gives the same bytes for unchanged sources, and different bytes for a fixed -Timestamp' {
        $a = Join-Path $TestDrive 'same-a.pbo'
        $b = Join-Path $TestDrive 'same-b.pbo'
        $c = Join-Path $TestDrive 'same-c.pbo'
        [void](Build-PboArchive -SourceFolder $source -Destination $a -Prefix 'NimbyTest' -Patterns $patterns)
        [void](Build-PboArchive -SourceFolder $source -Destination $b -Prefix 'NimbyTest' -Patterns $patterns)
        [void](Build-PboArchive -SourceFolder $source -Destination $c -Prefix 'NimbyTest' -Patterns $patterns -Timestamp ([datetime]'2001-01-01'))
        (Get-FileHash -LiteralPath $a).Hash | Should -Be (Get-FileHash -LiteralPath $b).Hash
        (Get-FileHash -LiteralPath $a).Hash | Should -Not -Be (Get-FileHash -LiteralPath $c).Hash
    }

    It 'refuses to overwrite an existing destination' {
        $pbo = Join-Path $TestDrive 'exists.pbo'
        Set-Content -LiteralPath $pbo -Value 'x' -Encoding ascii
        { Build-PboArchive -SourceFolder $source -Destination $pbo -Prefix 'NimbyTest' -Patterns $patterns } | Should -Throw -ExpectedMessage '*already exists*'
    }

    It 'refuses a folder with no matching file' {
        { Build-PboArchive -SourceFolder $source -Destination (Join-Path $TestDrive 'none.pbo') -Prefix 'NimbyTest' -Patterns @('*.nothing') } | Should -Throw -ExpectedMessage '*match the include patterns*'
    }
}

Describe 'Compare-PboToSource and Get-PboSourceFile' {
    It 'reports a source file missing from the PBO' {
        Compare-PboToSource -SourceRelativePaths @('config.cpp', 'Scripts\4_World\b.c') -PboEntries @('config.cpp') | Should -Be @('Scripts\4_World\b.c')
    }

    It 'accepts config.bin for config.cpp, and forward slashes for backslashes' {
        @(Compare-PboToSource -SourceRelativePaths @('config.cpp', 'Scripts\4_World\b.c') -PboEntries @('config.bin', 'Scripts/4_World/b.c')).Count | Should -Be 0
    }

    It 'lists every include-pattern file, including a *.layout the pattern list names' {
        Set-Content -LiteralPath (Join-Path $source 'Data\menu.layout') -Value 'l' -Encoding ascii
        Get-PboSourceFile -Folder $source -Patterns ($patterns + '*.layout') | Should -Contain 'Data\menu.layout'
        Get-PboSourceFile -Folder $source -Patterns $patterns | Should -Not -Contain 'Data\menu.layout'
        Remove-Item -LiteralPath (Join-Path $source 'Data\menu.layout')
    }
}
