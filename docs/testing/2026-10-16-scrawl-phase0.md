# Scrawl Phase 0 and Phase 1 - October 16, 2026

Plan: `docs/plans/2026-10-15-scrawl-plan.md` v0.3. Branch `feat/scrawl`, worktree `D:\GitHub\nimby-badlands-server-wt\feat-scrawl`.
Build: `build\@NIMBY-<STAMP>` (fill from `build.log`). Server: local replica `D:\DayZServer`. Vanilla control on instance 998 with `-Vanilla`; Scrawl runs on instance 999 (fresh world) for the write tests and on a copy of `storage_998` for the round trip. Tester: Jeff, one client. Items are spawned with the panel-free method of the session (fresh world: find paper and a pen, or use the dev harness if it exists by then).

Behavior under test: Paper has a quantity of 1-5 and splits; a sheet with text never combines or splits; writing needs a single sheet; the vanilla write and read actions are reachable again (N5, pending Jeff's ruling after step 3a).

## Results

| Step | Check | Result | Evidence |
| --- | --- | --- | --- |
| 1 | Build and reference hashes recorded | - | `docs/baselines/2026-10-15-dayz-1.30-scripts-reference.md`; `build.log` PBO SHA-256 |
| 2 | Draft N3 and N4 rows exist and match the live rows except the changed fields | - | `server/mission-overrides/db/types.scrawl.patch.xml` against `server/live/db/types.xml:18166,18409` |
| 2 | Builds, signs, verifies; boot check PASS; self-test 11 PASS 0 FAIL | - | `logs\build-<STAMP>\build.log` SELFTEST and SUMMARY lines |
| 3a | Vanilla control (`-Vanilla`, instance 998): pen in hands looking at paper, paper in hands looking at pen, paper in hands alone. Does any write or read action appear? | - | Screenshot of the action prompt or its absence; decides N5 |
| 3b | Scrawl loaded: write a note with each of the four pens; read it back | - | Screenshots; `script_*.log` has no `SCRIPT (E)` |
| 3c | Drop the note, restart the server, read it again | - | Screenshot after restart |
| 3d | Maximum accepted note length | - | Count from `NoteMenu` |
| 4 | Boot a copy of `storage_998` with Scrawl: existing sheets load at quantity 1 with text; 0 "Scripted variables corrupted"; save, reload, confirm | - | RPT lines; the content-addon gate of shared rules 6.1 |
| 5 | Note in a dead player's inventory, in a tent, on the ground inside a flag radius, each readable after restart | - | Screenshots |
| SC-002 | Five sheets combine to a stack of 5 and split back | - | Inventory screenshots |
| SC-003 | A written note refuses to combine with a blank sheet and with another note; a stack of 2 offers no write action | - | Action menu screenshots |
| SC-005 (preview) | Boot the same storage without Scrawl: sheets with quantity above 1 load as 1, text intact | - | RPT |

## Findings

- (fill in)

## Exit

Exit condition (plan): vanilla notes work end to end on 1.30 Nasdara and the quantity override is proven safe on existing storage. A failure in step 4 stops N2 and falls back to an own stack item. Verdict: (fill in)
