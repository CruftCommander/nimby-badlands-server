# Beards feasibility

Draft v0.2, October 21, 2026. Owner: Jeff Fabish. Status: Jeff ruled on October 21, 2026 (evening) that Beards takes the replacement-head-model route (B3), because vanilla's maximum beard is not long enough for Nasdara; Beards therefore stays on the board as a model project and the leave-the-board condition in v0.1 is withdrawn. Scope of that ruling, read from `D:\DayZServer\addons\characters_heads.pbo` on October 21, 2026: 22 male head models (`m_Adam`, `m_Boris`, `m_Cyril`, `m_Denis_2`, `m_Elias`, `m_Francis`, `m_Guo`, `m_Hassan`, `m_Indar`, `m_Jose`, `m_Kaito`, `m_Lewis`, `m_Manua`, `m_Niki_2`, `m_Oliver`, `m_Peter`, `m_Quinn`, `m_Rolf`, `m_Ruslan`, `m_Seth`, `m_Taiki`, plus the `m_white_01_above0` base) for 20 `SurvivorM_*` classes, each needing a longer beard mesh that keeps the vanilla hidden-selection groups (`beard`, `hair`, `Group_HairBeard`) so masks and helmets still hide it. Every model is Claude-made under the smoking study's D2 rule; the plan, when ordered, sequences one head first as the proof and the other 21 after Jeff accepts it. Every `file:line` is relative to `vendor/dayz-1.30-scripts/scripts/` (build 1.30.164078.38).

Target: NIMBY | HARDCORE | BADLANDS | 1pp, HostHavoc, DayZ 1.30 / Nasdara.

## Verdict

Jeff's rule ("a man's beard grows from spawn to a maximum length until he shaves it; not customizable") is vanilla DayZ already. `PluginLifespan` grows a male character's beard through four states (`BEARD_NONE`, `BEARD_MEDIUM`, `BEARD_LARGE`, `BEARD_EXTRA`) as a function of play time since the last shave, reaching the full beard at `LIFESPAN_MAX = 240` minutes; `ActionShave` with a razor resets it by stamping `SetLastShavedSeconds`; the state is net-synced so everyone sees it. Nothing about it is player-customizable. So the row as written is done, and what Jeff is actually asking for is a *longer* beard than vanilla's maximum and probably a *slower* clock than four hours.

Those are two different costs. Slower growth and more intermediate stages are cheap: the levels are texture and material swaps on the face (`SetFaceTexture`, `SetFaceMaterial`) read from the `CfgVehicles SurvivorMale_Base Lifespan Beard` config arrays, and the clock is a script constant; a modded `PluginLifespan` and a config override add stages and stretch the clock to days with no model work. A beard that is physically longer than vanilla's is geometry: the vanilla beard is a hidden selection on the head model (`simpleHiddenSelections` contains `beard`, `4_World/DayZ/Entities/ManBase/BodyParts/Head.c:20-22`) with fixed length, and 1.30 offers no runtime mesh morphing. Longer hair geometry therefore needs either a replacement head model per male head (the expensive item the research row warned about) or a worn item on the mask proxy, the way vanilla's `SantasBeard : Mask_Base` puts a beard on the face. The mask-proxy route is feasible and cheap to prototype but costs the mask slot, so a long-bearded player cannot wear a gas mask or a shemagh over the mouth, which on Badlands is a real penalty.

Under the leave-the-board condition: the head mesh cannot be morphed at runtime, so Beards as model work leaves the board. The study recommends keeping a small "Beards" row that is really a Tweaker item: a slower clock and two extra texture stages, and a Phase 0 experiment on the mask-proxy long beard only if Jeff accepts the mask-slot trade.

## What this is not

- Not a character customizer. No beard styles, no colours, no choice at spawn; vanilla picks the face texture per head and the mod only adds stages.
- Not a hair system. Hair length is a separate hidden-selection group (`Group_HairBeard`, `4_World/DayZ/Entities/ManBase/BodyParts/Head.c:95-100`) and stays vanilla.
- Not a new head model. Ruled out by cost; see Options.

## Current state (vanilla 1.30)

| Item | State | Source |
| --- | --- | --- |
| Beard states | `LifeSpanState`: `BEARD_NONE`, `BEARD_MEDIUM`, `BEARD_LARGE`, `BEARD_EXTRA` | `4_World/DayZ/Plugins/PluginBase/PluginLifespan.c:17-24` |
| Clock | Play time minus `GetLastShavedSeconds()`, in minutes; full beard at `LIFESPAN_MAX = 240` minutes; thresholds spread evenly over the levels found in config | `PluginLifespan.c:28-29,61,149,220-232` |
| Visual | Per level, `SetFaceTexture`/`SetFaceMaterial` on the player from the config `Lifespan Beard` `mat` and texture arrays; the head object texture is cleared first | `PluginLifespan.c:56-160,291-330` |
| Sync | `m_LifeSpanState` is a net-synced int; `SynchLifespanVisual` applies it on clients | `4_World/DayZ/Entities/ManBase/PlayerBase.c:53,559`; `PluginLifespan.c:199-201,365` |
| Shave | `ActionShave` (self) and `ActionShaveTarget` (another player); the player stamps `SetLastShavedSeconds(StatGet(STAT_PLAYTIME))` | `4_World/DayZ/Classes/UserActionsComponent/Actions/Continuous/ActionShave.c`; `PlayerBase.c:7031,7172-7180` |
| Beard geometry | `beard` is a `simpleHiddenSelections` entry on the head (`Head_Default`), toggled by masks and helmets; fixed mesh length | `4_World/DayZ/Entities/ManBase/BodyParts/Head.c:1-58,95-100` |
| Worn beard precedent | `SantasBeard : Mask_Base` renders a beard mesh on the mask proxy | `4_World/DayZ/Entities/ItemBase/Clothing/SantasBeard.c:1` |
| Customization | None; a head's beard textures are fixed per head class | Config |

## Options

| # | Option | Gets Jeff | Cost | Verdict |
| --- | --- | --- | --- | --- |
| B1 | Slower clock, more stages | A beard that takes days of play rather than four hours, with six states instead of four | Modded `PluginLifespan` (`LIFESPAN_MAX` from `$profile:` config, default 2,880 minutes = 48 hours played) plus a config override adding two `mat`/texture pairs between the vanilla ones; textures are Claude-made variants of the vanilla ones under the D2 model rule. No geometry, no persistence change (`m_LastShavedSeconds` already persists) | Yes; fits Tweaker as one row |
| B2 | Long beard on the mask proxy | Visibly long beard past `BEARD_EXTRA` | New `NimbyBeards_Long : Mask_Base`-like item auto-attached by the server when the clock passes the last vanilla stage and removed on shave; blocks the mask slot while worn; model is Claude-made (one mesh for all heads, so it will not match every face). Phase 0 proves the auto-attach does not fight a player equipping a mask (recommend: a real mask replaces the beard and the beard returns when the mask comes off) | Feasible; costs the mask slot; prototype only if Jeff accepts that |
| B3 | Replacement head models | Long beards that fit each face | A new head `.p3d` per male head (there are many), each with the hidden-selection groups vanilla expects; weeks of model work and a permanent maintenance tail on every Bohemia head change | No; leaves the board |

## Risks

| Risk | Mitigation |
| --- | --- |
| Config `Lifespan Beard` arrays are not overridable from a mod | Phase 0 confirms with a two-stage test; fallback is a modded `LoadFromCfg` that appends levels in script |
| Texture-only stages look the same past a point | Cap at six stages; the long look is B2 or nothing |
| B2 beard item appears in inventory and gets dropped or traded | Mark it non-takeable (`CanPutInCargo` false, `CanDetachAttachment` false except by shave), no `types.xml` row |

## Decisions for Jeff

| # | Decision | Recommendation |
| --- | --- | --- |
| BD1 | Board status | **Ruled by Jeff, October 21, 2026: B3, replacement head models.** Beards stays on the board, unordered, as the fleet's one model project; B1 (slower clock, more stages) ships with it so the long beard has a clock to arrive on |
| BD2 | B2 prototype | Withdrawn by the BD1 ruling |
| BD3 | B1 defaults | 48 hours of play time to the full beard, six stages (recommendation stands) |
| BD4 | First head | `m_Adam` (the default male head), as the proof before the other 21; Jeff accepts it in game before any further head is modelled |
