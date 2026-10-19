# Baseline - Hands Phase 0 (October 19, 2026)

Phase 0 item 1 of the Hands plan (`docs/plans/2026-10-15-hands-plan.md`): the client and server build, the source revision, and the shared 1.30 reference hashes that the Phase 0 evidence and the Phase 1 build were measured against. Recorded by the `feat/hands` implementation session.

## Source

| Item | Value |
| --- | --- |
| Branch | `feat/hands`, worktree `D:\GitHub\nimby-badlands-server-wt\feat-hands` |
| Cut from | `main` at `26b411ec05b95582088944bceff047cb4932c706` ("docs: Scrawl step 4 record ...") |
| Addon at the cut | `mods/own/nimby-pack/NimbyGiveTake/` as left by the October 14, 2026 CR-001 rewrite (six actions, `ServerTakeToDst` transfer, fixed 1,000 ms verify) |
| `pack.json` row at the cut | `folder`, `pboName`, `prefix` all `NimbyGiveTake` |

## Game builds

| Item | Value |
| --- | --- |
| Client | `F:\SteamLibrary\steamapps\common\DayZ\DayZ_x64.exe`, file version `1.30.164.078`, written 2026-10-15 05:37:29, Steam app 221100 build id `25956772` |
| Client exe SHA-256 | `F08F93B8CDA4CB0DAB34692B40570A7798B854CAFE2E1E49EE23B8316F69F3A0` |
| Server (Steam) | `F:\SteamLibrary\steamapps\common\DayZServer\DayZServer_x64.exe`, file version `1.30.164.078`, written 2026-10-15 07:19:30 |
| Server (local replica) | `D:\DayZServer\DayZServer_x64.exe`, same version and hash as the Steam server |
| Server exe SHA-256 | `67DEA3D44A5956CC4F57E9ECCE91A64C9E7EE9CDFFF0DE52A65E4CAE46F7FF20` |
| Live server Steam build | app 223350 build id `25460313` (mirror manifest `D:\HostHavoc\NIMBY Badlands\104.192.227.46_2302\steamapps\appmanifest_223350.acf`), as in `2026-10-16-replica-vanilla-control-boot.md` |
| DayZ Tools | app 830640 build id `25460346` (signs the PBOs) |

The client's Steam build id moved since the October 15 reference was taken, but its `scripts.pbo` hash below did not, so the reference still matches the client's scripts.

## Shared 1.30 script reference

| Item | Value |
| --- | --- |
| Folder | `vendor/dayz-1.30-scripts/` in the main clone `D:\GitHub\nimby-badlands-server` (git-ignored, read-only; the worktree has no `vendor/`) |
| File count | 3,731 (unchanged from `2026-10-15-dayz-1.30-scripts-reference.md`) |
| Client `dta\scripts.pbo` SHA-256, read 2026-10-19 | `E7851E289E6434494E4232F9A3E86D6D6675B5D35922A4044450F943F9935189` |
| Steam server `dta\scripts.pbo` SHA-256, read 2026-10-19 | `E7851E289E6434494E4232F9A3E86D6D6675B5D35922A4044450F943F9935189` |
| Replica `D:\DayZServer\dta\scripts.pbo` SHA-256, read 2026-10-19 | `E7851E289E6434494E4232F9A3E86D6D6675B5D35922A4044450F943F9935189` |
| Matches the October 15 reference baseline | Yes, all three |

Every file:line in the Hands plan's API evidence table (v0.5) cites this folder.

## Not yet recorded

The two-client run (Phase 0 item 6) records its own build stamp, server log folder, and client logs in `docs/testing/2026-10-19-hands-phase1.md` when it runs.
