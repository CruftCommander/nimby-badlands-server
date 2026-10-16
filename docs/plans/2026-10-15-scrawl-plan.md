# Plan 6 - Scrawl (`NimbyNotes`)

Draft v0.2, October 15, 2026. Owner: Jeff Fabish. Implementer: a Claude Code session in worktree `feat/scrawl`, coordinated by the orchestrator. Status: draft for Jeff's review; implementation waits on the shared rules v1.1 amendment being on `main`.

Target: NIMBY | HARDCORE | BADLANDS | 1pp, HostHavoc, DayZ 1.30.164078 / Nasdara.
Release: independent, gated (see [shared rules, 9.2](2026-10-15-shared-rules.md)), plus the content-addon gate in [shared rules, 6.1](2026-10-15-shared-rules.md).
Feasibility: `docs/design/puff-sprout-scrawl-earplugs-feasibility.md`.

Changes since v0.1: the stack is vanilla `Paper` with a quantity of 5 (N2, Jeff's call), not an own item; the lifetime is 30 days scalable to 14 or 7 (N3); base protection comes from the vanilla flag pole.

Scrawl is small because vanilla 1.30 already does the hard part. A pen used on a sheet of `Paper` opens a write menu, the text is saved with the item and survives restart, and anyone can read it. Scrawl makes `Paper` stack to 5, keeps written notes from merging, and sets the despawn policy through the live `types.xml`.

## What this is not

- Not a note system. Writing, reading, sanitizing, pen depletion, and persistence are vanilla: `Paper.c:1-29`, `WrittenNoteData.c`, `ActionWritePaper.c:57-59`, `NoteMenu.c`.
- Not a new persistence field. Quantity is a vanilla `ItemBase` variable written under the variables mask (`ItemBase.c:3011-3014`); Scrawl adds no `OnStoreSave`.
- Not a despawn mechanism. Vanilla CE `lifetime` in the live `types.xml` is the despawn; Scrawl ships no CE files.
- Not a base-cleanup mod. Jeff's idea of trimming oversized bases is a separate candidate in `docs/research/candidate-features.md`.

## Current state (verified October 15, 2026)

| Item | State | Source |
| --- | --- | --- |
| Source | None. New addon folder `mods/own/nimby-pack/NimbyNotes/` | - |
| Vanilla paper | `Paper : Inventory_Base`, 1x1, no quantity, flammable | `DZ/gear/consumables/config.cpp:19-60`, `Paper.c` |
| Note data | `WrittenNoteData` written in `OnStoreSave`, read when `version >= 108` | `Paper.c:12-29` |
| Quantity persistence | Written only when `m_VariablesMask` carries `VARIABLE_QUANTITY` | `ItemBase.c:3011-3014` (load), `:3089` (`OnStoreLoad`), `:3190` (`OnStoreSave`) |
| Write action | Pen must have `writingColor`; pen in hands on paper or paper in hands on pen | `ActionWritePaper.c:57-59` |
| Pens | `Pen_Black`, `Pen_Red`, `Pen_Green`, `Pen_Blue`; quantity depletes one per character | `Pen_ColorBase.c`, `WrittenNoteData.c:83-87` |
| Read action | `ActionReadPaper`, RPC `ERPCs.RPC_READ_NOTE` | `ActionReadPaper.c`, `scripts/3_Game/DayZ/Enums/ERPCs.c:70` |
| Base protection | `TerritoryFlag` refresher | `Totem.c:1,568` |
| Live loot rows | Read October 16, 2026: `Paper` nominal 0, `lifetime` 7200, spawning only as 50 percent cargo in `Bear_Beige`, `Bear_Dark`, and `Bear_Pink`; every `Pen_*` nominal 0 with no cargo entry. Nobody can write a note on Nasdara today | `server/live/db/types.xml`, `server/live/cfgspawnabletypes.xml:478-490` |

## Decisions

| # | Decision | Choice | Status |
| --- | --- | --- | --- |
| N1 | Display name | Scrawl. Code prefix `NimbyNotes`, log prefix `[NIMBY][Notes]`, RPC block `65506xx` reserved and unused | Decided October 15, 2026 |
| N2 | Paper stack | Vanilla `Paper` gets `varQuantityInit = 1`, `varQuantityMin = 0`, `varQuantityMax = 5`, `canBeSplit = 1` through a config override in Scrawl's `config.cpp`. A modded `Paper` class refuses `CanBeCombined` and `CanBeSplit` when either sheet carries note text, and `ActionWritePaper` is allowed only on a quantity of 1 (split first). Sheets already in `storage_1` have no quantity bit and load as 1 | Decided by Jeff, October 15, 2026 (D11) |
| N3 | Despawn | Live `types.xml`: `Paper` `lifetime` 2592000 (30 days). If it degrades the server, scale to 1209600 (14) or 604800 (7) at a restart. CE refreshes lifetime on interaction, so an untouched note despawns and a handled one does not. A raised `TerritoryFlag` refreshes every item in its radius, so notes in a flagged base live as long as the base; notes in a tent, barrel, or body follow the container | Decided by Jeff, October 15, 2026 (D12) |
| N4 | Loot | Live `types.xml`: `Paper` nominal 30 (residential tiers 1-2, the teddy-bear cargo stays) and 30 pens in total across the four colors (`Pen_Black` 8, `Pen_Blue` 8, `Pen_Red` 7, `Pen_Green` 7); a stack spawns as quantity 1-5 through the `Paper` row. These are the first live loot additions of the theme | Decided by Jeff, October 16, 2026 (totals); the per-color split is Claude's |

## Phase 0 - prove vanilla notes and the quantity round trip

1. Record builds and reference hashes in `docs/baselines/`.
2. Confirm the `Paper` and `Pen_*` rows in `server/live/db/types.xml` against the current-state table (both at nominal 0) and draft the N3 and N4 rows in `server/`.
3. On the local test server: write a note with each pen color, read it, drop it, restart the server, and read it again. Record the maximum accepted length from `NoteMenu`.
4. Boot a `storage_1` copy that contains vanilla sheets with the N2 config override loaded; confirm every existing sheet loads at quantity 1 with its text, with 0 "Scripted variables corrupted". Save, reload, and confirm again.
5. Confirm a note in a dead player's inventory, a note in a tent, and a note on the ground inside a flag radius are readable after restart.

Exit: vanilla notes work end to end on 1.30 Nasdara and the quantity override is proven safe on existing storage. A failure in item 4 stops N2 and falls back to an own stack item.

## Phase 1 - quantity and combine rule

- Addon skeleton; the `Paper` config override and the modded `Paper` class per N2; string table; `NOTICE`.
- Self-test: `Paper` quantity bounds; the combine rule as an engine-free predicate.
- `pack.json` row added by the orchestrator; the `lifetime` row drafted in `server/`.

## Phase 2 - release

- Apply N3 and N4 live at a restart with the file backed up.
- Content-addon release gate, including the boot without Scrawl (SC-005 proves that sheets with quantity above 1 load as 1 and nothing else changes).

## Tests

| Id | Case | Evidence |
| --- | --- | --- |
| SC-001 | A note written on the test server is readable after restart | Screenshot before and after |
| SC-002 | Five sheets combine into one stack of 5 and split back | Inventory screenshots |
| SC-003 | A written note refuses to combine with a blank sheet or another note; a stack of 2 cannot be written on | Action menu screenshots |
| SC-004 | A dropped note is gone after `lifetime` with no interaction; a note picked up at half-life is not; a note inside a flag radius survives | CE log and test record (run with a short test lifetime) |
| SC-005 | Existing vanilla sheets in a `storage_1` copy load with their text under the override, and again after Scrawl is removed | Release gate RPT, 0 "Scripted variables corrupted" |

## Risks

| Risk | Mitigation |
| --- | --- |
| The quantity override changes the saved layout of existing sheets | Phase 0 item 4 is a stop condition; fallback is an own stack item |
| Two written notes merge and one text is lost | N2 combine rule; SC-003 |
| Long notes grow `storage_1` | Vanilla `NoteMenu` caps the length; recorded in Phase 0 item 3 |
