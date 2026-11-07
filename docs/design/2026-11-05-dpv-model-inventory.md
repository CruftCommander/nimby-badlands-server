# DPV model inventory - carbuni "Desert Patrol Vehicle Low-poly"

November 5, 2026. Owner: Jeff Fabish. Phase 1 input (feasibility `docs/design/dpv-feasibility.md`, recon `docs/design/2026-11-05-dpv-phase0-recon.md`). Purchased by Jeff on November 5, 2026 from CGTrader (model 1875658, Royalty Free License, no AI, $29.50). Source files live at `D:\DayZProjects\NimbyDpv\vendor-carbuni\` and MUST NOT enter any repository (shared rules 9.3 exception, D-2). Read with Python from the zip's OBJ and the RAR's PSDs; nothing has been edited.

## Files

| File | Size | Notes |
| --- | --- | --- |
| `Desert_Patrol_Vehicle.obj` + `.mtl` | 1.9 MB | The exchange format we use. Three materials: `lambert6SG` (body, `Desert_Patrol_Vehicle_texture.psd`), `lambert8SG` (wheels, `_wheels.psd`), `lambert3SG` (alpha parts, `_alpha.psd`) |
| `Desert_Patrol_Vehicle.max` | 2.7 MB | 3ds Max 2011 native; not needed |
| `Desert_Patrol_Vehicle.flt`, `DPV_texture1.mb` | 3.7 MB, 1 MB | OpenFlight and a Maya scene; not needed |
| `Desert_Patrol_Vehicle_textures.rar` | 158 MB | RAR5. Holds the three PSDs (311 MB unpacked). Needs WinRAR or 7-Zip 23 or newer |

## Mesh

| Fact | Value | Consequence for DayZ |
| --- | --- | --- |
| Vertices / faces | 11,898 / 11,035 (8,641 quads, 2,388 tris, 6 n-gons); about 19,700 triangles | Fits LOD 0 as is; LOD 1 to 3 by decimation. Triangulate the 6 n-gons before export |
| Units and axes | Centimetres, Z up, Y along the length | Scale 0.01 and Y-up on export; DayZ models are metres, Y up, model faces -Z or +Z per the vanilla convention (checked against the ADA in Blender) |
| Bounding box | 215 cm wide, 412 cm long, 216 cm tall; lowest vertex at Z = -0.3 cm (wheels sit on Z = 0) | Real Chenowth DPV is 211 x 408 x 201 cm: the model is at 1:1 |
| Groups | `WHEELS` (1,208 faces), `BODY` (9,751), `ALPHA` (76) | Three materials become three `rvmat`s and three texture sets |
| Front | Light bar at Y = -193 to -200 (width 100 cm), so the front is -Y; the mesh cargo deck at Y = +172 is the rear | Decides the rotation on export |
| Wheels | **Four separate loose parts**, plus hub caps and brake discs as their own parts. Front pair (Y = -166.2): radius 38.8 cm, width 22.9 cm, centre height 38.9 cm. Rear pair (Y = +122.0): radius 39.8 cm, width 29.0 cm, centre height 39.4 cm. Track 189 to 191 cm, wheelbase 288 cm (real DPV 282 cm) | The wheels become the `NimbyDpv_Wheel` attachment model (front and rear differ slightly; one wheel class with the rear mesh, or two classes, decision D-8). Wheel centres give `wheel_1_1` to `wheel_2_2` memory points and the `Axles` geometry; radius 0.39 to 0.40 m is the `SimulationModule` wheel radius |
| Spare wheel | **None modelled** | DayZ's spare is an attachment proxy anyway; the proxy goes on the rear frame, or the slot is dropped (D-9) |
| Seats | Three seat parts of 232 faces: two at X = ±32, Y = +10, Z = 96 (driver and co-driver) and one at X = 0, Y = +105, Z = 152 (the raised rear seat on the gun platform) | `pos_driver`, `pos_codriver`, `pos_cargo1` memory points; the rear seat is `Cargo1` in the `Crew` block |
| Steering wheel | 144-face part at X = +30, Y = -35, Z = 103 | On the +X side; whether that is the left or the right of the vehicle depends on the handedness after export (check in Blender against the ADA, which has its wheel on the left). It becomes the `drivewheel` selection with `axis_drivewheel` |
| Weapon | **None.** The gun ring is not modelled; the rear platform is an empty raised seat | Nothing to remove; matches feasibility D-3 |
| Body parts | 324 loose parts: chassis tub (241 faces), the two side cages (130 each), the roll cage, the light bar, springs, bumpers, mirrors, fuel cans, bolts | Easy to select the suspension springs for `damper_*` animation and the lights for `light_*` hidden selections |
| ALPHA parts | 10 flat parts: the rear mesh deck (Y = +172), the four spring sleeves at the front, the roof mesh (Z = 153), two side mesh panels, two side skirts | Alpha-tested material (`_alpha.psd`, 1024, `spring` and `fence` layers) |
| UVs | 16,462 UV coordinates; the listing says overlapping (mirrored halves) | Harmless; no per-side decals without a re-unwrap |

## Textures (PSD, 8-bit RGB with alpha)

| PSD | Size | Layers | What we derive |
| --- | --- | --- | --- |
| `Desert_Patrol_Vehicle_texture.psd` | 4096 x 4096, 31 layers | `BASE` (black, red, white: three body colours), `ELEMENTS` (light, back_light, metal, metal2, fabric, seatbelts), `BUMP,HOLES` (lines_black, bump1), `BOLTS`, `TEXT` (a `CHENOWTH` type layer, gauges), `DIRT` (dirt1, dirt2, sand, sand2, leaks), `AO`, `Reflection`, `Reflection2`, `Shadow`, `Highlights`, `UV` (hidden) | `_co.paa` at 2048 from the flattened stack (black base by default; red and white are free colour variants, D-10); `_nohq.paa` from `BUMP,HOLES` and `BOLTS` height-to-normal; `_smdi.paa` specular from `metal`, `metal2`, `Reflection`, `Highlights` with `fabric` and `seatbelts` dull; the `CHENOWTH` text layer is dropped (trademark, D-11) |
| `Desert_Patrol_Vehicle_wheels.psd` | 2048 x 2048, 13 layers | `BASE` black, `ELEMENTS` (rubber, rubber_tread, black_paint, bolts, bump1), `DIRT` sand, `AO`, `Highligths`, `UV` (hidden) | `_co` at 1024, `_nohq` from `rubber_tread` and `bump1`, `_smdi` rubber dull and `black_paint` semi-gloss |
| `Desert_Patrol_Vehicle_alpha.psd` | 1024 x 1024, 6 layers | `BASE` black, `ELEMENTS` (spring, fence), `Highlights` | `_ca.paa` with alpha from the layer masks (springs and mesh panels) |

## What the model does not have

Doors, windows, a dashboard with dials, a hood or trunk, a spare wheel, a weapon, an interior engine mesh, separate pedals, or a horn. No radiator either: the DPV is air-cooled (Jeff, November 6, 2026, build 20), so the config has no `CarRadiator` slot, no coolant damage path, and no `dmgzone_radiator`. The DPV config drops the matching ADA slots and damage zones (recon, "What the DPV model must provide"); the engine is a `dmgzone_engine` hull at the rear with no visible part.

## Decisions for Jeff (added to the plan)

| # | Decision | Recommendation |
| --- | --- | --- |
| D-8 | Wheel classes | Claude's default (not asked): one `NimbyDpv_Wheel` class using the rear mesh (radius 0.40 m); the 1 cm front difference is invisible in game and halves the loot rows |
| D-9 | Spare wheel slot | **Ruled by Jeff, November 5, 2026: keep it.** `wheel_spare_1` proxy on the rear frame |
| D-10 | Body colour | **Ruled by Jeff, November 5, 2026: black only at launch.** Red and white stay available as later `_Red` and `_White` subclasses |
| D-11 | `CHENOWTH` text layer | Claude's default (not asked): dropped; the mark is not ours to print and the display name is DPV |
| D-12 | Driver side | Claude's default (not asked): left, as the vanilla cars; if the export lands the steering wheel on the right, mirror the steering wheel and the driver seat only |

## Open items

| # | Item | Owner |
| --- | --- | --- |
| M-1 | Extract the RAR on Windows into `D:\DayZProjects\NimbyDpv\vendor-carbuni\` (the two large PSDs did not copy from the session; the OBJ, MTL, MAX, FLT, MB and the alpha PSD did) | Jeff |
| M-2 | Blender import script: scale, axes, group split, selection names, memory points, LODs | **Done November 5, 2026:** `scripts/dpv/build_nimbydpv_model.py` (Blender 4.0 to 5.x headless, or plain Python without the decimated LODs) writes `dpv.p3d` (9 LODs) and `dpvwheel.p3d` (3 LODs) as MLOD directly, no add-on; `scripts/dpv/read_mlod.py` reads them back. Built in the session against Blender 4.0.2 and round-trip checked; first copies at `D:\DayZProjects\NimbyDpv\data\`. Next: open in DayZ Tools Object Builder (M-4) |
| M-4 | Object Builder check of `dpv.p3d` | **Done November 5, 2026, 19:14:** loads in Object Builder 2.3.0.164046, nine LODs listed, selections and proxies present at the hubs, seats and light bar, nose forward. Winding measured from the file (216 of 264 wheel faces clockwise from outside): correct, no flip. `P:` is mapped |
| M-7 | `ImageToPAA` on the nine TGAs in `D:\DayZProjects\NimbyDpv\data\` (command in the texture script output), then Buldozer on `dpv.p3d` for the first textured look | Jeff |
| M-5 | `pack.json` `includePatterns` needs `*.p3d` and `*.rvmat` before the pack can carry a model; the built `data\` folder stays out of git (derived from the licensed model) and is copied in at build time | Orchestrator; ruling for Jeff |
| M-3 | Texture derivation script | **Done November 5, 2026:** `scripts/dpv/build_nimbydpv_textures.py` (psd-tools, Pillow, numpy) writes `dpv_body_{co,nohq,smdi}` 2048, `dpv_wheel_{co,nohq,smdi}` 1024, `dpv_alpha_{ca,nohq,smdi}` 1024 as TGA plus the three Super-shader rvmats (layout from the ADA `engine_niva.rvmat`); black base, `TEXT` group dropped (D-11); body height from the PSD bump and bolt layers plus a high-pass of the diffuse; alpha from the `fence` and `spring` layer coverage. Output at `D:\DayZProjects\NimbyDpv\data\`. Jeff runs `ImageToPAA` on the nine TGAs (M-7) |
