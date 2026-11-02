# Fang Phase 0 recon - the 1.30 ambient-life animation graph

Draft v0.1, November 2, 2026. Plan 12 Phase 0 step 1. Owner: Jeff Fabish. Author: the Fang session, read-only, from `vendor/dayz-1.30-anim-graphs/` (copied November 2, 2026 from Jeff's WorkDrive extract of DayZ 1.30.164200) and `vendor/dayz-1.30-scripts/`. Paths below are relative to those two folders unless they start with `DZ/`, which is the game's own resource path as the files spell it.

This recon answers the three questions Plan 12 step 1 asked and adds two findings the plan did not anticipate: a scripted animal command cannot pick a clip, and the ambient graph never uses its attack command. Both change Phase 0 row F0-07 and the Phase 1 strike design; the recommendations are at the end. No code was written.

## What this is not

- Not the Blender or Workbench work. Step 2 and step 3 start after the go on this document.
- Not a Phase 1 design. The strike mechanism is ranked here because the placeholder must carry what it needs; the choice is a Plan 12 v0.3 decision.
- Not a reading of the `.anm` binaries. The clip files were not extracted into `vendor/`; what they contain (bone tracks, embedded events) is inferred from the graph's conditions and the add-on's README, and step 2 confirms it on the first import.

## Files read

| File | Size | Format | What it is |
| --- | --- | --- | --- |
| `animals/AmbientLife/ambientlife_graph.agr` | 2,007 B | text, `AnimSrcGraph` | The graph header: the animation-set template it uses, the control template (variables and commands the engine drives), and the list of graph files |
| `animals/AmbientLife/ambientlife_maingraph.agr` | 16,910 B | text, `$AnimGraph 7` | The state machine source: `Master_SM` and its sub-machines, every transition with its condition |
| `animals/AmbientLife/ambientlife_maingraph.agf` | 26,561 B | text | The same graph in its compiled-source form (nodes, conditions, `Source "Default.Normal.<slot>"` bindings); the file `ambientlife_graph.agr` lists under `GraphFilesResourceNames` |
| `animals/AmbientLife/ambientlife.ast` | 893 B | text, `AnimSetTemplateSource` | The slot template: one group `Default`, one column `Normal`, 33 animation names |
| `animals/AmbientLife/ambientlife.ae` | 100 B | text, `AnimEventTable` | Three named events: `Death_SimulationFinish`, `EatingPose1`, `TurnStartDuration` |
| `animals/AmbientLife/ambientlife.aw` | 1,982 B | text, `BaseSource` | The Workbench workspace: the template, the five instances, the graph, five preview `.xob` models, the event table |
| `animals/AmbientLife/*.asi` (hen, hen female, hare, fox) and `animals_takistan/AmbientLife/varanus_griseus_skeleton.asi` | 5.2-5.7 KB each | text, `AnimSetInstanceSource` | One instance per species: 33 lines, each `Default.Normal.<slot>` to one `.anm` resource |
| `DZ/animals_takistan/varanus_griseus/config.cpp:20-114,330-360` | | config | The monitor's `enfanimsys`, `AnimEvents`, `CommandMoveSettings` |
| `DZ/AI_takistan/config.cpp:2010-2060` | | config | `AmbientLife_Varanus_Griseus`: `HeadLookBoneName = "Head"`, alert ranges, slot speeds |
| `scripts/3_Game/DayZ/Entities/DayZAnimal.c:32,562-570,619-675,730-770` | | Enforce | `SetAnimationInstanceByName`, the command ids, `DayZAnimalCommandScript`, the `StartCommand_*` natives, the command handler |
| `scripts/3_Game/DayZ/Anim/AnimCommand.c:1-14` | | Enforce | `AnimCommandBase` |

Every resource reference in these files is a GUID plus a path, for example `"{9968FEA53DB3D19E}DZ/animals/animations/!graph_files/AmbientLife/AmbientLife.ast"`. Workbench assigns the GUID when a file is registered in a project; a hand-written reference with a wrong GUID is a Workbench problem to expect at step 3, not a game problem.

## Findings

### R1. The graph set is five text files, and the instance is a flat table

1.30's "text graph format" is exactly that: every file above opens in an editor. The instance file (`.asi`) is the only per-species file, and it is nothing but 33 lines of `slot name` to `.anm resource`. It names no skeleton, no bones, and no graph; the skeleton coupling lives in `config.cpp` `enfanimsys` (`meshObject`, `skeletonName`, `defaultinstance`, `graphname`, `startnode = "Master_SM"`) and inside the `.anm` files, whose bone tracks must match the `.xob` skeleton. This answers step 1 question 1: the instance binds by resource, not by bone list, and all five vanilla instances bind the same 33 slots.

### R2. All 33 slots are bound by every species, and one clip may serve several slots

Each vanilla `.asi` binds every slot in the template. The monitor binds `stand_nervous_idle` to the same `stand_nervous.anm` it uses for `stand_nervous`, and binds `run` to `run_escape.anm` and `run_escape` to `run.anm` (a swap, presumably deliberate: the monitor's escape gait is its ordinary run). So the engine accepts one `.anm` on several slots, and the placeholder can bind all 33 slots with two clips. Whether an unbound slot errors at load is untested and does not need testing: bind all 33. This answers step 1 question 2.

The slot list, grouped by the state that plays it:

| Slots | Played by | Placeholder clip |
| --- | --- | --- |
| `stand_eating`, `stand_eating_idle_1` to `_4`, `stand_eating_2_stand_nervous`, `stand_nervous_2_stand_eating`, `stand_nervous`, `stand_nervous_idle`, `stand_nervous_left`, `stand_nervous_right` | `Calm_SM` (eating and look-around states, driven by `behaviorAction` 1 and 3 and `alertLevel`) | `idle` |
| `walk`, `walk_eating`, `walk_eating_idle_1` | `Moving_SM` at `speed` below 2, `Calm_SM` walk-eating (`behaviorAction` 4) | `crawl` |
| `run`, `run_escape`, `run_turn_L144`, `run_turn_R144` | `Moving_SM` at `speed` 2 and above, `TurnWhileMoving_SM` | `crawl` |
| `start_walk_F`, `_L72`, `_L144`, `_R72`, `_R144`, `start_run_F`, `_L72`, `_L144`, `_R72`, `_R144` | `StartMove_SM`, chosen by `turn` in five sectors (`-36..36`, `36..108`, `108..180` and mirrored), `speed` 2.5 picks run over walk | `crawl` |
| `jump_walk_H110_L350`, `jump_run_H110_L350`, `jump_run_H160_L350` | `Jumps_SM` on `CMD_JumpType` | `crawl` |
| `death`, `death_end` | `Death_ANIM` (no loop) then `DeathEnd_ANIM` (loop) on `CMD_Death` | `idle` in Phase 0 (a killed placeholder keeps swaying; a third 10-frame `dead` clip removes that for about ten minutes of Blender work; see the go question) |

### R3. The engine drives the graph through eleven variables and two of eight commands

`ambientlife_graph.agr` declares the control template: floats `speed` (0-5), `turn` (-180..180), `lookXAngle`, `lookYAngle`, `SlopeAngleX`, `SlopeAngleZ`; ints `behaviorAction` (0-14), `alertLevel` (0-4), `InjuryLevel` (0-2), `behaviorSlotID` (0-31), `AnimalType` (1-34, default 1); commands `CMD_JumpType`, `CMD_Death`, `CMD_LookAt`, `CMD_Attack`, `CMD_AttackSuccess`, `CMD_AnimCallBack`, `CMD_Hit`, `CMD_LookAtXChange`. The engine's `BehaviourHLAmbientLife` sets the variables; `CommandMoveSettings.movementSpeedMapping[] = {0, 0.25, 0.8, 3, 5}` in the monitor's config maps its five gaits onto `speed`. No transition in either graph file reads `AnimalType`, `InjuryLevel`, `behaviorSlotID`, or the look angles; those exist for other graphs that share the template. The transitions that matter: `Master_SM` has seven states (`Moving_St` on `speed > 0`, `Calm_St` on `true`, `StartMove_ST`, `TurnWhileMoving_ST` on `turn` beyond 108 degrees, `Jump_ST`, `Death_ST`, `DeathEnd_ST`); `Moving_SM` splits walk and run at `speed >= 2` and gates turns and jumps at `speed >= 2.5`.

Only `CMD_JumpType` and `CMD_Death` appear in any transition. `CMD_Attack`, `CMD_AttackSuccess`, and `CMD_Hit` are declared and never used, so on this graph `DayZAnimal.StartCommand_Attack()` and `StartCommand_Hit()` change nothing visible. This is the fact behind the study's "no ambient template strikes", seen from the graph side, and it bears on Phase 1 (R6).

### R4. Clip events the graph waits for

Transitions read events by name: `IsEvent("LookLeft")`, `IsEvent("LookRight")`, `IsEvent("LookFront")` (the look-around states, each with a `RandomPerc`), `IsEvent("EatingPose1")` (walk-eating to stand), and the table's `TurnStartDuration` and `Death_SimulationFinish`. `ambientlife.ae` names only the last three, so `LookLeft`, `LookRight`, and `LookFront` are embedded in the `stand_nervous*.anm` clips themselves. A placeholder `idle` with no events still works: the look-around transitions never fire and the snake stays in `LookAroundNervous_ST` or `Eating_ST`, which is idle either way. The DayZ Animation Tools add-on edits animation events in its sidebar (README), so step 2 can add `LookLeft` at frame 20 and `LookFront` at frame 40 of `idle` to exercise the path; optional for Phase 0.

### R5. Bones the engine and the graph name

The graph names no bone. The engine reads `HeadLookBoneName = "Head"` from the AI template (`DZ/AI_takistan/config.cpp:2012`); the monitor's `AnimEvents.Damages.Bite` points at `WolfBiteDamage`, whose bone is the wolf's `tongue3`, which the monitor does not have, so a missing damage bone is tolerated in vanilla. For the placeholder the bone set is `root`, `spine1`, `head`, named exactly `Head` for the look bone because the AI template reads that name (the template is copied for `AmbientLife_NimbySnake`, so the name is Fang's to choose, and `Head` keeps the copy unchanged). No `pin_lookat` bone exists in anything read; the study's risk row on pin bones came from the player skeleton and does not apply to ambient animals. This answers step 1 question 3.

### R6. Script-side levers: a scripted command is physics only; the instance swap is the one clip-level lever

`DayZAnimalCommandScript` (`DayZAnimal.c:619-653`) exposes `SetFlagFinished`, pre-physics get and set translation and rotation in local space, post-physics position and rotation in world space, and `PostPhys_LockRotation`. It has no method that names a clip, a slot, a graph variable, or a command. A scripted command therefore holds or moves the body while the graph keeps playing whatever its variables select. `DayZCreature.SetAnimationInstanceByName(string animation_instance_name, int instance_uuid, float duration)` (`DayZAnimal.c:32`) swaps the whole `.asi` at runtime; it is the only script-reachable way to change which clip a slot plays.

Consequences:

1. Plan 12 row F0-07 ("the debug action plays `idle` on demand through `StartCommand_ScriptInst`") cannot pass as written. The replacement is two rows: F0-07 proves the scripted command path (the debug action starts a `NimbyFangHoldCommand` that zeroes translation and locks rotation for three seconds, logs `[NIMBY][Fang][Debug] hold start` and `hold end`, and returns control; the snake stops mid-crawl and resumes), and F0-09 proves the instance swap (the debug action calls `SetAnimationInstanceByName` to a second instance `NimbySnake_Idle.asi`, every slot bound to `idle`, for five seconds, then back; the snake stops animating its crawl while still moving, then resumes).
2. The Phase 1 strike has three candidate mechanisms, ranked. (a) A copied graph `NimbySnake_Graph.agr` with an `Attack_ST` wired to `CMD_Attack`: `StartCommand_Attack()` then works, `COMMANDID_ATTACK` flows through the vanilla command handler including `SignalAIAttackEnded()` (`DayZAnimal.c:739-760`), and the `Bite` damage event fires from the clip like the wolf's. Costs owning the graph (the bear precedent, study option P4's second rung) and one state. (b) Instance swap: a `NimbySnake_Strike.asi` whose `stand_nervous` slot is the strike clip, swapped in for the clip's length by the scripted command, swapped back; no graph change, but the damage event and the attack-ended signal are Fang's to raise by hand. (c) The jump slots: bind `jump_walk_H110_L350` to the strike clip and fire `StartCommand_Jump()`; cheapest, but the jump sub-machine translates the root along a 3.5 m arc. Recommendation: (a), decided at Plan 12 v0.3 after Phase 0 shows whether Workbench saves a copied graph; F0-09 keeps (b) proven as the fallback.

### R7. The Workbench workspace is reusable as a template

`ambientlife.aw` lists the template, the five instances, the graph, the preview models, and the event table. The Fang project at step 3 gets its own `.aw` naming the vanilla template and graph GUIDs and the new `NimbySnake_Skeleton.asi`; the five vanilla preview `.xob` paths show where a preview model sits (`!graph_files/PreviewModels/<species>_anim_preview.xob`), so the placeholder gets `nimbysnake_anim_preview.xob` there for the Animation Editor's viewport.

## Answers to the step 1 questions

| # | Question | Answer |
| --- | --- | --- |
| 1 | Does the `.asi` reference the skeleton by file or by bone list, and do the hare, fox and monitor instances bind the same slot set? | Neither: it binds slot names to `.anm` resources only. The skeleton is named in `config.cpp` `enfanimsys`, and the clips carry the bone tracks. All five instances bind the same 33 slots (R1, R2) |
| 2 | Which graph slots are mandatory for `Master_SM` to enter, and can `idle` fill every one of them? | Untested which are mandatory, and moot: bind all 33, which vanilla does. `idle` on the stand and death slots, `crawl` on walk, run, start and jump slots (R2). One clip on many slots is vanilla practice |
| 3 | Which bones do the ambient template and the monitor's `Bite` event read by name? | `Head` (look bone, from the AI template). The bite bone comes from the `CfgDamages` entry Fang writes in Phase 1, so Fang names it; `tongue3` is tolerated missing in vanilla (R5) |

## Placeholder specification for step 2

| Item | Value |
| --- | --- |
| Mesh | A tube of 12 segments, 1.0 m long, 0.06 m diameter, tapering to 0.02 m at the tail; a box head 0.08 by 0.06 by 0.04 m; 200 triangles or fewer; one 64 by 64 flat texture (`nimbysnake_co`) |
| Bones | `root` at the tail end on the ground, `spine1` at 0.5 m, `Head` at 0.92 m; all three on the X axis, Z up; no pin bones (R5) |
| Clip `idle` | 60 frames at 30 fps, loop: `Head` yaws 15 degrees left and right, `spine1` 5 degrees; root static. Optional events `LookLeft` at frame 20, `LookFront` at frame 40 (R4) |
| Clip `crawl` | 60 frames at 30 fps, loop: `spine1` and `Head` lateral wave 20 degrees out of phase; `root` translates 0.4 m forward per cycle (0.2 m/s, inside the walk band of `movementSpeedMapping` 0.25-0.8) |
| Clip `dead` (if ruled in) | 10 frames, no loop: whole body rolls 90 degrees about X and holds |
| Export | Skeleton and each clip as `.txa` through the add-on; files under `mods/own/nimby-pack/NimbyFang/src/blender/` (`.blend`) and `src/txa/` |
| Instances | `NimbySnake_Skeleton.asi` (the slot map in R2) and `NimbySnake_Idle.asi` (every slot `idle`) for F0-09 |

## Recommended Plan 12 v0.3 changes

1. Replace F0-07 with the hold-command row and add F0-09, the instance-swap row (R6, consequence 1).
2. Step 4's `NimbyFangScripts` debug action becomes two actions, `ActionNimbyFangHold` and `ActionNimbyFangSwapIdle`, both server side and present only with `$profile:NimbyFang\debug`.
3. Add the strike-mechanism ranking (R6, consequence 2) to the Phase 1 outline as the first v0.3 decision.
4. Remove the pin-bone line from the placeholder step and from the study's risk row when the study is next revised (R5).
5. Step 3 notes: a second `.asi` is authored beside the first; the Fang `.aw` lists both.

## Go questions

Three questions before step 2 starts, each with the recommendation.

| # | Question | Recommendation |
| --- | --- | --- |
| G1 | Accept the F0-07 replacement and F0-09 (R6)? | Yes; without it the Phase 0 exit cannot be met as written |
| G2 | Two clips as frozen in P5, or three with a 10-frame `dead`? | Three; the cost is about ten minutes and it removes a visibly wrong behaviour from the screenshots Jeff takes |
| G3 | Start step 2 (Blender) now, in this session, or at the next? | Now if Jeff is at the machine to run Blender headless exports and paste results; otherwise next session with this document as its brief |
