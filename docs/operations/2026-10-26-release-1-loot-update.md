# Release 1 loot update (October 26, 2026)

Jeff's ruling, October 26, 2026, 22:27: the first release bundles a loot-economy update, applied in one edit at the release restart (gate step 5, `docs/operations/2026-10-20-release-gate-legs-ears-scrawl.md`). Live values below were read from `server/live/db/types.xml`, `server/live/db/events.xml` and the replica's `mapgroupproto.xml`, all mirrored from live on October 26, 2026, 21:18. Proposed numbers are Claude's arithmetic on Jeff's percentages, rounded to the nearest whole number; Jeff rules each row before the panel edit. Nothing here is applied yet.

## 1. `types.xml` rows (HostHavoc panel, `mpmissions/dayzOffline.nasdara/db/types.xml`)

| Ask | Class | Live nominal / min | Proposed nominal / min | Note |
| --- | --- | --- | --- | --- |
| Pens spawn | `Pen_Black`, `Pen_Blue` | 0 / 0 | 8 / 4 each | From `server/mission-overrides/db/types.scrawl.patch.xml` (usage Town, Village, Office, School) |
| Pens spawn | `Pen_Red`, `Pen_Green` | 0 / 0 | 7 / 3 each | Same patch |
| Paper in the table for persistence | `Paper` | 0 / 0 | 30 / 15, lifetime 2592000 | Same patch. Jeff, October 26: paper can be crafted, spawning is optional, but the row must exist and carry a lifetime so written notes persist. Keep the patch's 30 or set nominal 0 with the 30-day lifetime; Jeff rules |
| Tack persistence | `NimbyTack_PinnedNote_Nail`, `_Tape`, `_Epoxy`, `_Knife` | absent | added, nominal 0, lifetime 2592000 | `types.tack.patch.xml`; required so pinned notes are not cleaned up |
| Sharpening stone +15% | `Whetstone` | 70 / 65 | 81 / 75 | 70 x 1.15 = 80.5; 65 x 1.15 = 74.75 |
| Barrels -25% | `Barrel_Blue`, `Barrel_Green`, `Barrel_Red`, `Barrel_Yellow` | 26 / 13 each | 20 / 10 each | 26 x 0.75 = 19.5; 13 x 0.75 = 9.75. `BarrelHoles_*` are crafted (nominal 0), untouched |
| 10rnd MAG-C -33% | `Mag_CMAG_10Rnd` | 20 / 17 | 13 / 11 | 20 x 0.67 = 13.4; 17 x 0.67 = 11.39. Read as the C-MAG 10-round; say so if a different magazine was meant |
| Luger P08 -33% | `Luger` | 55 / 50 | 37 / 34 | 55 x 0.67 = 36.85; 50 x 0.67 = 33.5 |
| Luger P08 -33% (magazine) | `Mag_Luger_8Rnd` | 75 / 65 | 50 / 44 | Proposed to move with the pistol; Jeff rules |

## 2. Report only

- **Humvee.** `events.xml` `VehicleOffroad02`: `active=1`, nominal 5, min 2, max 3, one child `Offroad_02_Beige` min 5 max 5, `position=fixed`, lifetime 300. It is configured to spawn (the CE keeps 2 to 3 wrecks-to-repair up; nominal 5 above max 3 is a live oddity, harmless). `types.xml` `Offroad_02` and `Offroad_02_Beige` nominal 0 is normal for event-spawned vehicles. Only the `BeigeRust` doors and trunk have nominal (4 / 2 each); the plain and `Beige` door and trunk rows are 0, so a found Humvee can only be completed with rust parts. Whether any is standing in the world right now needs the live CE state (the panel's console or a `storage_1` dump), not `types.xml`.
- **Golden AK.** `AKM_Gold` nominal 1, min 1, usage Hunting, tier Unique, lifetime 14400; `Mag_AKM_30Rnd_Gold` 2 / 1; `Mag_AKM_Drum75Rnd_Gold` 1 / 1; `AK_WoodBttstck_Gold` and `AK_WoodHndgrd_Gold` 0 / 0. Configured to keep one in the world at hunting locations. As above, presence right now needs the live CE state.

## 3. Water (cistern) trucks: `mapgroupproto.xml`, not `types.xml`

The water trucks are the proto groups `Land_Wreck_Truck01_Aban1_Cistern_DE` (4 points, one container, lootmax 4 and 2) and `Land_Wreck_Truck01_Aban2_Cistern_DE` (5 points, lootmax 5 and 2), usage `Industrial` only. Clothes, bottles and food come from `Town`, `Village` and `Farm` usage, so the change is to add `<usage name="Village"/>` (and `Farm` if food is wanted) to those two groups' containers in `mapgroupproto.xml`. This file is re-applied on every map change, so it goes into `server/mission-overrides/` as a patch and into the panel edit. Placement count: the groups did not match by name in `mapgrouppos.xml`; count them before the edit (`mapgroupcluster*.xml` or a case-insensitive search) so the effect size is known.

## 4. Military helicopter crash loot

`events.xml` `StaticHeliCrash`: nominal 8, min 0, max 0, lifetime 2100 s (35 min), `limit=child`, `position=fixed`, two children: `StaticObj_Wreck_Mi24_TK_DE` and `Wreck_Mi8_Crashed`, each min 2 max 6, lootmin 10, lootmax 15. So 4 to 12 crash sites at once, each with 10 to 15 items. The crash-site item pool is the 48 `deloot="1"` types in `types.xml` (nominal sum 238): the heaviest rows are `GasMask` 40, `AmmoBox_556x45_20Rnd` 26, the five `NBC*White` pieces 10 each, `Mag_FAMAS_25Rnd` 8, `Mag_PP19_64Rnd` 8, `PP19` 7, then the rifles `AKM`, `AugShort`, `FAMAS`, `M16A2` at 4 each.

Three levers, in order of bluntness; Jeff picks:

1. Fewer sites: children min 2 max 6 each, to min 1 max 3 each (2 to 6 sites at once). Halves the supply without touching what a site can hold.
2. Thinner sites: lootmin 10 lootmax 15, to lootmin 5 lootmax 8. Each crash stays worth a walk, never a full kit.
3. Trim the pool: lower the rifle and magazine rows in the `deloot` set (for example `AKM`, `AugShort`, `FAMAS`, `M16A2` from 4 / 2 to 2 / 1; the large magazines by a third) and leave the NBC and gas-mask rows, which are the crash sites' distinctive loot.

Recommendation: 1 and 2 together for release 1, 3 held until a week of play shows whether sites are still "too profitable".

## 5. Discord

A release post goes out with the restart (gate step 8). Draft to follow once the scope and these rows are ruled.

## 6. Order at the panel (gate step 5)

Stop the server inside the restart window; back up `types.xml`, `events.xml` and `mapgroupproto.xml` from the panel (step 4 covers storage and profiles); apply the rows above; validate the XML opens; start; watch the first CE log for `Type ... will be ignored` lines naming any edited class.
