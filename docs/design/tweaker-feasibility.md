# Tweaker feasibility - dust-storm dehydration, tanker fuel, cactus wounds, well rope

Draft v0.1, October 21, 2026. Owner: Jeff Fabish. Status: feasibility only, ordered by Jeff on October 21, 2026 together with the name. Display name Tweaker, code prefix `NimbyTweaker`, log prefix `[NIMBY][Tweaker]`, RPC block `65512xx` (reserved, nothing below needs an RPC). Not ordered on the mod board; a plan follows when Jeff orders it. Every `file:line` is relative to `vendor/dayz-1.30-scripts/scripts/` (build 1.30.164078.38) unless stated; Nasdara class and model names were read from the PBO headers under `D:\DayZServer\nasdara\addons\` on October 21, 2026.

Target: NIMBY | HARDCORE | BADLANDS | 1pp, HostHavoc, DayZ 1.30 / Nasdara.

## Verdict

All four ideas are feasible as one server-side addon on vanilla hooks, and none needs a persistence change, so Tweaker is not a content addon under shared rules 6.1. The surprises: the two abandoned cistern trucks on Nasdara are already `Well` subclasses in vanilla 1.30 (`Land_Wreck_Truck01_Aban1_Cistern : Well`), so "draw fuel from a wreck" has a one-line precedent and the work is deciding which wrecks give gasoline and how much; the 1.30 sandstorm exposes a per-position intensity (`SandstormController.GetIntensity(worldPosition)`), so dehydration can scale with how deep in the storm the player stands; the Nasdara brick well is a vanilla `Rebuilding` construction whose water exists only while its `roof` part is built, so "cut the rope" is a dismantle of that part and vanilla already persists it. Cactus wounds are the only item that needs a proximity sweep, because cacti are map vegetation with no script class; the sweep is cheap if it runs only while the player sprints or runs and only on Nasdara.

The one design question with consequences is the tanker fuel model: map statics are not persisted, so a wreck's remaining fuel lives in server memory and resets at every restart (twice a day), or in a profile-folder JSON keyed by position. The study recommends server memory with a restart reset and a per-wreck daily cap, because that matches how Nasdara's cistern wells already behave (infinite, no state) while still making a tanker a place rather than a pump.

## What this is not

- Not a content addon. No new item, no new persistent class, no `OnStoreSave`/`OnStoreLoad`. The release gate is shared rules 9.2 alone.
- Not Buzz or Sprout. An idea that touches smoking, growing, or notes moves to the addon that owns it (candidate-features rule).
- Not a client mod for the first three items. Dehydration, fuel, and the well run on the server; cactus wounds also run on the server. Tweaker ships as a client-and-server PBO only if a later idea needs client code; today it is server-side script that still has to be in the Workshop item because players' clients must load the same `config.cpp` for the modded well and wreck classes.
- Not balanced here. Every number below is a starting value in `$profile:NimbyTweaker\config.json`, each idea independently switchable, so a bad one is turned off at the next restart without a client update.

## Feasibility by idea

| # | Idea | Verdict | How | Source |
| --- | --- | --- | --- | --- |
| T1 | Dust-storm dehydration | Yes | Vanilla drains water in `ThirstMdfr.OnTick`: `GetWaterMetabolicSpeed(movement)` times a non-linear modifier, applied as `player.GetStatWater().Add(-metabolic_speed * deltaT)`. Tweaker ships `modded class ThirstMdfr` and multiplies `metabolic_speed` by `1 + stormFactor * dustDrainMultiplier` where `stormFactor` is `GetGame().GetWeather().GetSandstorm().GetIntensity(player.GetPosition())` (0 outside the storm, up to 1 at its core) and the player is not under a roof (`IsUnderRoof()` or `IsSoundInsideBuilding()`, the same predicates Buzz B4 uses). Default `dustDrainMultiplier` 2.0, so a sprinting player in the core drains water three times as fast. The sandstorm controller also exposes `IsActive()`, `GetPosition()`, `GetDirection()`, `GetSpeed()`, and `GetMagnitude()`, so a config switch can use a flat multiplier during any active storm instead of the per-position one | `4_World/DayZ/Classes/PlayerModifiers/Modifiers/Thirst.c:30-39`; `3_Game/DayZ/PlayerConstants.c:82-86` (basal 0.01, walk 0.1, jog 0.3, sprint 0.6 per second); `3_Game/DayZ/Weather.c:210` (`GetSandstorm`); `3_Game/DayZ/Sandstorm.c:32,37,47,52,71,83` |
| T1a | Sandstorm control for Central Scrutinizer | Yes (recorded for Plan 3) | `SandstormController.Start(duration, calledByWeather)` and `Stop(duration, calledByWeather)` are native and callable from script. Plan 3 Phase 3 says "sandstorm control is out of v1 unless separately specified and verified"; this is the verification. It stays out of Tweaker and goes to Plan 3 at its next revision | `3_Game/DayZ/Sandstorm.c:20,27` |
| T2 | Fuel from the wrecked tanker trailers | Yes | The engine fills a canister from any object whose `GetLiquidSourceType()` returns a liquid: `FuelStation` returns `LIQUID_GASOLINE` (`FuelStation.c:18-21`), and the action gate in `CCTLiquid` reads `GetWaterSourceObjectType()` and `GetLiquidSourceObjectType()` from the target object (`CCTLiquid.c:82-99`). Nasdara's two abandoned cistern trucks already do this for water: `class Land_Wreck_Truck01_Aban1_Cistern : Well {}` and `Aban2` (`4_World/DayZ/Entities/Building/Wrecks/Wreck_Truck01_Aban1_Cistern.c:1` and `Wreck_Truck01_Aban2_Cistern.c:1`). Tweaker adds `modded class Land_Wreck_Trailer_Double_Tanker_White` (and `_Blue`, `_Red`) returning `LIQUID_GASOLINE` with `FuelStation`'s throughput, plus the same for `Land_Wreck_Trailer_Cistern` (the small rusted two-wheel tank in Jeff's second photo is the vanilla `DZ\structures\Wrecks\Vehicles\Wreck_Trailer_Cistern.p3d`, not a Nasdara model). Jeff's first photo is `Land_Wreck_Trailer_Double_Tanker_White` (`structures_wrecks_nasdara.pbo`). Per-object chance and cap: a server-side table keyed by the object's position holds remaining litres; `GetLiquidSourceType()` returns `LIQUID_NONE` when the entry is empty. The table is rebuilt at restart from `config.json` (`tankerLitres` default 60, `tankerChanceFull` default 0.5 so half the wrecks are dry each restart, `cisternTrailerLitres` default 20). Whether the small cistern holds fuel or water is the design call Jeff flagged; recommend fuel for the tanker trailers and the MTVR fuel wreck (`StaticObj_Wreck_MTVR_Fuel`) and leave the small cistern as water, because Nasdara already uses the big cistern trucks as water sources and a second water trailer reads consistently | `4_World/DayZ/Entities/Building/FuelStation.c:1-21`; `CCTLiquid.c:70-100`; `3_Game/DayZ/Entities/Object.c:657-660` (base `GetLiquidSourceType` returns `LIQUID_NONE`); `4_World/DayZ/Entities/Building/Wrecks/Wreck_Truck01_Aban1_Cistern.c:1`; model names from `structures_wrecks_nasdara.pbo` and vanilla `structures_wrecks.pbo` |
| T3 | Cactus bleeding | Yes, with a sweep | Nasdara's cacti are prickly pear vegetation models `b_OpuntiaFicusIndica_1s`, `_1sb`, `_2s`, `_2sb`, `_3s`, `_3sb` (`plants_nasdara.pbo`), bush-class map objects with their own footstep sound sets (`cactus_run`, `cactus_walk`, `cactus_scuff` in the same PBO), which proves the engine knows a player is touching one but exposes that only to the sound system, not to script. Script side: `GetGame().GetObjectsAtPosition3D(pos, radius, objects, cargos)` returns vegetation; filter with `IsBush()` and `GetType()` starting with `b_OpuntiaFicusIndica`. Sweep radius 1.2 m around the player every 0.5 s on the server, only while `HumanCommandMove.GetCurrentMovementSpeed()` is 2 (run) or 3 (sprint) and the player is on Nasdara. On a hit: `GetBleedingManagerServer().AttemptAddBleedingSource(component)` on a leg or arm component (`BleedingSourcesManagerServer.c:194`), per-contact chance `cactusChance` default 0.25, cooldown `cactusCooldownSeconds` default 20, one `[NIMBY][Tweaker] cactus wound player=<name> steam64=<id>` line. Cost: 40 players times two sweeps a second of a 1.2 m radius is well inside the budget the vanilla action-target sweep already spends per frame; the sweep is skipped for players standing, walking, or in a vehicle. The rider case (moped brushing a cactus) is the same sweep run on the vehicle's position with `GetCommand_Vehicle()` set, recorded for the bike study | `3_Game/DayZ/Global/Game.c:930`; `3_Game/DayZ/Entities/Object.c:467,613` (`GetType`, `IsBush`); `3_Game/DayZ/human.c:468` (movement speed); `4_World/DayZ/Classes/BleedingSources/BleedingSourcesManagerServer.c:194,211`; `plants_nasdara.pbo` model list |
| T4 | Well rope | Yes, on the brick well; not on the pump wells | The Nasdara brick well is `Land_WaterWell_Brick : Well` with a vanilla `Rebuilding` construction (`Rebuilding : ConstructionBase`) whose parts are `rubble_rubble1`, `well_brick`, and `roof` (the roof carries the bucket proxy `WaterWell_Brick_Rebuild_Bucket.p3d`). Water exists only while `roof` is constructed: `GetLiquidSourceType()` returns `LIQUID_CLEANWATER` only then, and the drink action is the bucket variant (`Land_WaterWell_Brick.c:3-17,29-36`). "Cut the rope" is therefore a new continuous action with a knife-class item on the roof part that calls `DismantlePartServerEx(player, "roof", -1)` (`Rebuilding.c:40`); vanilla then persists the part state (`REBUILDING_STORAGE_VERSION`, `Rebuilding.c:3`) and syncs it to clients, and the well stops giving water until someone rebuilds the roof with the vanilla rebuild action. No new persistence, no new class. The other Nasdara wells (`Land_Water_Well1`, `Land_Water_Well2`, `Land_Misc_Well_Pump_*`) are plain `Well` subclasses with no construction and no rope; they are out of scope unless a rebuildable variant is placed on Nasdara. Open check: whether 1.30's vanilla dismantle on the brick well already exists for the roof (then Tweaker only changes the tool and the time), and what the rebuild cost is, so cutting is not free while rebuilding is expensive | `4_World/DayZ/Entities/Building/Land_WaterWell_Brick.c:1-37`; `4_World/DayZ/Classes/Rebuilding/Rebuilding.c:1-3,32-47`; `4_World/DayZ/Entities/Building/Well.c:1-35`; `structures_props_nasdara.pbo` proxies |

## Architecture

One server-side addon, `mods/own/nimby-pack/NimbyTweaker/`, with one script file per idea and a shared config loader:

| File | Idea | Hooks |
| --- | --- | --- |
| `3_Game/nimbytweaker_config.c` | all | `$profile:NimbyTweaker\config.json` via `JsonFileLoader.LoadFile`, defaults written on first boot, every idea has an `enable*` flag |
| `4_World/nimbytweaker_thirst.c` | T1 | `modded class ThirstMdfr`, sandstorm intensity, roof check |
| `4_World/nimbytweaker_wrecks.c` | T2 | `modded class` per wreck class, the litres table, `GetLiquidSourceType` override |
| `4_World/nimbytweaker_cactus.c` | T3 | `modded class PlayerBase` scheduled tick, the sweep, the bleeding call |
| `4_World/nimbytweaker_wellrope.c` | T4 | `ActionCutWellRope` (continuous, knife in hands, target part `roof`), registered in `modded class Land_WaterWell_Brick.SetActions` |
| `5_Mission/nimbytweaker_missionbase.c` | all | Self-test with `cases=N` per shared rules 8; engine-free validators for the config bounds, the drain formula, the chance and cooldown logic |

Each idea is independent: disabling one removes its hook at mission start and leaves the others running.

## Save footprint

Zero. T4 writes through vanilla's own `Rebuilding` storage, which exists with or without Tweaker. T2's litres table is memory only. Removing Tweaker leaves a cut well cut (vanilla rebuilds it) and no other trace.

## Risks

| Risk | Mitigation |
| --- | --- |
| `GetSandstorm()` returns null or `GetIntensity` is diag-only on a dedicated server | Phase 0 logs the controller and one intensity sample per minute during a storm on the replica; fallback is the flat multiplier on `IsActive()` |
| `GetObjectsAtPosition3D` does not return vegetation on a dedicated server | Phase 0 sweeps next to a known cactus and logs `GetType()` of every object; fallback is a one-time export of cactus positions from the Nasdara `.wrp` into a server-side list |
| A modded wreck class desyncs clients that lack the mod | Tweaker is in the Workshop item (shared rules 9.3), so every client has the class; the server refuses unmodded clients by signature anyway |
| Fuel table makes a wreck a free pump | Per-restart litres and chance; the table is logged at boot so the admin sees which wrecks are live |
| Cactus sweep costs frame time at 40 players | Only runs while running or sprinting, 0.5 s cadence, 1.2 m radius; Phase 0 measures server FPS with and without it on the replica |
| Vanilla already lets players dismantle the well roof | Then T4 shrinks to a tool and time change; checked in Phase 0 |

## Decisions for Jeff (when the plan is written)

| # | Decision | Recommendation |
| --- | --- | --- |
| TW1 | Order on the mod board | After Splash (order 5); it is small, server-side, and independent of the smoke trio |
| TW2 | Small cistern trailer: fuel or water | Water (consistent with the cistern trucks); the tanker trailers and the MTVR fuel wreck give gasoline |
| TW3 | Fuel state | Server memory, reset at restart, half the wrecks dry per restart |
| TW4 | Cactus wound chance and cooldown | 0.25 and 20 s, tuned on the replica |
| TW5 | Dust drain multiplier | 2.0 at storm core, scaling with intensity |
| TW6 | T1a sandstorm control | Promote into Plan 3 Phase 3 at the next revision (it is now verified) |
