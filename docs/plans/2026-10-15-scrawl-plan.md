# Plan 6 - Scrawl (`NimbyNotes`)

Draft v0.6, October 19, 2026. Changes since v0.5: step 4 (live-copy world, `storage_998`, `storageAutoFix = 0`) found a vanilla save bug the restored write action exposes: `Paper.OnStoreSave` serializes the whole `WrittenNoteData`, and `ActionWritePaper` leaves the pen and paper entity references in it, so a written sheet saves the pen's entity id and the reload throws a Virtual Machine Exception at `Paper.c:17` when that id resolves to another object; the sheet survived, the text is re-verified in the fix round. Fix: the references are cleared when a write completes (modded `WrittenNoteData.OnRPC`) and before every save (modded `Paper.OnStoreSave`); the saved layout is unchanged. The boot itself was clean: 0 "Scripted variables corrupted", 0 script errors. Changes since v0.4: self-review fixes on `main` (S2 `NoteMenu.OnHide` leaves the inventory input exclude and restriction alone when the inventory menu is open underneath, for N9; S10 the read and write menu watchers share one `NimbyNotesMenuWatch` instance class; N1 `NimbyNotesLog` with the `$profile:NimbyNotes\debug` flag replaces the bare `Print` calls, and the per-use menu lines are debug level). Owner: Jeff Fabish. Implementer: a Claude Code session in worktree `feat/scrawl`, coordinated by the orchestrator. Status: Phase 1 in the in-game test block (test record `docs/testing/2026-10-16-scrawl-phase0.md`); N5 decided by the vanilla control; N11 fixed and retested on `@NIMBY-20261017-182715` (Jeff: "Scrawl: Pass"); merged to `main` October 17, 2026.

Changes since v0.3: the vanilla control showed no write or read action in unmodified 1.30, so N5 stands as decided; write, read, and stacking pass in game; one defect found and fixed (N11, the never-ending note action); Jeff's October 17 requirements recorded as N7 (read a note on the ground by looking at it), N8 (pen ink is permanent, append only), N9 (pen plus paper in the inventory writes), N12 (note length cap), and N10 (a later formatting phase with pencils that erase). Changes since v0.2: the 1.30 write path is corrected (see N5 and the current-state table): `ActionWritePaper` is marked deprecated and neither it nor `ActionReadPaper` is registered anywhere in the 1.30 scripts, and the `WriteLetter` recipe that carries the `#write_note` label has an empty `Do()`; Scrawl therefore re-registers both vanilla actions (N5, proposed). The `pack.json` row and a stub landed on `main` ahead of the worktree (N6). Phase 1 code, the engine-free rules with 11 self-test cases, and the draft `types.xml` rows are in the worktree.

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
| Write action | `ActionWritePaper` exists with the pen-and-paper condition at `:57-59` but is marked `//!DEPRECATED` (`:36`) and is registered nowhere: `ActionConstructor.RegisterActions` (`ActionConstructor.c:27`, 343 actions) lists neither note action, `Paper.SetActions` adds only fireplace and attach actions, and `Pen_ColorBase` has no `SetActions` (`Pen_ColorBase.c:1`). The `#write_note` label now belongs to the `WriteLetter` recipe (`Recipes/WriteLetter.c:5`), whose `Do()` is empty and whose result is commented out. Only `ActionWritePaper` sends `RPC_WRITE_NOTE` (`:32`), so in unmodified 1.30 script the note menu has no caller | `ActionWritePaper.c:32,36,57-59`, `ActionConstructor.c:27`, `WriteLetter.c:1-70`, `WrittenNoteData.c:13-57` |
| Pens | `Pen_Black`, `Pen_Red`, `Pen_Green`, `Pen_Blue`; quantity depletes one per character | `Pen_ColorBase.c`, `WrittenNoteData.c:83-87` |
| Read action | `ActionReadPaper` exists and sends `RPC_READ_NOTE`, but is likewise unregistered | `ActionReadPaper.c`, `ActionConstructor.c:27`, `ERPCs.c:70` |
| Base protection | `TerritoryFlag` refresher | `Totem.c:1,568` |
| Live loot rows | Read October 16, 2026: `Paper` nominal 0, `lifetime` 7200, spawning only as 50 percent cargo in `Bear_Beige`, `Bear_Dark`, and `Bear_Pink`; every `Pen_*` nominal 0 with no cargo entry. Nobody can write a note on Nasdara today | `server/live/db/types.xml`, `server/live/cfgspawnabletypes.xml:478-490` |

## Decisions

| # | Decision | Choice | Status |
| --- | --- | --- | --- |
| N1 | Display name | Scrawl. Code prefix `NimbyNotes`, log prefix `[NIMBY][Notes]`, RPC block `65506xx` reserved and unused | Decided October 15, 2026 |
| N2 | Paper stack | Vanilla `Paper` gets `varQuantityInit = 1`, `varQuantityMin = 0`, `varQuantityMax = 5`, `canBeSplit = 1` through a config override in Scrawl's `config.cpp`. A modded `Paper` class refuses `CanBeCombined` and `CanBeSplit` when either sheet carries note text, and `ActionWritePaper` is allowed only on a quantity of 1 (split first). Sheets already in `storage_1` have no quantity bit and load as 1 | Decided by Jeff, October 15, 2026 (D11) |
| N3 | Despawn | Live `types.xml`: `Paper` `lifetime` 2592000 (30 days). If it degrades the server, scale to 1209600 (14) or 604800 (7) at a restart. CE refreshes lifetime on interaction, so an untouched note despawns and a handled one does not. A raised `TerritoryFlag` refreshes every item in its radius, so notes in a flagged base live as long as the base; notes in a tent, barrel, or body follow the container | Decided by Jeff, October 15, 2026 (D12) |
| N4 | Loot | Live `types.xml`: `Paper` nominal 30 (residential tiers 1-2, the teddy-bear cargo stays) and 30 pens in total across the four colors (`Pen_Black` 8, `Pen_Blue` 8, `Pen_Red` 7, `Pen_Green` 7); a stack spawns as quantity 1-5 through the `Paper` row. These are the first live loot additions of the theme | Decided by Jeff, October 16, 2026 (totals); the per-color split is Claude's |
| N5 | Restore the note actions | Scrawl registers `ActionWritePaper` and `ActionReadPaper` in a modded `ActionConstructor.RegisterActions`, adds `ActionReadPaper` to `Paper` and `ActionWritePaper` to `Pen_ColorBase`, and limits writing to a quantity of 1 in a modded `ActionWritePaper.ActionCondition`. Both classes are Bohemia's; Scrawl adds no menu, RPC, or note code of its own | Proposed by Claude, October 16, 2026; decided by the vanilla control on October 17, 2026 (test record 3a: no write or read action anywhere in unmodified 1.30) |
| N6 | `pack.json` row timing | The `NimbyNotes` row and a stub `config.cpp` landed on `main` (commit `95c69cf`) before the worktree branched, so Phase 1 builds and `main` stays buildable; same exception to shared rules section 3 as Ears E6 | Decided by Jeff, October 16, 2026 |
| N7 | Read in place | A written sheet lying in the world is readable by looking at it, without picking it up: a second read action targeting a `Paper` with text within reach, same note menu, same RPC | Requested by Jeff, October 17, 2026; Phase 2 |
| N8 | Pen ink is permanent | Writing with a pen can only append: the server accepts a new text only when it starts with the sheet's existing text, otherwise the write is refused and the player is told. Pencils (N10) are the erasable tool later | Requested by Jeff, October 17, 2026; Phase 2 |
| N9 | Pen plus paper in the inventory writes | Dragging a sheet onto a pen (or the reverse) opens the write menu instead of the dead vanilla "Combine": a modded `WriteLetter` recipe whose `Do()` opens the note menu for that pair through the write RPC. Until then the dead entry stays visible | Requested by Jeff, October 17, 2026; Phase 2 |
| N10 | Formatting and pencils | Later phase: fonts, sizes, bold, underline, strikeout, bullet and numbered lists in the note editor and reader (the vanilla reader is already an `HtmlWidget`); a `Pencil` writing tool whose text can be erased | Requested by Jeff, October 17, 2026; Phase 3, after the Phase 2 items |
| N11 | Controls after the note menu | Two layers. (1) `MissionGameplay.OnUpdate` adds the `inventory` input exclude and the INVENTORY restriction while the note menu is open (`missionGameplay.c:662-666`) and nothing removes them on close, so mouse look and jump stay dead until the inventory is opened and closed; `modded class NoteMenu.OnHide` removes both, the same calls `HideInventory` makes. (2) The vanilla read and write actions loop `CMD_ACTIONFB_VIEWNOTE` on `CAContinuousTime(-1)` and only end when `m_State == UA_FINISHED`; `NimbyNotesMenuWatch` ends either action when the menu closes, kept as a guard | Found October 17, 2026; layer 2 alone did not cure it in the first rebuild; layer 1 fix awaits the next rebuild |
| N12 | Note length cap | Vanilla accepts any length and the reader shrinks a long note to an unreadable size. Scrawl caps the accepted text at 1000 characters (the editor shows about three rows of 110 characters before scrolling); the cap is enforced on the server in the write handler and shown in the editor | Found October 17, 2026; 1000 ruled by Jeff, October 17, 2026; Phase 2 |

## Phase 0 - prove vanilla notes and the quantity round trip

1. Record builds and reference hashes in `docs/baselines/`.
2. Confirm the `Paper` and `Pen_*` rows in `server/live/db/types.xml` against the current-state table (both at nominal 0) and draft the N3 and N4 rows in `server/`.
3. On the local replica, first with the vanilla control boot (`-Vanilla`): put a pen in hands and look at a sheet of paper, and the reverse; record whether any write or read action appears. Then with Scrawl loaded: write a note with each pen color, read it, drop it, restart the server, and read it again. Record the maximum accepted length from `NoteMenu` (`NoteMenu.c:55` counts symbols; no cap is visible in script). The vanilla half decides N5.
4. Boot a `storage_1` copy that contains vanilla sheets with the N2 config override loaded; confirm every existing sheet loads at quantity 1 with its text, with 0 "Scripted variables corrupted". Save, reload, and confirm again.
5. Confirm a note in a dead player's inventory, a note in a tent, and a note on the ground inside a flag radius are readable after restart.

Exit: vanilla notes work end to end on 1.30 Nasdara and the quantity override is proven safe on existing storage. A failure in item 4 stops N2 and falls back to an own stack item.

## Phase 1 - quantity and combine rule

Written October 16, 2026.

- `config.cpp`: `CfgPatches` requiring `DZ_Data` and `DZ_Gear_Consumables`; `CfgVehicles.Paper` override `canBeSplit = 1`, `varQuantityInit = 1`, `varQuantityMin = 0`, `varQuantityMax = 5`, `varQuantityDestroyOnMin = 1` (the `Rag` pattern, `DZ/gear/consumables/config.cpp:343-346`).
- `Scripts/3_Game/nimbynotes_rules.c`: `NimbyNotesRules` with `CanCombine`, `CanSplit`, `CanWrite` as pure predicates and 11 self-test cases.
- `Scripts/4_World/nimbynotes_paper.c`: modded `Paper` applying the predicates in `CanBeCombined` and `CanBeSplit`, plus `ActionReadPaper` (N5). `nimbynotes_pen.c`: `ActionWritePaper` on `Pen_ColorBase` (N5). `nimbynotes_actions.c`: registration, the quantity-1 write condition, and the N11 menu watcher. `nimbynotes_selftest.c`: hook on `MissionBaseWorld` for the debug flag, the load line, and the `$profile:NimbyNotes\selftest` flag. `Scripts/3_Game/nimbynotes_log.c`: `NimbyNotesLog` (`[NIMBY][Notes]`, debug flag `$profile:NimbyNotes\debug`, shared rules 4.3). `Scripts/5_Mission/nimbynotes_notemenu.c`: `NoteMenu.OnHide` input restore (N11) with the open-inventory guard.
- `server/mission-overrides/db/types.scrawl.patch.xml`: the N3 and N4 rows, draft, with the apply procedure in `server/mission-overrides/README.md`. Paper usages Town, Village, Office, School and tiers 1-2 are Claude's reading of "residential tiers 1-2"; `min` is half of nominal.
- No string table: Scrawl adds no player-facing text (the vanilla `#write_note` and `#read` labels are reused).

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
| The vanilla `WriteLetter` recipe (`#write_note`, empty `Do()`) still appears when a pen meets paper and does nothing | Phase 0 item 3 records whether the dead recipe entry shows beside Scrawl's action; if it confuses players, a modded `WriteLetter.CanDo` returning false hides it (one line, Phase 2) |
| The `Paper` override is inherited by `MorsecodeLeaflet_*` (`Paper.c` tail), which gain a quantity too | The three leaflets spawn live at nominal 20 each (`server/live/db/types.xml:16321-16346`), so this is real: leaflets of the same kind would stack to 5 and the N2 combine rule would apply to them. Phase 0 item 4 boots existing leaflets through the override as part of the round trip; if stacking leaflets is unwanted, the override moves from `Paper` to a per-class block that leaves the leaflet subclasses at vanilla (Phase 2 decision for Jeff) |
