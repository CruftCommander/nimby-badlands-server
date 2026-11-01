# NimbyShared Phase 1 - build, boot, and the Greet regression

Test record for [Plan 11](../plans/2026-10-31-nimby-shared-plan.md) v0.2, rows SH-01 to SH-16. Seat: Claude Code in worktree `D:\GitHub\nimby-badlands-server-wt\nimby-shared`, branch `feat/nimby-shared`. Client steps: Jeff. Status: server-side rows done October 31, 2026; client rows pending.

## Artifact

| Item | Value |
| --- | --- |
| Source | `feat/nimby-shared` at `19672f7` (code), docs at `80bd577` and later |
| Build under test | `build\@NIMBY-20261031-210953` (`logs\build-20261031-210953\build.log`, `SUMMARY PASS`) |
| First green build | `build\@NIMBY-20261031-210751`, same sources, also `SUMMARY PASS` (the PBO packer stamps source write times, so the two builds pack the same files) |
| Static checks | `scripts\Test-NimbyRepo.ps1`: `SUMMARY PASS failures=0` (`logs\static-*`, after the build) |
| Test server | `Start-NimbyTestServer.ps1 -ServerPath D:\DayZServer -InstanceId 998 -BuildPath <build under test> -ProfileName nimby-shared-p1`, port 2302, started 21:13:39, `Init sequence finished` 21:14:08 |
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
| SH-02 | pending (client) | |
| SH-03 | pending (client) | |
| SH-04 | pending (client) | |
| SH-05 | pending (client) | |
| SH-06 | pending (client); the destructor order is PASS at boot (`lang: subclass destructor runs, then the base destructor`) | |
| SH-07 | pending (client) | |
| SH-08 | pending (client) | |
| SH-09 | pending (client) | |
| SH-10 | pending (client) | |
| SH-11 | pending (client) | Fixture above |
| SH-12 | self-test part PASS; live part pending (client) | The 21 receiver cases in `[NIMBY][Shared][SelfTest]`, among them `PASS fetch: chunk for another request dropped` and `PASS fetch: duplicate done dropped without refusing` |
| SH-13 | PASS (loud FAIL, no timeout) | `Build-NimbyPack.ps1 -Addons NimbyAutorun,NimbyHands,NimbyEarplugs,NimbyNotes,NimbyTack,NimbyGreet`, `logs\build-20261031-210921\build.log`: `BOOT FAIL 21:09:36.525 SCRIPT    (E): @"NimbyGreet/Scripts/3_Game/nimbygreet_clientfiles.c,106": Can't find variable 'NimbySharedRules'`, `SUMMARY FAIL`. The engine's missing-addon line from the RPT, verbatim, for `bootCheck.failurePatterns`: `21:09:24.584 Warning Message: Addon 'NimbyGreet' requires addon 'NimbyShared'`. That build (`build\@NIMBY-20261031-210921`) is not used anywhere |
| SH-14 | PASS on the file set; the row's hash wording cannot pass for any boot (orchestrator question below) | `storage_999` hashed before and after the boot of `build-20261031-210953` (`logs\sh14-storage-before.txt`, `logs\sh14-storage-after.txt`): 88 files before, the same 88 names after, no file added or removed. 30 files changed contents, all the engine's own world save: `players.db` 1, `animals.bin` 1, `building_*.bin` 12, `dynamic_*.bin` 12, `events.bin` 1, `types.bin` 1, `vehicles.bin` 1, `zombies.bin` 1. Neither addon has an `OnStoreSave` or `OnStoreLoad` (0 matches in `NimbyShared` and `NimbyGreet` scripts). The boot profile's `NimbyShared\` holds only the boot check's `selftest` flag |
| SH-15 | pending (client) | |
| SH-16 | pending (client) | |

## Open for the orchestrator

1. SH-14 wording: "storage folder hashes unchanged across the boot" fails for every boot, vanilla included, because the engine rewrites the world files of `storage_999` each time. Proposed evidence: the file set is unchanged, no addon defines `OnStoreSave` or `OnStoreLoad`, and nothing new appears under the addons' profile folders; or a same-build `-Vanilla` control boot whose changed-file list matches.
