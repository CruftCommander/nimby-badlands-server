#Requires -Version 5.1

<#
.SYNOPSIS
    Sends an in-game announcement (and optionally #lock) to the live NIMBY server over BattlEye RCON,
    for the warning every unscheduled restart owes players five minutes ahead.

.DESCRIPTION
    Jeff's rule (November 8, 2026): before any forced restart (a release restart, a panel restart
    outside the 12-hour chain), players get an RCON message five minutes in advance. The scheduled
    restarts already carry their 15, 5 and 1 minute warnings as panel RCON tasks
    (docs/operations/restart-schedule.md); this script is for the restarts that are not on the schedule.

    Sends "say -1 <Message>" with Send-NimbyRconCommand (NimbyRcon.psm1), then "#lock" when -Lock is
    given, so nobody new joins during the countdown. The RCON password is typed at the prompt (Jeff
    types RCON passwords; it is never read from a file, an environment variable, or an argument).

    Reads   : <repo>\scripts\testserver.local.json (git-ignored) for "sftpHost" (the server address)
              and "rconPort" (the live BattlEye RConPort; add it once). -Address and -Port override.
    Writes  : <repo>\logs\announce-<stamp>.log: address, port, the command text, the server's reply.
    Never   : writes or prints the RCON password; sends #shutdown, #restart, or any command other than
              say and #lock (the restart itself stays in the HostHavoc panel, by hand).

.PARAMETER Message
    The text players see. Keep it to one line; BattlEye truncates long messages.
.PARAMETER Lock
    Also send #lock after the message (no new joins until the server restarts or #unlock).
.PARAMETER Minutes
    Convenience: with no -Message, sends the standard text for a restart in N minutes (default 5).

.EXAMPLE
    # Five-minute warning plus lock, the standard release-restart line
    .\scripts\Send-NimbyLiveAnnouncement.ps1 -Minutes 5 -Lock

.EXAMPLE
    .\scripts\Send-NimbyLiveAnnouncement.ps1 -Message "Server restart in 1 minute. Log out now."
#>
[CmdletBinding()]
param(
    [string]$Message,
    [int]$Minutes = 5,
    [switch]$Lock,
    [string]$Address,
    [int]$Port,
    [int]$TimeoutSeconds = 5,
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }
$ErrorActionPreference = 'Stop'

Import-Module (Join-Path $PSScriptRoot 'modules\NimbyCommon.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'modules\NimbyRcon.psm1') -Force

function Get-NimbyAnnouncementText {
    # The standard restart warning for N minutes, or the caller's own text, trimmed to one line.
    param([string]$Message, [int]$Minutes)
    if ($Message) { return (($Message -replace '[\r\n]+', ' ').Trim()) }
    $unit = if ($Minutes -eq 1) { 'minute' } else { 'minutes' }
    return ("Server restart in {0} {1} for an update. Find a safe spot and log out before then; the server is locked to new joins." -f $Minutes, $unit)
}

function Get-NimbyAnnouncementCommand {
    # The BERCon command for a server-wide message: say -1 <text>.
    param([Parameter(Mandatory)][string]$Text)
    return ("say -1 {0}" -f $Text)
}

# ---------------------------------------------------------------- main
if ($MyInvocation.InvocationName -eq '.') { return }   # dot-sourced by tests: functions only

if (-not $Address) { $Address = [string](Get-NimbyLocalSetting -RepoRoot $RepoRoot -Name 'sftpHost') }
if (-not $Port) {
    $settingPort = Get-NimbyLocalSetting -RepoRoot $RepoRoot -Name 'rconPort'
    if ($null -ne $settingPort) { $Port = [int]$settingPort }
}
$missing = @()
if (-not $Address) { $missing += '-Address (sftpHost)' }
if (-not $Port) { $missing += '-Port (rconPort: the live BattlEye RConPort, from the panel)' }
if ($missing.Count -gt 0) { throw ("Missing {0}. Pass them, or set them in {1}." -f ($missing -join ', '), (Get-NimbyLocalSettingPath -RepoRoot $RepoRoot)) }

$text = Get-NimbyAnnouncementText -Message $Message -Minutes $Minutes
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$logDir = Join-Path $RepoRoot 'logs'
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
$log = Join-Path $logDir ("announce-{0}.log" -f $stamp)
Write-Manifest -LogPath $log -Line ("Send-NimbyLiveAnnouncement {0} address={1} port={2} lock={3}" -f $stamp, $Address, $Port, [bool]$Lock)

$secure = Read-Host -Prompt "RCON password for ${Address}:$Port (typed, never stored)" -AsSecureString
$plain = [System.Runtime.InteropServices.Marshal]::PtrToStringAuto([System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($secure))
if (-not $plain) { throw 'No password typed.' }

$fail = 0
try {
    $commands = @((Get-NimbyAnnouncementCommand -Text $text))
    if ($Lock) { $commands += '#lock' }
    foreach ($command in $commands) {
        $result = Send-NimbyRconCommand -Address $Address -Port $Port -Password $plain -Command $command -TimeoutSeconds $TimeoutSeconds
        $verdict = if ($result.Sent) { 'SENT' } else { 'FAIL' }
        if (-not $result.Sent) { $fail++ }
        $line = ("{0} {1} loggedIn={2} reply={3} {4}" -f $verdict, $command, $result.LoggedIn, ($result.Response -replace '[\r\n]+', ' '), $result.Reason)
        Write-Manifest -LogPath $log -Line $line
        Write-Information $line -InformationAction Continue
    }
}
finally {
    $plain = $null
}
$summary = "SUMMARY {0} commands={1} failed={2} log={3}" -f $(if ($fail -gt 0) { 'FAIL' } else { 'PASS' }), $commands.Count, $fail, $log
Write-Manifest -LogPath $log -Line $summary
Write-Information $summary -InformationAction Continue
if ($fail -gt 0) { exit 1 }
exit 0
