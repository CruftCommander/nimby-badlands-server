# Release 1 first-week watch

Started October 27, 2026. Covers the week after release 1 went live (October 26, 2026,
23:57:31 Central; see `docs/operations/2026-10-20-release-gate-legs-ears-scrawl.md`, step 9
and the post-release restart check).

## 1. Scope

After each 12-hour restart (the panel's 12:00 AM and 12:00 PM Central tasks; the panel clock
is 2 hours behind Central), pull the live profile folder with
`scripts\Get-ServerLogs.ps1` (Jeff runs it; it prompts for the panel password) and read every
boot since the previous pull for:

| Check | Where | Pattern | Clean means |
| --- | --- | --- | --- |
| Script errors | `script_*.log`, `*.RPT` | `SCRIPT (E)` | 0 |
| Hands outcomes | `script_*.log` | `[NIMBY][Hands] ... state=REQUESTED`, `state=CONFIRMED`, `state=REFUSED`, `state=UNCONFIRMED`, counted per boot | Every `REQUESTED` id has a `CONFIRMED`; `REFUSED` explained by its `detail=`; `UNCONFIRMED` 0 (the release-1 code on live ends every transfer `UNCONFIRMED` after 5,000 ms, NIMBY-T-010, so any count above 0 is a player trying to trade on the unpatched build) |
| Legs forged or runaway requests | `script_*.log` | `[NIMBY][Autorun] Rejected request`, `Throttled`, `Forced clear ... via=tick` | 0 / 0 / 0; a `sender does not own this player` line is a forged request and is escalated the same day |
| Greet text problems | `script_*.log` | `[NIMBY][Greet] ... invalid` | 0; each connect shows one `serve ... manual=false` |
| Greet refusals | `script_*.log` | `[NIMBY][Greet] ... refused reason=` counted by reason | 0 `sender-not-owner`; a few `cooldown` per player at most (the patch-1 build gates repeats per sender and prints `suppressed=N`) |
| Tack note persistence | `script_*.log` | `[NIMBY][Tack] door load result=` | `restored` or `found` for every note pinned before the restart; no `no-building` or `invalid-cleared` |
| Storage | `*.RPT` | `Scripted variables corrupted`, failed storage items | 0 / 0 |
| Badlands noise baseline | `*.RPT` | `[CE][LoadPrototype]` 7 lines, spawn-point 6 lines | Same counts as every boot since October 26 |

A pull is read-only: nothing on the server changes. Each pull gets its own
`logs\server-<stamp>\` folder (git-ignored).

Hands NIMBY-T-010 (two-client transfer never run in a test) and the Scrawl live write are
the open items this watch exists to catch. A first live report of a failed hand-over goes to
T-010; the first Scrawl note written on live and read after a restart closes the Scrawl
live check.

## 2. Pulls

| # | Pull (Central) | Boots covered (panel clock) | `SCRIPT (E)` | Hands REQUESTED / CONFIRMED / REFUSED / UNCONFIRMED | Legs rejected / throttled / tick | Greet invalid / serves / refused | Tack door load | Storage | Players | Verdict |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | October 27, 2026, 10:28 (`logs\server-20261027-102800\`, 223 files, 223 OK) | 21:57:31, 21:59:36, 22:01:18, 22:15:40 (the 22:15:40 boot still running, 10 h up; the noon restart had not fired) | 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 (no transfer attempted; re-read October 27 with the four-state pattern) | 0 / 0 / 0 (re-read October 27) | 0 / 0, 0, 2, 3 / 0 (re-read October 27) | 22:15:40: 1 `restored` (`NimbyTack_PinnedNote_Tape`, door 3, `Land_Workshop5`); no new pins | 0 / 0 | One connection since 23:57: `Road Dog` 22:31:01 to about 22:33 (panel), 598 of 600 one-minute samples `Players: 0` | Clean, idle. Hands, Scrawl and the loot changes have had no exposure |

### Pull 1 notes, October 27, 2026

- Overnight boot 22:15:40 (panel): `Used memory` 3,583,548 KB at 22:17, 4,126,548 KB at
  23:56, 4,134,153 KB at 8:16; flat after the first 100 minutes. `Average server FPS`
  about 3,600 idle (the 0.05 sample at 22:17:18 is the first measurement after
  `Player connect enabled`, on every boot).
- Greet serves on the overnight boot: `serve ... manual=false` 22:31:03 (on connect),
  `manual=true` 22:31:48 and 22:32:34 (reopened), all `asked=english served=english
  version=1 chunks=5 bytes=4778`.
- `error_*.log` for the four release boots is `ENTITY (W)` lines on `dz\structures_takistan\`
  and `dz\structures_bliss\` models (`config class missing`, `Unknown object class`) plus
  the `ENTITY (E)` `LargeTentClutterCutter`, `ManType` and `Inventory_BaseType` inheritance
  lines; every `(E)` text also appears in the pre-release boot `error_2026-10-26_10-01-25.log`.
  Badlands engine noise, not NIMBY.
- Checked and dismissed: `ANIMATION (E): Can't load @NIMBY/Anims/cfg/skeletons.anim.xml`
  at 22:15:52 on every release boot. The engine probes every loaded mod and DLC directory
  for that file; the same boot logs the same line for `nasdara/` and `sakhal/`, and
  `docs/baselines/2026-10-14-vanilla-server-logs.md` records the `sakhal` line on the
  vanilla server. `@NIMBY` is one more loaded directory. No addon references the file.
- Newest crash file in the profile folder is `crash_2026-10-24_10-01-26.log`, before
  release 1.

## 3. Findings

### NIMBY-T-061 (high): the launcher cannot download the pack; the server reports Workshop id 0

Closed October 27, 2026, 19:02 Central. Full write-up with the prevention steps:
`docs/operations/2026-10-27-incident-launcher-workshop-id.md`.

Reported by Jeff, October 27, 2026, 17:09 Central, with a launcher screenshot: players joining
the live server get "Content required by server: NIMBY" with only Ignore, Find on Steam Workshop
and Find on the web, instead of the automatic download. No new player can join.

- Cause: the DZSA launcher's query of the live server (`104.192.227.46`, query port 2303) returns
  `The mod 'NIMBY' has a Steam Workshop ID of '0'`. The server reads a mod's Workshop id from
  `meta.cpp` in the mod folder (`publishedid`) and reports it to the launcher. The release-1
  artifact `build\@NIMBY-20261026-214828` holds `addons\`, `keys\` and `mod.cpp` only;
  `Build-NimbyPack.ps1` never wrote `meta.cpp`, so the Workshop item and the server's copy
  installed from it have none.
- Correction found during the fix: the server's `@NIMBY` folder did hold a `meta.cpp` (84 bytes,
  written by the panel's Manual Workshop Installer at the release-1 install) reading
  `protocol = 1;`, `publishedid = 0;`, `name = "NIMBY";`, `timestamp = 5250972744356698013;`.
  The installer writes id 0 when the Workshop content carries no `meta.cpp`; the launcher reads
  that 0.
- Not the cause: the Workshop item. Item 3828109139 is public and subscribable, read October 27
  signed out and as its author (owner controls shown, no removal notice). The page source holds
  a hidden `bannedNotification` template on every item; it is not displayed.
- Live fix (Jeff's approval, Jeff's password): upload a `meta.cpp` with `protocol = 1;`,
  `publishedid = 3828109139;` and a `timestamp` to `/@NIMBY/meta.cpp` over SFTP, beside
  `mod.cpp`; it takes effect at the next server start. Verify: the launcher query lists NIMBY
  with id 3828109139, and a client with no subscription is offered the download.
- Permanent fix: `pack.json` `workshopId` and `Write-MetaCpp` in `Build-NimbyPack.ps1` (PR #2,
  merged October 27 as d1a40b1), so every client build carries `meta.cpp` and the next Workshop
  update ships it. The PBOs are unchanged by it.
- **Applied live, October 27, 2026** (Jeff's approval; Jeff typed the password). The running
  server holds `meta.cpp` open: a delete over SFTP failed with `I/O error` and an overwrite with
  `File is already exclusively open.` at 18:40 to 18:44 Central. With 0 of 80 players online,
  the panel's Stop at 18:45 (a kill, by Jeff's ruling), the upload at 18:46:38
  (`logs\upload-meta-20261027.ps1`; the old file backed up in
  `logs\live-meta-backup-20261027-184631\`; read back identical: `protocol = 1;`,
  `publishedid = 3828109139;`, `name = "NIMBY";`, `timestamp = 1793139728;`), Start at 18:47:00.
  Down about 2 minutes.
- **Verified 18:49 Central:** the DZSA launcher query returns `status 0`, `nasdara`, `0/80`,
  version 1.30.164200, mods `NIMBY` with `steamWorkshopId 3828109139`. Confirmed by Jeff at
  19:02: the launcher downloads and joins on the G15 laptop from a dedicated test Steam account,
  and on his desktop.
- Follow-ups: (1) after the patch-1 Workshop update, re-run the launcher query: if the installer
  writes id 0 again despite the shipped `meta.cpp`, repeat the stop-upload-start; (2)
  `Write-MetaCpp` writes no `name` line while the installer's file has one; add it in the next
  tooling change; (3) changing `meta.cpp` needs the server stopped, so a live fix is a panel
  Stop and Start, not an upload alone.

### NIMBY-T-062 (high): Hands transfers fail on live; pockets look closed to the other player

Reported by Jeff, October 27, 2026, 21:19 Central, from play on the live server with Jay in the hour before (about 20:19 to 21:19 Central). Logged only; no fix is started from this entry.

- What they saw: (1) an offer shows on the other client and the Take animation plays, but no item moves; (2) Jeff opened his pockets and Jay could not view them.
- Build on live: release 1, `@NIMBY-20261026-214828`. Its Hands code is the pre-P3 transfer, which ended every two-client transfer `UNCONFIRMED` after 5,000 ms in the October 20 tests (NIMBY-T-010; audit TGA-035). The fix Jeff refers to ("we experienced this in testing and fixed it") is the P3 two-step transfer, merged to `main` on October 27 (669ff12) and shipped in patch 1 (38b10b5), which is not yet on live. Item (1) is therefore most likely NIMBY-T-010 seen on live; the logs confirm or refute it.
- Item (2) against the plan: Hands G3 says opening pockets permits deposits only, into cargo only; another player is never shown the pocket contents. "Cannot view" is the designed behavior. A defect would be Jay seeing no "Put In Pockets" prompt while holding an item near Jeff, or a deposit that does not arrive (the same transfer path as item 1). Open question for Jeff and Jay: did Jay try Put In Pockets with an item in hands?
- Evidence to pull (read-only): the live profile folder with `scripts\Get-ServerLogs.ps1` (a new `logs\server-<stamp>\` folder), then in the `script_*.log` of the boot running since 18:47:00 Central (16:47 on the panel clock):
  - `[NIMBY][Hands] id=<n> ... state=REQUESTED` lines and each id's terminal state. Expected on release 1: `state=UNCONFIRMED` 5,000 ms after `REQUESTED`, no `CONFIRMED`, and no toast.
  - `[NIMBY][Hands] pockets player=... open=true reason=toggle` for Jeff, and any `state=REQUESTED` from Jay with action `ActionPutInPockets`.
  - Both players' identities on each line (shared rules 4.2), and 0 `SCRIPT (E)`.
- Evidence, read October 27, 2026, 21:30 Central from pull `logs\server-20261027-212221\` (232 files, 232 OK), boot `script_2026-10-27_16-47-00.log` (panel clock; the 18:47 Central start), log times on the panel clock, 2 h behind Central:
  - Four Take attempts, all `NimbyHandsActionTakeItem`, each `state=REQUESTED` then `state=UNCONFIRMED ... detail=no completion signal within 5000 ms; item at HANDS:` on the giver, exactly 5,000 ms later: ids 1, 3 and 4 (Jeff gives `VitaminBottle #028148` to Jay, 19:11:58 to 19:12:26 panel) and id 5 (Jay gives `Paper #037083` to Jeff, 19:13:29 to 19:13:34). Every `REQUESTED` named a cargo destination (`CARGO:Attack2Bag_Ttsko`, `CARGO:AliceBag_Green`). No `CONFIRMED` line.
  - Ids 2, 6 and 7: repeat presses while a request was open, `state=REFUSED detail=item in flight (id=<n>)`, as designed.
  - Pockets: Jeff `open=true reason=toggle` 19:12:42, `open=false` 19:12:53, `open=true` 19:12:54. No `ActionPutInPockets` request from Jay at all, so no deposit was attempted.
  - Both identities on every line; 0 `SCRIPT (E)` in the boot; no item lost or duplicated (each item stayed in the giver's hands).
  - Verdict: item (1) is NIMBY-T-010 on the release-1 build, exactly its October 20 signature. Item (2) shows the toggle working; whether Jay was offered Put In Pockets is untested.
- Path to closure: the patch-1 rows S6-30 to S6-33 (two-client Take, Give, Put In Pockets, consent withdrawn) on `@NIMBY-20261027-124130` tonight, then the patch-1 Workshop update under the every-row gate Jeff ruled at 21:11. Until then, the Greet text and Discord can tell players that trading is coming in the next update.
