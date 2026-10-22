# Bikes feasibility - two riders, bike storage, and who owns the motorbikes

Draft v0.1, October 21, 2026. Owner: Jeff Fabish. Status: feasibility only, ordered by Jeff on October 21, 2026. Covers the two-riders idea (October 16) and the bike-storage idea (October 17) from `docs/research/candidate-features.md`, and answers whether one addon should own them together with Payload's bike work. Every `file:line` is relative to `vendor/dayz-1.30-scripts/scripts/` (build 1.30.164078.38); config and model facts were read from the PBO headers of `D:\DayZServer\addons\vehicles_singletrack.pbo` on October 21, 2026.

Target: NIMBY | HARDCORE | BADLANDS | 1pp, HostHavoc, DayZ 1.30 / Nasdara.

## Verdict

Two riders is closer than the idea assumed: vanilla 1.30 already defines a `CoDriver` crew position on the `Motorbike` config base class, and both bike models (`Motorbike_01.p3d`, `Motorbike_02.p3d`) carry the `crewcodriver`, `pos_codriver`, and `pos_codriver_dir` memory points a second seat needs. If the seat is present but unreachable in game, the fix is script (seat reach, seat animation, get-in action), not a model; if it is reachable already, the idea is done and the row closes. That is a half-day Phase 0 on the replica, not a build.

Bike storage is feasible as attachment items with their own cargo (the backpack pattern), reachable through the bike's inventory; the open question is the visual, because neither bike model has a bag or basket proxy, so an attached bag is invisible unless the mod links a visible child object to the bike by script. Feasible, Phase 0 check.

Ownership: one addon, display name to be chosen by Jeff (working name Bikes, code prefix `NimbyBikes`), owns the two rider and storage ideas and Tweaker's rider-cactus case; Payload stays its own mod but its bike support depends on a fact this study settled: **the 1.30 motorbikes are not `CarScript`**. They are `MotorbikeScript : Motorbike : Transport`, with the horn on a `VehicleHornComponent` rather than `CarScript.SetCarHornState`, so Payload's horn trigger on bikes is a different hook from cars and its rename hook (`NameOverride`) is on `Object` and works for both.

## What this is not

- Not a model mod. Nothing here edits or replaces a Bohemia `.p3d`. If an item needs a proxy the vanilla model lacks, the answer is a script-linked child object or an invisible attachment, never a model swap.
- Not Payload. The bomb stays in its own feasibility study and plan; this study only records the bike facts Payload needs.
- Not a balance pass. Capacities and placement are starting values in the addon's config.

## The two bikes

| Fact | Value | Source |
| --- | --- | --- |
| Classes | `Motorbike_01_ColorBase` (`_Blue`, `_Yellow`, `_Red`) and `Motorbike_02_ColorBase` (`_Red`, `_Yellow`, `_Blue`, `_Green`), both `: MotorbikeScript` | `4_World/DayZ/Entities/Vehicles/InheritedMotorbikes/Motorbike_01.c:1`, `Motorbike_02.c:1` |
| Which is Jana 50 and which Bitrak 682 | Not stated in script. `Motorbike_02` has a windshield and left and right leg-shield proxies (`Motorbike_02_windshield.p3d`, `_shieldleft`, `_shieldright`), which is a moped's shape, so `Motorbike_02` is expected to be the Jana 50 and `Motorbike_01` the Bitrak 682. Confirmed in Phase 0 by reading the display name in game | `vehicles_singletrack.pbo` model list; `Motorbike_02.c` registers the shield attachments |
| Base chain | `MotorbikeScript : Motorbike : Transport`; **not `CarScript`** | `4_World/DayZ/Entities/Vehicles/MotorbikeScript.c:53`; `3_Game/DayZ/Vehicles/Motorbike.c:30` |
| Horn | `VehicleHornComponent m_HornComponent` with `SetMode`, `RegisterSound`, `HandleSound`; actions `ActionVehicleHornShort` and `ActionVehicleHornLong` | `MotorbikeScript.c:75,127-128,147,212-215,369-370`; `4_World/DayZ/Entities/Vehicles/Components/VehicleHornComponent.c:10,41,55` |
| Crew | Config `class Crew { class Driver; class CoDriver; }` on the `Motorbike` base: Driver uses `seat_driver`, `crewDriver`, `pos_driver`, `pos_driver_dir`; CoDriver uses `crewCoDriver`, `pos_codriver`, `pos_codriver_dir` | `vehicles_singletrack.pbo` `config.bin` string order; both `.p3d` headers list the memory points |
| Seat script | `GetSeatAnimationType(posIdx)` returns `VEHICLESEAT_DRIVER` for every seat; `CrewCanEject`, `CrewCanGetThrough`, `CanReachSeatFromSeat` return true; `CanReachSeatFromDoors` tests distance to the seat memory point; `CanGetIn` refuses a fallen bike | `MotorbikeScript.c:449-505,225-231` |
| Seat animation enum | `VEHICLESEAT_DRIVER`, `VEHICLESEAT_CODRIVER`, `VEHICLESEAT_PASSENGER_L`, `_R` | `3_Game/DayZ/dayzplayer.c:674-677` |
| Attachment slots handled in script | `Reflector_1_1`, `CarBattery`, `TruckBattery`, `SparkPlug`, `GlowPlug`, wheels; `Motorbike_02` adds `Motorbike_02_ShieldFront`, `_ShieldLeft`, `_ShieldRight` | `MotorbikeScript.c:523-600`; `Motorbike_02.c` |
| Cargo | No `cargo` selection or `crewCargo` point in either model (cars and boats have them); the bikes have no vanilla cargo | `vehicles_singletrack.pbo` selection strings vs `vehicles_wheeled.pbo` and `vehicles_water.pbo` |
| Spawn | Event `VehicleMotorbike`, nominal 30, children `Motorbike_01_*` | `server/live/db/events.xml:1144-1160` |

## Feasibility by idea

| # | Idea | Verdict | How | Source |
| --- | --- | --- | --- | --- |
| K1 | Two riders | Likely yes, script only | Phase 0 first: get on a bike on the replica and have the second client try the pillion. Three outcomes. (a) The CoDriver seat already works: close the row. (b) The seat exists but the get-in action never offers it: the gate is in the action target or in `CanReachSeatFromDoors` distance to `pos_codriver`, both scriptable (`MotorbikeScript.c:479-499`); the fix is a modded reach check or a wider `pDistance`. (c) The seat is reachable but the rider's pose is wrong: `GetSeatAnimationType` returns `VEHICLESEAT_DRIVER` for the pillion; returning `VEHICLESEAT_CODRIVER` for `posIdx 1` is one override, and whether 1.30's animation graph has a motorbike co-driver pose is the real unknown (Bohemia's 1.30 notes say the graph changed). Fallback for (c): keep the driver pose on the pillion point; it reads as two riders at a glance. Rider ejection on a fall already applies to every seat (`CrewShouldEject` returns `IsFallen()`) | `MotorbikeScript.c:449-505`; config `Crew` block; `dayzplayer.c:674-677` |
| K2 | Bitrak saddlebags | Yes, with an invisible-or-linked visual | New item `NimbyBikes_Saddlebag : ItemBase` with its own `Cargo` (the backpack pattern), a new attachment slot `NimbyBikes_Saddlebag` added to `Motorbike_01_ColorBase` by a modded `config.cpp` (`attachments[] += {...}`); one per side needs two slot names. Its cargo appears under the bike in the vanilla inventory the way an attached container's cargo does on a player; Phase 0 confirms the vehicle inventory view renders attachment cargo for a `Transport`. Visual: the model has no bag proxy, so the bag is invisible unless the addon links a child object (`EntityAI.AddChild` on the bike at a model-space offset near `crewcodriver`) on attach and removes it on detach, mirroring how vanilla shows the Motorbike_02 shields through proxies. Phase 0 checks that a linked child follows a moving bike without jitter on a second client | `MotorbikeScript.c:523-600` (attach and detach hooks); `Motorbike_02.c` (shield attachments as the vanilla precedent) |
| K3 | Jana built-in slot and basket | Yes, same shape | A small built-in cargo is a `Cargo` block on a modded `Motorbike_02_ColorBase` config (say 2x3); the basket is an attachment like K2 with a larger cargo; a basket spawning on the bike is a `cfgspawnabletypes.xml` row. Same visual caveat as K2 | Same |
| K4 | Rider brushes a cactus | Yes, in Tweaker | The Tweaker cactus sweep runs on the bike's position when the player's `GetCommand_Vehicle()` is set, both seats; recorded here so the two studies agree | `docs/design/tweaker-feasibility.md` T3 |
| K5 | Payload on bikes | Horn mode is a different hook; remote mode is the same | Payload's car horn trigger hooks `CarScript.SetCarHornState` (`CarScript.c:2481`), which bikes never call; the bike hook is `VehicleHornComponent.SetMode` (`VehicleHornComponent.c:41`) or the two horn actions. The rename (`Object.NameOverride`, `Object.c:1344`) and the synced armed flag work on any entity. Cargo eligibility (the October 16 rule: rig anything whose cargo holds the charge) means a stock bike cannot be rigged at all, because it has no cargo; with K2 or K3 attached it can. That is a design consequence Jeff should see: bike storage is what makes bikes riggable | `4_World/DayZ/Entities/Vehicles/CarScript.c:2481`; `VehicleHornComponent.c:41`; `3_Game/DayZ/Entities/Object.c:1344`; `docs/design/rigged-ada-feasibility.md` banner |

## Ownership

| Option | Consequence |
| --- | --- |
| One addon, Bikes, owns K1-K3; Tweaker owns K4; Payload stays separate and reads K5 (recommended) | Three small addons with clean lines: Bikes changes seats and storage, Tweaker changes world behavior, Payload is a weapon. A Bohemia bike change breaks Bikes and Payload's bike branch only |
| Fold K1-K3 into Tweaker | Tweaker grows a client-facing item and a config override on vehicle classes; its "server-side behavior tweaks" line blurs |
| Fold K1-K3 into Payload | Payload gains storage items it does not need; a player who never rigs a bike still downloads the bomb mod's bike parts |

## Save footprint

K1: zero. K2 and K3: the saddlebag and basket are new persistent item classes (they hold cargo), so Bikes is a content addon under shared rules 6.1 and takes the two extra boots at its release gate. The built-in Jana cargo (K3) is a config change on a vanilla class and persists through vanilla's own cargo storage.

## Risks

| Risk | Mitigation |
| --- | --- |
| No motorbike co-driver pose in the 1.30 animation graph | Driver pose on the pillion point (K1 fallback); recorded as a known visual gap |
| Attachment cargo not shown on a `Transport` in the inventory UI | Fallback: the bike gets a direct `Cargo` block sized by the attached bag, with the bag as a key rather than a container |
| Linked child object jitters or desyncs | Invisible bag with a HUD line "saddlebag fitted" as the fallback; the cargo still works |
| Payload horn mode silently never fires on bikes | K5 is handed to the Payload re-verification so the plan hooks `VehicleHornComponent` from the start |

## Decisions for Jeff (when the plan is written)

| # | Decision | Recommendation |
| --- | --- | --- |
| K-1 | Ownership | One Bikes addon (table above) |
| K-2 | Display name | Jeff names it; working name Bikes, code prefix `NimbyBikes`, log prefix `[NIMBY][Bikes]` |
| K-3 | Order on the board | After Tweaker; K1's Phase 0 can run earlier because it is a replica test with no code |
| K-4 | Saddlebag visual | Linked child object if Phase 0 shows it stable, else invisible |
| K-5 | Capacities | Saddlebag 4x4 each side (buttpack size), Jana built-in 2x3, basket 4x3 |
