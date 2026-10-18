# Scrawl Phase 0 and Phase 1 - October 16, 2026

Plan: `docs/plans/2026-10-15-scrawl-plan.md` v0.3. Branch `feat/scrawl`, worktree `D:\GitHub\nimby-badlands-server-wt\feat-scrawl`.
Build: `build\@NIMBY-20261016-173319` (`logs\build-20261016-173319\build.log`: SUMMARY PASS, self-test 11 PASS 0 FAIL shown as 22). Server: local replica `D:\DayZServer`. Vanilla control on instance 998 with `-Vanilla` (run `logs\testserver-20261017-161551`); Scrawl on instance 999 (fresh world) for the write tests (runs `testserver-20261017-162430` and `-163854`, the second one the restart for 3c); a copy of `storage_998` for the round trip (step 4, pending). Tester: Jeff, one client, `DayZ_BE.exe -mod=<build>`. Items from the replica `init.c` test kit (every connect hands over paper, four pens, hammer, nails, duct tape, epoxy, knife, matches, wrench; what does not fit lands on the ground). Client script logs `%LOCALAPPDATA%\DayZ\script_2026-10-17_16-25-09.log` and `_16-39-57.log`, 0 `SCRIPT (E)`; server script logs 0 `SCRIPT (E)`.

Behavior under test: Paper has a quantity of 1-5 and splits; a sheet with text never combines or splits; writing needs a single sheet; the vanilla write and read actions are reachable again (N5, pending Jeff's ruling after step 3a).

## Results

| Step | Check | Result | Evidence |
| --- | --- | --- | --- |
| 1 | Build and reference hashes recorded | PASS | `docs/baselines/2026-10-15-dayz-1.30-scripts-reference.md`; `build.log` PBO SHA-256 lines |
| 2 | Draft N3 and N4 rows exist and match the live rows except the changed fields | PASS | `server/mission-overrides/db/types.scrawl.patch.xml` against `server/live/db/types.xml:18166,18409` |
| 2 | Builds, signs, verifies; boot check PASS; self-test 11 PASS 0 FAIL | PASS | `logs\build-20261016-173319\build.log` SELFTEST and SUMMARY lines |
| 3a | Vanilla control (`-Vanilla`, instance 998): pen in hands looking at paper, paper in hands looking at pen, paper in hands alone. Does any write or read action appear? | NO ACTION (decides N5: restore) | Run `logs\testserver-20261017-161551` (`-Vanilla`, instance 998, October 17, 2026, 16:15 CT). Screenshots: pen in hands looking at a sheet on the ground shows only "Take"; dragging a sheet onto the pen in hands offers "Combine" and "Swap", and Combine does nothing (the empty `WriteLetter` recipe, plan risk table). No write or read action anywhere. Items came from the replica `init.c` test kit (14 `[NIMBY][TestKit] ... -> inventory` lines in the server script log) |
| 3b | Scrawl loaded: write a note with each of the four pens; read it back | PASS (write and read; four colors not individually confirmed) | Screenshots October 17, 2026: pen in hands looking at a sheet on the ground opens the write menu; "Hello World" written, the sheet picked up and read. Blank sheets still stack when combined (N3 holds). Open items from Jeff: (a) a written note on the ground should be readable by looking at it, not only in hands (N7); (b) dragging paper onto the pen in hands still shows the dead vanilla "Combine" and writes nothing (N9); (c) pen text must be permanent, append only (N8). Defect: after every read the player is stuck in the note pose (WASD moves, no mouse look, no jump) until the sheet leaves the hands; cause and fix in N11 |
| 3c | Drop the note, restart the server, read it again | PASS | Rebuild runs `testserver-20261017-174819` and `-175236` (October 17, 2026): the dropped note read back with its text after the restart (Jeff). The first attempt's "gone" is unexplained; the storage held the text throughout (`dynamic_008.bin`) |
| 3d | Maximum accepted note length | OBSERVED, no count | No cap in script (`NoteMenu.c` has none; `WrittenNoteData` stores the string). The write box scrolls past what is visible, and the read view renders a long note at an unreadably small size (screenshot: three full rows of "f" in the editor, a tiny two-block render on read). Scrawl adds a cap (N12, number pending Jeff) |
| 4 | Boot a copy of `storage_998` with Scrawl: existing sheets load at quantity 1 with text; 0 "Scripted variables corrupted"; save, reload, confirm | - | RPT lines; the content-addon gate of shared rules 6.1 |
| 5 | Note in a dead player's inventory, in a tent, on the ground inside a flag radius, each readable after restart | - | Screenshots |
| SC-002 | Five sheets combine to a stack of 5 and split back | - | Inventory screenshots |
| SC-003 | A written note refuses to combine with a blank sheet and with another note; a stack of 2 offers no write action | - | Action menu screenshots |
| SC-005 (preview) | Boot the same storage without Scrawl: sheets with quantity above 1 load as 1, text intact | - | RPT |

## Findings

- N5 is decided by 3a: vanilla 1.30 offers no write or read action; Scrawl keeps the re-registration.
- Stuck controls after reading (N11): two causes, the second is the real one. First build fix (ending the looping read action when the menu closes) changed nothing in the 17:48 rebuild run: no mouse look, no jump, WASD fine, cured by dropping the note. Real cause: while the note menu is open `MissionGameplay.OnUpdate` adds the `inventory` input exclude and the INVENTORY input restriction (`missionGameplay.c:662-666`) and nothing removes them when the note menu closes; `HideInventory` does it for the inventory menu (`:1216-1217`), which is why dropping the note (inventory open and close) cured it. Fix on `feat/scrawl`: `modded class NoteMenu.OnHide` removes both (`Scripts/5_Mission/nimbynotes_notemenu.c`, new `missionScriptModule` in `config.cpp`). Retest in the next rebuild.
- The dead vanilla "Combine" on pen plus paper (the empty `WriteLetter` recipe) shows with Scrawl loaded too and confuses: Jeff expects it to write (N9).
- Long notes are accepted without limit and render tiny on read (N12).
- Jeff, October 17, 2026, for a later phase: fonts, sizes, bold, underline, strikeout, bullet and numbered lists; pencils that allow erasing (N10).
- Test kit: the replica `init.c` hands items on every connect; a fresh 999 character has little inventory, so part of the kit lands on the ground beside the spawn (expected).

## Exit

Exit condition (plan): paper stacks, written notes never merge, and the vanilla write and read path works again. Verdict: PASS on 3a, 3b, and stacking, October 17, 2026, 3c passed in the rebuild; one defect (N11) survived the first fix, real cause found and fixed, retest pending; 3d and step 4 pending. Phase 2 gains N7, N8, N9, N12; Phase 3 gains N10.
