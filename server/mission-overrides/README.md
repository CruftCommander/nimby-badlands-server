# Mission overrides

Files here are applied to the active mission folder on the server
(`mpmissions/<mission>/`). They are per-mission: re-apply them on every map change.

| File | Applies to | How |
| --- | --- | --- |
| `LOOT-CHANGELOG.md` | (record) | Every loot change: who, before and after values, when, why. Add the row before the panel edit, mark it APPLIED after (Jeff's rule, October 26, 2026). |
| `db/messages.xml` | `mpmissions/<mission>/db/messages.xml` | Replace the stock file. Update "Last wipe" on every wipe. |
| `cfggameplay.patch.json` | `mpmissions/<mission>/cfggameplay.json` | Merge onto the stock file: set only the listed keys. Keep the map's own temperatures and other values. |
| `db/types.scrawl.patch.xml` | `mpmissions/<mission>/db/types.xml` | DRAFT until Scrawl Phase 2. Replace the five `<type>` rows of the same name (Paper and the four pens); leave every other row. Back up `types.xml` first; apply at a scheduled restart. |
| `db/types.tack.patch.xml` | `mpmissions/<mission>/db/types.xml` | DRAFT, not applied. Add the four `NimbyTack_PinnedNote_*` rows at Tack Phase 2, after Scrawl's `types.scrawl.patch.xml`, at a restart with the file backed up. |
| `db/types.release1-loot.patch.xml` | `mpmissions/<mission>/db/types.xml` | DRAFT for the release 1 restart (Jeff, October 26, 2026; `docs/operations/2026-10-26-release-1-loot-update.md`). Replace the 18 `<type>` rows of the same name (Whetstone, the four barrels, `Mag_CMAG_10Rnd`, Luger and its magazine, four crash-site rifles, six crash-site magazines); only nominal and min differ from live. Apply after the Scrawl and Tack patches, same restart. |
| `db/events.helicrash.patch.xml` | `mpmissions/<mission>/db/events.xml` | DRAFT for the release 1 restart. Replace the `StaticHeliCrash` event: each wreck child min 1 max 3, lootmin 5 lootmax 8. |
| `mapgroupproto.cisterns.patch.xml` | `mpmissions/<mission>/mapgroupproto.xml` | DRAFT for the release 1 restart. Add `Village` and `Farm` usage and the `containers` and `food` categories to the two `Land_Wreck_Truck01_Aban*_Cistern_DE` groups; points and dispatch unchanged. |
| `db/messages.nimby-mods.patch.xml` | `mpmissions/<mission>/db/messages.xml` | SUPERSEDED, kept for reference (Greet plan G8, October 26, 2026). One on-connect how-to line per mod (Legs, Hands, Ears, Scrawl, Tack). Not copied into `db/messages.xml`: Greet's Mods tab carries the how-to text, updated at each mod's release (release gate step 5). |

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
