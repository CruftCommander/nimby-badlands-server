# Mission overrides

Files here are applied to the active mission folder on the server
(`mpmissions/<mission>/`). They are per-mission: re-apply them on every map change.

| File | Applies to | How |
| --- | --- | --- |
| `LOOT-CHANGELOG.md` | (record) | Every loot change: who, before and after values, when, why. Add the row before the panel edit, mark it APPLIED after (Jeff's rule, October 26, 2026). |
| `db/messages.xml` | `mpmissions/<mission>/db/messages.xml` | Replace the stock file. Update "Last wipe" on every wipe. |
| `cfggameplay.patch.json` | `mpmissions/<mission>/cfggameplay.json` | Merge onto the stock file: set only the listed keys. Keep the map's own temperatures and other values. |
| `db/types.scrawl.patch.xml` | `mpmissions/<mission>/db/types.xml` | APPLIED October 26, 2026 (release 1 restart); was DRAFT until Scrawl Phase 2. Replace the five `<type>` rows of the same name (Paper and the four pens); leave every other row. Back up `types.xml` first; apply at a scheduled restart. |
| `db/types.tack.patch.xml` | `mpmissions/<mission>/db/types.xml` | APPLIED October 26, 2026 (release 1 restart); was DRAFT. Add the four `NimbyTack_PinnedNote_*` rows at Tack Phase 2, after Scrawl's `types.scrawl.patch.xml`, at a restart with the file backed up. |
| `db/types.release1-loot.patch.xml` | `mpmissions/<mission>/db/types.xml` | DRAFT for the release 1 restart (Jeff, October 26, 2026; `docs/operations/2026-10-26-release-1-loot-update.md`). Replace the 18 `<type>` rows of the same name (Whetstone, the four barrels, `Mag_CMAG_10Rnd`, Luger and its magazine, four crash-site rifles, six crash-site magazines); only nominal and min differ from live. Apply after the Scrawl and Tack patches, same restart. |
| `db/events.helicrash.patch.xml` | `mpmissions/<mission>/db/events.xml` | DRAFT for the release 1 restart. Replace the `StaticHeliCrash` event: each wreck child min 1 max 3, lootmin 5 lootmax 8. |
| `mapgroupproto.cisterns.patch.xml` | `mpmissions/<mission>/mapgroupproto.xml` | APPLIED October 26, 2026 (release 1 restart); a partial record (changed lines only, no `<point>` rows), never a `Deploy-MissionPatches.ps1` input. Was DRAFT for the release 1 restart. Add `Village` and `Farm` usage and the `containers` and `food` categories to the two `Land_Wreck_Truck01_Aban*_Cistern_DE` groups; points and dispatch unchanged. |
| `db/messages.nimby-mods.patch.xml` | `mpmissions/<mission>/db/messages.xml` | SUPERSEDED, kept for reference (Greet plan G8, October 26, 2026). One on-connect how-to line per mod (Legs, Hands, Ears, Scrawl, Tack). Not copied into `db/messages.xml`: Greet's Mods tab carries the how-to text, updated at each mod's release (release gate step 5). |
| `db/types.dpv.patch.xml` | `mpmissions/<mission>/db/types.xml` | DRAFT for the release 2 restart (Jeff's rulings, November 8, 2026; D-5 in `docs/design/dpv-feasibility.md`). Add three rows: `NimbyDpv_Car` (nominal 0, the vanilla event-spawned car pattern), `NimbyDpv_Wheel` (12 / 9, Military and Industrial, Tier 2 to 3) and `NimbyDpv_Wheel_Ruined` (0, lifetime 7200). Apply after the @NIMBY Workshop update that carries NimbyDpv, same restart; refuse if a row already exists. |
| `cfgspawnabletypes.dpv.patch.xml` | `mpmissions/<mission>/cfgspawnabletypes.xml` | DRAFT for the release 2 restart. Add the `NimbyDpv_Car` block: four road wheels at 1.00, spare 0.40, battery 0.60, spark plug 0.60, two bulbs 0.40; one storage roll per area (rail can 0.30, rail bag 0.40, rail tent 0.25, roof 0.30, first aid 0.40). Same restart as the types rows. |
| `db/events.dpv.patch.xml` | `mpmissions/<mission>/db/events.xml` | DRAFT for the release 2 restart. Add the `VehicleNimbyDpv` event: nominal 4, min 4, max 6, lifetime 300, no secondary infected; shape of `VehicleOffroad02`. Same restart. |
| `cfgeventspawns.dpv.patch.xml` | `mpmissions/<mission>/cfgeventspawns.xml` | DRAFT for the release 2 restart. Add the `VehicleNimbyDpv` event with 10 positions taken from the Humvee's list, and replace the `VehicleOffroad02` event with the 11 it keeps (no position is shared). Pull the live file first: this file is not in `server/live/` before the November 8 `Sync-LiveReplica.ps1` change. Same restart. |
| `db/types.release2-loot.patch.xml` | `mpmissions/<mission>/db/types.xml` | DRAFT for the release 2 restart (Jeff, November 8, 2026). Replace the four `<type>` rows of the same name (`Colt1911` 33 / 28, `Engraved1911` 5 / 4, `Mag_1911_7Rnd` 35 / 30, `Binoculars` 22 / 20); only nominal and min differ from live. Same restart as the DPV and Oasis rows. |
| `cfgeconomycore.logging.patch.xml` | `mpmissions/<mission>/cfgeconomycore.xml` | DRAFT for the release 2 restart (Jeff and Jay, November 8, 2026). Replace two `<default>` rows under `<defaults>`: `log_ce_vehicle` and `log_ce_dynamicevent` from `false` to `true`, so the CE writes vehicle and event lifecycle lines to the RPT and a missing DPV or Humvee can be read from the log. Same restart. |

## cfggameplay decisions (2026-10-14, updated 2026-10-15)

| Setting | Value | Vanilla | Why |
| --- | --- | --- | --- |
| `disablePersonalLight` | `true` | `false` on Chernarus, `true` on Nasdara | Hardcore profile; matches `serverDZ.cfg` so neither file can undo the other |
| `lightingConfig` | not patched on Nasdara (stock `3`) | `0` on Chernarus, `3` on Nasdara | Chernarus used `1` (dark nights). Nasdara ships an undocumented `3`, likely a map-specific profile; kept stock on 2026-10-15 pending a night check in game. |
| Hit direction indicator | off (`hitDirectionOverrideEnabled = true`, `hitDirectionBehaviour = 0`) | static splash | Locate fire by sound; the red-flash hit effect stays on |
| `staminaWeightLimitThreshold` | `5000` (5 kg) | `6000` | Slightly harsher weight penalty |
| `staminaKgToStaminaPercentPenalty` | `2.0` | `1.75` | At 15 kg carried: 80% max stamina vs 84.25% vanilla |
| `disableIsCollidingAngleCheck`, `disableIsCollidingBBoxCheck` | `true` | `false` | More base-building spots. Building through walls, floors or terrain is an admin call under the one rule, not a written exception (Jeff, October 25, 2026) |

Deferred: spawn-gear presets ship with theme drop 1 on Nasdara. Loot (`types.xml`) tuning
starts at Nasdara launch from Nasdara's stock file.
