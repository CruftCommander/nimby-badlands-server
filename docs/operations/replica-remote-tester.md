# Replica test server for a remote tester

Version 1, October 18, 2026. Owner: Jeff Fabish. Applies to the local replica at `D:\DayZServer` started by `scripts\Start-NimbyTestServer.ps1`. The live HostHavoc server is out of scope and is never touched by this procedure.

## 1. What this is

One trusted tester outside the LAN joins the local replica over a Tailscale tunnel to help with in-game test blocks. No public port is opened: the tester reaches the desktop through the tailnet, Windows Firewall admits only the DayZ game and Steam query ports from Tailscale addresses, and RCON stays bound to loopback.

Non-goals: a public or semi-public test server, more than a handful of testers, anything on the live server, and persistence beyond `storage_998` and `storage_999`.

## 2. What the test server already does

`Start-NimbyTestServer.ps1` writes a throwaway `serverDZ.test.cfg` with a random 12-character join password, `passwordAdmin = ""`, `BattlEye = 1`, `verifySignatures = 2`, `forceSameBuild = 1`, `maxPlayers` from `pack.json` (4), and a throwaway `BEServer_x64.cfg` with a random RCON password on game port plus 3 (2305) bound to `RConIP 127.0.0.1`. It never opens a firewall port and redacts both passwords when the server stops. Nothing in this runbook changes those files.

## 3. Ports

| Port | Protocol | Purpose | Reachable from |
| --- | --- | --- | --- |
| 2302 | UDP | Game port (`pack.json` `testServer.port`) | Tailscale addresses only (`100.64.0.0/10`) |
| 27016 | UDP | Steam query port (DayZ default when `steamQueryPort` is unset; the launcher reads mods and the password gate from it) | Tailscale addresses only |
| 2305 | UDP | BattlEye RCON | Loopback only. `RConIP 127.0.0.1` in the generated config, plus a firewall block rule as a second fence. Windows Firewall does not filter loopback, so the script's own `#shutdown` still works |

## 4. One-time setup

Steps 1 and 2 are the tester's; the rest run on the desktop. Run the firewall steps in PowerShell 7 started as Administrator.

1. Tester: install Tailscale (https://tailscale.com/download) and sign in with any supported identity provider. A free personal account is enough.
2. Tester: install DayZ (Steam, stable branch). The build must match the server (`forceSameBuild = 1`), so no experimental branch.
3. Desktop: install Tailscale and sign in. Note the desktop's tailnet address: `tailscale ip -4`.
4. Tailscale admin console (https://login.tailscale.com/admin/machines): on the desktop's row, choose **Share**, create the invite, and send the link to the tester. The tester accepts it in their own account. Sharing exposes one machine, not the tailnet; the firewall rules below decide which ports answer.
5. Desktop, firewall allow rule for the two game ports from Tailscale addresses, bound to the server executable:

   ```powershell
   New-NetFirewallRule -DisplayName 'NIMBY replica - DayZ game and query from Tailscale' -Group 'NIMBY replica' -Direction Inbound -Action Allow -Protocol UDP -LocalPort 2302, 27016 -RemoteAddress 100.64.0.0/10 -Program 'D:\DayZServer\DayZServer_x64.exe' -Profile Any
   ```

6. Desktop, firewall block rule for RCON from any network address (block rules win over allow rules; loopback is unaffected):

   ```powershell
   New-NetFirewallRule -DisplayName 'NIMBY replica - block RCON from the network' -Group 'NIMBY replica' -Direction Inbound -Action Block -Protocol UDP -LocalPort 2305 -RemoteAddress Any -Profile Any
   ```

7. Desktop: confirm the two rules exist and no other inbound rule admits `DayZServer_x64.exe` on the public profile:

   ```powershell
   Get-NetFirewallRule -Group 'NIMBY replica' | Format-Table DisplayName, Enabled, Action, Profile
   Get-NetFirewallApplicationFilter -Program 'D:\DayZServer\DayZServer_x64.exe' | Get-NetFirewallRule | Format-Table DisplayName, Enabled, Direction, Action, Profile
   ```

   If an earlier Windows prompt created a broad allow rule for `DayZServer_x64.exe` on the Public profile, disable it: a program rule with no port or address scope admits RCON too.

## 5. Each test session

1. Desktop: start the test server as usual, for example:

   ```powershell
   pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Start-NimbyTestServer.ps1 -RepoRoot D:\GitHub\nimby-badlands-server -ServerPath 'D:\DayZServer' -BuildPath D:\GitHub\nimby-badlands-server\build\@NIMBY-<stamp> -ProfileName <name> -InstanceId 999
   ```

   The console prints the join password once. Leave that window open; Enter stops the server.
2. Desktop: confirm RCON is on loopback only while the server runs:

   ```powershell
   Get-NetUDPEndpoint -LocalPort 2305 | Format-Table LocalAddress, LocalPort, OwningProcess
   ```

   Expected: one row with `LocalAddress` `127.0.0.1`. A `0.0.0.0` row means the generated `BEServer_x64.cfg` lost its `RConIP` line; stop the server and fix that before the tester connects.
3. Desktop: zip the exact build folder the server loaded (`build\@NIMBY-<stamp>`, about 170 KB) and send it to the tester with the tailnet address, the port (2302), and the join password over a direct channel. `verifySignatures = 2` rejects any other build of the pack.
4. Tester: unzip so that `@NIMBY` sits in the DayZ install folder (next to `DayZ_BE.exe`), then start the game with the mod loaded. Either the DayZ Launcher, **Mods**, **Local mod**, pointing at that folder, or a shortcut: `DayZ_BE.exe -mod=@NIMBY`.
5. Tester: in the launcher, **Servers**, **Direct connect**: address `100.x.y.z`, port `2302`, then the join password. If the launcher reports no server at 2302, retry with port `27016` (some launcher versions query the Steam port).
6. Desktop: the first join shows in `logs\testserver-<stamp>\testserver.log` and the `.ADM` log under the profile; every `[NIMBY]` line of the session lands in `nimby-events.log` when the server stops.

## 6. Teardown

1. Desktop: press Enter in the test-server window. The script sends `#shutdown` over loopback RCON, waits, and redacts both passwords.
2. Desktop, optional between sessions: `Disable-NetFirewallRule -Group 'NIMBY replica'`. Re-enable with `Enable-NetFirewallRule -Group 'NIMBY replica'`.
3. When the tester's involvement ends: remove the machine share in the Tailscale admin console and `Remove-NetFirewallRule -Group 'NIMBY replica'`.

## 7. What can go wrong

| Symptom | Cause | Action |
| --- | --- | --- |
| Launcher finds no server | Query port not reached, or the tester is not yet in the share | `tailscale ping 100.x.y.z` from the tester; check the share is accepted; retry with port 27016 |
| "Server requires mods" or kicked on join | Different `@NIMBY` build than the server loaded | Send the zip of the exact `build\@NIMBY-<stamp>` folder from the running session |
| Kicked with a BattlEye or signature message | Client PBO signature not in the server's `keys\` | The script installs `nimby.bikey`; confirm `D:\DayZServer\keys\nimby.bikey` exists and matches `build\@NIMBY-<stamp>\keys\nimby.bikey` |
| Version mismatch | `forceSameBuild = 1` and the tester runs experimental or an older client | Both on the same stable build |
| RCON row shows `0.0.0.0` | `RConIP` missing from the generated config | Stop the server; see section 5 step 2 |
