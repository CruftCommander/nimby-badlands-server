# Tack merged-main round - October 18-19, 2026

Plan: `docs/plans/2026-10-15-tack-plan.md` v0.9. Branch `main` (Scrawl, Ears, and Tack merged October 17, 2026). Predecessor record: `docs/testing/2026-10-17-tack-phase1.md`.
Builds: `@NIMBY-20261017-235142` (self-review fix commit `0712def`; run 1), `@NIMBY-20261018-170109` (physics normal and read action, `ced91c5`; run 2), `@NIMBY-20261019-124534` (client trace, `7c28437`; run 3), `@NIMBY-20261019-131632` (read end fix, `9d1ecf8`; run 4). Every build: SUMMARY PASS, self-test Tack 48, Notes 11, Earplugs 16, 0 FAIL, boot PASS on instance 999, 0 `SCRIPT (E)`.
Server: local replica `D:\DayZServer`, instance 999, profile `main-round1` (`NimbyTack\config.json` with `wallOffsetMeters` 0.06 for run 1, 0.01 from run 2; Tack and Notes debug flags). Client: `scripts\Start-NimbyTestClient.ps1`, client debug flags for Notes and Tack from run 3. Logs: `logs\testserver-20261018-104820` (run 1, 3140 events), `-110908` (run 1 restart check), `-211457` (run 2), `logs\testserver-20261019-124858` (run 3), the run 4 folder after it, and the client `%LOCALAPPDATA%\DayZ\script_2026-10-19_12-50-51.log` and the next one. Tester: Jeff.

## Results

| Step | Check | Result | Evidence |
| --- | --- | --- | --- |
| R1-1 | Mud-brick clip at 6 cm | FAIL, then PASS at 1 cm in run 2 | Run 1: the sheet vanished inside both the mud-brick house and the wooden shed. The `pin` lines on one flat shed wall carried `normal=<0.06, 0.18, -0.98>`, `<0.15, 0.03, -0.99>`, `<-0.04, -0.20, -0.98>`: a wall's normal does not move with the player's pitch, so `RaycastRVResult.dir` is the ray, not the normal, and every offset so far went into the wall. The relief and leaning-wall explanations in the October 17 record are withdrawn; the fire-geometry and view hits were 6 mm apart (`gap=0.0059`). Fix `ced91c5`: normal from `DayZPhysics.RayCastBullet` (building layer), flipped toward the player; `FaceDirection` is the outward normal. Run 2 at 1 cm: flat on the wall, outside it, text side toward the player, on the house, the shed square on, and the shed from a crouch looking up |
| R1-2 | Take-down, nail: sheet back, health 10 | PASS | `takedown fastener=nail sheet=inventory health=10 damage=5` |
| TK-007 | Tape pins, loses 10; take-down costs 3 | PASS | `pin fastener=tape ... health=10`, `takedown fastener=tape ... health=7 damage=3` |
| TK-008 | Epoxy pins, loses 10; take-down costs 7 | PASS | `pin fastener=epoxy`, take-down 7 (Jeff) |
| TK-010 | Wrench with a nail offers the nail action; Screwdriver does not | PASS (wrench half) | No screwdriver in the kit; the refusal half stays open |
| TK-009 | Knife pins wood, refuses mud brick, knife returns | PASS, with a limitation | Pin and refusal PASS. The knife is an inventory attachment of the pinned note and the Paper model has no proxy for the slot, so it does not render on the wall; it shows in vicinity. The "visible on the wall" wording in the October 17 record is withdrawn |
| S5 | Burn a knifed note | PASS | `burn class=NimbyTack_PinnedNote_Knife source=Matchbox knife=ground player=...`: note gone, knife on the ground |
| R1-8 | Burn a nailed note | PASS | `burn class=NimbyTack_PinnedNote_Nail source=Matchbox player=...` |
| TK-005 | Area cap | NOT RUN | Skipped by Jeff (paper and patience); the cap is self-tested |
| N11 | Write a note; controls return at once | PASS | Scrawl write path on the merged build |
| TK-001 | Read a written note on the wall | FAIL, then PASS in run 2, pose fixed in run 4 | Run 1: no read action offers on a wall; vanilla `ActionReadPaper` is in-hands only (`CCTNone`, `HasTarget` false). Fix `ced91c5`: `ActionNimbyTackRead`, hold-F target action, vanilla `RPC_READ_NOTE` to the reader, NoteMenu opens in place. Run 2: text shows; after closing, an empty progress bar and the note pose stayed until a jump or right click. Run 3 client trace: `state=2`, `end-input` the frame the menu opened (the menu takes the held key), `state=4` (finished), menu closed, `watcher: menu closed but not ending; state=4`. Fix `9d1ecf8`: the shared watcher ends `UA_FINISHED` as well as `UA_PROCESSING`, as vanilla `ActionReadPaper.OnUpdate` does. Run 4: `tack-read ended: note menu closed`, `read end-request state=4`, `read end state=4` 33 ms after the close, every read; controls back with no input |
| TK-003 | Take down a written note: text intact | PASS | Jeff, run 1 |
| TK-012 | Written note survives a restart in place | PASS | Pinned before the run 1 stop, present and readable in run 2 |
| P0-5 | Ruined sheet | PASS | Third take-down returns a ruined sheet (Jeff) |
| S1 | Two take-downs in one frame | NOT RUN | Needs a second player; the guard is `IsSetForDeletion` plus the default target lock |
| R2-2 | Read first in the hold-F list, take-down second | PASS | Run 2 |
| R2-5 | Read a sheet in hands after the watcher change | PASS | Run 4 |

## Findings

- The "normal" was the ray. Every Tack placement before `ced91c5` moved the sheet into the wall along the player's line of sight, which produced the 1 cm "sits inside the relief", 3 cm "still clips", and 6 cm "vanished" sequence. `RaycastRVResult.dir` is documented as "direction outside or, for a line-object collision, direction and size of the intersection" (`DayZPhysics.c:104`); the second reading applies.
- Reading a Paper on a wall needs a target action; the vanilla read is in-hands only.
- A continuous read whose held key is taken by the note menu finishes its component while the menu is open and then waits in the pose; whatever ends it must act on `UA_FINISHED`, not only `UA_PROCESSING`.
- Self-review S1, S2, S7, S8, S9, S10, S12, S13, N1 landed in `0712def` before the round; none regressed.

## Open

- TK-010 screwdriver half, TK-005 area cap in game, S1 two-player take-down, EP-002 with Jay, Scrawl step 4 (`storage_998` round trip).
- Knife render on a knifed note: needs a model proxy or a separate frozen object; Phase 2 or 3.
- Default `wallOffsetMeters` stays 0.03 (Jeff, October 19, 2026); the profile ran 0.01.

## Exit

Phase 1 exit met on the merged `main` build `@NIMBY-20261019-131632`: pin with all four fasteners, read in place, take-down with the text, burn, restart survival, and the ruined-sheet path all pass with a tester present.
