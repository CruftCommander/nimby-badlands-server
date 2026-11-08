# Pester 5 tests for scripts\Send-NimbyLiveAnnouncement.ps1: the message text and the BERCon command.
# No network; the send itself is NimbyRcon.psm1's, tested in NimbyRcon.Tests.ps1.

BeforeAll {
    . (Join-Path $PSScriptRoot 'TestHelpers.ps1')
    $repo = Get-TestRepoRoot
    $script = Join-Path $repo 'scripts\Send-NimbyLiveAnnouncement.ps1'
    . (Get-ScriptFunctionBlock -Path $script -Name 'Get-NimbyAnnouncementText', 'Get-NimbyAnnouncementCommand')
}

Describe 'Get-NimbyAnnouncementText' {
    It 'builds the standard warning for N minutes' {
        Get-NimbyAnnouncementText -Minutes 5 | Should -Match '^Server restart in 5 minutes'
        Get-NimbyAnnouncementText -Minutes 1 | Should -Match '^Server restart in 1 minute '
    }

    It 'keeps a caller message, flattened to one line' {
        Get-NimbyAnnouncementText -Message "Back in`r`nten" -Minutes 5 | Should -Be 'Back in ten'
    }
}

Describe 'Get-NimbyAnnouncementCommand' {
    It 'is a server-wide say' {
        Get-NimbyAnnouncementCommand -Text 'Hello' | Should -Be 'say -1 Hello'
    }
}

Describe 'Safety' {
    It 'never sends a shutdown or restart command' {
        $raw = Get-Content -LiteralPath $script -Raw
        $raw | Should -Not -Match "'#shutdown'"
        $raw | Should -Not -Match "'#restart'"
        $raw | Should -Match 'Read-Host .*-AsSecureString'
    }
}
