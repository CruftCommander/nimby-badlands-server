# Shared rules amendment draft: NimbyDpv registry line (November 8, 2026)

Draft for the orchestrator-owned `2026-10-15-shared-rules.md` (v1.11). Written by Claude at Jeff's ask (release prerequisite f, November 8, 2026); Jeff pastes or edits it in. Nothing below changes a rule; it records the DPV's identifiers and the engine facts the DPV sessions found, each with the test record that holds the evidence.

## 4.1 row

| Mod | Display name | Code and PBO prefix | Log prefix | RPC id block |
| --- | --- | --- | --- | --- |
| DPV | DPV | `NimbyDpv` | `[NIMBY][Dpv]`; diagnostics `[NIMBY][Dpv][Diag]`, written only while `$profile:NimbyDpv\spawn-testcar` exists (release builds are silent) | `65512xx` (reserved; the DPV uses the vanilla `CarScript` netcode and the action framework, no RPC today) |

## 6 (save footprint) note

NimbyDpv is a content addon. It persists only vanilla `CarScript` state on its own classes (`NimbyDpv_Car`, `NimbyDpv_Wheel`, `NimbyDpv_Wheel_Ruined`); it overrides neither `OnStoreSave` nor `OnStoreLoad`. Removing the addon removes the cars and their wheels from `storage_1` and nothing else (release gate 9.2 control boot, pre-publish save-and-reload PASS November 7, 2026, build 58). The vanilla item classes it patches (below) gain slot names only; nothing saved changes.

## 7.2 DPV engine facts (November 5 to 8, 2026)

Evidence: `docs/testing/2026-11-05-dpv-phase1.md`, `docs/testing/2026-11-06-dpv-wheels-and-first-drive.md` (builds 13 to 64), `docs/design/dpv-feasibility.md`, `docs/design/2026-11-05-dpv-model-inventory.md`.

Vehicle config and model:
- NIMBY-T-069: a car class with no `healthLevels[]` on its zones has engine health 0 at `CarScript.c:848-854` and stops within a second of starting; every zone and `GlobalHealth` need the five-level table.
- NIMBY-T-070: `CarWheelType::SetReplacementTypes` throws for a wheel class without a `<class>_Ruined` config entry.
- NIMBY-T-071, T-073: crew proxy triangles follow the Arma2Quad convention (origin at the right angle, long leg up, short leg along minus forward) and need a skeleton bone of the same name, or the player sits at the model origin.
- NIMBY-T-072: `binarize.exe` merges every `model.cfg` under the source folder; a stale copy in a subfolder wins silently.
- NIMBY-T-074, T-075: the steering animation source is `steeringwheel`; `wheelHub="wheel_x_y_damper_land"` names a Geometry LOD component the engine refuses to simulate without.
- NIMBY-T-078, T-079 (tooling): `scripts/dpv/build_nimbydpv_model.py` writes MLOD P3Ds for the local build; a release ships binarized ODOL v56 (ruled November 5, 2026, confirmed against the CGTrader license November 8). `scripts/dpv/read_mlod.py` and `odol_hdr.py` read the two formats.
- NIMBY-T-082: the get-in prompt needs the player within 1.0 m, flat, of the seat's condition point. T-084: `VehicleAnimInstances` decides which seat changes exist.
- NIMBY-T-086, T-094, T-098: the vanilla redline tick damages the spark plug and engine on spinning or airborne wheels and on an over-revved downshift; the DPV's server-side traction control (`nimbydpv_car.c` `EOnPostSimulate`) is the answer, not a config value.
- NIMBY-T-089 (procedural): cars saved before a model or slot change come back broken; wipe the test world (`storage_998`) before a boot that changed the model or the slots.
- NIMBY-T-102, T-103: steering-wheel axis points must be symmetric about the wheel centre; reflector proxies sit inside the housing.

Attachments and proxies (builds 37 to 64):
- One frame per slot: `InventorySlotsOffsets` `position[]` and `orientation[]` move the server entity, never the drawn attachment (build 51); an attachment is drawn in its slot proxy's frame only. Per-class fit is admission, a slot set of its own, or an animated proxy bone (unproven).
- Vanilla item patch rule (Jeff, November 6, 2026, 23:08): an addon may patch a vanilla item class's `inventorySlot[]` to add its own slot names, as a full list and never `+=` (`+=` appends to the parent's list, not the class's own). NimbyDpv patches the jerry can, ammo box, protector case, scientific briefcase, first aid kit, Field Transceiver, wooden crate, teddy bear, three tents, the Attack2Bag and eighteen backpack classes (`config.cpp`); per-class action changes live in `nimbydpv_railitems.c`. Config only, nothing saved changes.
- `CreateInInventory` on a car has no cargo fallback: a class with no free matching slot returns null (test spawner, build 57).
- An rvmat `emmisive` is multiplied by the face's own texel; a near-black lens needs its own bright emissive material (`dpv_lights_on.rvmat`, build 64), and the selection must cover the lens faces only.
- `TentBase.CanProxyObstructSelf` (`TentBase.c:78`) is true for a packed tent too, so `MiscGameplayFunctions.IsObjectObstructedEx` (`:1630-1640`) hides a packed tent's own prompts behind its camo-net and light proxies; NimbyDpv returns false while packed (build 62).
- Hatchback_02 sound set is used by copying the `Sounds` block values from `DZ/sounds/hpp/config.cpp:519454`; the sound shaders stay in the game data (Jeff, November 7, 2026, 23:58).
