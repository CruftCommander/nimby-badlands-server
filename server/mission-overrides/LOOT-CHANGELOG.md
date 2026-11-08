# Loot change log

Every change to the live loot economy (`types.xml`, `events.xml`, `mapgroupproto.xml`, `cfgspawnabletypes.xml`) is logged here before it is applied and marked applied afterwards, with who, what (value before and after), when, and why (Jeff's standing rule, October 26, 2026, 22:57). The patch files in this folder carry the exact rows; this file is the history. Live values are read from `server/live/` as mirrored by `Sync-LiveReplica.ps1`; "ruled by" is the person who decided, "drafted by" the one who wrote the row.

Status values: DRAFT (written, not on live), APPLIED <date, restart> (on live), REVERTED <date>.

## Release 1 batch (ruled October 26, 2026; target: the release restart)

Ruled by: Jeff Fabish. Drafted by: Claude (this session), from the live files mirrored October 26, 2026, 21:18. Status: **APPLIED October 26, 2026, 23:47, release 1 restart (server started 23:57)**, by Jeff (SFTP upload with Posh-SSH) from the staged set `D:\DayZServer\_release1-upload\` built by Claude; backups: panel `manual-backup-2026-10-26_21-03.zip` and mirror zip `D:\DayZServer\_backups\live-pre-release1-20261026-2318.zip`. The `server/live/` reference copies move forward at the next `Sync-LiveReplica.ps1` run. Reference: `docs/operations/2026-10-26-release-1-loot-update.md`.

| File | Entry | Before (live) | After | Why |
| --- | --- | --- | --- | --- |
| `types.scrawl.patch.xml` | `Paper` | nominal 0, min 0, lifetime 7200 | nominal 30, min 15, lifetime 2592000 | Scrawl needs paper in the world and written notes must outlive the 2-hour vanilla lifetime (Scrawl plan N3, N4; Jeff, October 15 and 16, 2026; confirmed October 26, 2026, /decide 2 of 10) |
| `types.scrawl.patch.xml` | `Pen_Black` | nominal 0, min 0 | nominal 8, min 4 | Pens do not spawn on live; Scrawl cannot be used without one (Jeff, October 26, 2026, 22:06 and 22:27) |
| `types.scrawl.patch.xml` | `Pen_Blue` | nominal 0, min 0 | nominal 8, min 4 | As Pen_Black |
| `types.scrawl.patch.xml` | `Pen_Red` | nominal 0, min 0 | nominal 7, min 3 | As Pen_Black |
| `types.scrawl.patch.xml` | `Pen_Green` | nominal 0, min 0 | nominal 7, min 3 | As Pen_Black |
| `types.tack.patch.xml` | `NimbyTack_PinnedNote_Nail, _Tape, _Epoxy, _Knife` | absent | added: nominal 0, min 0, lifetime 2592000 | Tack pinned notes are server entities; without a types row the CE cleans them up (Tack plan, NIMBY-T-011) |
| `types.release1-loot.patch.xml` | `Whetstone` | nominal 70, min 65 | nominal 81, min 75 | Sharpening stones +15% (Jeff, October 26, 2026, 22:27) |
| `types.release1-loot.patch.xml` | `Barrel_Blue, Barrel_Green, Barrel_Red, Barrel_Yellow` | nominal 26, min 13 each | nominal 20, min 10 each | Barrels -25% (Jeff, October 26, 2026, 22:27) |
| `types.release1-loot.patch.xml` | `Mag_CMAG_10Rnd` | nominal 20, min 17 | nominal 13, min 11 | 10rnd MAG-C -33% (Jeff, October 26, 2026, 22:27; class confirmed /decide 4 of 10) |
| `types.release1-loot.patch.xml` | `Luger` | nominal 55, min 50 | nominal 37, min 34 | Luger P08 -33% (Jeff, October 26, 2026, 22:27) |
| `types.release1-loot.patch.xml` | `Mag_Luger_8Rnd` | nominal 75, min 65 | nominal 50, min 44 | Magazine moves with the pistol (Jeff, October 26, 2026, /decide 3 of 10) |
| `types.release1-loot.patch.xml` | `AKM, AugShort, FAMAS, M16A2 (crash-site rifles, deloot=1)` | nominal 4, min 2 each | nominal 2, min 1 each | Helicopter crash loot too profitable, lever 3: trim the pool (Jeff, October 26, 2026, /decide 5 of 10) |
| `types.release1-loot.patch.xml` | `Mag_AK74_45Rnd` | nominal 6, min 4 | nominal 4, min 3 | Crash-site magazines -33% (lever 3) |
| `types.release1-loot.patch.xml` | `Mag_Aug_30Rnd` | nominal 6, min 4 | nominal 4, min 3 | Crash-site magazines -33% (lever 3) |
| `types.release1-loot.patch.xml` | `Mag_FAMAS_25Rnd` | nominal 8, min 6 | nominal 5, min 4 | Crash-site magazines -33% (lever 3) |
| `types.release1-loot.patch.xml` | `Mag_PP19_64Rnd` | nominal 8, min 6 | nominal 5, min 4 | Crash-site magazines -33% (lever 3) |
| `types.release1-loot.patch.xml` | `Mag_CMAG_30Rnd` | nominal 4, min 2 | nominal 3, min 1 | Crash-site magazines -33% (lever 3) |
| `types.release1-loot.patch.xml` | `Mag_AKM_Drum75Rnd` | nominal 2, min 1 | nominal 1, min 1 | Crash-site magazines -33%, floor of 1 (lever 3) |
| `events.helicrash.patch.xml` | `StaticHeliCrash child StaticObj_Wreck_Mi24_TK_DE` | min 2, max 6, lootmin 10, lootmax 15 | min 1, max 3, lootmin 5, lootmax 8 | Helicopter crash loot too profitable, levers 1 and 2: fewer and thinner sites (Jeff, October 26, 2026, /decide 5 of 10) |
| `events.helicrash.patch.xml` | `StaticHeliCrash child Wreck_Mi8_Crashed` | min 2, max 6, lootmin 10, lootmax 15 | min 1, max 3, lootmin 5, lootmax 8 | As above |
| `mapgroupproto.cisterns.patch.xml` | `Land_Wreck_Truck01_Aban1_Cistern_DE` | usage Industrial; categories tools, clothes, weapons | usage Industrial, Village, Farm; categories tools, clothes, weapons, containers, food | Water trucks should carry clothes, bottles and food (Jeff, October 26, 2026, 22:27; usages ruled /decide 6 of 10) |
| `mapgroupproto.cisterns.patch.xml` | `Land_Wreck_Truck01_Aban2_Cistern_DE` | as above | as above | As above |

When applied, add a line here: `APPLIED <date and restart>, by <name>, from backup <panel backup name or zip>`, and move the values in `server/live/` forward with the next `Sync-LiveReplica.ps1` run so the mirror shows the new state.

## Helicopter crashes -33% (ruled October 27 and 28, 2026; applied October 28, 2026, 13:38 Central)

Ruled by: Jeff Fabish, October 27, 2026, 22:36 Central (/decide 2 of 8, after first proposing it at 21:25), for the Mi-24 and Mi-8 rows; Jeff Fabish, October 28, 2026 ("Cut Chinooks by a third too"), for the Chinook row. Drafted by: Claude (this session). Status: **APPLIED October 28, 2026, 13:38 Central (panel Restart, 0 players on the panel before it)**, by Jeff (SFTP upload of the patched `events.xml` with `logs/upload-heli-20261028.ps1`, 13:29 Central; server copy hash-identical to the patched file) and Claude (panel Restart, after Jeff logged out). Backup for this change: panel `manual-backup-2026-10-28_11-01.zip` (panel clock; taken 13:01 Central, October 28, 2026, server running, 0 players). The noon restart of October 28 passed before the PR merged.

| File | Entry | Before (live) | After | Why |
| --- | --- | --- | --- | --- |
| `events.helicrash.patch.xml` | `StaticHeliCrash child StaticObj_Wreck_Mi24_TK_DE` | min 1, max 3, lootmin 5, lootmax 8 | min 1, max 2, lootmin 5, lootmax 8 | Helicopter crashes still too common after release 1: another 33% off the site count (up to 4 sites map-wide instead of 6), loot per site unchanged (Jeff, October 27, 2026) |
| `events.helicrash.patch.xml` | `StaticHeliCrash child Wreck_Mi8_Crashed` | min 1, max 3, lootmin 5, lootmax 8 | min 1, max 2, lootmin 5, lootmax 8 | As above |
| `events.helicrash.patch.xml` | `StaticHeliCrashBig child StaticObj_Wreck_Chinook_DE` | min 6, max 6, lootmin 10, lootmax 15 | min 4, max 4, lootmin 10, lootmax 15 | Chinook crashes cut by a third as well (Jeff, October 28, 2026); loot per site unchanged |

Before (live) was confirmed against the live `events.xml` pulled at 13:29 Central on October 28, 2026: the upload script refuses to patch unless each tag reads exactly the Before value. The pulled and patched copies are kept in the git-ignored `logs/heli-20261028-132908/` (`live/`, `patched/`, `readback/`).

APPLIED October 28, 2026, 13:38 Central (panel Restart), by Jeff (upload) and Claude (restart), from backup panel `manual-backup-2026-10-28_11-01.zip`. After the restart the panel query and the DZSA launcher API both answered: map `nasdara`, 0/80, version 1.30.164200, @NIMBY 3828109139 offered. The `server/live/` copy of `events.xml` moves forward at the next `Sync-LiveReplica.ps1` run.

## Oasis military loot (ruled November 7, 2026; applied November 8, 2026, 13:17 Central)

Ruled by: Jeff Fabish, November 7, 2026, 19:35 Central (the ask: tone down the military loot at the Oasis; golden AK Oasis-only and very rare; plate carriers rare there; night vision bunker-only; LARs and VSDs rare there, fewer LAR buttstocks; anything golden very rare; Akinaka very rare). Figures ruled by Jeff at 19:55 Central (golden AK, Magnum and Deagle 2 or 3 per map counted everywhere, including survivors' inventories; Magnum_Gold 3; plate carriers 3/1 each; Akinaka 2/1). Drafted by: Claude (this session), from the live files mirrored October 26, 2026, 21:33 (`docs/operations/2026-11-07-oasis-military-loot.md`). Status: **APPLIED November 8, 2026, 13:17 Central (release 2 restart)**, see the APPLIED line below. Patch file: `db/types.oasis-military.patch.xml` (16 rows). Usage and value flags are unchanged on every row; the Oasis-only rows (Hunting + Unique, which only the three trade tents at grid 148 084 can take) stay Oasis-only.

"Counted everywhere" below means `count_in_cargo`, `count_in_hoarder`, `count_in_map` and `count_in_player` all `1` (live rows count on the map only, except `Deagle_Gold`, which already counted players).

| File | Entry | Before (live) | After | Why |
| --- | --- | --- | --- | --- |
| `types.oasis-military.patch.xml` | `AKM_Gold` | nominal 1, min 1, restock 2700, map only | nominal 2, min 1, restock 43200, counted everywhere | Golden AK: two on the server at a time, in hands and stashes included; when down to one, the next spawns no sooner than 12 h later; was respawning 45 min after every pickup (Jeff, November 7, 2026, 19:55) |
| `types.oasis-military.patch.xml` | `Mag_AKM_Drum75Rnd_Gold` | nominal 1, min 1, restock 7200, map only | nominal 2, min 1, restock 43200, counted everywhere | Golden drum, as the rifle |
| `types.oasis-military.patch.xml` | `Mag_AKM_30Rnd_Gold` | nominal 2, min 1, restock 0, map only | nominal 2, min 1, restock 43200, counted everywhere | Anything golden very rare: count unchanged but now counted everywhere with a 12 h wait; each golden AK still spawns with one attached |
| `types.oasis-military.patch.xml` | `Deagle_Gold` | nominal 5, min 2, restock 0, map and player | nominal 3, min 1, restock 43200, counted everywhere | Anything golden very rare (Jeff: 2 or 3 per map) |
| `types.oasis-military.patch.xml` | `Magnum_Gold` | nominal 24, min 20, restock 0, map only | nominal 3, min 1, restock 43200, counted everywhere | Anything golden very rare (Jeff ruled 3) |
| `types.oasis-military.patch.xml` | `FAL` | nominal 1, min 1, restock 3600, map only | nominal 1, min 0, restock 21600, counted everywhere | LAR rare at the Oasis: one at a time, 6 h wait |
| `types.oasis-military.patch.xml` | `Fal_FoldingBttstck` | nominal 3, min 1, restock 0 | nominal 1, min 0, restock 21600 | Too many LAR buttstocks: six to two across the two rows |
| `types.oasis-military.patch.xml` | `Fal_OeBttstck` | nominal 3, min 1, restock 0 | nominal 1, min 0, restock 21600 | As above |
| `types.oasis-military.patch.xml` | `Mag_FAL_20Rnd` | nominal 2, min 1, restock 2700 | nominal 1, min 0, restock 10800 | Moves with the rifle |
| `types.oasis-military.patch.xml` | `SVD_Wooden` | nominal 1, min 1, restock 1800, map only | nominal 1, min 0, restock 21600, counted everywhere | VSD rare at the Oasis; the crash-site `SVD` row (deloot 1) untouched |
| `types.oasis-military.patch.xml` | `PlateCarrierVest`, `PlateCarrierHolster`, `PlateCarrierPouches` | nominal 6, min 4, restock 0 each | nominal 3, min 1, restock 7200 each | Plate carriers rare at the Oasis (Jeff ruled 3/1); shared with the tunnel; every active bunker crate still holds a set |
| `types.oasis-military.patch.xml` | `NVGoggles` | nominal 2, min 1, restock 7200 | nominal 0, min 0, restock 0 | Night vision bunker-only: out of the loot economy, the active bunker crate (`StorageBox_Wooden`, chance 1.0) is the only source |
| `types.oasis-military.patch.xml` | `NVGHeadstrap` | nominal 4, min 2, restock 0 | nominal 0, min 0, restock 0 | As the goggles |
| `types.oasis-military.patch.xml` | `Akinaka` | nominal 45, min 35, restock 0 | nominal 2, min 1, restock 14400 | Akinaka knife very rare (Jeff ruled 2/1) |

Before applying, the live `types.xml` of the day was pulled and every Before value confirmed (`Deploy-MissionPatches.ps1` plan: 16 REPLACE, 0 FAIL; live hash `1C05ABE6...` identical to the November 8, 10:57 mirror).

APPLIED November 8, 2026, 13:17 Central (release 2 restart; server stopped 13:11 with 0 players, started 13:22), by Jeff (`scripts/Deploy-MissionPatches.ps1 -Upload`, run `logs/deploy-20261108-131721`, five files, this section's 16 rows among them, every readback hash-identical to the staged copy) and Claude (panel stop, backup, Workshop install, `greet.json` v2 upload, start), from backup panel `manual-backup-2026-11-08_11-12.zip` (panel clock; 13:12 Central). First RPT (`DayZServer_x64_2026-11-08_11-22-07.RPT`, panel clock): 0 `SCRIPT (E)`, 0 `Scripted variables corrupted`, 0 VME, every `[CE][Storage]` block `valid:yes`, 0 failed items; the 27 `Type ... will be ignored` lines are the same set as the previous boot (none on a patched class); `Loaded 40 dynamic events 201 total types` (39/200 before); `[CE][DE] [VehicleNimbyDpv] Spawning` four times, `NimbyDpv_Car` at 4739/7361, 6110/9142, 11875/10622, 12818/6755 (all in the ten-position list); `[NIMBY][Greet] greet.json loaded ... version=2 truncations=0`; `[NIMBY][Hands] loaded`. Live server build `1.30.164289` (the gate ran on `1.30.164078.38`). The `server/live/` copies move forward at the next `Sync-LiveReplica.ps1` run.

## Release 2: the DPV enters the economy, the 1911 and binoculars halved (ruled November 8, 2026; applied November 8, 2026, 13:17 Central)

Ruled by: Jeff Fabish, November 8, 2026, 01:30 to 01:40 Central (/interview after the 01:25 draft), on D-5 of `docs/design/dpv-feasibility.md` (November 5, 2026: nominal 4, max 6, military sites only, per-part chance). Drafted by: Claude (this session), from `server/live/` as mirrored October 26, 2026, 21:33 (`types.xml`, `events.xml`, `cfgspawnabletypes.xml`) and the stock October 15, 2026 `cfgeventspawns.xml` at `D:\DayZServer` (not mirrored until the November 8 `Sync-LiveReplica.ps1` change). Status: **APPLIED November 8, 2026, 13:17 Central (release 2 restart)**, see the APPLIED line below. Target was the release 2 restart, in the same restart as the @NIMBY Workshop update that carries NimbyDpv (the rows are meaningless to the CE without the addon loaded) and the Oasis rows (`types.oasis-military.patch.xml`, PR #22), applied with `scripts/Deploy-MissionPatches.ps1`, which pulls each live file first and refuses on any Before mismatch. Backup: panel `manual-backup-2026-11-08_11-12.zip`. First-12-hours check: the CE log shows no `Type NimbyDpv_Car will be ignored`; at least four DPVs stand at the ten positions; a DPV survives a restart with its rail, roof and cabin items.

| File | Entry | Before (live) | After | Why |
| --- | --- | --- | --- | --- |
| `types.dpv.patch.xml` | `NimbyDpv_Car` | no row | nominal 0, lifetime 3, restock 1800, min 0, vanilla car flags | Registers the class with the CE so it is counted and persists; the event owns the count (OffroadHatchback and Hatchback_02 pattern) |
| `types.dpv.patch.xml` | `NimbyDpv_Wheel` | no row | nominal 12, min 9, lifetime 28800, lootdispatch, Military and Industrial, Tier2 and Tier3 | The only source of spare DPV wheels, about three per DPV at nominal 4; pattern `Offroad_02_Wheel` 26/20 (Jeff: 12 / 9) |
| `types.dpv.patch.xml` | `NimbyDpv_Wheel_Ruined` | no row | nominal 0, lifetime 7200, crafted="1", lootdispatch | A ruined wheel is tracked and cleaned up like `HatchbackWheel_Ruined` |
| `cfgspawnabletypes.dpv.patch.xml` | `NimbyDpv_Car` block | no block | wheels 1.00 x4, spare 0.40, battery 0.60, spark plug 0.60, bulbs 0.40 x2; storage rolls rail can 0.30, rail bag 0.40, rail tent 0.25, roof 0.30 (bear or large storage, one group), first aid 0.40 | D-5 per-part chance: every DPV stands on its wheels (Jeff), the rest at hatchback values; storage at the harsher odds, one roll per area, about 1.6 pieces per DPV (Jeff) |
| `events.dpv.patch.xml` | `VehicleNimbyDpv` | no event | nominal 4, min 4, max 6, lifetime 300, radii 500/500/200, remove_damaged="1", position fixed, limit mixed, child min 4 max 6, no secondary | D-5 counts; min 4 so the CE tops the DPVs back up at every pass (Jeff); no `InfectedArmy` around a DPV (Jeff) |
| `cfgeventspawns.dpv.patch.xml` | `VehicleNimbyDpv` positions | no event | 10 positions: 029 091, 039 052, 043 129, 047 073, 061 091, 072 096, 078 118, 082 057, 118 106, 128 067 | Military sites only (D-5); the ten are known-good vanilla vehicle spots taken from the Humvee list, one per site (Jeff: split, no sharing) |
| `cfgeventspawns.dpv.patch.xml` | `VehicleOffroad02` positions | 21 positions | 11 positions (#1, 3, 5, 8, 9, 11, 12, 13, 15, 18, 20 of the stock list) | The ten moved to the DPV; the Humvee event (nominal 5, max 3, child max 5) keeps two spots per car |
| `types.release2-loot.patch.xml` | `Colt1911` | nominal 65, min 55 | nominal 33, min 28 | Colt 1911 halved (ruled October 21, 2026, held during vanilla testing; confirmed November 8, 2026: the whole 1911 family) |
| `types.release2-loot.patch.xml` | `Engraved1911` | nominal 10, min 7 | nominal 5, min 4 | As above |
| `types.release2-loot.patch.xml` | `Mag_1911_7Rnd` | nominal 70, min 60 | nominal 35, min 30 | As above; magazines keep the pistol ratio |
| `types.release2-loot.patch.xml` | `Binoculars` | nominal 45, min 40 | nominal 22, min 20 | Too many binoculars at the SW military checkpoint (carried in the hub since October 30, 2026; Jeff, November 8: half); usage flags unchanged |
| `cfgeconomycore.logging.patch.xml` | `defaults/log_ce_vehicle` | false | true | CE vehicle lifecycle lines in the RPT, so DPV and Humvee spawns can be verified (Jeff and Jay, November 8, 2026, 12:17) |
| `cfgeconomycore.logging.patch.xml` | `defaults/log_ce_dynamicevent` | false | true | CE event lines in the RPT (vehicle events included); set back to false at a later restart if noisy |

APPLIED November 8, 2026, 13:17 Central (release 2 restart; server stopped 13:11 with 0 players, started 13:22), by Jeff (`scripts/Deploy-MissionPatches.ps1 -Upload`, run `logs/deploy-20261108-131721`, five files, every readback hash-identical to the staged copy) and Claude (panel stop, backup, Workshop install, `greet.json` v2 upload, start), from backup panel `manual-backup-2026-11-08_11-12.zip` (panel clock; 13:12 Central). First RPT (`DayZServer_x64_2026-11-08_11-22-07.RPT`, panel clock): 0 `SCRIPT (E)`, 0 `Scripted variables corrupted`, 0 VME, every `[CE][Storage]` block `valid:yes`, 0 failed items; the 27 `Type ... will be ignored` lines are the same set as the previous boot (none on a patched class); `Loaded 40 dynamic events 201 total types` (39/200 before); `[CE][DE] [VehicleNimbyDpv] Spawning` four times, `NimbyDpv_Car` at 4739/7361, 6110/9142, 11875/10622, 12818/6755 (all in the ten-position list); `[NIMBY][Greet] greet.json loaded ... version=2 truncations=0`; `[NIMBY][Hands] loaded`. Live server build `1.30.164289` (the gate ran on `1.30.164078.38`). The `server/live/` copies move forward at the next `Sync-LiveReplica.ps1` run.

## Earlier changes

Changes made in the HostHavoc panel before this log existed (the October 15, 2026 wipe and map switch, CF removal, `lightingConfig`, the hardcore profile) are recorded in `docs/operations/` and the Notion hub, not here.
