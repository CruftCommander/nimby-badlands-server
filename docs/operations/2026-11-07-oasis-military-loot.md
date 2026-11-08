# Oasis military loot (November 7, 2026)

Jeff's ask, November 7, 2026, 19:35: tone down the military loot at the Oasis. Players keep finding the golden AK and its drum magazine, plate carriers, night vision, LARs and VSDs (and far too many LAR buttstocks) there; anything golden and the Akinaka knife should be very rare; the golden AK should stay Oasis-only; night vision should be bunker-only.

Live values below were read from `server/live/db/types.xml` (mirrored October 26, 2026, 21:33) and the replica's `mapgroupproto.xml`, `mapgrouppos.xml`, `areaflags.map`, `cfgspawnabletypes.xml` and `cfgbunkerbroadcast.json` at `D:\DayZServer\mpmissions\dayzOffline.nasdara\`. Jeff ruled the figures at 19:55 (section 4). The rows are in `server/mission-overrides/db/types.oasis-military.patch.xml` (DRAFT, not on live). Status and the per-row log are in `server/mission-overrides/LOOT-CHANGELOG.md`.

## 1. What the Oasis is, in loot terms

The Oasis is the caravanserai at Qalat-e Lut, grid 148 084 (world x 14752 to 14960, z 7768 to 8048). Read from the mission files:

| Fact | Evidence |
| --- | --- |
| It is one of only two places on the map painted with the `Unique` value flag. The other is the Fostkova Mehran road tunnel (the contaminated area, grid 055 107). | `areaflags.map`: header `4096 4096 16384 16384 32 0`; the 8-bit value layer has 5305 `Unique` cells in two usable clusters (plus two stray slivers at the map edge with no buildings). |
| The Oasis area carries usage `Military`, `Hunting`, `MilitaryEast` and values `Tier1`, `Tier2`, `Tier3`, `Unique`. | Same file, cells inside the cluster. |
| Its buildings: `Land_Caravanserai_Front` (155 loot points) and `_Back` (78), usage `Military`; `Land_Mil_Tent_Big2_2_Desert` (27) and `Land_Mil_Fortified_Nest_Watchtower_Desert` (11), `Military`; three trade tents `Land_TradTent_Medium2`, `_Medium3`, `_Small` (6, 4 and 5 points, 15 in all), usage `Hunting`. | `mapgrouppos.xml` positions inside the cluster; usages from `mapgroupproto.xml`. |
| The three trade tents are the only `Hunting`-usage loot points anywhere inside a `Unique` zone. | No other Hunting group sits inside either cluster. |

So every `types.xml` row flagged `Hunting` plus `Unique` can spawn in exactly 15 loot points on the whole map, all at the Oasis. That pool is: `AKM_Gold`, `Mag_AKM_30Rnd_Gold`, `Mag_AKM_Drum75Rnd_Gold`, `AK_Suppressor`, `FAL`, `Fal_FoldingBttstck`, `Fal_OeBttstck`, `Mag_FAL_20Rnd`, `SVD_Wooden`, `M79`, and (also flagged `ContaminatedArea`, so the tunnel too) `NVGoggles`, `NVGHeadstrap`, `PlateCarrierVest`, `PlateCarrierHolster`, `PlateCarrierPouches`. That is why the tents read like an armoury: 15 points, one small pool, and `min` equal to `nominal` on the rifles.

Why the golden AK is "always" there: `AKM_Gold` is nominal 1, min 1, restock 2700, counted on the map only (`count_in_player="0"`). The moment a player picks it up the map count is 0, below min, and the CE puts a new one in the tents 45 minutes later. The LAR (`FAL` 1/1, restock 3600), the wooden VSD (`SVD_Wooden` 1/1, restock 1800) and the golden drum (1/1, restock 7200) work the same way. The two LAR buttstocks are 3/1 each, six buttstocks chasing 15 points.

The caravanserai itself (233 `Military` points at Tier 1 to 3) is ordinary military loot, the same table as any Tier 3 base; nothing below touches it. Say so if that is the part to tone down next.

## 2. Night vision already has a bunker-only path

Each of the three active broadcast bunkers spawns a `StorageBox_Wooden` crate when it activates (`vendor/dayz-1.30-scripts/scripts/4_World/DayZ/Entities/Building/Bunker.c`: `BroadcastedBunker.SpawnStorageBox`, `ECE_EQUIP_CARGO`; all six `Land_Bunker_*` classes derive from `BroadcastedBunker`, lines 331 to 336), and `cfgspawnabletypes.xml` fills that crate with, at chance 1.0 each: a gun case, `PlateCarrierVest`, `PlateCarrierPouches`, `PlateCarrierHolster`, two `Battery9V`, `NVGHeadstrap`, `NVGoggles` and `BallisticHelmet_Desert`. A batch lasts 720 minutes (`cfgbunkerbroadcast.json` `batchLifetimeLength`, counted in minutes by `BunkerBroadcastScheduler.c`); three bunkers are active at a time out of 23.

So "night vision, bunker only" needs no new flag and no `mapgroupproto.xml` edit: set `NVGoggles` and `NVGHeadstrap` to nominal 0 in `types.xml` and the crate remains their only source. (The bunkers themselves have plain `Military` usage at Tier 1 to 3, with no flag of their own; the only way to make a loot-table item bunker-only would be a `mapgroupproto.xml` edit on the six `Land_Bunker_*` groups. Not needed here.) The one other NVG source is the `Xmas_Lights_Small` random preset at chance 0.01, which only seasonal gift boxes use, and those are nominal 0 on Nasdara.

## 3. `types.xml` rows (as ruled)

Only nominal, min, restock and the `count_in_*` flags change. Usage and value flags are untouched, so every Oasis-only item stays Oasis-only.

How the levers work: `nominal` is the target count; respawning starts when the count falls to `min`; `restock` is the wait, in seconds, before the CE spawns the item again; the `count_in_*` flags decide what is counted (`count_in_player="1"` counts the one in a player's hands, `count_in_cargo="1"` the one in a tent or vehicle, `count_in_hoarder="1"` the one in a buried stash). Nominal 2, min 1, all four flags on, means two in the world at a time wherever they are, and when the count falls to one the next spawns only after `restock` seconds. Nominal 1, min 0, all four flags on, means one in the world at a time, and the next one spawns only after `restock` seconds once that one is gone (ruined, despawned, or on a character the hive no longer counts).

| Ask | Class | Live nominal / min / restock | Ruled | Note |
| --- | --- | --- | --- | --- |
| Golden AK: Oasis only, very rare | `AKM_Gold` | 1 / 1 / 2700, map only | 2 / 1 / 43200, counted everywhere | Two on the server at a time, hands and stashes included; when down to one, the next no sooner than 12 h later. Spawn place unchanged (Hunting + Unique: the three tents) |
| Golden drum | `Mag_AKM_Drum75Rnd_Gold` | 1 / 1 / 7200, map only | 2 / 1 / 43200, counted everywhere | As the rifle |
| Anything golden | `Mag_AKM_30Rnd_Gold` | 2 / 1 / 0, map only | 2 / 1 / 43200, counted everywhere | Count unchanged, now counted everywhere with the 12 h wait; each golden AK still arrives with one attached (`cfgspawnabletypes`) |
| Anything golden | `Deagle_Gold` | 5 / 2 / 0, map and player | 3 / 1 / 43200, counted everywhere | Towns map-wide; three at a time |
| Anything golden | `Magnum_Gold` | 24 / 20 / 0, map only | 3 / 1 / 43200, counted everywhere | Farm, Town, Village, Hunting; three at a time (Jeff ruled 3) |
| LAR rare at the Oasis | `FAL` | 1 / 1 / 3600, map only | 1 / 0 / 21600, counted everywhere | One at a time, 6 h wait |
| Too many LAR buttstocks | `Fal_FoldingBttstck` | 3 / 1 / 0 | 1 / 0 / 21600 | From six buttstocks to two across the two rows |
| Too many LAR buttstocks | `Fal_OeBttstck` | 3 / 1 / 0 | 1 / 0 / 21600 | |
| LAR | `Mag_FAL_20Rnd` | 2 / 1 / 2700 | 1 / 0 / 10800 | |
| VSD rare at the Oasis | `SVD_Wooden` | 1 / 1 / 1800, map only | 1 / 0 / 21600, counted everywhere | The Oasis VSD. The black `SVD` row is `deloot="1"` (crash sites only, 2 / 1) and is not touched |
| Plate carriers rare at the Oasis | `PlateCarrierVest`, `PlateCarrierHolster`, `PlateCarrierPouches` | 6 / 4 / 0 each | 3 / 1 / 7200 each | Shared between the tents and the tunnel; nominal is map-wide, so this thins both (Jeff ruled 3 / 1). Every active bunker crate still holds a full set. The camo variants (`_Camo`, Military Tier 3, 2 / 0) are untouched |
| Night vision bunker only | `NVGoggles` | 2 / 1 / 7200 | 0 / 0 / 0 | Out of the loot economy; bunker crate only (section 2) |
| Night vision bunker only | `NVGHeadstrap` | 4 / 2 / 0 | 0 / 0 / 0 | As the goggles |
| Akinaka very rare | `Akinaka` | 45 / 35 / 0 | 2 / 1 / 14400 | Town, Village, Hunting; two on the map, 4 h respawn wait (Jeff ruled 2 / 1) |

Not touched, for the record: `AK_Suppressor` (Hunting + Unique, 2 / 1) and `M79` (Hunting + ContaminatedArea + Unique, 3 / 1) also live in the tents and were not asked about; `AK_WoodBttstck_Gold` and `AK_WoodHndgrd_Gold` are 0 / 0 and arrive attached to the rifle; `SawedoffMagnum_Gold` is crafted.

## 4. Rulings (Jeff, November 7, 2026, 19:55 Central)

1. Golden AK, Magnum and Deagle: 2 or 3 in total per map, counted everywhere including survivors' inventories. Applied as `AKM_Gold` 2 (with its two golden magazines at 2), `Deagle_Gold` 3, `Magnum_Gold` 3, all min 1 and a 12 h restock. The LAR and the wooden VSD keep the drafted one-in-the-world treatment (1 / 0, counted everywhere, 6 h).
2. `Magnum_Gold`: 3.
3. Plate carriers: 3 / 1 each.
4. Night vision at nominal 0 (bunker crate only): not objected to; stands as drafted.
5. `Akinaka`: 2 / 1.

Still open: the go-ahead to apply, and the restart to apply at.

## 5. Apply (inside a scheduled restart, Jeff's approval first)

1. Pull the live `types.xml` fresh (`scripts\Get-ServerLogs.ps1` does logs only; use the SFTP pull from the October 28 heli change, `logs/heli-20261028-132908/` pattern) and confirm every Before value in section 3 against it; refuse any row that differs.
2. Panel backup of the server (storage and mission) while it runs; note the backup name in the changelog.
3. Replace the 16 `<type>` rows by name from `types.oasis-military.patch.xml`; leave every other row. Validate the XML opens.
4. Restart inside the chain (`docs/operations/restart-schedule.md`). Watch the first CE log for `Type ... will be ignored` lines naming any edited class.
5. Mark the changelog section APPLIED; run `Sync-LiveReplica.ps1` so `server/live/` moves forward.

Players holding a golden AK, LAR or VSD today keep it. With the counted-everywhere flags, every one already in a hand, tent or stash counts against the nominal (2 for the golden AK and its magazines, 3 for the golden pistols, 1 for the LAR and the wooden VSD), so those rows spawn again only once the existing count is below it; that is the intended effect, not a fault.

## 6. Check after the first restart cycle

- The tents at grid 148 084 after 12 hours: no more than two golden AKs and one LAR or wooden VSD have appeared in total, and none reappears within its restock window while the count is still at nominal (the ADM log shows who picked it up).
- An opened bunker crate still holds goggles and headstrap.
- No `will be ignored` line for the 16 classes in the CE log.
