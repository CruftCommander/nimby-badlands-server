# Mission overrides

Files here are applied to the active mission folder on the server
(`mpmissions/<mission>/`). They are per-mission: re-apply them on every map change.

| File | Applies to | How |
| --- | --- | --- |
| `db/messages.xml` | `mpmissions/<mission>/db/messages.xml` | Replace the stock file. Update "Last wipe" on every wipe. |
| `cfggameplay.patch.json` | `mpmissions/<mission>/cfggameplay.json` | Merge onto the stock file: set only the listed keys. Keep the map's own temperatures and other values. |
| `db/types.scrawl.patch.xml` | `mpmissions/<mission>/db/types.xml` | DRAFT until Scrawl Phase 2. Replace the five `<type>` rows of the same name (Paper and the four pens); leave every other row. Back up `types.xml` first; apply at a scheduled restart. |
| `db/types.tack.patch.xml` | `mpmissions/<mission>/db/types.xml` | DRAFT, not applied. Add the four `NimbyTack_PinnedNote_*` rows at Tack Phase 2, after Scrawl's `types.scrawl.patch.xml`, at a restart with the file backed up. |
| `db/messages.nimby-mods.patch.xml` | `mpmissions/<mission>/db/messages.xml` | DRAFT, not applied. One on-connect how-to line per mod (Legs, Hands, Ears, Scrawl, Tack), 160 characters or fewer each. Copy a mod's `<message>` block into `db/messages.xml` at the restart that releases that mod, never before. |

## cfggameplay decisions (2026-10-14, updated 2026-10-15)

| Setting | Value | Vanilla | Why |
| --- | --- | --- | --- |
| `disablePersonalLight` | `true` | `false` on Chernarus, `true` on Nasdara | Hardcore profile; matches `serverDZ.cfg` so neither file can undo the other |
| `lightingConfig` | not patched on Nasdara (stock `3`) | `0` on Chernarus, `3` on Nasdara | Chernarus used `1` (dark nights). Nasdara ships an undocumented `3`, likely a map-specific profile; kept stock on 2026-10-15 pending a night check in game. |
| Hit direction indicator | off (`hitDirectionOverrideEnabled = true`, `hitDirectionBehaviour = 0`) | static splash | Locate fire by sound; the red-flash hit effect stays on |
| `staminaWeightLimitThreshold` | `5000` (5 kg) | `6000` | Slightly harsher weight penalty |
| `staminaKgToStaminaPercentPenalty` | `2.0` | `1.75` | At 15 kg carried: 80% max stamina vs 84.25% vanilla |
| `disableIsCollidingAngleCheck`, `disableIsCollidingBBoxCheck` | `true` | `false` | More base-building spots; watch for glitch bases (rule 1) |

Deferred: spawn-gear presets ship with theme drop 1 on Nasdara. Loot (`types.xml`) tuning
starts at Nasdara launch from Nasdara's stock file.
