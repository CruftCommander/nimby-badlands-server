# Release gate - release 2 (Hands, Greet v2, DPV) - 2026-11-08

Gate 9.2 record for the release artifact built from `main` at `f52cb81` (PR #23).
Pattern follows `2026-10-20-release-gate-legs-ears-scrawl.md`.

## Artifact

| Item | Value |
| --- | --- |
| Client pack | `build\@NIMBY-20261108-123656` |
| Server pack | `build\@NIMBY-Server-20261108-123656` |
| Build gate | SIGN 9/9, SELFTEST 784/784, BOOT PASS |
| `NimbyDpv.pbo` | 28 entries, 9,260,004 bytes, ODOL v56 models inside (verified by hand - see T-106) |
| Void builds | `@NIMBY-20261108-110446` (wrong model set), `@NIMBY-20261108-123417` (packed without model data) - delete |

## Boots

Both boots used `storage_998`, a copy of live `storage_1`, on the live-replica mission, 1PP, fresh profile.

| Check | Artifact boot `logs\testserver-20261108-123942` | Vanilla control `logs\testserver-20261108-124039` |
| --- | --- | --- |
| `Init sequence finished` | 1 | 1 |
| `SCRIPT (E)` | 0 | 0 |
| `SCRIPT (W)` | 22 (all vanilla `disinfectantspray.c` / RoboclientUtils) | 22 (same) |
| `Scripted variables corrupted` | 0 | 0 |
| Virtual Machine Exception | 0 | 0 |
| `[CE][Storage] valid:yes` | all (`types.bin`, `types.001/002`, `events.bin`) | all |
| `[CE][LoadMap] Group` | 13719 loaded, 0 failed | 13719 loaded, 0 failed |
| `Type ... will be ignored` | 27 (all vanilla Nasdara rows) | 31 = same 27 + 4 `NimbyTack_PinnedNote_*` (mod absent, expected) |
| `[CE][DynamicEvent] Load` | 39 primary / 11 secondary | 39 / 11 |
| `[NIMBY]` events | 20 (Tack, Notes, Hands, Greet, Autorun loaded; Tack door restored from storage) | 0 |

Verdict: **PASS**. No artifact-only error, warning, or ignored type. The `[CE][DE] child max 0`
lines for `VehicleOffroadHatchback` and `VehicleSedan02` appear in both boots and are vanilla
Nasdara.

## Not covered by this gate

- `VehicleNimbyDpv` and `NimbyDpv_*` CE rows are not in the test mission; the DPV patches are
  applied to production by `Deploy-MissionPatches.ps1` at the release restart. First production
  RPT must show no `Type 'NimbyDpv...' will be ignored` and CE vehicle lines
  (`log_ce_vehicle` / `log_ce_dynamicevent` patch).
- Hands is tested live on production by Jay and Jeff (collective test rows 11-12 skipped).
- Greet loaded `version=1` default here because the profile was fresh; `greet.json` v2 ships by
  copying into `$profile:NimbyGreet\` at the restart.

## Production restart (November 8, 2026, 13:02 to 13:34 Central)

Driven by Claude in DayZ Tools Publisher and the HostHavoc panel at Jeff's request; Jeff ran the two `Deploy-MissionPatches.ps1` passes and `Get-ServerLogs.ps1`. Jeff waived the five-minute RCON warning for this restart because the server was empty (0/80 at the stop).

| Step | Time (Central) | Evidence |
| --- | --- | --- |
| Workshop update | 13:02 | Publisher: update of item 3828109139 from `build\@NIMBY-20261108-123656`, "all signed", "The item was successfully published", 9.7 MB. Workshop page: Updated Nov 8, 10.209 MB, 2 change notes, release 2 description (`workshop-description.md`). |
| Dry run | 13:08 | `Deploy-MissionPatches.ps1 -WhatIf`, run `logs/deploy-20261108-130815`: 7 patch files, 5 targets, 29 entries ADD or REPLACE, 0 FAIL; live hashes identical to the 10:57 mirror. |
| Stop | 13:11 | Panel Stop, 0 players, status Stopped. |
| Backup | 13:12 | Panel Create Backup: `manual-backup-2026-11-08_11-12.zip` (panel clock). |
| Pack install | 13:14 | Manual Workshop Installer, id 3828109139, Friendly Name mode: 10,209,264 bytes, "Completed: @NIMBY", keys installed. `/@NIMBY/addons/` nine PBOs at release 2 sizes (Hands 206 KB, DPV 9 MB); `/@NIMBY/meta.cpp` kept (93 B, October 27). |
| Mission patches | 13:17 | `Deploy-MissionPatches.ps1 -Upload`, run `logs/deploy-20261108-131721`: 5 of 5 uploaded, every readback hash equal to the staged copy, staged hashes equal to the dry run. |
| `greet.json` v2 | 13:19 | Panel File Manager upload to `/HostHavocDayZServer/NimbyGreet/greet.json` (8,019 B). |
| Start | 13:22 | Panel Start, "The service was started successfully". |
| Launcher check | 13:25 | `dayzsalauncher.com/api/v1/query/104.192.227.46/2303`: `status 0`, map `nasdara`, version `1.30.164289`, mods `NIMBY` `steamWorkshopId 3828109139`. |
| First RPT | 13:34 | `logs/server-20261108-133214/DayZServer_x64_2026-11-08_11-22-07.RPT` and `script_2026-11-08_11-22-07.log` (panel clock), see below. |

First RPT against the previous boot (`DayZServer_x64_2026-11-08_10-01-15.RPT`, release 1):

- `SCRIPT (E)` 0 / 0; `Scripted variables corrupted` 0 / 0; VME 0 / 0; `SCRIPT (W)` 24 / 24 (vanilla); `Warning Message` 0; every `[CE][Storage]` block `valid:yes`, 0 failed items.
- `Type ... will be ignored` 27 / 27, identical set (ChristmasTree, Land_House_*_Dam*, ZmbM_takiCitizenUrbanB_BlueBright child); none on a patched class.
- `Loaded 40 dynamic events 201 total types` (39 / 200 before); `[CE][DynamicEvent] Load Events:[39] Primary spawners: 39 Secondary spawners: 11`.
- `[CE][DE] [VehicleNimbyDpv] Spawning` four times (event nominal 4): `NimbyDpv_Car` at 12818/6755, 4739/7361, 11875/10622, 6110/9142, all in the ten-position list of `cfgeventspawns.dpv.patch.xml`. The previous boot had no `[CE][DE] Spawning` lines at all, so `log_ce_dynamicevent` is on. Humvees were already in storage, so no Humvee spawn line is expected on this boot.
- `[NIMBY][Autorun]`, `[NIMBY][Hands]`, `[NIMBY][Notes]`, `[NIMBY][Tack] loaded`; `[NIMBY][Greet] greet.json loaded source=greet.json version=2 ... bytes=5810 truncations=0`; Tack `door load result=restored` for the October 27 note.
- New on this boot: two `!!! [Inputs] Cannot Register "UANimbyGreetPageUp" / "UANimbyGreetPageDown" non-existent input - Fix Preset!` lines. Greet's page keys moved to `NimbyShared` as `UANimbySharedPageUp/Down` (Plan 11, `NimbyGreet/Data/Inputs.xml` header); the server profile's saved input preset still names the old inputs. Server-side only, harmless; a client whose preset carries the old names gets the same line once and keeps the new binds.
- Live server build is `1.30.164289`; the gate boots ran on `1.30.164078.38`.

## Open tooling items

- NIMBY-T-106: `Build-NimbyPack.ps1` does not verify model files in the PBO; a fresh clone packs
  no model because addon `data\` is git-ignored. Build `123417` PASSed with 11 entries.
