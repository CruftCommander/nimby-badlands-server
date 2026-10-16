# Plan 5 - Sprout (`NimbyGrow`)

Draft v0.3, October 16, 2026. Changes since v0.2: tobacco and cannabis are independent crops (S7, Jeff). Owner: Jeff Fabish. Implementer: a Claude Code session in worktree `feat/sprout`, coordinated by the orchestrator. Status: draft for Jeff's review; implementation waits on Puff Phase 1 (the tobacco leaf item) being on `main`.

Target: NIMBY | HARDCORE | BADLANDS | 1pp, HostHavoc, DayZ 1.30.164078 / Nasdara.
Release: independent, gated (see [shared rules, 9.2](2026-10-15-shared-rules.md)), plus the content-addon gate in [shared rules, 6.1](2026-10-15-shared-rules.md).
Feasibility: `docs/design/puff-sprout-scrawl-earplugs-feasibility.md`.

Sprout grows two crops, tobacco and cannabis, and neither depends on the other (S7). It adds tobacco to vanilla gardening and makes weed a grown crop rather than loot. Weed growing is vanilla 1.30 in full: seeds, water, fertilizer, six growth stages, a harvest of seven, and a bigger harvest from fertilizer. Sprout does not touch that logic; it changes what spawns and adds a second crop beside it.

## What this is not

- Not a gardening rewrite. `GardenBase`, `Slot`, and `PlantBase` are used as they are.
- Not a loot mod. The `types.xml` rows it needs are listed here and applied by Jeff at a restart; the addon ships no CE files.
- Not the hash recipe. Pressing weed into hash is Phase 4 and depends on Puff's hash item.
- Not independent of Puff. `NimbySmoke_TobaccoLeaf` is a Puff item; `requiredAddons` names `NimbySmoke`.

## Current state (verified October 15, 2026)

| Item | State | Source |
| --- | --- | --- |
| Source | None. New addon folder `mods/own/nimby-pack/NimbyGrow/` | - |
| Vanilla weed | `CannabisSeeds : SeedBase`, `PlantType = "Plant_Cannabis"` | `DZ/gear/cultivation/config.cpp:231-245` |
| Vanilla plant | `Plant_Cannabis : PlantBase`, model `Cannabis_plant.p3d`, 6 growth stages, 7 crops of `Cannabis` | `DZ/gear/cultivation/config.cpp:910-926`, `cultivation.c:15` |
| Textures per plant | `CfgHorticulture Plants` entries with healthy and infested texture and material | `DZ/gear/cultivation/config.cpp:17-64` |
| Harvest size | `Ceil(CropsCount * fertility * harvesting_efficiency)` | `PlantBase.c:124-125` |
| Fertilizer | `Slot` fertility fields and setters | `Slot.c:30-38,191-211` |
| Seed packs | `CannabisSeedsPack : SeedPackBase` | `SeedPackBase/CannabisSeedsPack.c` |
| Seed recipes | `CutOutSeeds` family (pepper, tomato, zucchini, pumpkin) | `Classes/Recipes/Recipes/CutOut*Seeds.c` |
| Dried stage | `Cannabis` textures include a dried material | `DZ/gear/food/config.cpp:5765-5770` |
| Cooking gate | `Cooking.ProcessItemToCook` cooks only when `CanBeCooked()` is true; vanilla `Cannabis` returns false | `Cooking.c:44-47`, `Cannabis.c:3-6` |
| Live loot rows | Read October 16, 2026: no `Cannabis`, `CannabisSeeds`, or `CannabisSeedsPack` row exists in the Nasdara `types.xml`, so nothing cannabis spawns today | `server/live/db/types.xml` |

## Decisions

| # | Decision | Choice | Status |
| --- | --- | --- | --- |
| S1 | Display name | Sprout. Code prefix `NimbyGrow`, log prefix `[NIMBY][Grow]`, RPC block `65505xx` reserved and unused | Decided October 15, 2026 |
| S2 | Weed as a grown crop | Live `types.xml`: add `CannabisSeeds` and `CannabisSeedsPack` rows with a nominal; add no `Cannabis` row, so the crop stays grow-only (a type with no row never spawns from CE) | Decided (requirement); rows are additions, not edits |
| S3 | Tobacco plant | `Plant_Tobacco : PlantBase`, `GrowthStagesCount = 6`, `CropsCount = 6`, `CropsType = "NimbySmoke_TobaccoLeaf"` (the leaf class is defined in Puff Phase 1; Sprout only grows and dries it); `CfgHorticulture Plants Plant_Tobacco` reusing the pepper textures until the model lands | Claude default |
| S4 | Tobacco seeds | `NimbyGrow_TobaccoSeeds : SeedBase` on `cannabis_seeds.p3d` with an own texture; `NimbyGrow_TobaccoSeedsPack : SeedPackBase`; a cut-out-seeds recipe from a tobacco leaf | Claude default |
| S5 | Drying | A fresh leaf is `NimbySmoke_TobaccoLeaf` at the raw stage; rolling and the honey recipe accept only the dried stage; drying path confirmed in Phase 0 | Claude default |
| S7 | Independent crops | Tobacco and cannabis are separate features that ship and switch independently: the tobacco plant, seeds, and drying (S3-S5) have no cannabis dependency; the cannabis `types.xml` rows (S2) and hash cooking (S6) have no tobacco dependency. `$profile:NimbyGrow\config.json` carries `enableTobacco` and `enableCannabis` (both default true), and the Phase 2 loot rows for each crop are applied separately. The only shared dependency is Puff (the tobacco leaf and hash items) | Decided by Jeff, October 16, 2026 |
| S6 | Hash by cooking | A configured count of dried `Cannabis` in a cooking pot or pan over a fireplace becomes one `NimbySmoke_Hash` after the configured cooking time, and burns to nothing if left; no new action or tool. Implemented as a modded `Cooking` class that handles dried `Cannabis` before the `CanBeCooked()` gate (`Cooking.c:44-47`); script only, no persistence, so shared rules 6.1 allows it | Decided by Jeff, October 15, 2026 (D15) |

## Phase 0 - prove a modded crop grows

Depends on Puff Phase 1 on `main`.

1. Record builds and reference hashes in `docs/baselines/`.
2. From the live `types.xml` in `server/live/`, record the `Cannabis`, `CannabisSeeds`, `CannabisSeedsPack`, `GardenLime`, and `PlantMaterial` rows.
3. Build a throwaway `NimbyGrow_TobaccoSeeds` and `Plant_Tobacco` on the pepper model and textures, plant it in a garden plot on the local test server, water it, and confirm it reaches each of the six stages and harvests `NimbySmoke_TobaccoLeaf`, with and without `GardenLime`; record the two harvest counts.
4. Confirm the plant survives a server restart mid-growth (vanilla `PlantBase` persistence, `PlantBase.c:236-343`).
5. Record which vanilla path moves a fruit-type edible to the `DRIED` stage (cooking method, time, and container) and whether `Cannabis` reaches it.
6. With a throwaway modded `Cooking` class, put dried `Cannabis` in a pot over a fire and confirm the hook runs each cooking tick and can replace the item; record the tick interval and the equipment classes it sees (`Cooking.c:130,196`).

Exit: a modded seed grows, harvests, and persists; the drying path is known or S5 is revised; the cooking hook (item 6) intercepts dried `Cannabis` before the `CanBeCooked()` gate or S6 falls back to a recipe without fire and Jeff is asked.

## Phase 1 - tobacco

- Addon skeleton with `requiredAddons[] = {"NimbySmoke"}`; items per S3 and S4; string table; `NOTICE`.
- Drying per S5; the leaf's stages drive which recipes accept it.
- Self-test: config class names resolve (`ConfigIsExisting`) for every Sprout and referenced Puff class at mission start.
- `pack.json` row added by the orchestrator.

## Phase 2 - seeds and loot

- Cut-out-seeds recipe from a dried leaf (`CutOutSeeds` pattern).
- `types.xml` rows drafted in `server/`: tobacco seeds and seed packs spawn where vanilla seed packs do; weed per S2.
- Release through the content-addon gate.

## Phase 3 - model

- Tobacco plant model with six growth-stage selections from the Claude pipeline; acceptance screenshot from Jeff; purchase fallback per D2.

## Phase 4 - hash by cooking (S6)

- The modded `Cooking` hook per S6; counts, time, and burn window in `$profile:NimbyGrow\config.json`.
- Test SP-006: dried `Cannabis` in a pot over a fire becomes hash at the configured time and burns away past the window; raw `Cannabis` is untouched.

## Tests

| Id | Case | Evidence |
| --- | --- | --- |
| SP-001 | Tobacco seeds planted, watered, and unfertilized yield the base harvest; fertilized yield more | Harvest counts in the test record |
| SP-002 | A plant mid-growth survives restart and continues | RPT and screenshot |
| SP-003 | A fresh leaf cannot be rolled; a dried leaf can | Action menu screenshots |
| SP-004 | `Cannabis` no longer spawns loose after the `types.xml` change; seeds do | CE log sample after a restart |
| SP-005 | Save-reload round trip, then a boot without Sprout, drops only Sprout items; Puff items remain | Release gate RPT |
| SP-006 | Dried `Cannabis` in a pot over a fire becomes hash at the configured time and burns away past the window; raw `Cannabis` is untouched | Test record with timestamps |

## Risks

| Risk | Mitigation |
| --- | --- |
| `CfgHorticulture` does not accept a modded plant entry | Phase 0 item 3 is a stop condition |
| Drying has no vanilla path for a fruit | S5 falls back to a time-based stage change on the leaf itself |
| The modded `Cooking` class cannot run before the `CanBeCooked()` gate | Phase 0 item 6; fallback is a no-fire recipe, which reverses D15 and goes back to Jeff |
| Live loot change starves weed entirely | Seeds get a visible nominal; Jay reports after one restart cycle |
