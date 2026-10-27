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

## Earlier changes

Changes made in the HostHavoc panel before this log existed (the October 15, 2026 wipe and map switch, CF removal, `lightingConfig`, the hardcore profile) are recorded in `docs/operations/` and the Notion hub, not here.
