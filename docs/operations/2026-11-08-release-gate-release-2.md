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

## Open tooling items

- NIMBY-T-106: `Build-NimbyPack.ps1` does not verify model files in the PBO; a fresh clone packs
  no model because addon `data\` is git-ignored. Build `123417` PASSed with 11 entries.
