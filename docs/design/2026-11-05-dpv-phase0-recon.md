# DPV Phase 0 recon - toolchain proof and vanilla car reference

Draft v0.1, November 5, 2026. Owner: Jeff Fabish. Feasibility: `docs/design/dpv-feasibility.md`. Script `file:line` is relative to `vendor/dayz-1.30-scripts/scripts/` (build 1.30.164078.38). Config `file:line` is relative to `vendor/dayz-1.30-vehicles/` (copied November 5, 2026 from `D:\DayZProjects\DZ`, the DayZ Tools "Extract Game Data" output of 1.30, derapified by the tools; see `SOURCE.txt` there). Model names are from `vendor/dayz-1.30-vehicles/offroadhatchback.p3d.names.txt`, the identifier strings inside the binarized `offroadhatchback.p3d`.

## What changed since the feasibility study

Bohemia's `DayZ-Samples` repository (ADPL-SA, read November 5, 2026) ships **no sample vehicle model**. Its `Test_SmartCar` is a 29-line config plus a `modded class CarScript` that drives a vanilla car by script; there is no `.p3d` and no `model.cfg`. So the "boot the sample car" step in the feasibility study is replaced by a NIMBY test class on the vanilla ADA model, which proves the same things and one more: that our own pack can carry a vehicle class on 1.30.

The DayZ Tools extraction at `D:\DayZProjects\DZ` already holds every vanilla vehicle config as text (`vehicles\wheeled\config.cpp`, 24,369 lines; `vehicles\parts\config.cpp`), so no derap tool is needed.

## Phase 0 stub (in `mods/own/nimby-pack/NimbyDpv/`)

| File | What it does |
| --- | --- |
| `config.cpp` | `CfgPatches NimbyDpv` requires `DZ_Data` and `DZ_Vehicles_Wheeled` (`wheeled.config.cpp:3`); `CfgVehicles NimbyDpv_TestCar : OffroadHatchback` (`:1250`), `scope = 2`, display name "DPV test car (Phase 0, ADA model)". Everything else inherits: model, crew, `SimulationModule`, `DamageSystem`, attachments, sounds, cargo |
| `Scripts/4_World/nimbydpv_testcar.c` | `class NimbyDpv_TestCar extends OffroadHatchback` (`4_World/DayZ/Entities/Vehicles/InheritedCars/OffroadHatchback.c:1`), empty |
| `Scripts/3_Game/nimbydpv_selftest.c` | Four config cases through `CGame.ConfigIsExisting` (`3_Game/DayZ/Global/Game.c:612`), `ConfigGetInt` (`:539`), `ConfigGetFloat` (`:524`), `ConfigGetText` (`:449`): class exists, scope 2, inherited `fuelCapacity` 42 (`wheeled.config.cpp:1346`), inherited `Crew Driver actionSel` "seat_driver" (`:269`) |
| `Scripts/5_Mission/nimbydpv_missionbase.c` | The self-test hook, same shape as Ears (`MissionBase.OnInit`, flag `$profile:NimbyDpv\selftest`), log prefix `[NIMBY][Dpv]` |
| `Scripts/5_Mission/nimbydpv_missionserver.c` | Test spawn, server only, flag `$profile:NimbyDpv\spawn-testcar`: on `MissionServer.InvokeOnConnect` (`5_Mission/DayZ/mission/missionServer.c:435`, called on both the new-character path `:336` and the ready path `:357`; `OnClientReadyEvent` is skipped for a new character, found November 5, 18:14) the first connecting player gets one `NimbyDpv_TestCar` six metres ahead via `CreateObjectEx` (`Game.c:703`) with `ECE_PLACE_ON_SURFACE` (`3_Game/DayZ/CE/CentralEconomy.c:38`), then `OnDebugSpawn` (`3_Game/DayZ/Entities/EntityAI.c:3955`; the ADA override at `OffroadHatchback.c:465` spawns wheels, doors, battery, plug, radiator and fills fluids). One car per boot. This closes NIMBY-T-003 (no vehicle spawn on the local server, open since October 14) for this addon's own tests |
| `NOTICE` | Origin, the planned model and its license, change log |
| `pack.json` row | `NimbyDpv` / `NimbyDpv` / `NimbyDpv` / logPrefix `Dpv` (orchestrator file; added with the stub, as the rules foresee for a stub) |

Identifiers for the shared rules registry (section 4.1, to be added in the plan's pull request): display name DPV (working; Jeff names it), code prefix `NimbyDpv`, log prefix `[NIMBY][Dpv]`, RPC block: none needed in Phase 0; reserve the next free block when the plan lands.

## Phase 0 procedure (Jeff runs, Claude reads the logs)

From `D:\GitHub\nimby-badlands-server` in PowerShell 7:

1. Static gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Test-NimbyRepo.ps1`. Expect PASS on `cfgmods` (NOTICE present, every `CfgMods files[]` folder exists).
2. Build, sign, verify, boot: `pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build-NimbyPack.ps1`. Expect in `logs\build-<stamp>\build.log`: `SELFTEST Dpv ran=yes cases=4 pass=4 fail=0` and `SUMMARY PASS`. The script log carries `[NIMBY][Dpv][SelfTest] PASS class-exists`, `PASS scope-is-2`, `PASS inherits-ada-fuel-42`, `PASS inherits-ada-driver-seat`. A `Warning Message: Addon 'NimbyDpv' requires addon 'DZ_Vehicles_Wheeled'` line is a FAIL pattern and means the requiredAddons name is wrong.
3. Flag the test spawn in a named profile (the test server uses `pack.json` `testServer.instanceId` 998; add `-ServerPath D:\DayZServer` in step 4 to drive on a copy of live storage instead of the Steam install's fresh world):
   `New-Item -ItemType Directory -Force .\profiles\dpv-phase0\NimbyDpv | Out-Null; New-Item -ItemType File -Force .\profiles\dpv-phase0\NimbyDpv\spawn-testcar | Out-Null`
4. Start the test server on the newest PASS build with that profile: `pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Start-NimbyTestServer.ps1 -ProfileName dpv-phase0`
5. Join from a client that loads the same `build\@NIMBY-<stamp>` folder. The car appears six metres ahead at spawn; the script log has `[NIMBY][Dpv] test-car spawned class=NimbyDpv_TestCar at=<x y z> player=<id>/<steam64>`.
6. In game, one numbered test per row (standing rule). Record in `docs/testing/2026-11-xx-dpv-phase0.md` with `NIMBY-T-nnn` findings:

| # | Test | Pass condition |
| --- | --- | --- |
| DPV-0-1 | Open the car's inventory | Name reads "DPV test car (Phase 0, ADA model)"; four wheels, battery, plug, radiator present |
| DPV-0-2 | Enter the driver seat in 1PP | The get-in action is offered and the player sits (this is the MBM failure mode on 1.30) |
| DPV-0-3 | Start, drive 200 m, stop, exit | Engine starts; the car moves; `[NIMBY][Dpv]` lines show no error; 0 `SCRIPT (E)` |
| DPV-0-4 | Co-driver and both rear seats | Each seat can be entered and left |
| DPV-0-5 | Headlights and horn | Both work (horn matters to Payload) |
| DPV-0-6 | Persistence round trip | `#shutdown` through the script, restart on the same `storage_998`, the car is where it was left with its parts; 0 `Scripted variables corrupted` |
| DPV-0-7 | Removal boot (shared rules 6.1) | Boot the same storage with `-Vanilla`: only the test car is missing; 0 `Scripted variables corrupted`, 0 `SCRIPT (E)` |

Delete `profiles\dpv-phase0\NimbyDpv\spawn-testcar` afterwards; release-gate boots never carry it.

## Purchase gate

Buy the carbuni model (feasibility D-2) when DPV-0-2 and DPV-0-3 pass: that is the moment it is known that a NIMBY vehicle class can be entered and driven on 1.30 through our pipeline. DPV-0-6 and DPV-0-7 can follow the purchase; they test persistence, which the model does not change.

## What the DPV model must provide (from the ADA)

The named selections and memory points the vanilla config and `CarScript` read, taken from `offroadhatchback.p3d.names.txt` and `wheeled.config.cpp`. The DPV model gets these names in Blender before export; the DPV config then needs no renaming.

| Group | Names | Read by |
| --- | --- | --- |
| Crew | `pos_driver`, `pos_driver_dir`, `pos_codriver`, `pos_codriver_dir`, `pos_cargo1`, `pos_cargo1_dir`, `pos_cargo2`, `pos_cargo2_dir` (get-in points); `crewdriver`, `crewcodriver`, `crewcargo1`, `crewcargo2` (proxy positions); `seat_driver`, `seat_codriver`, `seat_cargo1`, `seat_cargo2` (action selections); `seatdriver`, `seatcodriver`, `seatback_*` (seat animation) | `Crew` block, `wheeled.config.cpp:265-282` (base) and `:1348-1370` (ADA) |
| Wheels | `wheel_1_1`, `wheel_1_2`, `wheel_2_1`, `wheel_2_2`, each with `_axis`, `_damper_land`, `_removing`; `wheel_spare_1`; `damper_1_1` to `damper_2_2`; `turnfrontleft`, `turnfrontright`; `steering_hub_1_1`, `steering_hub_2_1`; `susp_*` and `suspension_*` bones | `Axles` block `:1431-1499`, `animTurn` and `animDamper` `:294-326` |
| Driver controls | `drivewheel`, `axis_drivewheel`, `steeringwheel`, `throttle`, `brake`, `clutch`, `handbrake`, `gearshift`, `gearshift_1`; dials `dial_speed`, `dial_rpm`, `dial_fuel`, `dial_temp`, `dial_oil`, `dial_brakes`, `dial_voltage`, `clock_hour`, `clock_minute`; `axis_speed`, `axis_rpm`, `axis_fuel`, `axis_temp` | `AnimationSources` `:351-430`, `:1507-1539`; the DPV has no dashboard, so the dials can be omitted |
| Parts | `carbattery`, `sparkplug`, `carradiator`, `battery`, `radiator`, `engine`, `engine_rotate`, `belt_hide`, `reflector_1_1`, `reflector_2_1` (headlights), `reflector_1_2`, `reflector_2_2` | `attachments[]` `:1255-1270`; `DamageZones` `:1577-2282` |
| Lights | `light_left`, `light_right`, `light_left_dir`, `light_right_dir`, `light_brake_1_2`, `light_brake_2_2`, `light_reverse_1_2`, `light_reverse_2_2`, `light_1_2`, `light_2_2`, `light_dashboard`, `lights_left`, `lights_right`, `switch_lights` | `hiddenSelections[]` `:1271-1290`; `OffroadHatchbackLightProfileFront` and `Rear` (`OffroadHatchback.c:24-25`) |
| Fluids and effects | `fuel`, `refill`, `coolant`, `drown_engine`, `ptcexhaust_start`, `ptcexhaust_end`, `ptcenginepos`, `ptcdustpos`, `ptccoolantpos`, `pos_carhorn` | `CarScript` fluid actions and particle points; `SetEnginePos` (`OffroadHatchback.c:29`) |
| Damage | `dmgzone_chassis`, `dmgzone_front`, `dmgzone_back`, `dmgzone_engine`, `dmgzone_fueltank`, `dmgzone_fender_1_1` to `_2_2`, `dmgzone_bumper_1`, `_2`, `dmgzone_lights_1_1`, `_2_1`; `damage_front`, `damage_back`, `damage_left`, `damage_right`, `damage_top`, `damage_bottom`, `damage_bumperfront`, `damage_bumperback` | `DamageZones` `:1577-2282`; the DPV drops the roof, door and window zones |
| Cargo and access | `carcargoaccess`, `cargo`-style `Cargo` block `:1501-1506` (`itemsCargoSize` 10x30 on the ADA) | `ActionOpenCarCargo` family |
| Misc | `camo_chassi`, `lodnoshadow`, `vehicle`, `bumper_0_1`, `bumper_1_1`, `bumper_2_1` | hidden selections and LOD properties |

Doors (`doors_*`, `nivadriverdoors`, `nivacodriverdoors`, `nivahood`, `nivatrunk`) and windows (`glass1`, `glass2`, `dmgzone_window*`) do not apply: the DPV is open. Its config drops those attachment slots, animation sources and damage zones, and `GetDoorSelectionNameFromSeatPos` (`OffroadHatchback.c:266`) returns an empty string so seat reach never waits for a door.

## Vanilla physics base for the DPV config

`RearDrive : CarScript` (`wheeled.config.cpp:437-532`) is the rear-wheel-drive base the sedan family uses and the right parent for a rear-engine DPV (feasibility D-4); `AllDrive` (`:629`) is the ADA's. The DPV plan starts its `SimulationModule` from `RearDrive` and the ADA's suspension numbers (`:1448-1454`, `:1476-1482`: stiffness 40000, compression 2100/2200, damping 7500/7600, travel 0.088/0.083 front and 0.159/0.106 rear) with the travel roughly doubled for the long-travel buggy suspension, then tunes by drive.

## Open items

| # | Item | Owner |
| --- | --- | --- |
| P0-1 | Run the procedure above; record DPV-0-1 to DPV-0-7 | **Done November 5, 2026, 18:24 to 18:35: all seven PASS** (`docs/testing/2026-11-05-dpv-phase0.md`); purchase gate open |
| P0-2 | Branch: the stub sits untracked on the `fix/hands-t063-tack-t065` checkout; per shared rules 2.1 it belongs on `feat/dpv` in `D:\GitHub\nimby-badlands-server-wt\feat-dpv`. Create the worktree and move the files before the first commit | Jeff or a Claude Code session on Windows |
| P0-3 | Confirm Blender MLOD `.p3d` export (the community P3D add-on) loads in DayZ 1.30 unbinarized, or binarize with DayZ Tools; decide before Phase 1 | Claude prepares, Jeff runs |
| P0-4 | Shared rules registry rows for DPV (4.1) in the plan's pull request | Claude drafts |
