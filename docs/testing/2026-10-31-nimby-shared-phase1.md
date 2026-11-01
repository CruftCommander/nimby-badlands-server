# NimbyShared Phase 1 - build, boot, and the Greet regression

Test record for [Plan 11](../plans/2026-10-31-nimby-shared-plan.md) v0.2, rows SH-01 to SH-16. Seat: Claude Code in worktree `D:\GitHub\nimby-badlands-server-wt\nimby-shared`, branch `feat/nimby-shared`. Client steps: Jeff. Status: every row done October 31, 2026 (server-side rows by the seat 21:08-21:20; client rows by Jeff 21:30-21:43 Central, recorded by the orchestrator from `logs\sh-client\nimby-lines.txt` and the server script log).

## Artifact

| Item | Value |
| --- | --- |
| Source | `feat/nimby-shared` at `19672f7` (code), docs at `80bd577` and later |
| Build under test | `build\@NIMBY-20261031-210953` (`logs\build-20261031-210953\build.log`, `SUMMARY PASS`) |
| First green build | `build\@NIMBY-20261031-210751`, same sources, also `SUMMARY PASS` (the PBO packer stamps source write times, so the two builds pack the same files) |
| Static checks | `scripts\Test-NimbyRepo.ps1`: `SUMMARY PASS failures=0` (`logs\static-*`, after the build) |
| Test server | `Start-NimbyTestServer.ps1 -ServerPath D:\DayZServer -InstanceId 998 -BuildPath <build under test> -ProfileName nimby-shared-p1`, port 2302, started 21:13:39, `Init sequence finished` 21:14:08; server script log `profiles\nimby-shared-p1\script_2026-10-31_21-13-39.log`, 0 `SCRIPT (E)` through the client session |
| Client | `Start-NimbyTestClient.ps1 -BuildPath <build under test> -Connect 127.0.0.1 -Port 2302`, started 21:29:22 (`logs\testclient-20261031-212922.log`); `$profile:NimbyShared\debug` and `$profile:NimbyGreet\debug` present; connected 21:30:22 as Road Dog (`h8o5rWLPVptWLIPrMHl9JnGQMT0bM8CfAyoaOMMnH5Q=`, `76561198354705597`), disconnected 21:43:54. Client lines: `logs\sh-client\nimby-lines.txt` (575 lines, the `[NIMBY]` subset of `C:\Users\Buffe\AppData\Local\DayZ\script_2026-10-31_21-29-*.log`, copied as `logs\sh-client\client-script.log`); screenshots in `logs\sh-client\` |
| Server profile | `profiles\nimby-shared-p1\` (git-ignored): `NimbyGreet\greet.json` generated for SH-11 (below), `NimbyGreet\debug`, `NimbyShared\debug` |
| Replica storage | `D:\DayZServer\...\storage_998` as the Hands session left it at 21:07:26 (not a fresh copy of live `storage_1`); no row here depends on it |

A first test-server start at 21:12:58 (`logs\testserver-20261031-211258`) stopped at once because the script read end of input in a background shell, and the server was killed before RCON came up (`SERVER KILLED ... treat this run's storage as suspect`). `storage_998` shows no write after 21:07:26, so that run wrote nothing. The server was restarted with its input held open.

## Boot check verdict lines

```
SELFTEST Shared ran=yes cases=53 pass=53 fail=0
SELFTEST Autorun ran=yes cases=112 pass=112 fail=0
SELFTEST Hands ran=yes cases=156 pass=156 fail=0
SELFTEST Earplugs ran=yes cases=28 pass=28 fail=0
SELFTEST Notes ran=yes cases=54 pass=54 fail=0
SELFTEST Tack ran=yes cases=155 pass=155 fail=0
SELFTEST Greet ran=yes cases=87 pass=87 fail=0
SELFTEST TOTAL pass=645 fail=0 flags=7 expected=7 source=script
BOOT PASS 21:08:28.753 [CE][Hive] :: Init sequence finished.
SUMMARY PASS mod=D:\GitHub\nimby-badlands-server-wt\nimby-shared\build\@NIMBY-20261031-210751
```

Script log `logs\build-20261031-210751\server-profile\script_2026-10-31_21-07-53.log`: 0 `SCRIPT (E)`; risk 1 closed at boot:

```
21:08:08.482  SCRIPT       : [NIMBY][Shared][SelfTest] PASS lang: subclass destructor runs, then the base destructor
21:08:08.482  SCRIPT       : [NIMBY][Shared] selftest complete; cases=53 failures=0
21:08:08.501  SCRIPT       : [NIMBY][Greet] selftest complete; cases=87 failures=0
```

The rerun for SH-14 (`build-20261031-210953`) gave the same `SELFTEST` lines and `SUMMARY PASS`.

## SH-11 fixture

The Phase 0 payload of G-P0-31 (16 sections, 40,359 characters, 74 chunks) cannot be rebuilt as a valid `greet.json` today: the validator refuses a section body above 1,000 characters (NIMBY-T-059, `scripts\Test-GreetText.ps1`), so 16 sections hold at most 16,000. The fixture is the largest valid Russian language instead: the committed English language from `server\profile-overrides\NimbyGreet\greet.json` plus a Russian language of 16 sections, 15,230 body characters, longest body 953 characters, longest line 155 bytes, each section ending in a two-column table. `Test-GreetText.ps1`: `PASS: 0 errors, 0 warning(s)`. Server load line:

```
21:13:52.738  SCRIPT       : [NIMBY][Greet] greet.json loaded source=greet.json version=1 forceShow=false languages=english,russian default=english bytes=33401 truncations=0 openDelay=1 cooldown=5 reread=10
```

The 74-chunk scale stays proven by G-P0-31 on the same receiver logic (moved verbatim) and by the NimbyShared self-test receiver cases.

## Results

| Id | Result | Evidence |
| --- | --- | --- |
| SH-01 | PASS | Boot check lines above: NimbyShared 53 of 53, Greet 87 of 87, 0 `SCRIPT (E)` |
| SH-02 | PASS | Client 21:30:23.475 `OnPlayerLoaded`, 21:30:23.680 `request sent id=1 ... manual=false`, 21:30:26.637 `payload complete id=1 ... chunks=5 bytes=4778 sections=9`, 21:30:27.616 `[NIMBY][Greet] open version=1 source=spawn language=english section=welcome sections=9 topMenu=65511`; server 21:30:23.709 `serve id=1 ... manual=false`. Jeff closed the first window by accident at 21:30:46 and reopened with Insert at 21:32:15 for the screenshot; the window matched release 1 |
| SH-03 | PASS | Two presses: 21:39:37.880 `hotkey: manual request` then 21:39:38.014 `open ... source=hotkey`; 21:40:14.175 `hotkey: manual request` then 21:40:14.309 `open ... source=hotkey`. One `open` line per press, no double open; a press on an open window closes it (`close reason=hotkey`) |
| SH-04 | PASS | 21:41:39.611 `[NIMBY][Greet][Debug] pause menu: Server welcome clicked`, 21:41:39.718 `open ... source=menu`; repeated 21:41:47 and 21:42:07. Pause-menu screenshot shows Server welcome under the vanilla column |
| SH-05 | PASS | 21:30:27.616 `[NIMBY][Shared][Debug] input exclude added: menu menu=65511`, 21:30:46.604 `[NIMBY][Shared][Debug] input exclude removed: menu menu=65511` (and the same pair on every later open and close); W held while open, character did not move (Jeff) |
| SH-06 | PASS on the log lines and the self-test; the manual wheel check was not possible | 21:30:27.616 `[NIMBY][Shared][Debug] zeroing disabled=true menu=65511`, 21:30:46.603 `zeroing disabled=false menu=65511`, and the same pair on every later open and close cycle (every `true` has its `false`). Jeff's character had no weapon with zeroing (fists only), so the wheel check after close could not be run; the release of the guard is shown by the `false` lines. Destructor order: `PASS lang: subclass destructor runs, then the base destructor` at boot. No menu was destroyed without `OnHide` in this session (the last close was at 21:42:11, the exit to the main menu at 21:43:54) |
| SH-07 | PASS | Mouse: 21:33:14.855 to 21:33:35.563 `section selected index=1 ... index=8` in order. Tab-left and Tab-right: 21:33:53.686 to 21:33:56.089 indexes 1, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7 stepping one at a time |
| SH-08 | PASS (manual) | Page Down and Page Up paged the body (Jeff); Options > Controls lists "NIMBY: page the window up" and "NIMBY: page the window down" on Page Up and Page Down beside "Greet: open server welcome" on Insert (screenshot). No log line is written for paging |
| SH-09 | PASS | 21:35:34.224 `[NIMBY][Greet] pref textScale=1`, 21:35:34.853 `textScale=1.25`, 21:35:35.388 `textScale=1.5`, 21:35:36.029 `textScale=1`; the text changed size each time (Jeff) |
| SH-10 | PASS | 21:39:35.417 `[NIMBY][Greet] close reason=player neverShow=true dismissedVersion=1 written=true section=discord` (Esc after SH-16); 21:30:46.604 and 21:41:37.642 `close reason=player` as well |
| SH-11 | PASS with the fixture above | 21:36:57.598 `pref language=russian`, `request sent id=3 language=russian`, 21:36:58.352 `done id=3 chunkCount=29 ... served=russian ... received=29`, `payload complete id=3 language=russian version=1 chunks=29 bytes=28623 sections=16 clientCuts=0 elapsedMs=753`, `content replaced in the open window language=russian`; no `payload rejected` line; server 21:36:57.624 `serve id=3 asked=russian served=russian ... chunks=29 bytes=28623`. Cyrillic rendered in every section (screenshot). Back to English at 21:38:46 (`chunks=5`) |
| SH-12 | PASS | Self-test: the 21 receiver cases in `[NIMBY][Shared][SelfTest]`, among them `PASS fetch: chunk for another request dropped` and `PASS fetch: duplicate done dropped without refusing`. Live: 21:40:15.808 `request sent id=7` 1.6 s after the id=6 open, 21:40:15.897 `done id=7 chunkCount=0 version=0 served= reason=cooldown received=0`, `request refused by cooldown; retry in 3498 ms`; server 21:40:15.845 `request refused reason=cooldown id=7`; the retry `id=8` at 21:40:19.379 was served. Further cooldown refusals at ids 9 to 12, 14, 15, 17, each with a cancelled or completed retry; no `chunk dropped` line, since no chunk of a superseded request was in flight |
| SH-13 | PASS (loud FAIL, no timeout) | `Build-NimbyPack.ps1 -Addons NimbyAutorun,NimbyHands,NimbyEarplugs,NimbyNotes,NimbyTack,NimbyGreet`, `logs\build-20261031-210921\build.log`: `BOOT FAIL 21:09:36.525 SCRIPT    (E): @"NimbyGreet/Scripts/3_Game/nimbygreet_clientfiles.c,106": Can't find variable 'NimbySharedRules'`, `SUMMARY FAIL`. The engine's missing-addon line from the RPT, verbatim, for `bootCheck.failurePatterns`: `21:09:24.584 Warning Message: Addon 'NimbyGreet' requires addon 'NimbyShared'`. That build (`build\@NIMBY-20261031-210921`) is not used anywhere |
| SH-14 | PASS on the file set; the row's hash wording cannot pass for any boot (orchestrator question below) | `storage_999` hashed before and after the boot of `build-20261031-210953` (`logs\sh14-storage-before.txt`, `logs\sh14-storage-after.txt`): 88 files before, the same 88 names after, no file added or removed. 30 files changed contents, all the engine's own world save: `players.db` 1, `animals.bin` 1, `building_*.bin` 12, `dynamic_*.bin` 12, `events.bin` 1, `types.bin` 1, `vehicles.bin` 1, `zombies.bin` 1. Neither addon has an `OnStoreSave` or `OnStoreLoad` (0 matches in `NimbyShared` and `NimbyGreet` scripts). The boot profile's `NimbyShared\` holds only the boot check's `selftest` flag |
| SH-15 | PASS (visual) | Screenshots in `logs\sh-client\`: normal state (dark panels under Never, Text, Language, Close; flat link labels), Close on hover (black panel, red label), Never on arrow-key focus (black panel, red label); each as the release-1 window (Jeff) |
| SH-16 | PASS | 21:39:05.436 `[NIMBY][Greet] open url=https://discord.gg/kH3eJ5mdEv section=discord` (system browser opened), 21:39:09.440 `[NIMBY][Greet][Debug] copied url=https://discord.gg/kH3eJ5mdEv`; 21:39:35.417 `close reason=player neverShow=true dismissedVersion=1 written=true section=discord`; the next spawn-time request carried `dismissedVersion=1 neverShow=true` (21:39:37.880) |

## Rulings (orchestrator, October 31, 2026, 21:25 Central)

1. SH-14 wording: accepted as proposed. The row's evidence is the unchanged file set, no `OnStoreSave` or `OnStoreLoad` in either addon, and nothing new under the addons' profile folders; no vanilla control boot is needed for a utility addon (shared rules 6). Plan 11 v0.3 carries the new wording.
2. SH-11 fixture: accepted in place of the G-P0-31 payload, since NIMBY-T-059 made that payload invalid; the 74-chunk scale stays proven by G-P0-31 and the receiver self-test.
3. SH-13: PASS. `Warning Message: Addon 'NimbyGreet' requires addon 'NimbyShared'` goes into `bootCheck.failurePatterns` with the next orchestrator `pack.json` pull request.
4. SH-06: PASS on the log lines and the destructor self-test; the manual wheel check is recorded as not run (no weapon with zeroing on the test character). The release-1 build passed the same check with the same `ForceDisable` calls (NIMBY-T-058), and the code that releases the guard is the code that moved.

## Verdict

SH-01 to SH-16 PASS on `build\@NIMBY-20261031-210953`; Phase 1 exit met. The PR to `main` is Jeff's call (Plan 11 Phase 2).

## Client run 1, server side (21:30 to 21:43)

One client (Steam64 `76561198354705597`) joined the test server, ran rows, and logged out at 21:43:53; the server then stopped on its input and reached `~DayZGame()` at 21:45:10 (`logs	estserver-20261031-211338	estserver.log`: `STOP requested by input`, `SERVER EXITED before stop code=-1`, `SUMMARY events=90`). `Read-NimbyTestRun.ps1` is not on this branch (not on `main` at `8d08d2b`), so the run was read by hand. Server lines from `profiles
imby-shared-p1\script_2026-10-31_21-13-39.log`, player ids cut:

```
21:30:23.709  SCRIPT       : [NIMBY][Greet] serve id=1 asked=english served=english version=1 chunks=5 bytes=4778 manual=false
21:32:15.034  SCRIPT       : [NIMBY][Greet] serve id=2 asked=english served=english version=1 chunks=5 bytes=4778 manual=true
21:36:57.624  SCRIPT       : [NIMBY][Greet] serve id=3 asked=russian served=russian version=1 chunks=29 bytes=28623 manual=true
21:38:46.732  SCRIPT       : [NIMBY][Greet] serve id=4 asked=english served=english version=1 chunks=5 bytes=4778 manual=true
21:39:37.921  SCRIPT       : [NIMBY][Greet] serve id=5 asked=english served=english version=1 chunks=5 bytes=4778 manual=true
21:40:14.195  SCRIPT       : [NIMBY][Greet] serve id=6 asked=english served=english version=1 chunks=5 bytes=4778 manual=true
21:40:15.845  SCRIPT       : [NIMBY][Greet] request refused reason=cooldown id=7
21:40:19.382  SCRIPT       : [NIMBY][Greet] serve id=8 asked=english served=english version=1 chunks=5 bytes=4778 manual=true
21:40:20.888  SCRIPT       : [NIMBY][Greet] request refused reason=cooldown id=10
21:40:26.970  SCRIPT       : [NIMBY][Greet] serve id=13 asked=english served=english version=1 chunks=5 bytes=4778 manual=true
21:40:27.340  SCRIPT       : [NIMBY][Greet] request refused reason=cooldown id=14
21:40:40.886  SCRIPT       : [NIMBY][Greet] serve id=16 asked=english served=english version=1 chunks=5 bytes=4778 manual=true
21:40:44.803  SCRIPT       : [NIMBY][Greet] request refused reason=cooldown id=17
21:40:46.071  SCRIPT       : [NIMBY][Greet] serve id=18 asked=english served=english version=1 chunks=5 bytes=4778 manual=true
21:41:39.621  SCRIPT       : [NIMBY][Greet] serve id=19 asked=english served=english version=1 chunks=5 bytes=4778 manual=true
21:41:47.052  SCRIPT       : [NIMBY][Greet] serve id=20 asked=english served=english version=1 chunks=5 bytes=4778 manual=true
21:42:07.002  SCRIPT       : [NIMBY][Greet] serve id=21 asked=english served=english version=1 chunks=5 bytes=4778 manual=true
```

These carry SH-11 (serve id=3: `asked=russian served=russian chunks=29 bytes=28623`, the fixture above) and the server half of SH-12 (cooldown refusals at 21:40:15 and later). The client lines and screenshots for every client row are still owed (Jeff), and the client half of SH-11 (`payload complete ... chunks=29`) and SH-12 (`done ... reason=cooldown`) is in the client RPT.
