# Fang feasibility

Draft v0.1, November 1, 2026. Owner: Jeff Fabish. Status: feasibility only, ordered by Jeff on November 1, 2026 in the Fang interview (twelve rulings, recorded in Decisions below). Working name Fang, code prefix `NimbyFang`, log prefix `[NIMBY][Fang]`, RPC block `65512xx` (reserved; nothing in Phase 0 or Phase 1 needs an RPC, the bite and venom state travel on vanilla sync paths). Every `file:line` is relative to `vendor/dayz-1.30-scripts/` (build 1.30.164200): `scripts/...` for Enforce script, `DZ/...` for the unRaP'd configs. Mission paths are relative to `D:\DayZServer\mpmissions\dayzOffline.nasdara\` (the replica copy of the live mission).

Target: NIMBY Badlands | Enhanced Vanilla | RP | 1PP, HostHavoc, DayZ 1.30 / Nasdara.

## Verdict

Feasible, and Nasdara already ships the proof. The Badlands DLC adds a reptile to `AnimalBase`: the desert monitor `Animal_VaranusGriseus` (`DZ/animals_takistan/varanus_griseus/config.cpp:20`) runs on `simulation = "dayzanimal"` with its own skeleton (`varanus_griseus_skeleton.xob`, `:109,113`) and its own animation instance (`Varanus_Griseus_Skeleton.asi`, `:111`) bound to the *shared* ambient-life graph `dz\animals\animations\!graph_files\AmbientLife\AmbientLife_Graph.agr` (`:110`), the same graph the hare and the fox use. Its AI is config, not code: `aiAgentTemplate = "AmbientLife_Varanus_Griseus"` (`:37`) selects a `CfgAIBehaviours` entry (`DZ/AI_takistan/config.cpp:2010-2028`) that parameterizes the engine's `BehaviourHLAmbientLife`. It spawns through the vanilla central economy: `events.xml` plus a territory file with 171 zones (`env/monitor_territories.xml`). A snake is the monitor with a different mesh, a different skeleton, and a different bite rule. Nothing about the creature needs new engine behaviour.

The cost sits in two places. First, the animation pipeline: the purchased FBX must become a DayZ skeleton (`.xob`) and clips (`.anm`) and an animation instance (`.asi`) that fills the slots of a vanilla graph, in the DayZ Tools Workbench Animation Editor on 1.30's new text graph format. No page Claude could read documents that path end to end; the community route is Blender to `.txa` text clips through the DayZ Animation Tools Blender add-on, then Workbench import. Phase 0 exists to prove this one step on a crude mesh before the real model is touched. Second, the strike: the ambient-life behaviour flees and never attacks (its alerted slot is `BehaviourGoToTarget`, `DZ/AI_takistan/config.cpp:2087-2105`, and no attack slot exists in any ambient template), while the predator behaviour (`BehaviourHLPredator`, wolf and Dobermann) hunts. A snake that strikes only when crowded is neither, so Fang scripts the strike: `DayZAnimal` exposes `StartCommand_Script` and `DayZAnimalCommandScript`, a "fully scriptable command" (`scripts/3_Game/DayZ/Entities/DayZAnimal.c:613-619,673-675`), and `DayZCreature.ModCommandHandlerBefore/Inside/After` (`scripts/3_Game/DayZ/Entities/DayZAnimal.c:175-185`) is the mod hook that decides each frame which command runs. The snake stays on the ambient template for idle, wander, and flee, and a scripted command plays the strike clip and fires the bite damage sphere when a survivor stands inside strike range.

The venom is the cheap part. DayZ already has a transmissible-agent model with a fatal untreated course: the nerve agent (`scripts/4_World/DayZ/Classes/TransmissionAgents/Agents/NerveAgent.c`) grows a `CHEMICAL_POISON` count that three staged modifiers read (`Contamination3.c:3-6,26-28` activates at 400 agents and drains shock and health). A `VENOM` agent (`eAgents` is a bitmask, `scripts/3_Game/DayZ/Enums/EAgents.c:2-16`, next free bit 2048), one staged `NimbyVenomMdfr`, and an injector item modelled on `Morphine` (`scripts/4_World/DayZ/Entities/ItemBase/Gear/Medical/Morphine.c:1-17`) reuse everything vanilla has for symptoms, sync, persistence, and drugs. The bite itself is the one place Fang touches the player: `PlayerBase.EEHitBy` already branches on the ammo name (`"Bullet_CupidsBolt"`, `scripts/4_World/DayZ/Entities/ManBase/PlayerBase.c:1255,1333`), so a `MeleeSnake` ammo class and one `ammo == "MeleeSnake"` branch is the whole hook.

Recommendation: proceed to Phase 0 after release 1, as ruled. Buy the model only after Phase 0 proves the Workbench path on the placeholder, because the purchase is non-refundable and the pipeline is the unproven step, not the model.

## What this is not

- Not a new AI brain. The snake uses the engine's ambient-life behaviour for everything except the strike; Fang never writes a behaviour tree.
- Not a boot-check mod yet. The snake-in-the-boot roll and the `Check boots` action are Phase 2 and reuse the venom code; nothing in Phase 0 or 1 touches footwear.
- Not a Workshop dependency. Ambient Animals Pack (Hunterz) ships snakes, but its page forbids repacks and gives no source, and Jeff ruled a purchased model in the NIMBY pack. It stays listed under Options as the fallback if the pipeline fails twice.
- Not a model mod in the D2 sense. The mesh and clips are bought, not Claude-made; Claude's model work is the placeholder for Phase 0 and the retarget.
- Not a loot overhaul. One `types.xml` row for the antivenom; no change to any vanilla medicine's spawn.

## Current state (vanilla 1.30)

| Item | State | Source |
| --- | --- | --- |
| Reptile precedent | `Animal_VaranusGriseus : AnimalBase`, `simulation = "dayzanimal"`, `scope = 2`, own `.p3d`, two hidden selections | `DZ/animals_takistan/varanus_griseus/config.cpp:20-36` |
| Graph reuse | Monitor's `enfanimsys`: own `meshObject` and `skeletonName` (`varanus_griseus_skeleton.xob`), own `defaultinstance` (`Varanus_Griseus_Skeleton.asi`), shared `graphname` `AmbientLife_Graph.agr`, `startnode = "Master_SM"` | `DZ/animals_takistan/varanus_griseus/config.cpp:108-114` |
| Added-animal addon precedent | Bear ships in its own addon with its own `CfgAmmo MeleeBear : MeleeDamage` and its own graph `Bear_Graph.agr` | `DZ/animals_bliss/ursus_arctos/config.cpp:1-40,4058-4069,4311-4315` |
| AI templates | `CfgAIBehaviours` entries select an engine behaviour: `BehaviourHLAmbientLife` (hare, fox, hen, monitor), `BehaviourHLPredator` (wolf, Dobermann, German Shepherd), `BehaviourHLBear`, `BehaviourHLDeer`, `BehaviourHLDomestic`, `BehaviourHLZombie`; no other high-level behaviour exists | `DZ/AI/config.cpp:14-6214`, `DZ/AI_takistan/config.cpp:3-2180`, `DZ/AI_bliss/config.cpp:16-26` |
| Ambient template | `instantAlertRangeMin/Max` 1.5/2.5, slots Cowering, Calm (walk and three idles with weights and durations), NonSpecificThreat (look, sniff), Alerted = `BehaviourGoToTarget` (flee at speed 2-6); vision 4-14 m | `DZ/AI_takistan/config.cpp:2010-2175` |
| Predator template | `proximityAttackRange = 2.5`, `attackCooldown = 3`, slots Cowering, CalmGrazing, Siege (`attackDistance = 3.5`), Eating, Hunting (`attackRange = 3.5`), CalmResting, CalmTravelling, Attracted, Fireplace, Scared | `DZ/AI/config.cpp:5420-6213` |
| Animal bite mechanics | `AnimEvents.Damages` names a `CfgDamages` entry per clip event (`WolfBiteDamage`: bone `tongue3`, ammo `MeleeWolf`, radius 0.4, duration 0.2); the monitor declares a `Bite` damage event too | `DZ/AI/config.cpp:6942-6975`, `DZ/animals_takistan/varanus_griseus/config.cpp:344-350`, `DZ/weapons/melee/config.cpp:71948` |
| Damage sphere API | `DayZCreatureAI.AddDamageSphere(bone, ammo, radius, duration, invertTeams)` and `OnDamageEvent` from clip events | `scripts/3_Game/DayZ/Entities/DayZAnimal.c:200,232,287` |
| Scriptable command | `DayZAnimalCommandScript` ("fully scriptable command"); `DayZAnimal.StartCommand_Script`, `StartCommand_ScriptInst`, `GetCommand_Script`; `StartCommand_Attack`, `_Death`, `_Hit`, `_Move`, `_Jump` | `scripts/3_Game/DayZ/Entities/DayZAnimal.c:613-619,655-675` |
| Mod command hook | `ModCommandHandlerBefore/Inside/After(pDt, pCurrentCommandID, pCurrentCommandFinished)` on `DayZCreature`; `DayZCreatureAI` overrides `Before` | `scripts/3_Game/DayZ/Entities/DayZAnimal.c:175-185,433` |
| Animal script classes | `AnimalBase extends DayZAnimal`; per-species classes override `RegisterHitComponentsForAI`, `IsDanger`, `CaptureSound` | `scripts/4_World/DayZ/Entities/Creatures/Animals/AnimalBase.c:1-110` |
| Player hit hook | `PlayerBase.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef)`; branches on `ammo` string (`FlashGrenade_Ammo`, `Bullet_CupidsBolt`); `MDF_BROKEN_LEGS` activation sits here | `scripts/4_World/DayZ/Entities/ManBase/PlayerBase.c:1255,1282,1326-1337` |
| Agents | `eAgents` bitmask, 11 values, highest `EYES_IRRITATION = 1024`; agents registered in `PluginTransmissionAgents.RegisterAgent`; `AgentBase` fields `m_Invasibility`, `m_DieOffSpeed`, `m_MaxCount`, `m_DrugResistances` | `scripts/3_Game/DayZ/Enums/EAgents.c:2-16`, `scripts/4_World/DayZ/Plugins/PluginBase/PluginTransmissionAgents.c:22-39`, `.../Agents/NerveAgent.c:1-15` |
| Staged fatal course precedent | `ContaminationStage3Mdfr`: activates at `GetSingleAgentCount(CHEMICAL_POISON) >= 400`, queues `SYMPTOM_VOMIT`, drains shock to unconsciousness then health `-3/s` | `scripts/4_World/DayZ/Classes/PlayerModifiers/Modifiers/diseases/Contamination3.c:1-58` |
| Modifiers | `ModifiersManager.AddModifier(new ...Mdfr)` list; `eModifiers` enum (59 ids) | `scripts/4_World/DayZ/Classes/PlayerModifiers/ModifiersManager.c:93-101`, `.../eModifiers.c` |
| Injector precedent | `Morphine : Inventory_Base` adds `ActionInjectMorphineSelf/Target`; `OnApply` resets and activates `MDF_MORPHINE`; `Epinephrine` model `\dz\gear\medical\Epinephrine.p3d`, 1x2, 60 g | `.../Medical/Morphine.c:1-17`, `.../Actions/SingleUse/Medical/ActionInjectMorphineSelf.c`, `DZ/gear/medical/config.cpp:806-815` |
| Symptoms | `SymptomIDs`: `SYMPTOM_PAIN_LIGHT`, `SYMPTOM_PAIN_HEAVY`, `SYMPTOM_HAND_SHIVER`, `SYMPTOM_FEVERBLUR`, `SYMPTOM_GASP`, `SYMPTOM_FAINT`, `SYMPTOM_VOMIT`, `SYMPTOM_UNCONSCIOUS` | `scripts/4_World/DayZ/Classes/PlayerSymptoms/StateManager.c:9-30` |
| Post-process | `PPERequesterBank`: `REQ_SHOCKHITEFFECTS`, `REQ_HEALTHHITEFFECTS`, `REQ_PAINBLUR`, `REQ_CONTAMINATEDAREA` tint | `scripts/3_Game/DayZ/PPEManager/PPERequesterBank.c:33-41` |
| Worn clothing lookup | `PlayerBase.GetItemOnSlot(slot_type)` by slot name | `scripts/4_World/DayZ/Entities/ManBase/PlayerBase.c:980` |
| Spawning | `events.xml` `AnimalWolf` (nominal 11, min 5, max 7, `position fixed`, `limit child`, `active 0` on the replica), `AnimalDog` active with `Animal_CanisFamiliaris_Dobermann_*` children; `cfgenvironment.xml` lists territory files and binds `territory type="Herd|Ambient" name=... behavior=DZ*GroupBeh` with `agent`/`spawn configName` and `globalCountMax`, `zoneCountMin/Max`, `playerSpawnRadiusNear/Far`; zones are `x z r` with `dmin dmax` | `db/events.xml:89-101,146-160,281-293`, `cfgenvironment.xml:4-113`, `env/monitor_territories.xml`, `env/wolf_territories.xml` |
| Loot row precedent | `Epinephrine`: nominal 20, min 13, lifetime 28800, category `tools`, tag `shelves`, usage `Medic` | `db/types.xml:7425-7437` |

## What Jeff's WorkDrive extract adds

Jeff ran `WorkDrive.exe /ExtractGameData` on November 1, 2026 (DayZ Tools at `F:\SteamLibrary\steamapps\common\DayZ Tools`, game at `C:\Program Files (x86)\Steam\steamapps\common\DayZ`). It unpacked and unRaP'd `animals.pbo` and `animals_bliss.pbo` into `C:\Users\Buffe\OneDrive\Documents\DayZ Projects\DZ\animals\` and `DZ\animals_bliss\` with no errors. That folder holds the real `!graph_files` (`AmbientLife_Graph.agr`, the `.asi` instances, the `.agf` sub-graphs) and the animal skeletons and clips that the repo's script extract does not carry (`vendor/dayz-1.30-scripts/DZ/animals/animations/` holds only `cfg/profiles/exportFn.c`). Phase 0 reads the graph slot names from there.

**Caution:** that folder is inside OneDrive. OneDrive re-uploads every binarized asset Workbench writes and holds file locks while it syncs; the Workbench P: drive source belongs on a local, non-synced folder (for example `D:\DayZProjects`). Move it before Phase 0 and re-run `/ExtractGameData` there; the extract takes about 12 minutes.

## Options

### O1. Creature base

| # | Option | Gets Jeff | Cost | Verdict |
| --- | --- | --- | --- | --- |
| C1 | `AnimalBase` on the ambient template plus a scripted strike | A real animal: central-economy spawning, territories, skinning, vanilla despawn, `IsDanger` and lifetime handling; idle, wander and flee from the engine; strike from `DayZAnimalCommandScript` | Workbench skeleton, `.asi` instance against `AmbientLife_Graph.agr`, clip import; one scripted command and one `ModCommandHandlerBefore` override; monitor config as the template | **Yes (ruled by Jeff: full AnimalBase).** The scripted strike is the Phase 1 risk item |
| C2 | `AnimalBase` on the predator template | Engine-driven attack and chase | The template hunts (Siege, Hunting slots); zeroing weights is untested and the engine may still pathfind to targets; a snake that chases is wrong | No; keep as fallback if the scripted strike fails |
| C3 | Scripted ambush entity, no AI agent | Lowest import risk | No central-economy spawning, no territories, no vanilla skinning | Withdrawn by the ruling |

### O2. Model and animation source

| # | Option | Gets Jeff | Cost | Verdict |
| --- | --- | --- | --- | --- |
| M1 | Purchased rigged and animated snake, ceiling $150 | Idle, crawl, attack, death clips on a snake skeleton; PBR textures | Retarget in Blender to a skeleton the graph accepts; `.txa` export; binaries gitignored (`vendor/` already is), Workshop-only | **Yes (ruled).** Shortlist below |
| M2 | Claude-made mesh and clips (D2) | Committable source | A snake skeleton, five clips, and a first-ever animal import in one attempt | No; the D2 rule is suspended for Fang by the ruling |
| M3 | Ambient Animals Pack as a Workshop dependency | Snakes with "own animations, AI and sounds" today; 248K subscribers | "No repacks allowed", no source, last changelog entry June 16, one undocumented October 22 update; 1.30 status not stated; Fang would hook its classes blind | Fallback only, if Phase 0 fails twice |

### O3. Venom mechanic

| # | Option | Gets Jeff | Cost | Verdict |
| --- | --- | --- | --- | --- |
| V1 | New agent `VENOM = 2048` plus one staged modifier | Rides the agent pool: persisted, synced, drug-resistance table, antibiotics interplay for free; symptoms via `SymptomManager`; PPE via existing requesters | `modded enum eAgents`, `NimbyVenomAgent : AgentBase`, `NimbyVenomMdfr : ModifierBase` with a `m_SyncID`; the Y-minute clock is `GetAttachedTime()` on the modifier; the Z roll fires once at the deadline | **Yes.** Mirrors nerve agent, the one vanilla course that kills untreated |
| V2 | Modifier only, no agent | Simpler | Loses the drug-resistance and transmission scaffolding; harder to let Charcoal or Saline modify odds | No |
| V3 | Direct damage over time | Trivial | Not treatable in any vanilla-shaped way; no symptoms | No |

### O4. Treatment

Ruled: new `NimbyAntivenom` cures outright; vanilla items modify odds.

| Item | Effect on venom (config keys) | Why |
| --- | --- | --- |
| `NimbyAntivenom` injector | `RemoveAgent(VENOM)` and deactivate the modifier; message `You feel the burning ease` | The cure; placeholder model `Epinephrine.p3d` with a NIMBY texture until a bespoke injector exists; loot row nominal 4, min 2, usage `Medic`, tag `shelves` (`antivenomNominal`, `antivenomMin`) |
| `Morphine`, `PainKillers` | Suppress the pain symptoms and the pain blur for their vanilla duration; no change to the clock or the roll (`painkillersSuppressSymptoms`) | Symptom relief, as in life |
| `CharcoalTablets` | Multiply `deathChance` by `charcoalDeathFactor` (default 0.75) while active | A nod to realism without pretending it cures |
| `SalineBagIV`, `BloodBagIV` | Add `salineExtraMinutes` (default 5) to the deadline once per bite | Volume support buys time |
| `Epinephrine` | None | Realistic, and keeps it from becoming a second antivenom |

### O5. Clothing protection

Ruled: config table by item class prefix. The bite roll runs on the server at the strike's damage event:

```
feet   = factor(GetItemOnSlot("Feet"))   // 0.0 when empty or ruined
legs   = factor(GetItemOnSlot("Legs"))
protection = clamp(feet * feetWeight + legs * legsWeight, 0, 1)   // weights 0.7 / 0.3
biteChance = biteChanceBase * (1 - protection)                      // base 0.9
```

`factor()` matches the longest class-name prefix in `protection.feet` / `protection.legs`, falling back to `protection.defaultFeet` / `protection.defaultLegs` (0.5 / 0.3). Shipped table: `CombatBoots*` 0.9, `MilitaryBoots*` 0.9, `HikingBoots*` 0.8, `WorkingBoots*` 0.8, `JungleBoots*` 0.8, `Wellies*` 0.9, `Sneakers*` 0.4, `JoggingShoes*` 0.3, `AthleticShoes*` 0.3, `DressShoes*` 0.3, empty slot (barefoot) 0.0; `CargoPants*` 0.5, `HunterPants*` 0.5, `Jeans*` 0.5, `GorkaPants*` 0.6, `TTSKOPants*` 0.6, `ParamedicPants*` 0.4, `PolicePants*` 0.4, `ShortJeans*` 0.0, `Skirt*` 0.0, `Breeches*` 0.5. Every prefix above matches at least one class in `DZ/characters/*/config.cpp` (1.30.164200); vanilla 1.30 ships no sandal or slide, so "sandals" in the brief is the barefoot case until a modded or future item adds one, which then needs one row. Every value is a `config.json` entry; a new Bohemia item needs one row or inherits the default.

### O6. Spawning

Ruled: authored territories through `events.xml`. One `AnimalSnake` event (`nominal 6`, `min 2`, `max 4`, `position fixed`, `limit child`, `active 1`, child `Animal_NimbySnake`) and one `env/snake_territories.xml` with `Zone_Snake` entries (`dmin 0`, `dmax 2`, `r 100-200`) in rock fields, dry wadis, ruins, and abandoned farm yards, bound in `cfgenvironment.xml` as `territory type="Ambient" name="AmbientSnake" behavior="DZAmbientLifeGroupBeh"` with `globalCountMax 24`, `zoneCountMin 0`, `zoneCountMax 2`, `playerSpawnRadiusNear 25`, `playerSpawnRadiusFar 75`. Claude derives the zone coordinates from the monitor territory (same biome preference) minus town radii; Jeff edits the numbers from the panel like any other `env/` file. Live changes go through `server/mission-overrides` with a panel backup first, per the loot-tuning rule.

### O7. Bite feedback

Ruled: vanilla-style staged.

| Moment | Server | Owning client |
| --- | --- | --- |
| Strike | `AddDamageSphere("head", "MeleeSnake", 0.3, 0.15)` from the attack clip's damage event; hiss `SoundSet` on the snake (`AnimEvents.Sounds`); bite roll; on a bite, `MeleeSnake` ammo applies 5 health, 0 blood, 15 shock and adds `VENOM` 150 | `REQ_SHOCKHITEFFECTS` flash from the vanilla hit path; message `You feel a sharp bite on your leg`; player pain voice |
| Venom stage 1 (0-40% of `untreatedMinutes`) | Agent grows at `venomGrowthPerSecond`; modifier active | `SYMPTOM_PAIN_LIGHT` every 60-90 s, `REQ_PAINBLUR` low |
| Stage 2 (40-80%) | Shock drain `-1/s` when no primary symptom is playing | `SYMPTOM_HAND_SHIVER`, `SYMPTOM_GASP`, `REQ_PAINBLUR` medium, periodic `SYMPTOM_FEVERBLUR` |
| Stage 3 (80-100%) | Stamina cap halved; `SYMPTOM_FAINT` queued | Heavy pain, desaturation via a tint requester |
| Deadline | Roll `deathChance`: death sets health to 0 with a `[NIMBY][Fang] bite death` log line and a Discord line through Central Scrutinizer when its queue exists; survival removes the agent and leaves a `recoveryMinutes` tail of stage 1 effects | Message `The burning fades` or death |
| Cure | Agent removed, modifier off, no tail | Message `You feel the burning ease` |

Sounds come from a CC0 or CC-BY set (freesound) recorded in `NOTICE` with attribution; the bite and hiss `SoundSet` and `SoundShader` entries live in `NimbyFang`'s config.

## Config schema

`$profile:NimbyFang/config.json`, read on server start and on `fang.reload` from Central Scrutinizer once Plan 3 Phase 2 ships; every value below is the default.

```json
{
  "version": 1,
  "bite": { "biteChanceBase": 0.9, "feetWeight": 0.7, "legsWeight": 0.3, "strikeRangeMeters": 1.6, "strikeCooldownSeconds": 4, "venomPerBite": 150 },
  "venom": { "untreatedMinutes": 20, "deathChance": 0.6, "recoveryMinutes": 10, "charcoalDeathFactor": 0.75, "salineExtraMinutes": 5, "painkillersSuppressSymptoms": true },
  "protection": { "defaultFeet": 0.5, "defaultLegs": 0.3, "feet": { "CombatBoots": 0.9, "Sandals": 0.0 }, "legs": { "Jeans": 0.5, "Shorts": 0.0 } },
  "spawn": { "antivenomNominal": 4, "antivenomMin": 2 },
  "debug": { "logRolls": false }
}
```

`untreatedMinutes` is real time, counted by the modifier's attached time, so it survives a relog (modifiers persist with the player). `spawn.*` values are documentation of the `types.xml` row, not live overrides; the row itself lives in the mission.

## Addon split

Two PBOs, per the pack rule that separates what a Bohemia update breaks:

| PBO | Holds | Breaks when |
| --- | --- | --- |
| `NimbyFang` (content) | `config.cpp` (`Animal_NimbySnake`, `NimbyAntivenom`, `MeleeSnake`, `CfgAIBehaviours AmbientLife_NimbySnake`, sounds), `.p3d`, `.xob`, `.anm`, `.asi`, textures, `Data/` | The animation graph format or `AnimalBase` config keys change (1.30 changed the graph format once already) |
| `NimbyFangScripts` | `3_Game` (`eAgents`, agent class), `4_World` (`Animal_NimbySnake` script, strike command, `NimbyVenomMdfr`, `PlayerBase.EEHitBy` branch, injector actions), `5_Mission` (messages) | Script API changes |

`NimbyAntivenom` is a persisted item, so `NimbyFang` is a content addon under shared rules 6.1 and takes the two extra boots at its gate. Neither PBO overrides `OnStoreSave/OnStoreLoad` on a vanilla class; the venom state lives in the vanilla agent pool and modifier, which already persist.

## Phases

| Phase | Proves | Exit |
| --- | --- | --- |
| 0 - pipeline proof | A new skeleton and `.asi` against the vanilla `AmbientLife_Graph.agr` loads, animates, and spawns on the replica | A box-and-tube "snake" (Claude-made, three bones) with two clips (idle, crawl) imported through Blender `.txa` and Workbench; `Animal_NimbySnake` spawned by `events.xml` in one test zone wanders and flees; `StartCommand_ScriptInst` plays the idle clip on demand from a debug action; zero `[NIMBY][Fang]` errors over a 30-minute boot. Model purchase happens after this exit |
| 1 - live snake | The purchased model, the strike, the venom course, the antivenom | Gate table: strike only inside `strikeRangeMeters`; bite roll honours the table (sandals vs combat boots, 20 trials each); clock, deadline roll and cure; relog mid-venom; Morphine, Charcoal and Saline effects; sounds and PPE; skinning yields `NimbySnakeMeat`; `storage_1` round trip and the no-addon boot (6.1) |
| 2 - boot check | Snake-in-the-boot roll on footwear left on the ground or in a tent, `Check boots` action, equip-unchecked bite | Reuses `MeleeSnake` and the agent; its own feasibility addendum |

Phase 0 runs in its own worktree (`D:\GitHub\nimby-badlands-server-wt\fang`) after the five release-1 mods are handed to Jay, with its own `storage_` instance so it never shares a port with the Hands or Central Scrutinizer servers.

## Risks

| Risk | Mitigation |
| --- | --- |
| A mod cannot bind a new `.asi` to the vanilla `AmbientLife_Graph.agr` (slot names, template coupling, or the 1.30 editor refusing a vanilla graph) | Phase 0 question 1. Fallback: copy the graph into `NimbyFang` as `NimbySnake_Graph.agr` (the bear ships its own graph) and own it |
| 1.30 changed the animation graph to a text format and community tooling lags; the only converter found covers a subset and has not been loaded in game | Phase 0 reads the shipped `.agr`/`.asi` from Jeff's WorkDrive extract as the authority; no converter is used |
| Purchased snake skeleton has no `pin_lookat` or equivalent bones the ambient template's `HeadLookBoneName` and `CommandLookAtSettings` expect | Add the pin bones in Blender during retarget; Phase 0 placeholder carries them from the start |
| Scripted strike fights the engine's flee: the agent starts `BehaviourGoToTarget` the moment a player is inside `instantAlertRangeMax` | Set `instantAlertRangeMin/Max` below `strikeRangeMeters` so the strike window comes first; `ModCommandHandlerBefore` returns true while the strike plays; Phase 1 gate row |
| Store license on PBO packing: CGTrader and TurboSquid both require a "proprietary format that cannot be extracted without reverse engineering"; a PBO opens with free tools | TurboSquid allows "Game mods" by name; Jeff reads both pages before buying and prefers TurboSquid-listed items; binaries never enter the public repo |
| `eModifierSyncIDs` bitmask is 32 wide and vanilla uses many | Fang's modifier syncs through the symptom system, not a new sync id, unless Phase 1 needs one; checked at Phase 1 recon |
| 80 players times territories: `globalCountMax` too high costs AI budget | Start at 24 and 2 per zone; Central Scrutinizer's observation feed shows the count |
| Players never see the snake before the strike (small, brush-coloured) | Hiss `SoundSet` fires in NonSpecificThreat (the monitor pattern) at 6-8 m before any strike, so the warning is audible |
| Antivenom too common or too rare | `types.xml` row, live-tunable; start nominal 4 |

## Model shortlist

Ceiling $150 (ruled). FBX required. Every row below is from the listing text read on November 1, 2026; polygon counts and clip names are the seller's words.

| # | Model | Store, price | Clips | Mesh and textures | Note |
| --- | --- | --- | --- | --- | --- |
| 1 | Boa - Game Ready (viverna) | CGTrader, $59.99, Royalty Free License | Attack (2), Fall asleep, Awake, Get hit, Death, Idle, Look around, Retreat, Swim, Crawl (11) | 5,516 tris, 4K PBR (albedo, normal, roughness, AO, packed), 5 skins; FBX 7.2, BLEND 2.8, Unity, UE | **Recommended.** The only candidate with idle, crawl, two attacks, hit and death; a boa's girth also reads at DayZ view distances |
| 2 | Snake - Blue Viper with animations | CGTrader and TurboSquid, $19.99 | Idle1, Idle2, Crawl1, Crawl2, Attack, Hanging, Dead (7) | Polycount not stated, 6 colour variants, channel-packed textures; FBX, Maya rig | Second choice; viper look fits Nasdara better, retexture to a desert horned viper |
| 3 | Rattlesnake | CGTrader, $65 | Slow Crawling, Fast Crawling, Attack; Resting is a pose, not a clip | 9,972 tris, 4K diffuse, normal, gloss, spec (not PBR); FBX, OBJ, Maya | No idle or death clip; would need two Claude-made clips |
| 4 | Snake Rigged and Animated | CGTrader, $50 | Snake_Attack, Snake_Attack_2, Snake_Die, Snake_Idle (listing says 7, names 4) | Polycount not stated, 4K PBR variants; FBX, C4D, OBJ | No crawl named; confirm before buying |
| 5 | Animated Snakes Pack | CGTrader $14 / TurboSquid $10 | 16 incl. idle, attack, walk, run, death, hit (summary) | 512 tris, 2K hand-painted, not PBR | Mobile-grade; too low for 1PP close range |
| 6 | Crawling Snake - Animated Game Asset | TurboSquid, $99 | Crawl loop only | 3,439 polys, 2K PBR | One clip; not enough |

License clauses, both stores (quoted from the license pages, see Sources): TurboSquid's 3D Model License lists "Game mods" under allowed game uses and requires the files "incorporated into a larger creation and not in an open format"; CGTrader's Royalty Free License permits a Product "as part of a game if the Product is contained inside a proprietary format" and forbids redistribution otherwise. A PBO is an open, documented container; whether packing counts is Jeff's judgment, and the mitigation is that the purchased FBX, Blender files and textures stay in the gitignored `vendor/` and only the compiled `.p3d`, `.xob`, `.anm` and `.paa` enter the Workshop item. Over $150 or FBX-less items are listed in the research notes and excluded.

## Decisions for Jeff

| # | Decision | Status |
| --- | --- | --- |
| FD1 | Creature | **Ruled November 1, 2026: snake.** |
| FD2 | Model source | **Ruled: buy a rigged and animated model, ceiling $150; purchase after Phase 0 proves the Workbench import on the placeholder** (Jeff, November 1, 2026, accepting the study's recommendation) |
| FD3 | First loop | **Ruled: live snake first; boot check is Phase 2** |
| FD4 | Treatment | **Ruled: new antivenom cures; vanilla items modify odds** (table in O4) |
| FD5 | Lethality defaults | **Ruled: hardcore (20 min, 60%), every value in `config.json`** |
| FD6 | Protection | **Ruled: config table by item class prefix** |
| FD7 | AI base | **Ruled: full `AnimalBase`**; the study adds the scripted strike on the ambient template (C1) because no engine behaviour strikes without hunting |
| FD8 | Spawning | **Ruled: authored territories through `events.xml`** |
| FD9 | Bite feedback | **Ruled: vanilla-style staged** |
| FD10 | Name | **Ruled: Fang, `NimbyFang`, `[NIMBY][Fang]`** |
| FD11 | Sequencing | **Ruled: study now, Phase 0 after release 1** |
| FD12 | Model pick | Open until Phase 0 exits. Recommendation: #1 Boa (CGTrader, $59.99) for its clip set; #2 Blue Viper ($19.99) if Jeff prefers a viper look and accepts a thinner idle set |
| FD13 | License read | Open. Jeff reads the CGTrader section 21A and TurboSquid section 7 text on the live pages before purchase; the study could not read 21A verbatim |
| FD14 | Snake meat | Default: skinning yields two `NimbySnakeMeat` (cookable, `SnakeMeat` does not exist in vanilla 1.30); Phase 1 |
| FD15 | P: drive move | Default: move the Workbench project out of OneDrive before Phase 0 (Caution above) |

## Resources this study could not reach

| Resource | Why it matters | How to provide it |
| --- | --- | --- |
| `C:\Users\Buffe\OneDrive\Documents\DayZ Projects\DZ\animals\animations\!graph_files\AmbientLife\` and `DZ\animals_takistan\...\Varanus_Griseus_Skeleton.asi` | The graph slot names decide whether a new `.asi` can bind to the vanilla graph (risk 1) | Folder access was refused by the session's policy; copy those two folders into `D:\GitHub\nimby-badlands-server\vendor\dayz-1.30-anim-graphs\` and the Phase 0 recon reads them there |
| Steam-authenticated view of Ambient Animals Pack (3114410963) | Logged-out fetches showed a "removed" banner and an undocumented October 22 update; its 1.30 state decides whether M3 is a real fallback | Jeff opens the page in Steam and reads the October 22 change note |
| DayZ Animation Tools Blender add-on (`github.com/jdfnc24/DayZAnimationPluginDemo`, Blender 3.6.8 or later) | The documented community route from Blender to `.txa` to Workbench `.anm` | Install in Blender before Phase 0 |

## Sources read

- `vendor/dayz-1.30-scripts/` (1.30.164200 extract) at the lines cited above; `D:\DayZServer\mpmissions\dayzOffline.nasdara\` mission files.
- WorkDrive log `WorkDrive - DayZ - 1.2.142.6803.txt` (Jeff, November 1, 2026).
- [AnimGraphMigration (1.29 to 1.30 graph converter)](https://github.com/weltvolk/AnimGraphMigration), [DayZ-Modding-Knowledge-Pack PR #139](https://github.com/willy92wins/DayZ-Modding-Knowledge-Pack/pull/139), [StarDZ Workbench guide](https://stardz-team.github.io/DayZ-Modding-Wiki/en/04-file-formats/07-workbench-guide), [DayZAnimationPluginDemo](https://github.com/jdfnc24/DayZAnimationPluginDemo).
- [Ambient Animals Pack](https://steamcommunity.com/sharedfiles/filedetails/?id=3114410963), [CRDTN Creatures](https://steamcommunity.com/sharedfiles/filedetails/?id=3054182440) (mechanics on vanilla wolves, not a new animal).
- [Boa - Game Ready](https://www.cgtrader.com/3d-models/animal/reptile/snakes-boa), [Snake - Blue Viper](https://www.cgtrader.com/3d-models/animal/reptile/snake-blue-viper-with-animations), [Rattlesnake](https://www.cgtrader.com/3d-models/animal/reptile/rattlesnake-fde40f9b-856f-43aa-86c1-92d60eb8bbfe), [Snake Rigged and Animated](https://www.cgtrader.com/3d-models/animal/reptile/snake-rigged-and-animated), [Animated Snakes Pack](https://www.turbosquid.com/3d-models/animated-snakes-pack-1056734), [Crawling Snake](https://www.turbosquid.com/3d-models/crawling-snake-loop-animation-3d-model-1639323); [TurboSquid 3D Model License](https://www.turbosquid.com/licensing), [CGTrader terms](https://www.cgtrader.com/pages/terms-and-conditions).
- The DayZ account's 1.30 animation-mod notice on X could not be fetched (robots block); its gist is reflected in the converter and PR sources above.
