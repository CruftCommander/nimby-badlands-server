# Tack feasibility study - NIMBY-T-013 (relief clipping) and NIMBY-T-014 (missing prompt)

Draft v0.1, October 20, 2026. Owner: Jeff Fabish. Status: Part A of the study brief (`logs\brief-tack-t013-t014-study-20261020.md` in the main clone, git-ignored); no addon code changed; Part B waits on Jeff's ruling (section 6). Branch `study/tack-t013-t014`, cut from `main` at `7e9af5c`. Plan: `docs/plans/2026-10-15-tack-plan.md` v0.11. Findings come from `docs/testing/2026-10-18-tack-merged-main.md` ("NIMBY-T-011 verification and mud-brick walls"). Every vanilla API was read in `vendor/dayz-1.30-scripts/` (build 1.30.164078.38); `file:line` names the file under `vendor/dayz-1.30-scripts/scripts/` (for example `ActionTargets.c` is `4_World/DayZ/Classes/UserActionsComponent/ActionTargets.c`, `DayZPhysics.c` is `3_Game/DayZ/Global/DayZPhysics.c`, `EnEntity.c` is `1_Core/DayZ/proto/EnEntity.c`) unless a path is given, and addon paths are relative to `mods/own/nimby-pack/NimbyTack/Scripts/`.

Target: NIMBY | HARDCORE | BADLANDS | 1pp, DayZ 1.30 / Nasdara; evidence from the local replica `D:\DayZServer`, profile `main-round1`, build `@NIMBY-20261019-215218`.

## Verdict

- **NIMBY-T-014 (no prompt):** the logs cannot name the cause, but they narrow it. The client session in which Jeff saw no prompt (`script_2026-10-20_18-25-58.log`, Tack debug on) holds no `probe:` line of any kind, so on every frame the pin action's condition stopped before Tack's raycast or was never called. That excludes the three probe refusals that already write a debug line (`no-hit`, `no-physics-normal`, `not-wall-angle`) for that session. The test kit put a fresh single sheet in inventory on both connects of that session, and fresh tape and nails on the second, which makes the session-wide gates (stacked sheet, empty tool, no nail) unlikely. The two leading causes are both per wall and both silent: vanilla cursor targeting never hands Tack a usable target on those faces, and Tack's reach (1.5 m from the head) is shorter than the vanilla prompt's (2.0 m from the feet or the head). One Part B session decides between them.
- **NIMBY-T-013 (clipping):** no script ray reads the drawn stones. At the clipped pin the fire-geometry layer stood 3.0 cm and the physics shell about 6 cm in front of the view-geometry hit, so a sheet at 3 cm sits on the fire layer and inside the physics shell. Recommended: a per-material offset for `mud_brick` (option a, 6 cm, tunable from the profile, the same shape Jeff ruled for tents in NIMBY-T-008), with a debug-only footprint measurement in Part B to decide whether the corner probe (option b) is worth its code.
- **Normals:** the "fire-geometry normal" is not a normal. It is `RaycastRVResult.dir`, the ray, the same misreading the October 18 round found for the view hit. Placement never reads it. No code path pins a note on the far side of a wall. One new observation: a pin can take the normal of a different surface that stands in front of the visible one (section 4.3).

## What this is not

- Not a code change. Part A changes no addon file; Part B is a proposal (section 5).
- Not a reopening of the 3 cm `wallOffsetMeters` default. Both T-013 options leave flat walls at 3 cm.
- Not a change to `types.xml`, the replica, Hands, Scrawl, or any orchestrator-owned file.

## 1. Evidence

| Source | What it holds |
| --- | --- |
| `profiles\main-round1\DayZServer_x64_2026-10-20_18-05-44.RPT` | Server Tack lines 18:05-18:21, including both mud-brick pins (18:16:41.982, 18:17:01.518) and their `probe:` lines |
| `profiles\main-round1\DayZServer_x64_2026-10-20_18-24-46.RPT` and `DayZServer_x64_2026-10-20_18-24-47.ADM` | The no-prompt session: three Tack lines (config and `loaded`), no `pin` or `refuse`; the `.ADM` positions, death, respawn, and the item in hands |
| `profiles\main-round1\script_2026-10-20_18-24-46.log` | The test kit placement per connect (`[NIMBY][TestKit] <class> -> inventory|ground`) |
| `%LOCALAPPDATA%\DayZ\script_2026-10-20_18-07-15.log` | Client trace, 18:07-18:21: 847 probe triplets on `Land_Shed_W6`, `Land_House_2M1`, and a mud-brick object with no config class |
| `%LOCALAPPDATA%\DayZ\script_2026-10-20_18-22-35.log` | Client trace, 18:22-18:24: two probe triplets on `Land_Shed_W6` and two reads |
| `%LOCALAPPDATA%\DayZ\script_2026-10-20_18-25-58.log` | Client, 18:25-18:29: `[NIMBY][Tack] loaded; server=false debug=true` at 18:26:36 and no other Tack line |
| Client logs October 19-20 and server `logs\testserver-20261018-*\nimby-events.log` | Fire-to-view `gap=` values by surface and object (section 3.2) |
| `D:\DayZServer\mpmissions\dayzOffline.nasdara\init.c` (replica, not in the repository) | The test kit: `InvokeOnConnect` creates three `Paper`, four pens, `Hammer`, `Nail`, `DuctTape`, `EpoxyPutty`, and seven more items with `CreateInInventory`, falling back to the ground |

Precision. `float.ToString()` prints about six significant digits, so a logged position carries 1 cm in x (`4026.13`), 10 cm in z (`14111.5`), and 1 mm in y (`299.118`). Depths derived from logged positions are coarse wherever the normal has a large z part. `gap=` (distance from the fire hit to the view hit) and `fraction=` are computed in the engine and printed at full precision; section 3 leans on them.

## 2. NIMBY-T-014: why no prompt appears

### 2.1. The path from the cursor to Tack's condition

1. Every frame the client casts one ray from the camera, 5 m, all objects (`ActionTargets.c:245-255`, `c_RayDistance` `:840`). Per distance tier it keeps the first result, skipping objects that cannot be action targets (`:267`) and proxy hits (`hierLevel > 0`) whose parent is a player or whose object has `GetID() < 1`, unless the parent is a tent (`:271-275`, `:411-417`). The first kept result sets the cursor position used by every target (`:283-284`, `:368`).
2. Candidates get a utility (`:490-537`): a hit more than 3 m from the feet is out (`:493`); the object under the cursor scores 0.01 when it has no config class (`:501-502`) and 0.25 when it is a building (`:504-505`); any other candidate scores up to 0.35 by how close its origin is to the ray (`:528-529`, constants `:849-851`). The cone search that adds nearby objects runs only when looking down at least 45 degrees (`:324-325`).
3. `FindActionTarget` takes the highest-utility target that has an object or a parent (`ActionManagerClient.c:498-527`).
4. The hold-left-mouse input reads the actions of the item in hands (`ActionInput.c:627-636`, `:377-380`, `:310-352`).
5. `ActionBase.Can` checks, in order: the player-state mask (`ActionBase.c:931`; the mask is built at `:875-925`; Tack inherits the defaults, so raised hands, throwing mode, lying on the back, swimming, ladders, vehicles, and restraint refuse, `:371-409`), the target condition `CCTCursor` (`:947`), the item condition `CCINonRuined` (`:951`, `ItemConditionComponents/CCINonRuined.c:3-6`), then Tack's `ActionCondition` (`:954`).
6. `CCTCursor.Can` takes the object, else the parent; refuses a destroyed one; and requires the cursor within 2.0 m (`UAMaxDistances.DEFAULT`, `Actions/ActionConstants.c:114`) of the feet or the head (`TargetConditionsComponents/CCTCursor.c:15-36`).

### 2.2. What `GetObject()` and `GetParent()` return here

| Cursor hit | `GetObject()` | `GetParent()` | Source |
| --- | --- | --- | --- |
| A landscape object (`hierLevel` 0): a `Land_*` building or a map object with no config class | That object | null | `ActionTargets.c:348-352,368`; `DayZPhysics.c:100-106` |
| A proxy with a valid id (`GetID() >= 1`) | The proxy | The most-parent object, the building | `ActionTargets.c:271-275`; `DayZPhysics.c:100-101` |
| A proxy with no id (`GetID() < 1`), parent not a tent | Not a target. The tier is skipped and the cursor lands on the next kept hit along the ray, or on nothing | - | `ActionTargets.c:274`; `EnEntity.c:518` |

Tack reads the target the same way `CCTCursor` does, object first, then parent (`4_World/nimbytack_world.c:70-74`). The `wall=` token in the `pin` line does not come from the target: it is the first non-player, non-item object on Tack's own ray (`:104-146`), so `wall=` and the action target can differ.

The building at 4026, 14111 is more than one object. The client trace hits `Land_House_2M1` (mud brick on two faces, 18:16:19-18:16:41) and, on the face where the 18:17 note hangs, an object with an empty class name whose physics body also has an empty class name (18:16:57-18:17:01). An empty class is what both a building proxy and a plain map object return. Neither the logs nor the code record `hierLevel` or the id, so which one it is stays open; Part B logs both.

### 2.3. Every false path and whether it fires at a visible mud-brick wall

"Logged" means a line exists today with the Tack debug flag on.

| Id | Path | Where | Logged | Fires in front of a visible mud-brick wall? |
| --- | --- | --- | --- | --- |
| V1 | Player state (raised, throwing, on back, and so on) | `ActionBase.c:931` | No | Yes, but it follows the player, not the wall |
| V2 | No target object: every hit on the cursor ray dropped, or only hits more than 3 m from the feet | `CCTCursor.c:20-25`; `ActionTargets.c:267-275,493` | No | Yes, per face, if the face is a proxy piece with no id |
| V3 | Target destroyed | `CCTCursor.c:24` | No | Not expected on a static building |
| V4 | Cursor more than 2.0 m from both feet and head | `CCTCursor.c:32-35` | No | Yes: when V2 skips the near face and the cursor lands farther away, or the player stands back |
| V5 | Tool ruined | `CCINonRuined.c:5` | No | No: kit tools are pristine |
| A1 | Tool is not this fastener | `nimbytack_actions.c:75-78` | No | No |
| A2 | Tape or epoxy below one portion (10) | `nimbytack_actions.c:80-83` | No | Only session-wide, for one tool |
| A3 | No nail (hammer path) | `nimbytack_actions.c:84-87` | No | Only session-wide |
| A4 | No single sheet: stacks, ruined sheets, and pinned notes are skipped | `nimbytack_actions.c:88-91`; `nimbytack_world.c:162-203` | No | Only session-wide: every wall goes silent at once |
| P1 | `no-target`, `no-object` | `nimbytack_world.c:65-79` | No | No: cannot fail once `CCTCursor` passed, which tested the same thing |
| P2 | `target-not-building`: the target is an item or a player | `nimbytack_world.c:80-84` | No | Yes, per spot: an item whose origin lies within about 0.64 m of the ray outranks a class-less wall (0.01), within about 0.35 m a `Land_*` building (0.25) |
| P3 | `out-of-reach`: head more than `reachMeters` (1.5) from the cursor | `nimbytack_world.c:85-91` | No | Yes, per stance: any aim between Tack's 1.5 m from the head and vanilla's 2.0 m from the feet or the head |
| P4 | `no-hit`: Tack's ray hits nothing | `nimbytack_world.c:98-103` | Yes | Excluded for the 18:25 session |
| P5 | `no-building-hit`: Tack's ray finds only terrain, items, or players | `nimbytack_world.c:148` | No | Rarely: Tack's ray starts at the feet plus 1.6 m (`4_World/DayZ/Static/MiscGameplayFunctions.c:759-787`, `3_Game/DayZ/PlayerConstants.c:13`), the cursor ray at the camera; near edges and openings the two can disagree |
| P6 | `no-physics-normal` | `nimbytack_world.c:115-119`, `:276-285` | Yes, from `WallNormal` (`physics ray had no building hit`, `zero normal`), though the reason token itself is not printed | Yes: four frames on `Land_House_2M1` at 18:16:19.927-18:16:20.344. Tack's physics ray ends 10 cm past the view hit (`:94`), so a physics face more than 10 cm behind the visible face is never found. Excluded for the 18:25 session |
| P7 | `not-wall-angle` | `nimbytack_world.c:122-127`; `nimbytack_rules.c:132-140` | Yes | No. Over 378 mud-brick frames the largest vertical part of the physics normal was 0.102 (5.9 degrees) against a limit of 0.423 (25 degrees); the physics shell is one plane per face, so surface roughness never reaches this test |
| K1 | Knife on a non-wood surface | `nimbytack_actions.c:100-103` | No | Not in this report (tape, nail, epoxy) |

The brief lists `no-physics-normal` among the silent returns; `WallNormal` does write a debug line for it, so with the debug flag on it is visible.

### 2.4. What the October 20 evening logs show

- Client debug was on in all three sessions: `[NIMBY][Tack] loaded; server=false debug=true` at 18:07:53, 18:23:12, and 18:26:36.
- In the 18:07 session, holding a fastener at a wall in reach writes about 30 `probe:` triplets a second (576 on `Land_Shed_W6`, 156 on `Land_House_2M1`, 115 on the class-less mud-brick object).
- In the 18:25 session there is no `probe:` line at all from 18:26:36 to the end of the log, and the server has no `pin` or `refuse` line. So V1-V5, A1-A4, P2, P3, and P5 are the only candidates there; P4, P6, and P7 each write a line and did not.
- The session had two lives (`.ADM`): connected 18:26:41 at 4034.2, 14101.0; suicide 18:27:09 at 4028.6, 14106.4; respawned 18:27:51 at the fixed dam spawn (3760, 14275); at 18:28:58 at 4031.5, 14105.3, sitting with `DuctTape` in hands; disconnected 18:29:13.
- The test kit ran on both connects. At 18:26:41 the inventory was full: one `Paper` went to inventory and every other kit item (two more sheets, pens, hammer, nail, tape, epoxy, knife, and the rest) to the ground at 4034.2, 14101.0. At 18:27:51 three `Paper`, the hammer, the nail, and the tape went to inventory, the epoxy and the rest to the ground at the dam. A kit sheet has quantity 1 (`NimbyNotes/config.cpp:51`), a kit tape 100 (`DZ/gear/consumables/config.cpp:151`), a kit `Nail` 70 (`:1909`); no epoxy pin was made on October 20, so the epoxy carried from the 18:07 and 18:23 kits was full. In both lives, then, a single sheet and a full fastener were in inventory unless Jeff combined or moved them by hand.

### 2.5. Ranked causes

1. **V2 and V4, vanilla targeting.** The face is a proxy piece without an id, so the cursor ray skips it and Tack's action is checked against a farther hit or no object; `CCTCursor` refuses before Tack runs. Fits: per face, no line at all, a building made of more than one object. Not proven: `hierLevel` and the id are not logged.
2. **P3, Tack's reach.** Tack measures 1.5 m from a point 1.6 m above the feet; the prompt's vanilla gate allows 2.0 m from the feet or that point. Standing a normal step back, or aiming low or high, falls in between and is silent. On `Land_House_2M1` a physics face stands 0.6 to 0.7 m in front of the visible face at knee height (18:16:37-18:16:41, `fraction=0.38`), something like a bench or plinth that keeps the player back; the 18:16:41 pin there was made from about 1.4 m (derived from `fraction` and the logged positions). Weaker if Jeff stood against the wall.
3. **P2, an item wins the target.** Needs an item near the ray: the kit put 17 items (16 kit items and the bag) on the ground at 4034.2, 14101.0 at 18:26:41, and two pinned notes hang on this building. Fires per spot.
4. **P5, Tack's ray misses what the cursor ray hit.** Edges and openings only.
5. **A4, A2, A3, the session gates.** Unlikely for this session given the kit, and they silence every wall at once, but they are the only class that also silences the control wall in section 2.6.

### 2.6. Telling a stacked sheet from a bad wall in game

- **Inventory.** Hover every `Paper`: a Scrawl stack shows a quantity (canBeSplit, up to 5); a single sheet shows 1 or none. At least one single, unruined sheet must be in inventory, not on the ground.
- **Control wall.** `Land_Shed_W6` at 4039.8, 14103.7 (the shed where the 18:15 nail note hangs) prompted on every frame in the 18:07 and 18:22 sessions. A prompt on the shed and none on the mud-brick face means the face. No prompt on the shed either means a session gate (sheet, portion, nail, or player state).
- **Part B line.** It names the reason directly: `reason=no-single-sheet` for a stack, a vanilla or probe reason for a face.

## 3. NIMBY-T-013: relief depth and two fixes

### 3.1. What the probes can and cannot measure

Tack reads three simplified geometry layers: the view layer (`RaycastRVProxy` with `ObjIntersectView`; places the pin), the fire layer (`ObjIntersectFire`; names the surface), and the physics shell (`RayCastBullet` on the building layer; gives the normal). The drawn stones live in the model's resolution level, which no script ray reads: the intersection types are fire, view, geometry, indirect fire, and none (`DayZPhysics.c:67-71`). The probes therefore bound how far the layers disagree; how deep the drawn stones go is answered only by eye.

### 3.2. Measured

At the T-013 pin (server, 18:17:01.517), along x (the precise axis, 82 percent of the normal): view hit 4026.13, fire hit 4026.15, physics hit 4026.20, with the ray running toward lower x. The fire layer stood 3.0 cm in front of the view hit (`gap=0.0303839`, along the ray) and the physics shell about 6 cm in front (plus or minus 3 cm from z precision; `fraction=0.79` agrees). The sheet went to the view hit plus 3 cm: on the fire layer and inside the physics shell, and Jeff saw stones over its left part.

Fire-to-view `gap=` in centimeters, from the October 19-20 client traces and the October 18 server runs:

| Surface | Object | Frames | Median | p99 | Max | Over 3 cm |
| --- | --- | --- | --- | --- | --- | --- |
| `mud_brick` | No class (the T-013 wall, October 20) | 249 | 4.8 | 6.4 | 10.8 | 95% |
| `mud_brick` | No class (October 18) | 128 | 6.7 | 6.7 | 6.7 | 100% |
| `mud_brick` | `Land_House_1M4` | 181 | 0.1 | 3.8 | 4.1 | 12% |
| `mud_brick` | `Land_House_2M2` | 114 | 17.8 | 21.0 | 21.1 | 100% |
| `mud_brick` | `Land_House_2M1` | 135 | 72.5 | 83.2 | 86.3 | 97% (one face, section 4.3) |
| `mud_brick` | `Land_House_1M3_Dam2`, `Land_House_2S1_Dam3`, `Land_House_1M1` (October 18) | 128 each | 0.6, 0.1, 0.1 | - | - | 0% |
| `wood` | `Land_Shed_W6` | 578 | 7.2 | 7.3 | 7.3 | 100% |
| `wood_desk` | `Land_Shed_W1` | 97 | 0.1 | 0.2 | 3.2 | 1% |
| `concrete` | `Land_KirovDam_*` | 324 | 0.0 to 0.1 | - | 1.3 | 0% |

The order of the layers changes from model to model. On the class-less mud-brick piece the fire and physics layers both stand in front of the view hit. On `Land_Shed_W6` the physics shell stands in front and the fire layer 7 cm behind (18:14:47), and the 18:15 shed note did not clip. No single layer tracks the drawn surface across models.

Reading: on the Takistan mud-brick pieces the outermost simplified layer stands 3 to 11 cm proud of the view hit; a sheet needs roughly 6 to 8 cm there to clear what the physics shell encloses. `Land_House_2M2`'s 18 cm and `Land_House_2M1`'s 73 cm are recesses or features in front of the face, not stone relief.

### 3.3. Options

Both leave every flat wall of every other material at the 3 cm default.

| | (a) Per-material offset | (b) Footprint probe |
| --- | --- | --- |
| What | New T6 key `roughSurfaceOffsetMeters` (default 0.06, clamped 5 mm to 20 cm, next to `unknownSurfaceOffsetMeters`); `NimbyTackRules.OffsetFor` gains a branch for surface names on a code list, starting with `mud_brick`; the `pin` line's `offset=` shows the value used | On the server, at pin time only: 5 short rays (corners and center of the sheet's footprint, size from the Paper model's collision box, `Object.c:370`) cast back along the normal from 30 cm out, against the view and fire layers of the same object; the standoff is the largest depth in front of the hit plane, ignoring samples that miss the object or exceed a cap (about 10 cm); offset = `wallOffsetMeters` + standoff |
| Flat walls | Unchanged for every material off the list; a smooth (plastered) face that reports `mud_brick` floats 6 cm | Standoff 0, so 3 cm everywhere |
| Cost | About 20 lines in `nimbytack_rules.c` and `nimbytack_config.c`, none in `4_World`; 4 self-test cases; tunable from the profile without a rebuild | About 80 to 100 lines in `nimbytack_world.c`, a pure standoff function and about 6 self-test cases in `nimbytack_rules.c`; 10 raycasts per pin on the server, none per frame |
| Risk | The list is a guess per material; a face deeper than the value still clips; plastered mud-brick faces float. Jeff accepted 6 cm on tents (NIMBY-T-008) | Samples a simplified layer, not the stones: under-corrects where only the drawn model has relief, over-corrects where a layer stands off (the shed's fire layer, 7 cm); corners near windows, doors, edges, and adjacent pieces; the sheet's size is not measured yet; more code in the half a Bohemia update breaks |

### 3.4. Recommendation

Option (a) now, with `mud_brick` alone on the list and 0.06 as the default, because it is the pattern Jeff already ruled for tents, it is tunable from `config.json`, and it adds no engine-facing code. Measure with the debug-only footprint line in Part B (B5) on the same build. Move to option (b) only if B5 shows that the view or fire layer follows the stones and option (a) visibly floats notes on plastered faces.

## 4. Why the two normals disagree, and the far side

### 4.1. The fire-geometry "normal" is the ray

The fire line's `normal=` is `RaycastRVResult.dir` (`nimbytack_world.c:348-349`), which for a line-object hit is the direction and size of the intersection, that is, the ray (`DayZPhysics.c:104`), not the face normal. Evidence: on one `Land_House_2M1` face, where the physics normal stays at `<0.800, 0.064, -0.597>` (18:16:19.676 to .876), the fire "normal" swings from `<-0.040, -0.239, 0.970>` to `<-0.669, 0.055, 0.741>` as Jeff's view moves. Over 1,069 frames on October 20 its dot product with the physics normal has a median of -0.993: it points into the wall, the physics normal points out. At the T-013 pin, `<-0.706, -0.005, 0.708>` against `<0.821, -0.075, -0.566>` gives -0.98. Nothing but the debug line reads this value.

### 4.2. No path pins on the far side

The sheet goes to the view hit plus the offset along the physics normal (`nimbytack_rules.c:163-166`), and the normal is flipped to face back along Tack's ray (`nimbytack_world.c:287-290`). The sheet therefore always sits on the near side of the view hit. A far-side note needs the view hit itself on the far face: a view layer with no near face (one-sided cloth), or Tack's ray starting inside geometry (it starts 1.6 m above the feet, not at the eye, so a player pressed into a corner can start it inside a wall). No log shows either; the 18:17 note rendered on Jeff's side.

### 4.3. New observation: a normal from a surface in front of the face

The 18:16:41.982 tape pin on `Land_House_2M1` (taken down by Jeff 10 seconds later, 18:16:52.456): the physics ray hit at `fraction=0.381536`, about 0.6 m along the normal (0.73 m along the ray by `gap=0.725088`) in front of the view hit, which was at y 297.926, 0.7 m above Jeff's feet. The note went to the view hit with the normal of whatever stands in front of it. `WallNormal` accepts any building-layer hit on the ray at any distance before the view hit (`nimbytack_world.c:276-298`). A guard would accept the physics normal only when its hit lies within a set distance (for example 15 cm) of the view hit along the ray, and refuse otherwise. This is not T-013 or T-014; Jeff rules whether it becomes a finding (D3).

## 5. Part B proposal: diagnostic lines

### 5.1. Lines

All lines are `[NIMBY][Tack][Debug]`, written only with `$profile:NimbyTack\debug` (shared rules 4.3), on the side that evaluates the condition. No persisted class, field, or config key changes; no orchestrator-owned file is touched.

| Id | Line | When | Fields |
| --- | --- | --- | --- |
| B1 | `pin-check` | Client and server. Once when the pair (reason, target object) changes for the fastener in hands, with at least 250 ms between lines; never per frame | `fastener`, `result=ok|refuse`, `reason`, `detail`, the target's class (`-` when empty), `shape=` (`GetShapeName()`, `Object.c:503`), `id=` (`GetID()`), `proxy=` (`ActionTarget.IsProxy()`, `ActionTargets.c:152`), the parent's class and shape, `component=`, `cursor=` (`GetCursorHitPos()`), `head=` and `feet=` distances in meters. Built in three statements of at most 16 concatenations, printed once; booleans through `NimbyTackLog.Bool` |
| B2 | `cursor-ray` | Client, once per change to `no-target-object`, `cursor-too-far`, or `target-not-building` | The vanilla cursor ray rebuilt (camera position and direction, 5 m, all objects, as `ActionTargets.c:245-255`): up to four hits with class, shape, `hierLevel`, id, parent class, distance, and `CanBeActionTarget()`. Confirms or kills cause 1 |
| B3 | Trace reduction | Client | The per-frame `probe:` lines from the pin condition go behind the B1 gate; the server keeps its full trace at pin time. Answers the pin half of the carried cleanup item (the October 19 record attributes the 1,950 lines to the read action, which runs no probe; they come from the pin condition, `nimbytack_actions.c:94-96`) |
| B4 | Precision | Server and client | The fire line's `normal=` becomes `ray=` (section 4.1); the fire and physics lines gain `depth=`, the signed distance from the view hit along the normal, computed in the engine, so relief no longer depends on printed positions |
| B5 | `footprint` (optional) | Server, at pin time, debug only, placement unchanged | The section 3.3 option (b) samples logged without being used: corner and center depths on the view and fire layers, and the sheet size from `GetCollisionBox` once per start |

Reasons B1 names, each from the first check that fails:

| Reason | Check | Detail |
| --- | --- | --- |
| `player-state` | `ActionBase.c:931` | The mask bits that refused |
| `no-target-object` | `CCTCursor.c:20-25` | - |
| `target-destroyed` | `CCTCursor.c:24` | - |
| `cursor-too-far` | `CCTCursor.c:32-35` | `head`, `feet`, limit 2.0 |
| `tool-ruined` | `CCINonRuined.c:5` | - |
| `no-portion` | `nimbytack_actions.c:80` | quantity and portion |
| `no-nail` | `nimbytack_actions.c:84` | - |
| `no-single-sheet` | `nimbytack_actions.c:88` | `singles=`, `stacks=`, `ruined=` |
| `target-not-building` | `nimbytack_world.c:80` | the target's class |
| `out-of-reach` | `nimbytack_world.c:87` | `head` against `reachMeters` |
| `no-hit`, `no-building-hit` | `nimbytack_world.c:98,148` | first result's class |
| `no-physics-normal` | `nimbytack_world.c:115` | `none` or `zero` |
| `not-wall-angle` | `nimbytack_world.c:122` | the normal's vertical part |
| `not-wood` | `nimbytack_actions.c:100` | the surface |
| `ok` | - | the surface and the offset the server would use |

How: `ActionNimbyTackPinBase` overrides the public `Can(player, target, item, condition_mask)` (`ActionBase.c:929`). It clears the reason, calls `super.Can`, and on success logs `ok`; Tack's own checks set the reason as they refuse. When `super.Can` fails with no reason set, the failure happened in the vanilla stage, and the override re-runs the cheap vanilla tests (mask, target, distance, item) to name it. Those tests run only on a refusal. The server's production `refuse pin` line also gains the target's class, shape, and the cursor position.

Self-test: the gate (key change plus the 250 ms floor), the signed depth, and the sheet summary become pure functions in `nimbytack_rules.c` with about 8 new cases (57 to about 65). `NOTICE` gains a line. The test record gains a dated section with the steps below, evidence filled from the lines, result column empty. Iteration builds use `-SkipBootCheck`; the boot check that runs the self-test starts a real server on `D:\DayZServer`, so it waits for Jeff (D4).

Example (shapes shown for illustration, not measured):

```
[NIMBY][Tack][Debug] pin-check fastener=tape result=refuse reason=out-of-reach detail=head=1.71 limit=1.5 target=Land_House_2M1 shape=<p3d path> id=<n> proxy=false parent=- component=172 cursor=4032.4 298.6 14109.4 head=1.71 feet=1.38
[NIMBY][Tack][Debug] pin-check fastener=tape result=refuse reason=no-target-object detail=- target=- shape=- id=0 proxy=false parent=- component=-1 cursor=4027.9 298.4 14112.6 head=3.42 feet=3.31
[NIMBY][Tack][Debug] cursor-ray hit=1 class=- shape=<p3d path> hier=1 id=0 parent=Land_House_2M1 dist=0.84 target=false
```

### 5.2. In-game steps at 4026, 14111

Setup: the Part B build (path in the record), replica `D:\DayZServer`, instance 999, profile `main-round1`, server and client Tack debug flags on (both already exist). Only one server runs on this machine, so the NIMBY-T-011 lifetime run ends first. Note the wall-clock time at each step; the lines carry it.

| Step | Do | Expect |
| --- | --- | --- |
| G0 | Open the inventory. Hover each `Paper`; keep one written and one blank single sheet (quantity 1, not ruined) in inventory, not on the ground. Note the tape and epoxy bars and the nail count | Singles present; tape and epoxy at or above 10 |
| G1 | Control: tape in hands, stand at arm's length from `Land_Shed_W6` (4039.8, 14103.7), aim beside the 18:15 note at chest height, hold 2 seconds | Prompt; `pin-check fastener=tape result=ok target=Land_Shed_W6`. No prompt here: stop and read the reason (a session gate) |
| G2 | Known-good mud brick: aim 30 cm beside the 18:17 taped note (4026.15, 299.116, 14111.5) | Prompt; `result=ok`, an empty class, and a `shape=` path |
| G3 | Each face of the building with no prompt: (a) walk forward until the character stops, aim at chest height, hold 2 seconds; (b) aim at knee height; (c) step back one step, aim at chest height. Screenshot each face | One `pin-check` line per change naming the reason; a `cursor-ray` line when the reason is `no-target-object`, `cursor-too-far`, or `target-not-building` |
| G4 | One failing face with the hammer (nail in inventory), then with epoxy in hands | Same reason as tape if the cause is the face; a different one if it is the tool |
| G5 | Stack signature: combine the two blank sheets into a stack of 2, put the written sheet on the ground, aim at the shed | No prompt; `reason=no-single-sheet detail=singles=0 stacks=1 ruined=0`. Split the stack and pick the sheet up again |
| G6 | B5 only: pin one tape note on the class-less face and one on a `Land_House_2M1` face that now prompts; say whether stones show through | `footprint` lines with the corner depths; Jeff's eye on each note |

## 6. Decisions for Jeff

| Id | Decision | Options | Recommended |
| --- | --- | --- | --- |
| D1 | Part B scope | B1, B2, B4 as the core; B3 (client per-frame `probe:` trace behind the gate) yes or no; B5 (footprint measurement) yes or no | Core plus B3 and B5 |
| D2 | NIMBY-T-013 path | (a) now with `mud_brick` and 0.06; (a) after B5 data; (b) | (a) now, B5 alongside |
| D3 | Section 4.3, the normal from a surface in front of the face | Record as a finding and guard it (refuse when the physics hit is more than 15 cm from the view hit along the ray); or wait for Part B data | Record now, guard with the T-013 change |
| D4 | The full build with the boot check (runs the self-test) | When the replica server is free of the NIMBY-T-011 lifetime run | Jeff names the window |

Pending Part B data, not asked now: if `out-of-reach` dominates, whether Tack's reach moves to the vanilla 2.0 m measured from the feet or the head (plan T2 says 1.5 m); if `no-target-object` with a proxy of id 0 dominates, whether to accept building proxies as targets through a modded `ActionTargets.IsRaycastResultException` (`ActionTargets.c:411-417`), which changes targeting for every vanilla action and needs its own study.

## 7. Rulings, October 20, 2026

| Id | Ruling (Jeff) |
| --- | --- |
| D1 | Part B approved in full, B1 to B5 |
| D2 | Option (a) now: `roughSurfaceOffsetMeters` (default 0.06, clamped like `unknownSurfaceOffsetMeters`) for rough materials, starting with `dz\data_takistan\data\penetration\mud_brick.bisurf`; `wallOffsetMeters` stays 3 cm for every other surface; the material match and the clamp are self-tested; the config version goes to 2 with a one-time upgrade of existing files |
| D3 | Section 4.3 is recorded as NIMBY-T-015 in the Tack test record; the 15 cm guard waits for the Part B data |
| D4 | Iteration builds use `-SkipBootCheck`; the full build with the boot check waits until Jeff says `D:\DayZServer` is free of the NIMBY-T-011 lifetime run |

Built on this branch as plan v0.12 (decision T16, T3 and T6 updated). Section 5.2's steps, updated for the option (a) check, are rows TK-P0 to TK-P10 in `docs/testing/2026-10-18-tack-merged-main.md`, section "T-013 option (a) and T-014 diagnostics (Part B)". Two details differ from section 5.1: the `pin-check` gate runs on the client only, and the server writes one line per request, because the server evaluates `Can` once per request (`ActionManagerServer.c:168`) and its action objects are shared by every player (`ActionManagerBase.c:50`); and a refusal after Tack's own checks passed is named `stance-change` (`ActionBase.c:957-963`).
