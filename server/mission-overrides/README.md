# Mission overrides

Files here are applied to the active mission folder on the server
(`mpmissions/<mission>/`). They are per-mission: re-apply them on every map change.

| File | Applies to | How |
| --- | --- | --- |
| `db/messages.xml` | `mpmissions/<mission>/db/messages.xml` | Replace the stock file. Update "Last wipe" on every wipe. |
| `cfggameplay.patch.json` | `mpmissions/<mission>/cfggameplay.json` | Merge onto the stock file: set only the listed keys. Keep the map's own temperatures and other values. |

## cfggameplay decisions (2026-10-14)

| Setting | Value | Vanilla | Why |
| --- | --- | --- | --- |
| `disablePersonalLight` | `true` | `false` | Hardcore profile; matches `serverDZ.cfg` so neither file can undo the other |
| `lightingConfig` | `1` | `0` | Hardcore profile (dark nights); matches `serverDZ.cfg` |
| Hit direction indicator | off (`hitDirectionOverrideEnabled = true`, `hitDirectionBehaviour = 0`) | static splash | Locate fire by sound; the red-flash hit effect stays on |
| `staminaWeightLimitThreshold` | `5000` (5 kg) | `6000` | Slightly harsher weight penalty |
| `staminaKgToStaminaPercentPenalty` | `2.0` | `1.75` | At 15 kg carried: 80% max stamina vs 84.25% vanilla |
| `disableIsCollidingAngleCheck`, `disableIsCollidingBBoxCheck` | `true` | `false` | More base-building spots; watch for glitch bases (rule 1) |

Deferred: spawn-gear presets ship with theme drop 1 on Nasdara. Loot (`types.xml`) tuning
starts at Nasdara launch from Nasdara's stock file.
