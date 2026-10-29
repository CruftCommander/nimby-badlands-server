# Patch 1 session, October 27, 2026 (evening)

Release 1 patch 1 (`main` 38b10b5, PR #1) verified in game before the Workshop update. Two clients, both played by Jeff: **D** is the desktop on his main account, **L** is the G15 laptop on the second Steam account. Test numbers continue after S6-25 of the six-addon session (`docs/testing/2026-10-26-six-addon-session.md`); each row is one test and names the plan row or audit finding it stands for (audit: `claude/reviews/2026-10-27-test-gap-audit-report.md`, TGA-033 to TGA-060). Findings continue at NIMBY-T-063 (NIMBY-T-061, the launcher seeing Workshop id 0, and NIMBY-T-062, live Hands transfers failing on the release-1 build, are recorded in the first-week watch). Locations are named by place, never by map coordinate.

Status: rows written 2026-10-27 15:40 Central, before the session. Result and Evidence are filled during the run.

## Build and pre-session checks

- Artifact `build\@NIMBY-20261027-124130`, built from f9d849c (the same tree as `main` 38b10b5) in the worktree `nimby-badlands-server-wt\fix-release-1-patch-1`. Qualification boot on `storage_998` with `storageAutoFix = 0`: `SUMMARY PASS`, `SELFTEST TOTAL pass=630 fail=0 flags=6 expected=6` (Autorun 112, Hands 156, Earplugs 28, Notes 54, Tack 155, Greet 125), 0 `SCRIPT (E)`, 0 `Scripted variables corrupted`.
- The artifact folder and `logs\build-20261027-124130\` were copied into the main clone (both git-ignored) on 2026-10-27 at 15:30 Central. `Start-NimbyTestServer.ps1` reads the build log from its own repository's `logs` folder, so run from the main clone with the worktree path it would record `BUILD explicit verdict=UNKNOWN` and refuse without `-Force`; run from the worktree it would have no `scripts\testserver.local.json` and fall back to a random join password. The six copied PBOs hash equal to the `ADDON OK` lines of the build log (`CB566645...` Autorun, `F74A4827...` Hands, `8DA4D147...` Earplugs, `7A8679F8...` Notes, `84CDAE4A...` Tack, `DC9B760F...` Greet).
- CI: static-gate run #20 (40546999645, push of 669ff12) failed in step **Install Pester 5.9.1** of the `Pester (powershell)` job, and the job log (read signed in, 17:57 Central) gives the cause: `WARNING: Source Location 'https://www.powershellgallery.com/api/v2/package/Pester/5.9.1' is not valid.` then `Install-Package : Package 'Pester' failed to download.` (`PackageFailedInstallOrDownload`), exit code 1. The suite never ran on that job, so no test case failed: PowerShell Gallery did not serve the package to the Windows PowerShell 5.1 runner. The same job passed on run #22 (38b10b5), on PR #1, and on PR #2 (run #23: 175 passed, 0 failed under PowerShell 5.1.26100, after a 2 min 45 s install), and Pester was 168/168 under `powershell.exe` locally on 669ff12. Because the ruleset requires this check, a repeat blocks a PR until the job is re-run; a retry in the install step would absorb it.

## Setup

1. Replica test kit (replica only; `Sync-LiveReplica.ps1` overwrites `init.c` from the mirror). The replica `types.xml` has `Paper` and every `Pen_*` at nominal 0, so without the kit there is no pen, as on October 26 (S6-08). `init.c.range-candidate-9` (candidate 8 plus a tent, written October 27, 2026) tops up every connecting player with exactly one pen (extra pens are removed), Paper as two singles plus stacks of 5 up to 22 sheets, Hammer, Nail, DuctTape, EpoxyPutty, CombatKnife, Matchbox, Wrench, Screwdriver, Handcuffs and keys, and a TaloonBag_Blue (Rag and CanOpener inside); with the `NimbyTestKit\tent` flag it also drops a packed MediumTent 5 m behind the player unless a tent stands within 50 m (row S6-51). A new character spawns in the Kirov Dam power-plant yard by the dam water. The range (instance 999 only) and the motorbike stay off.

   ```powershell
   Copy-Item -LiteralPath 'D:\DayZServer\mpmissions\dayzOffline.nasdara\init.c' -Destination 'D:\DayZServer\_backups\init.c.stock-20261027'
   Copy-Item -LiteralPath 'D:\DayZServer\_backups\init.c.range-candidate-9' -Destination 'D:\DayZServer\mpmissions\dayzOffline.nasdara\init.c' -Force
   ```

2. Server profile `patch-1` with the six debug flags, the test-kit tent flag, and the release `greet.json`:

   ```powershell
   $serverProfile = 'D:\GitHub\nimby-badlands-server\profiles\patch-1'
   foreach ($prefix in 'NimbyAutorun', 'NimbyHands', 'NimbyEarplugs', 'NimbyNotes', 'NimbyTack', 'NimbyGreet') {
       New-Item -ItemType File -Path (Join-Path $serverProfile "$prefix\debug") -Force | Out-Null
   }
   New-Item -ItemType File -Path (Join-Path $serverProfile 'NimbyTestKit\tent') -Force | Out-Null
   Copy-Item -LiteralPath 'D:\GitHub\nimby-badlands-server\profiles\release-1\NimbyGreet\greet.json' -Destination (Join-Path $serverProfile 'NimbyGreet\greet.json')
   ```

3. Client debug flags, on D and again on L; on D only, move the Greet dismissal file aside so S6-26 starts from a fresh join:

   ```powershell
   foreach ($prefix in 'NimbyAutorun', 'NimbyHands', 'NimbyEarplugs', 'NimbyNotes', 'NimbyTack', 'NimbyGreet') {
       New-Item -ItemType File -Path (Join-Path $env:LOCALAPPDATA "DayZ\$prefix\debug") -Force | Out-Null
   }
   $seen = Join-Path $env:LOCALAPPDATA 'DayZ\NimbyGreet\seen.json'
   if (Test-Path -LiteralPath $seen) { Rename-Item -LiteralPath $seen -NewName 'seen.json.pre-patch-1' }
   ```

4. Copy `D:\GitHub\nimby-badlands-server\build\@NIMBY-20261027-124130` to the laptop, keeping the folder name. The desktop's LAN address for L's `-connect=`: `(Get-NetIPAddress -AddressFamily IPv4 | Where-Object { $_.PrefixOrigin -eq 'Dhcp' }).IPAddress` on the desktop.

5. Server (from the main clone; expect `BUILD explicit verdict=PASS` in the manifest and "Join password: standing value (not shown)"):

   ```powershell
   pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Start-NimbyTestServer.ps1 -ServerPath D:\DayZServer -InstanceId 998 -ProfileName patch-1 -BuildPath D:\GitHub\nimby-badlands-server\build\@NIMBY-20261027-124130
   ```

6. D client (the game asks for the join password on connect; Jeff types it):

   ```powershell
   pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Start-NimbyTestClient.ps1 -BuildPath D:\GitHub\nimby-badlands-server\build\@NIMBY-20261027-124130 -Connect 127.0.0.1
   ```

   L: DayZ_BE.exe on the laptop with `-mod=` its copy of the folder, `-connect=` the desktop address, `-port=2302`.

Evidence sources: the server run folder `logs\testserver-<stamp>\` (`nimby-events.log`, `testserver.log`), `profiles\patch-1\script_*.log`, D's `%LOCALAPPDATA%\DayZ\script_*.log`, and L's client log copied off the laptop after the session.

Order: Greet and Ears on join, Hands, Legs, Scrawl, Tack, the Ears death and rebind rows, the restart row, then the dev-harness segment on its own build.

## Rows on @NIMBY-20261027-124130

| Test | Plan row | Check | Result | Evidence |
| --- | --- | --- | --- | --- |
| S6-26 | Greet G-2 (G-P1-52, TGA-039) | D, fresh join: the window opens by itself; tick Don't show this again, Close | PASS | Run `testserver-20261028-143930`. Client 15:17:57 `open version=1 source=spawn`, 15:18:01 `close reason=player neverShow=true dismissedVersion=1 written=true`; on the 15:18:53 relaunch the client read the file back: `client ready ... neverShow=true dismissedVersion=1`. Server 15:17:40 `serve id=1 ... chunks=5 manual=false` |
| S6-27 | Greet G-2 (TGA-039) | D disconnects and reconnects: no window | PASS | Server 15:18:58 `serve id=2 chunks=0 reason=dismissed version=1 dismissedVersion=1`; client `done id=2 chunkCount=0 reason=dismissed`, `nothing to show id=2`, no `open` line for id=2 |
| S6-28 | Greet G-26 (TGA-039) | D: Insert opens the window; untick, Close; reconnect: the window opens by itself | PASS | Insert: server `serve id=3 ... manual=true`, client `open ... source=hotkey` 15:19:09 (Insert again closed it, `close reason=hotkey written=false`); reopened from the pause menu `serve id=4 manual=true`, `open source=menu`, untick, 15:19:48 `close reason=player neverShow=false dismissedVersion=1 written=true`. Reconnect: client `client ready ... neverShow=false`, server 15:20:43 `serve id=5 ... chunks=5 manual=false`, client 15:20:45 `open ... source=spawn`. `seen.json` after: `dismissedVersion 1, neverShow 0` |
| S6-29 | Ears E15 | D's loaded line on a fresh client launch reads the DeferredInit cache | PASS | Client 15:17:28 and 15:18:53 `[NIMBY][Earplugs] loaded; input=UANimbyEarplugsToggle bound=true launch_sound=1 launch_from=cache debug=true saved_sound=1`. 0 `SCRIPT (E)` in the client log |
| S6-30 | Hands HD-001 (T-010, TGA-035) | D holds the kit wrench and offers (O); L uses Take. 20 repetitions; record the count run | FAIL | Run `testserver-20261028-152811`, 15:54 to 16:01. 12 Take requests (ids 1-7, 9, 10, 12 D to L and L to D; ids 8, 11, 13-15 `REFUSED detail=item in flight`, correct). Every request: `REQUESTED detail=stage=CARGO:TaloonBag_Blue@<giver> dst=CARGO:...@<receiver>`, then no `STAGED` line, then at +5000 ms `UNCONFIRMED detail=no completion signal within 5000 ms; item with giver at HANDS:<giver>`. 0 `CONFIRMED`. Toasts on both sides ("Transfer outcome unknown ... It is still in your hands" / "... in the giver's hands"). Wrench never left the giver's hands. NIMBY-T-063 (the T-010 and T-062 signature) |
| S6-31 | Hands HD-002 (T-010, TGA-035) | L requests with empty hands (O); D uses Give. 20 repetitions | NOT RUN | Blocked by NIMBY-T-063: no transfer completes, so the row cannot be exercised on this build |
| S6-32 | Hands HD-003 (T-010, TGA-035) | L opens pockets; D uses Put In Pockets. 20 repetitions | NOT RUN | Blocked by NIMBY-T-063: no transfer completes, so the row cannot be exercised on this build |
| S6-33 | Hands HD-013 | D stops offering before L's Take executes | NOT RUN | Blocked by NIMBY-T-063: no transfer completes, so the row cannot be exercised on this build |
| S6-34 | Hands H6 cargo only (October 21 ruling) | L empties one worn slot (cap, glasses, gloves or a bag slot); D gives an item that fits that slot | NOT RUN | Blocked by NIMBY-T-063: no transfer completes, so the row cannot be exercised on this build |
| S6-35 | Hands cost path A (H1, TGA-035) | D's cargo full, wrench in hands, D offers | NOT RUN | Blocked by NIMBY-T-063: no transfer completes, so the row cannot be exercised on this build |
| S6-36 | Hands HD-014 receiver drop (TGA-041) | D offers; L starts Take; `Stop-Process -Name DayZ_x64 -Force` on the laptop before the Take completes | NOT RUN | Blocked by NIMBY-T-063: no transfer completes, so the row cannot be exercised on this build |
| S6-37 | Hands giver drop (TGA-041) | L offers the wrench; D starts Take; `Stop-Process -Name DayZ_x64 -Force` on the laptop | NOT RUN | Blocked by NIMBY-T-063: no transfer completes, so the row cannot be exercised on this build |
| S6-38 | Legs logout window (TGA-038, LEGS-R2-01) | L autoruns at sprint on open ground; `Stop-Process -Name DayZ_x64 -Force` on the laptop; D watches L's body for 20 s | | Server `Forced clear player=(...) "<L>" reason=logout via=hook` within 1 s of the drop; D: the body stands; L's next ADM position matches where it stopped |
| S6-39 | Legs logout window, swimming (TGA-038) | As S6-38, L swimming fast in the dam water | | As S6-38; the body does not swim on |
| S6-40 | Legs graceful logout | L autoruns, then Exit from the pause menu while running | | `Forced clear ... reason=logout via=hook` when the logout timer starts, not 15 s later |
| S6-41 | Legs stamina sprint (e6b95e3) | L autoruns at sprint until the stamina bar is empty | | L drops from sprint and keeps running; no `Toggle ... running=false`; no rubber-banding |
| S6-42 | Scrawl SC-001 (S6-08) | D, pen in hands, writes on a kit sheet and reads it back | | Server `write opened sheet=Paper ...`, `write finished; pen reference and writer record cleared`; D reads the text |
| S6-43 | Scrawl second reader (S6-09) | D drops the note; L picks it up and reads it | | L reads D's text (Jeff); L's client log |
| S6-44 | Scrawl noticeboard N18 (TGA-036) | D pins a written note on a map-building wall (tape) and starts a write on it; L tries to start a write on the same note | | L sees the notice "Someone is writing on this sheet."; server `write refused: another write is open sheet=...` once for L; D clicks OK and the note reads D's text |
| S6-45 | Scrawl noticeboard N18, second writer (TGA-036) | After D's OK, L writes on the same note | | L's start is allowed; L's text lands; read shows L's edit; no `write refused` |
| S6-46 | Scrawl write grace N18 (300 s) | L opens the write menu on the note; `Stop-Process -Name DayZ_x64 -Force` on the laptop; D starts a write at once, then again after 300 s | | Record which line ends L's open write: `write record cleared sheet=...` at the drop, or `write expired: open write forgotten after the grace sheet=...`; D refused before that line, allowed after |
| S6-47 | Scrawl pen taken mid-write (TGA-036, N17) | D holds a sheet with a pen at his feet and starts the write; L picks up the pen while D's menu is open; D clicks OK | | D's text lands; no `write refused: sender is not the writer`. If vanilla offers no write with the pen on the ground, NOT REACHABLE |
| S6-48 | Scrawl SC-T1 (S6-11) | Take down a written note pinned with a nail | | `takedown fastener=nail sheet=inventory ...`; the sheet reads its text |
| S6-49 | Scrawl A5 step 1 | Combine three kit sheets into a stack; drop the stack, one blank sheet and one written note in the dam yard | | Stack quantity 3 on pickup; the written note does not combine (`combine refused: a sheet carries text`) |
| S6-50 | Tack T28 door (S6-18) | Pin a tape note on a door leaf; open and close the door | | `pin ... door=` line, no `pin door dropped`; the note follows the leaf |
| S6-51 | Tack T29 player builds | Pitch the test-kit MediumTent (dropped 5 m behind D on connect, server line `[NIMBY][TestKit] MediumTent (packed) -> ground`); with tape and a written sheet, try to pin a note on the pitched tent | | Notice "Notes pin on map buildings" / "Player-built walls and tents are not supported yet."; no `pin` line on the server; tape quantity and the sheet unchanged |
| S6-52 | Ears E15, death-screen rejoin (TGA-045) | L kills D with the kit knife; D picks Main menu on the death screen and rejoins without closing the client; 3 s after spawn, N in, N out | | Client `loaded; ... launch_from=cache`, `launch_sound=1`; the take-out line `target=1` |
| S6-53 | Ears mid-fade put-in (TGA-045) | On the next respawn, N in within 1 s of the character appearing, then N out | | Take-out `target=1`, not a mid-fade value |
| S6-54 | Ears EP-005 rebind (TGA-045) | Controls > NIMBY: rebind Earplugs, Apply; press the new key, then the old one; no relog | | A toggle line for the new key only; 0 `SCRIPT (E)` |
| S6-55 | Ears revert | Controls: revert the binding; press N | | N toggles again |
| S6-56 | Greet hotkey after rebind (UAIDWrapper) | Controls > NIMBY: rebind the welcome-window key, Apply; press the new key | | The window opens (`serve id=<n> manual=true`) |
| S6-57 | Scrawl A5 step 2, Tack TK-003 | Graceful stop (Enter); start the same server command; both rejoin; read the ground note, the pinned notes and the door note; pick up the stack | | Server `written flag synced from=store-load written=true` per written sheet; texts intact; the stack still 3; `door load result=restored`; 0 `SCRIPT (E)`, 0 `Scripted variables corrupted` on both boots |

## Rows on @NIMBY-20261028-164019 (PR #12, October 28, 2026)

Second run, on the fix builds for NIMBY-T-063 and NIMBY-T-065: `-164019` (5aea683), then `-215106` (46444bd, step 2 reads the root through the location's parent and runs 50 ms later) and `-223604` (954d860, step 2 compares type and parent only, one debug line per native). Only `NimbyHands.pbo` differs between the three (`A49C72AA`, `39AECA1B`, `203BBC16`); the other five PBOs are byte-identical, so S6-26 to S6-29 stand for all three. Artifact `build\@NIMBY-20261028-164019` built from 5aea683 (`fix/hands-t063-tack-t065`, PR #12) in the main clone: `SUMMARY PASS`, `SELFTEST TOTAL pass=637 fail=0 flags=6 expected=6` (Hands 163), 0 `SCRIPT (E)`, 0 `Scripted variables corrupted` on the boot check. Every PBO hash differs from `-124130`, the four addons whose sources did not change included (`88FA3DB1` Autorun, `A49C72AA` Hands, `728397F9` Earplugs, `936F9844` Notes, `0AAE4BDE` Tack, `14C648E5` Greet), so PBO output is not byte-stable across builds; this run stands for this artifact only (ruled by Jeff, October 28: the whole record re-runs on the new artifact). Setup as above with the stamp `20261027-124130` replaced by `20261028-164019` in every path; `D:\DayZServer\nimby-server.cmd 20261028-164019` and `nimby-client.cmd 20261028-164019` carry it. New expectation for the Hands rows: `REQUESTED` carries `waited=<ms>`; with the debug flag the server also writes `wait start`, `interrupt player=... action=NimbyHandsAction(Offer|Request)Item` and `wait free waited=<ms>` per id.

| Test | Plan row | Check | Result | Evidence |
| --- | --- | --- | --- | --- |
| S6-26 | Greet G-2 (G-P1-52, TGA-039) | D, fresh join: the window opens by itself; tick Don't show this again, Close | | Client `close reason=player neverShow=true dismissedVersion=1 written=true`; `%LOCALAPPDATA%\DayZ\NimbyGreet\seen.json` holds `"dismissedVersion":1` and `"neverShow":1` |
| S6-27 | Greet G-2 (TGA-039) | D disconnects and reconnects: no window | | Server `serve id=<n> chunks=0 reason=dismissed version=1`; no open line on the client |
| S6-28 | Greet G-26 (TGA-039) | D: Insert opens the window; untick, Close; reconnect: the window opens by itself | | Client `close reason=player neverShow=false ... written=true`; on the reconnect server `serve id=<n> ... manual=false` |
| S6-29 | Ears E15 | D's loaded line on a fresh client launch reads the DeferredInit cache | | Client `loaded; input=UANimbyEarplugsToggle bound=true ... launch_from=cache`, `launch_sound=1` |
| S6-30 | Hands HD-001 (T-010, TGA-035) | D holds the kit wrench and offers (O); L uses Take. 20 repetitions; record the count run | FAIL (server crash) | Three builds, one Take each, D offers, L takes. Step 1 now works on every try: `wait start`, `interrupt ... NimbyHandsActionOfferItem`, `wait free waited=799..802`, `REQUESTED ... waited=`, `STAGED` (wrench in the giver's TaloonBag). Step 2 then crashed the server (access violation, `error_*.log` `ENGINE (F): Crashed`, minidump in `profiles\patch-1`): (1) `-164019` 21:42:17, `CollectStep2Facts` line 453 (the staged-location compare), 0 ms after the signal; (2) `-215106` 22:28:38, the same statement, 49 ms after the signal, with the root read through the location's parent; (3) `-223604` 23:00:42, `RunStep2` line 387, `giver.ServerTakeToDst(src, dst)` itself, after every step-2 fact passed (`at source=true`). No toast either side. Runs `testserver-20261028-212738`, `-221907`, `-223924`. NIMBY-T-063, open |
| S6-31 | Hands HD-002 (T-010, TGA-035) | L requests with empty hands (O); D uses Give. 20 repetitions | | As S6-30 |
| S6-32 | Hands HD-003 (T-010, TGA-035) | L opens pockets; D uses Put In Pockets. 20 repetitions | | As S6-30, plus `pockets player=... open=true reason=...` for L |
| S6-33 | Hands HD-013 | D stops offering before L's Take executes | | One `state=REFUSED` line for the id with a consent detail, a toast on both, the wrench still in D's hands |
| S6-34 | Hands H6 cargo only (October 21 ruling) | L empties one worn slot (cap, glasses, gloves or a bag slot); D gives an item that fits that slot | | `CONFIRMED` with a `CARGO:` or `PROXYCARGO:` detail and the item in L's cargo, never in the slot; if L's cargo has no room, `REFUSED` with the item still with D |
| S6-35 | Hands cost path A (H1, TGA-035) | D's cargo full, wrench in hands, D offers | | L sees no Take prompt (`GiverHasSpace` in the condition); if the prompt shows, `REFUSED detail=no space in giver inventory` and the wrench stays in D's hands |
| S6-36 | Hands HD-014 receiver drop (TGA-041) | D offers; L starts Take; `Stop-Process -Name DayZ_x64 -Force` on the laptop before the Take completes | | Either no `REQUESTED` (the action died with the client) or a terminal `REFUSED` naming the receiver as disconnected or missing; the wrench exists exactly once |
| S6-37 | Hands giver drop (TGA-041) | L offers the wrench; D starts Take; `Stop-Process -Name DayZ_x64 -Force` on the laptop | | As S6-36 with the giver named; the wrench with L's body, once |
| S6-38 | Legs logout window (TGA-038, LEGS-R2-01) | L autoruns at sprint on open ground; `Stop-Process -Name DayZ_x64 -Force` on the laptop; D watches L's body for 20 s | | Server `Forced clear player=(...) "<L>" reason=logout via=hook` within 1 s of the drop; D: the body stands; L's next ADM position matches where it stopped |
| S6-39 | Legs logout window, swimming (TGA-038) | As S6-38, L swimming fast in the dam water | | As S6-38; the body does not swim on |
| S6-40 | Legs graceful logout | L autoruns, then Exit from the pause menu while running | | `Forced clear ... reason=logout via=hook` when the logout timer starts, not 15 s later |
| S6-41 | Legs stamina sprint (e6b95e3) | L autoruns at sprint until the stamina bar is empty | | L drops from sprint and keeps running; no `Toggle ... running=false`; no rubber-banding |
| S6-42 | Scrawl SC-001 (S6-08) | D, pen in hands, writes on a kit sheet and reads it back | | Server `write opened sheet=Paper ...`, `write finished; pen reference and writer record cleared`; D reads the text |
| S6-43 | Scrawl second reader (S6-09) | D drops the note; L picks it up and reads it | | L reads D's text (Jeff); L's client log |
| S6-44 | Scrawl noticeboard N18 (TGA-036) | D pins a written note on a map-building wall (tape) and starts a write on it; L tries to start a write on the same note | | L sees the notice "Someone is writing on this sheet."; server `write refused: another write is open sheet=...` once for L; D clicks OK and the note reads D's text |
| S6-45 | Scrawl noticeboard N18, second writer (TGA-036) | After D's OK, L writes on the same note | | L's start is allowed; L's text lands; read shows L's edit; no `write refused` |
| S6-46 | Scrawl write grace N18 (300 s) | L opens the write menu on the note; `Stop-Process -Name DayZ_x64 -Force` on the laptop; D starts a write at once, then again after 300 s | | Record which line ends L's open write: `write record cleared sheet=...` at the drop, or `write expired: open write forgotten after the grace sheet=...`; D refused before that line, allowed after |
| S6-47 | Scrawl pen taken mid-write (TGA-036, N17) | D holds a sheet with a pen at his feet and starts the write; L picks up the pen while D's menu is open; D clicks OK | | D's text lands; no `write refused: sender is not the writer`. If vanilla offers no write with the pen on the ground, NOT REACHABLE |
| S6-48 | Scrawl SC-T1 (S6-11) | Take down a written note pinned with a nail | | `takedown fastener=nail sheet=inventory ...`; the sheet reads its text |
| S6-49 | Scrawl A5 step 1 | Combine three kit sheets into a stack; drop the stack, one blank sheet and one written note in the dam yard | | Stack quantity 3 on pickup; the written note does not combine (`combine refused: a sheet carries text`) |
| S6-50 | Tack T28 door (S6-18) | Pin a tape note on a door leaf; open and close the door | | `pin ... door=` line, no `pin door dropped`; the note follows the leaf |
| S6-51 | Tack T29 player builds | Pitch the test-kit MediumTent (dropped 5 m behind D on connect, server line `[NIMBY][TestKit] MediumTent (packed) -> ground`); with tape and a written sheet, try to pin a note on the pitched tent | | Notice "Notes pin on map buildings" / "Player-built walls and tents are not supported yet."; no `pin` line on the server; tape quantity and the sheet unchanged |
| S6-52 | Ears E15, death-screen rejoin (TGA-045) | L kills D with the kit knife; D picks Main menu on the death screen and rejoins without closing the client; 3 s after spawn, N in, N out | | Client `loaded; ... launch_from=cache`, `launch_sound=1`; the take-out line `target=1` |
| S6-53 | Ears mid-fade put-in (TGA-045) | On the next respawn, N in within 1 s of the character appearing, then N out | | Take-out `target=1`, not a mid-fade value |
| S6-54 | Ears EP-005 rebind (TGA-045) | Controls > NIMBY: rebind Earplugs, Apply; press the new key, then the old one; no relog | | A toggle line for the new key only; 0 `SCRIPT (E)` |
| S6-55 | Ears revert | Controls: revert the binding; press N | | N toggles again |
| S6-56 | Greet hotkey after rebind (UAIDWrapper) | Controls > NIMBY: rebind the welcome-window key, Apply; press the new key | | The window opens (`serve id=<n> manual=true`) |
| S6-57 | Scrawl A5 step 2, Tack TK-003 | Graceful stop (Enter); start the same server command; both rejoin; read the ground note, the pinned notes and the door note; pick up the stack | | Server `written flag synced from=store-load written=true` per written sheet; texts intact; the stack still 3; `door load result=restored`; 0 `SCRIPT (E)`, 0 `Scripted variables corrupted` on both boots |

## Dev-harness segment (TGA-034)

The harness covers Legs and Hands only (Scrawl, Tack and Greet forged rows do not exist yet). It loads through `-mod=`, so every client on this server loads it beside the pack, and `Start-NimbyTestClient.ps1` takes one folder, so the client starts by hand.

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Build-NimbyPack.ps1 -ServerPath D:\DayZServer -BootInstanceId 998 -Dev -Qualification
$repo = 'D:\GitHub\nimby-badlands-server'
$devBuild = Get-ChildItem -LiteralPath (Join-Path $repo 'build') -Directory -Filter '@NIMBY-Dev-*' | Sort-Object LastWriteTime | Select-Object -Last 1
$packBuild = Join-Path $repo ('build\@NIMBY-' + ($devBuild.Name -replace '^@NIMBY-Dev-', ''))
$forge = Join-Path $repo 'profiles\patch-1\NimbyDevHarness\forge'
New-Item -ItemType Directory -Path (Split-Path -Parent $forge) -Force | Out-Null
Set-Content -LiteralPath $forge -Value 'online' -Encoding ascii
pwsh -NoProfile -ExecutionPolicy Bypass -File (Join-Path $repo 'scripts\Start-NimbyTestServer.ps1') -ServerPath D:\DayZServer -InstanceId 998 -ProfileName patch-1 -BuildPath $packBuild
```

D client, in a second window after the server is up (same `$devBuild` and `$packBuild` lines first):

```powershell
& 'F:\SteamLibrary\steamapps\common\DayZ\DayZ_BE.exe' "-mod=$packBuild;$($devBuild.FullName)" '-connect=127.0.0.1' '-port=2302'
```

| Test | Plan row | Check | Result | Evidence |
| --- | --- | --- | --- | --- |
| S6-58 | TGA-034 boot cases, H7 | `-Dev -Qualification` build of `main` | | `SUMMARY PASS`; `SELFTEST DevHarness ran=yes cases=<n> fail=0`; the six client `ADDON OK` sha256 values equal the `-164019` ones above (client output byte-identical with `-Dev` in the same tree) |
| S6-59 | TGA-034 online cases | Forge file `online`; D joins and waits 10 s in the world | | `[NIMBY][DevHarness]` PASS for `legs-unreadable-payload`, `legs-idx-9-angle-0`, `legs-flood-50-in-one-second`, `legs-request-aimed-at-another-player-from-identity`, `hands-stale-item-replaced`, `hands-stale-receiver-replaced`; the complete line `failures=0`; Legs `Rejected request ... sender does not own this player` once for the sender |
| S6-60 | Hands cost path B (H1, H7) | Add the line `step2-refuse` to the forge file, restart the server; L also loads the dev folder; D offers the wrench, L uses Take | | `armed one forced Hands step-2 refusal`; `state=STAGED`, then `state=REFUSED ... detail=no space in receiver inventory (forced by dev harness)`; the wrench in D's cargo; a toast on both |

## Not in tonight's plan

- Receiver killed between step 1 and step 2, and the corpse-delivery ruling Jeff owes (TGA-041): needs a third player.
- Pockets open, relog inside the logout window (TGA-041, HD-022): the reconnect takes longer than the 15 s default `logoutTime`.
- HD-032 two takers with a third client; Scrawl, Tack and Greet forged rows (TGA-034); the content rollback boot with knifed, door, stack and leaflet content (TGA-037); the Tack carry-over rows S6-12, S6-14, S6-16, S6-19.

## Findings

- **NIMBY-T-063 (Hands, release blocker).** On `@NIMBY-20261027-124130` no player-to-player transfer completes: step 1 (hands to the giver's own cargo) never lands, every request ends `UNCONFIRMED` after 5000 ms with the item still at `HANDS:` (S6-30, 12 of 12). Same signature as NIMBY-T-010 (October 20) and NIMBY-T-062 (live, October 27). Cause, read from the tree: the production P3 (`moveitemtoplayer.c`, built October 21) dispatches step 1 with `giver.ServerTakeToDst(src, stage)` immediately, from inside the Take, Give or Put In Pockets action, while the giver is still in the offer or request action command. The October 20 diagnostic's V3, the only P3 that ever `CONFIRMED` in game, held the request `PENDING` until `NimbyHandsDiag.IsFree` found both players free (no running action; for the holder also no action command and an idle hand inventory), then dispatched step 1 (`diag/hands-t010`, `nimbyhands_diag.c:485`, `moveitemtoplayer.c:147-156`). Production dropped that wait; the feasibility study's H-B (the giver's hand state machine cannot complete the move while the giver is in an action, `docs/design/2026-10-20-hands-t010-feasibility.md` section 4) predicts exactly this. `NimbyHands.pbo` was not changed by PR #1 (release-1 `5C404D82...`, patch-1 `F74A4827...` differ only through the shared build inputs), so live runs the same step 1. Fix shape: port the V3 wait into production step 1 (defer dispatch until the giver is free, bounded, `REFUSED` on the cap), then S6-30 to S6-37 on the new build. **October 28 evening, three fix builds:** the wait is in (PR #12) and step 1 lands every time (`STAGED`, item in the giver's cargo); step 2 crashes the server, in three different natives on three builds (S6-30 in the second table). The diagnostic's step 2 made the same `giver.ServerTakeToDst(src, dst)` call on game build 1.30.164078 and confirmed; `D:\DayZServer\DayZServer_x64.exe` was replaced on October 22 and every boot since reports 1.30.164200. Open: whether the hotfix changed the server-side cargo-to-cargo move between players, or a state the hand-event step leaves behind breaks the natives that follow. Next: the dev-harness bot-to-bot case (Jeff, October 28) so each hypothesis is one boot check, and step 2 split into `LocationSyncMoveEntity` and `SendServerMove` with a debug line between.
- **NIMBY-T-064 (Hands, cosmetic).** After the giver presses O again to stop offering, the giver's own view lowers the arm but other players keep seeing the offer stance (Jeff, both directions). The remote view of the offer state is not released. Not yet read from the logs.
- **NIMBY-T-065 (Tack, noise).** With a hammer in hands and a nail and a single sheet in inventory, the T29 box "Notes pin on map buildings / Player-built walls and tents are not supported yet." shows whenever the cursor rests on another player or on a ground item, not only on a player build: `NimbyTackWorld.ProbeWall` returns `target-not-building` for `wall.IsMan() || wall.IsInherited(ItemBase)` (`nimbytack_world.c:166-169`) and the action shows the box for that reason unconditionally (`nimbytack_actions.c:465-468`). Client pin-check lines in `client-D-script.log`: `reason=target-not-building detail=SurvivorF_Baty` (twice) and `detail=Matchbox`. Fix shape: show the box only for player-build classes (base building, tents), stay silent for Man and loose items.

## Exit

Ruled by Jeff, October 27, 2026, 21:11 Central: the Workshop update with patch 1 goes ahead only when **every row in this record passes**, S6-26 to S6-60 including the dev-harness segment, and every boot shows 0 `SCRIPT (E)` and 0 `Scripted variables corrupted`. A row that proves unreachable in vanilla play (S6-47 may be: vanilla may offer no write with the pen on the ground) is recorded with the evidence and goes back to Jeff, who rules then whether it blocks. Then Workshop procedure steps 6 to 9 (NIMBY-T-061) and gate 9.2 (storage and `$profile:Nimby*` backup first) at a scheduled restart. The Workshop update is published from a build made after `fix/workshop-meta` merges, so the item carries `meta.cpp` (NIMBY-T-061); the PBOs are byte-identical to `-124130`, so these rows stand for it.
