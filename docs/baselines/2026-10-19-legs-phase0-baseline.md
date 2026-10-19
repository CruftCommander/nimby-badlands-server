# Baseline - Legs Phase 0 (October 19, 2026)

The build, source, and reference that Legs (`NimbyAutorun`) Phase 1 was written and booted against (Legs plan, Phase 0 item 1). Later Legs boots and in-game runs compare against this record. A change to any hash below means the reference or the game moved, and the API evidence table in the plan is re-checked before it is trusted.

## Game and server

| Item | Value |
| --- | --- |
| Game build | DayZ 1.30, `Version 1.30.164078.38` (boot-check RPT header, `logs/build-20261019-152118/server-profile/DayZServer_x64_2026-10-19_15-21-20.RPT`) |
| Server binary | `D:\DayZServer\DayZServer_x64.exe` (the replica, binaries from the Steam `DayZServer` install), file version 1.30.164.078, exe timestamp 2026-10-15 07:19:30, SHA-256 `67DEA3D44A5956CC4F57E9ECCE91A64C9E7EE9CDFFF0DE52A65E4CAE46F7FF20` |
| Server Steam build | 25460313, as recorded on October 16, 2026 (`2026-10-16-replica-vanilla-control-boot.md`). Not re-read today: `F:\SteamLibrary\steamapps\appmanifest_223350.acf` (1,073 bytes, October 15) did not read as text |
| Client binary | `F:\SteamLibrary\steamapps\common\DayZ\DayZ_x64.exe`, file version 1.30.164.078 |
| Client Steam build | 25956772 (`appmanifest_221100.acf`) |
| Mission | `dayzOffline.nasdara`, boot check on instance 999 (`pack.json` `bootCheck`) |

## Script reference

| Item | Value |
| --- | --- |
| Folder | `vendor/dayz-1.30-scripts/` in the main clone (git-ignored, read-only; the worktree has no copy and reads the main clone's) |
| Files | 3,731, the same count as the October 15 extract (`2026-10-15-dayz-1.30-scripts-reference.md`) |
| `dta\scripts.pbo` SHA-256, client | `E7851E289E6434494E4232F9A3E86D6D6675B5D35922A4044450F943F9935189` |
| `dta\scripts.pbo` SHA-256, Steam server install | `E7851E289E6434494E4232F9A3E86D6D6675B5D35922A4044450F943F9935189` |
| `dta\scripts.pbo` SHA-256, replica `D:\DayZServer` | `E7851E289E6434494E4232F9A3E86D6D6675B5D35922A4044450F943F9935189` |

All three hashes match the October 15 reference, so the file:line citations in the plan's API evidence table hold for the build above.

## Source

| Item | Value |
| --- | --- |
| Branch | `feat/autorun` in `D:\GitHub\nimby-badlands-server-wt\feat-autorun`, cut from `main` at `26b411e` |
| Phase 1 code | `0d2e6e5` (scripts, `config.cpp`, `Data/Inputs.xml`) |
| `NOTICE` | `685e2fe` |
| Addon before Phase 1 | The October 14, 2026 fork: two scripts, numpad plus binding, last built as `@NIMBY-20261014-175505` on 1.29 Chernarus, never built on 1.30 |

## Phase 1 artifact

| Item | Value |
| --- | --- |
| Test artifact | `build\@NIMBY-20261019-152515` (`-Qualification`), log `logs\build-20261019-152515\build.log` (both git-ignored) |
| Verdict | `SUMMARY PASS`; `SIGN PASS` 5 of 5 with the unsigned control flagged; `BOOT PASS` in qualification mode (`storageAutoFix=0`, `storage_999`) |
| `NimbyAutorun.pbo` | 9 entries, SHA-256 `2554DB1237C416C2313F01B0168D4AB5A21E1E3C81F1E5B56E112AC10633FA66` |
| Self-test | Autorun 70 cases, 0 FAIL (the log reports `pass=140`: RPT and script log are counted together). Other addons unchanged: Notes 11, Tack 48, Earplugs 16 |
| Boot-check RPT | 0 `SCRIPT (E)`, 0 `Scripted variables corrupted` |
| Server profile | `NimbyAutorun\config.json` written with defaults on first boot: `version` 1, `requestsPerSecond` 10 |

Two more builds of the Phase 1 code passed the same checks in quick mode (`storageAutoFix=1`): `@NIMBY-20261019-151835` from `0d2e6e5` before the `NOTICE` change, and `@NIMBY-20261019-152118` from `685e2fe` (Legs PBO `4B9F6C3AA7C319A031F1043BFB19C0DEB700EABC988761130B61FB9420C80BA4`), the same sources as the test artifact.

PBO hashes change on every build, also for addons whose sources did not change (Earplugs and Tack differ across all three builds), so a hash identifies one build, not one source revision. Name the build folder, not the hash, when a record has to point at the artifact.
