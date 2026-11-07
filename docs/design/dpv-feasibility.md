# DPV feasibility - a Desert Patrol Vehicle buggy for Nasdara

Draft v0.1, November 5, 2026. Owner: Jeff Fabish. Status: feasibility only, ordered by Jeff on November 5, 2026. Reference photo: `docs/design/reference/2026-11-05-dpv-reference.jpg` (two Chenowth DPVs, desert, roll cage, rear-mounted spare, gunner platform), added by Jeff on November 5, 2026. Vanilla facts below are from `server/live/db/events.xml` and `D:\DayZServer\addons\vehicles_wheeled.pbo` as listed on November 5, 2026; Workshop and model facts are from the Steam and marketplace pages read the same day. Every `file:line` still owed for the vanilla car API is marked Phase 0.

Target: NIMBY | HARDCORE | BADLANDS | 1pp, HostHavoc, DayZ 1.30 / Nasdara.

## Verdict

There is no Desert Patrol Vehicle for DayZ on the Steam Workshop (searched November 5, 2026: "DPV", "desert patrol vehicle", "fast attack vehicle", "sandrail", "buggy", with and without "1.30"). The DPV exists as a mod for other games (Project Zomboid `ChenowthDPV`, Arma 3 Pook FAV Pack, Arma 2 EMERY DPV), and those assets cannot come to DayZ: Arma data is licensed for Arma only, and the other authors' terms do not allow it. The DayZ buggies that do exist are civilian dune buggies, none a DPV, and the best-known one is broken on 1.30 (seat entry, see the table). So "find an existing model" splits into two routes:

- **Route A, adopt a Workshop dune buggy.** Cheapest; it is a second client mod next to `@NIMBY`, it is a dune buggy rather than a DPV, and its 1.30 health is the author's problem to fix, not ours. Nothing on the list today is both working on 1.30 and close to the photo.
- **Route B, build a NIMBY DPV addon from a licensed 3D model.** The only route that produces the vehicle in the photo. It is the largest piece of in-house work on the board so far, because a DayZ vehicle is a model (LODs, memory points, proxies, skeleton) plus a physics config plus a script class plus CE rows, and vehicle modding is the DayZ category with the fewest shortcuts. Feasible; weeks, not days.

Recommendation: Route B, as a content addon (shared rules 6.1), gated by a Phase 0 that proves the toolchain on Bohemia's sample car before any money is spent on a model. Route A stays as the fallback and as a Phase 0 comparison drive.

## What this is not

- Not a weapon. Vanilla DayZ has no vehicle-mounted weapons and no turret simulation. The DPV's gun ring, if it is modeled at all, is a prop. A working vehicle weapon is out of scope (decision D-3).
- Not a Bohemia model edit. Nothing here edits a Bohemia `.p3d`. The sample car is a scaffold for the config and model.cfg shape only; the DPV is its own model.
- Not Payload, Bikes, or Tweaker. Payload's cargo rule applies to the DPV like any other vehicle with storage (it can be rigged if its cargo holds the explosive); that is a Payload fact, not a DPV feature.
- Not a loot or balance pass. Spawn counts and placement are starting values for the plan.

## Existing DayZ buggies (Route A candidates)

| Candidate | Workshop | What it is | 1.30 state (November 5, 2026) | Terms | Fit |
| --- | --- | --- | --- | --- | --- |
| MBM_DuneBuggy (mebrad) | [3166659854](https://steamcommunity.com/workshop/filedetails/?id=3166659854) | Civilian dune buggy, AWD, 300 cargo slots, two wooden-case slots, types file and key in the mod folder, 31 MB, 24,000 subscribers | Description says "1.30 compatible"; October comments report no driver or passenger entry after 1.30, rear wheels sinking, shaking since 1.29; author replied "working on updates" October 17 | Free to use on any server; model edits forbidden, changes by override only; repack terms read two ways on the page, confirm before any fork | Closest working-ish buggy; not a DPV; dead on 1.30 until the author ships |
| DBO_Buggy (DeanosBeano) | [1878762615](https://steamcommunity.com/sharedfiles/filedetails/?id=1878762615) | Buggy on the `OffroadHatchback` part set (hatchback wheels, car battery, radiator, belt, plug, H8 headlights), 2019; model by janmatys (Blendswap) | Unknown; no update evidence since the early 1.x cycle | The author's ATV is ADPL-SA with source; the buggy's terms are not stated on the page | Oldest; likely dead; the part-set choice is a good pattern for ours |
| RUSForma_vehicles (BAJA sand rail) | [2536888090](https://steamcommunity.com/workshop/filedetails/?id=2536888090) | A 100-plus-vehicle Soviet pack that includes one custom sand rail called BAJA | Last update June 5, 2026; players report vehicles vanishing on restart (September and October 2026 threads) | Pack terms | Pulls a hundred vehicles onto a near-vanilla server for one buggy; no |
| Techs 4x4 All Terrain Vehicles | in the EJGames collection [3138381423](https://steamcommunity.com/sharedfiles/filedetails/?id=3138381423) | Jeep pack, 1.30 compatible, with buggies | The page says "Buggys are broken at this time" | Pack terms | No |

Every Route A candidate is a second `-mod=` entry on the launch line, a second key in `/keys`, a second Workshop subscription for every player, and a second author's update cadence to track at every Bohemia patch. The October 26 CF/COT removal was made to get away from exactly that dependency class (do not reopen; this row only records the cost).

## Model sources (Route B)

| Source | What | Fit | License question |
| --- | --- | --- | --- |
| **cgtrader "Desert Patrol Vehicle Low-poly" by carbuni** ([page](https://www.cgtrader.com/3d-models/military/military-vehicle/desert-patrol-vehicle), model 1875658) | Chenowth-style DPV: open tube frame, roll cage, exposed coil springs, mesh side panels, light bar, rear cargo deck, spare-tire frame; **no weapon modeled**. 19,688 polygons, 11,898 vertices. Formats MAX (2011), OBJ, MA. Textures diffuse only: body 4096, wheels 2048, alpha 1024 (the mesh panels), with layered PSDs. UVs overlap (mirrored halves). Author has 26 reviews; published January 19, 2019. Read in the browser November 5, 2026 | **Recommended.** Closest match to the reference photo of anything found; polygon budget fits a DayZ vehicle LOD 0 without retopology; separate wheel texture suggests separate wheel meshes (confirm on import). Needs normal and specular maps made from the PSD layers, or a DayZ `rvmat` tuned on the diffuse | Royalty Free License (no AI), $29.50 on sale from $59, one-time purchase. CGTrader's royalty-free terms allow use in games and apps where the model is not extractable in its original form: the binarized `.p3d` and `.paa` textures ship in the Workshop PBO; the MAX, OBJ, MA, and PSD sources MUST NOT be committed to the public repository (shared rules 9.3 exception, recorded in the plan and `NOTICE`). Jeff reads the license text before purchase; not a legal opinion |
| cgtrader "DESERT PATROL VEHICLE MILITARY DPV DUNE BUGGY" (surf3d, $33; also RenderHub $29.90) | 1.7 million polygons, CAD formats, 1.4 GB | CAD model; a full retopology before DayZ. No |
| Sketchfab "Desert Vehicle Patrol" (rio3dstudios, free, CC-BY) | 88,500 triangles; viewed in the browser November 5, 2026: a green low-poly civilian buggy with a cab, not a DPV | No |
| Renderosity "Desert Patrol" and "Desert Patrol Vehicle" (dexsoft-games, $20 and $15) | Viewed November 5, 2026: post-apocalyptic armored cars with mounted guns, not a DPV | No |
| cgtrader "Light Fast Attack Desert Buggy" | Marked AI-generated by the vendor; topology and UVs unknowable before purchase | No |
| TurboSquid "Desert Patrol Vehicle DPV" (OODINDD, $69) | 148,888 polygons | Marked Editorial Uses Only. No |
| Sketchfab, free CC-BY or CC0 | Searched November 5, 2026 ("DPV", "desert patrol vehicle", "Chenowth", "FAV", "buggy"): no DPV; the buggies found are civilian or sci-fi | A CC model would have let the source be published under 9.3; none exists today | Re-check once before purchase (one browser pass) |
| Bohemia DayZ Samples (GitHub, ADPL-SA) and `DayZ-Misc` | The sample car and the vehicle configuration page ([Vehicle Configuration](https://community.bistudio.com/wiki/DayZ:Vehicle_Configuration)) | Not a DPV; the scaffold that proves the toolchain and gives the `model.cfg` and `SimulationModule` shape | ADPL-SA: attribution in `NOTICE`, no commercial use, DayZ only, share-alike for anything derived. Our config and model.cfg are written fresh against it, not copied |
| Build the model in Blender from the photo | Jeff's or a commissioned artist's model | Full control of the license (MIT with the rest of the source) | Time; a commissioned low-poly game vehicle is a few hundred dollars and one to three weeks |

## What a DayZ vehicle needs

Read from the shape of `OffroadHatchback` (`vehicles_wheeled.pbo`, `vehicles_parts.pbo`, `vehicles_data.pbo`, `sounds_vehicles.pbo` on the replica) and the Bohemia vehicle configuration page; every row gets its `file:line` in Phase 0.

| Layer | Content | Owner of the work |
| --- | --- | --- |
| Model (`.p3d`, Object Builder) | Visual LODs (3 to 4), Geometry, Fire Geometry, View Geometry, Hit-points, Memory, Land Contact; named selections for each wheel, damper, steering wheel, pedals, lights; memory points for `pos_driver`, `pos_codriver`, `pos_cargo` or rear seat, get-in points, `cargo`, fuel, exhaust, lights; proxies for four wheels and the spare, battery, spark plug, radiator, headlights | Model phase |
| `model.cfg` | Skeleton (bones for wheels, dampers, steering, pedals, doors if any) and animations driven by the vehicle simulation (`wheel`, `damper`, `drivingWheel`) | Model phase |
| `config.cpp` | `CfgVehicles` class on `CarScript` (via the vanilla car base, forward-declared, `requiredAddons[] = {"DZ_Vehicles_Wheeled"}`), `SimulationModule` (engine torque curve, gearbox, differential, suspension, wheel radius and friction), `Crew` (Driver, CoDriver, rear seat), `DamageSystem` zones, attachment slots (wheels, `CarBattery`, `CarRadiator`, `EngineBelt`, `SparkPlug`, `HeadlightH8`), `Cargo`, sounds (reuse the vanilla hatchback sound set at first), `fuelCapacity`, textures and `hiddenSelections` | Config phase |
| Script (`4_World`) | `class NimbyDpv extends CarScript` with the fluid, light, and seat hooks the hatchback implements; `GetSeatAnimationType` for an open-cage seat; 1PP camera memory points | Script phase |
| CE rows (`server/mission-overrides`) | `types.xml` rows for the vehicle, wheel, and any new part; `cfgspawnabletypes.xml` pre-attached parts; `events.xml` a `VehicleNimbyDpv` event; `cfgeventspawns.xml` positions near Nasdara military sites | CE phase; applied live by Jeff (6.1) |
| Pack | A `pack.json` row (orchestrator), self-test cases, `NOTICE` entries for the model and the Bohemia sample, preview image unchanged | Orchestrator |

## Phases and estimates

| Phase | Work | Exit evidence | Estimate |
| --- | --- | --- | --- |
| 0. Toolchain proof and recon | Bohemia ships no sample vehicle model (checked November 5, 2026), so the proof is `NimbyDpv_TestCar`, a config-only child of the vanilla ADA in the `NimbyDpv` stub, packed and booted by `Build-NimbyPack.ps1`, then entered and driven on instance 998 with the flag-gated test spawn. Vanilla configs are read as text from `D:\DayZProjects\DZ` (copied to `vendor/dayz-1.30-vehicles/`). Record: `docs/design/2026-11-05-dpv-phase0-recon.md` | DPV-0-1 to DPV-0-7 pass; the API and model-name tables have their lines; purchase gate passed at DPV-0-3 | 2 sessions |
| 1. Model | Acquire or commission the model (D-2); Blender: triangles under about 60,000 in LOD 0, LODs, selections, memory points, proxies; Object Builder import, `model.cfg`, binarize; `rvmat` and `.paa` textures; collision tuned so the cage does not catch on Nasdara fences (the MBM complaint) | The DPV loads as a static object in the test server with correct LOD switching and no texture warnings | 1 to 3 weeks, mostly model work, depends on the source quality |
| 2. Config and script | `config.cpp` on the hatchback part set, `SimulationModule` tuned for a light rear-engine buggy (fast, loose on sand, 200 hp class, two-wheel drive like the real DPV or AWD for fun, D-4), crew and seats, damage zones, cargo, sounds, `NimbyDpv` script class, `Inputs.xml` not needed | Drive, enter and exit every seat in 1PP, headlights, horn, refuel, repair, wheel swap, roll-over and recovery, damage to ruin, all on instance 998 against a copy of live `storage_1` | 1 to 2 weeks |
| 3. CE rows and placement | Types, spawnable types, the event, and spawn positions on Nasdara; nominal low (D-5) | Vehicles spawn at the chosen sites on a fresh instance 999 world with parts as configured | 2 sessions |
| 4. Qualification | Self-test cases, boot gate, test record `docs/testing/` with `NIMBY-T-nnn` findings, the two 6.1 boots (save-reload round trip with the addon, then the same storage without it: only the DPV missing, 0 `Scripted variables corrupted`), Pester for any new tooling | `SUMMARY PASS`, release-gate table filled | 1 week |
| 5. Release | Shared rules 9.2 gate, `NOTICE` and `LICENSE` review, `@NIMBY` Workshop update (first update that carries a model and textures; expect the item to grow from 711 KB to several megabytes), live CE rows at a restart with backups | Live boot with the DPV loaded, 0 `SCRIPT (E)`; first player drive logged | 1 restart window |

## Save footprint

The DPV, its wheel class, and any new part persist in `storage_1` the vanilla way, so this is a content addon under shared rules 6.1: no override of `OnStoreSave` or `OnStoreLoad` on any vanilla class, no persisted player state, the two extra release-gate boots. Removing the addon drops every DPV from storage (the engine discards entities of a class it cannot resolve), which is the behavior Tack's removal boot already showed on October 26, 2026.

## Risks

| Risk | Mitigation |
| --- | --- |
| 1.30 broke vehicle entry for third-party vehicles (MBM's October reports) and the cause is in the animation graph or seat reach, not the config | Phase 0 test DPV-0-2 (enter the NIMBY test car on the vanilla model) answers the config and script half before any model money; the model half (memory points) is Phase 1's first milestone |
| Model license forbids publishing the source (shared rules 9.3 says the source of every NIMBY mod is public) | Prefer CC-BY or CC0 (Sketchfab) or a commissioned model owned outright; if a marketplace model is bought, the plan records a 9.3 exception: the `.p3d` and textures ship in the PBO, the source stays in `D:\Keys`-class private storage, and `NOTICE` names the vendor and license |
| Vehicle physics tuning eats the schedule (`SimulationModule` has no shortcut) | Start from the hatchback's numbers, change one group at a time, keep a tuning log in the plan |
| The open cage gives 1PP a camera that clips the roll bars | Camera memory points placed after the first 1PP drive, not before |
| Workshop item size grows and the launcher download takes longer | One texture set, DXT-compressed `.paa`, no 4096 body texture in the shipped PBO (2048 is enough at DayZ draw distances) |
| A rear gunner seat with no gun reads as a broken feature | Ship two seats and a cargo bed unless D-3 says the ring is a prop |
| Nasdara fences and posts catch the cage (MBM complaint) | Geometry LOD is a simple hull, not the visual mesh |
| Route B is weeks of work on a board that has Central Scrutinizer Phase 2 in flight | D-6 orders it; Phase 0 is cheap and can run now |

## Decisions for Jeff (when the plan is written)

| # | Decision | Recommendation |
| --- | --- | --- |
| D-1 | Route | **Ruled November 5, 2026: Route B.** The purchase of the carbuni model closed it; Phase 0 passed the same evening |
| D-2 | Model source | **Done November 5, 2026:** carbuni "Desert Patrol Vehicle Low-poly" bought at $29.50, Royalty Free License; sources at `D:\DayZProjects\NimbyDpv\vendor-carbuni\`, never in a repository (9.3 exception) |
| D-3 | The gun ring | **Settled by the model:** no gun ring is modelled; the raised rear position is the third seat (`Cargo1`) |
| D-4 | Drivetrain and part set | **Ruled by Jeff, November 5, 2026: rear-wheel drive** from vanilla `RearDrive` (`vendor/dayz-1.30-vehicles/wheeled.config.cpp:437`); hatchback part set (`CarBattery`, `CarRadiator`, `EngineBelt`, `SparkPlug`, `HeadlightH8`). **November 6, 2026:** radiator dropped (air-cooled, no `CarRadiator` slot, build 20). **RWD stands** (Jeff, 20:35): the AWD ADA control climbed 187 m of 18 to 25 degrees that the RWD DPV could not, so AWD is logged as an option and the lever order for hill restarts is tyre friction, then a scripted traction control, then mass and rear bias in the builder |
| D-5 | Spawn | **Ruled by Jeff, November 5, 2026:** nominal 4, max 6, military sites only; each spawn has a chance of being complete or of missing battery, spark plug, fuel, or other parts (a per-part chance in `cfgspawnabletypes.xml`, values set in the CE phase). Retuned live later |
| D-6 | Display name and prefix | **Ruled by Jeff, November 5, 2026: display name DPV**; code prefix `NimbyDpv`, log prefix `[NIMBY][Dpv]`; RPC block assigned in the shared rules registry when the plan lands |
| D-7 | Order on the board | **Ruled by Jeff, November 5, 2026: DPV first, now.** Phase 1 starts November 5; Central Scrutinizer Phase 2 waits |
| D-13 | Toolchain | Jeff has Blender 5.2.2 LTS (November 5, 2026). The mesh script targets the 5.x API; P3D export through a Blender P3D add-on if one supports 5.x, else DayZ Tools Object Builder import of the Blender export |

## Phase 2 tune (November 6 and 7, 2026)

Builds 25 to 34, four sessions, test record `docs/testing/2026-11-06-dpv-wheels-and-first-drive.md`. Everything below is in `mods/own/nimby-pack/NimbyDpv/config.cpp` with the build history in comments, except the traction control, which is script (`Scripts/4_World/nimbydpv_car.c`).

| Item | Value | Why |
| --- | --- | --- |
| Rear `Differential.ratio` | 5.2 (was 3.9), `DIFFERENTIAL_LOCKED` | The Olga's 3.9 on a 0.40 m tyre was about half the ADA's effective final drive; second gear starved at 2100 to 2300 rpm on 15 to 20 degrees (build 26). 5.2 puts first at about 8.5 kN and second at about 5.1 kN at the wheels; redline speeds 39/65/98/137 km/h. Build 28 held 55 km/h on 17.5 degrees. The locked axle is what climbs; it is also the source of the pull on uneven grip (NIMBY-T-097), because every vanilla car except the M3S runs `DIFFERENTIAL_OPEN`. Opening it is logged, not planned |
| Engine | Flat-four `torqueCurve[] = { 400,0, 1000,160, 1500,205, 2200,220, 3200,215, 4200,180, 5000,100, 5400,0 }`, `rpmRedline` 5500; inertia 0.45, friction 130, idle 900, min 1000, clutch 1250 | **Configured rev limiter (Jeff, 20:35, keep for release):** torque reaches zero 100 rpm under the redline, so the vanilla redline damage tick (`CarScript.c:876-883`, 20-hitpoint spark plug) is unreachable under load or free-revving. Plug 20 and engine 1000 on every run since build 29 except NIMBY-T-098 |
| `NimbyDpv_Wheel` tyre | `tyreGrip` 1.0, `tyreLongitudinalFriction` 3.0, `tyreLateralFriction` 3.6 (build 32, release candidate) | Passenger-car 0.85/0.90/0.92 could not restart on 22 degrees. Vanilla off-road 0.95/1.5/1.8 (build 28) climbed on the move, spun from rest. 3.0/2.5 (build 30) restarted on 23.5. 4.5/3.0 (build 31) still spun on 25.5 and inverted the vanilla ratio (lateral above longitudinal on every intact vanilla wheel), which made the tail step out on every shift and the car feel like ice on dirt. 3.0/3.6 restarts from rest on 22 and climbs through 25.9, holds forward motion on 28.7, 133.9 km/h top, hands-off heading held to the decimal on tarmac. Tyre friction above vanilla's 1.5 is not clamped |
| Mass | 1200 kg (builder `TOTAL_MASS_KG`) | Real DPV 950 kg curb; the rebuild is logged for a later revisit (Jeff, 20:36). The Geometry mass split (tub 55 per cent at 0.46 to 1.00 m, cage 10 per cent at 1.5 to 2.1 m) puts the centre of mass about 0.85 m up on a 1.88 m track, a static rollover threshold of about 1.1 g; lateral grip above about 1.8 can exceed it. Lower the cage share into the tub in the same rebuild |
| Suspension, steering | ADA values; `Steering` `maxSteeringAngle` 35, `increaseSpeed` `{ 0, 50, 30, 30, 100, 10 }` | Unchanged through Phase 2 |
| Traction control (build 34, script) | `NimbyDpv_Car.EOnPostSimulate`, server side: each step compares each present rear wheel's surface speed (`WheelGetAngularVelocity` rad/s times 0.40 m) with ground speed (`GetSpeedometer()/3.6`); a wheel ahead by more than max(3 m/s, 25 per cent) or a present rear wheel with no contact, while the throttle is above 0.3 in a forward gear, caps the throttle at 0.3 with `SetThrottle` | The locked axle's two failure modes share one cause, open throttle on a tyre that is not carrying the car: from rest on a steep slope the tyre breaks loose and the engine pins at the limiter (T-096), and airborne the rear wheels spin up and land at tyre speed, which was the heading swings and the rollover of build 33 (T-099). Proven on build 34: `SetThrottle` from `EOnPostSimulate` holds against the driver (throttle reads 0.3 while W is held); `Transport.OnInput`, which the vanilla comment recommends for setters, never runs on the server with a remote driver. Reverse is not covered (NIMBY-T-100, one-line extension if wanted). Jeff, 22:38: happy with the handling |

**Remaining plug-wear path (NIMBY-T-098):** wheel-driven overrev from a two-gear downshift at speed (second at 101 km/h reached 5410 rpm and took the plug from 20 to 15.5). The limiter cannot reach it and vanilla cars wear the same way; player technique.

**Traction control (build 34, November 7, 2026):** in, proven, and the table row above records it. T-099 is fixed; T-096 is open only for a re-measured ceiling on a 26 to 28 degree slope (build 32 measured 25.9 from rest; build 34's steepest restart was 16.4, clean). Rollovers at 85 to 105 km/h off-road remain (three on build 34, each recovered) and belong to the centre-of-mass rebuild under Mass, which Jeff has ruled out of scope for now.
