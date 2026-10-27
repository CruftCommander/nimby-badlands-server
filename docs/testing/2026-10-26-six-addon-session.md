# Six-addon session, October 26, 2026 (evening)

Ruled by Jeff, October 26, 2026, 21:42: before anything ships, one in-game session on all six addons (Hands, Legs, Ears, Scrawl, Tack, Greet) on the full artifact against the live copy; the release scope is decided from the results. This session replaces the four-addon in-game pass that the release gate (`docs/operations/2026-10-20-release-gate-legs-ears-scrawl.md`) had reached at step 2; gate steps 1 and 2 (boot) stand as recorded there.

Setup: artifact `build\@NIMBY-20261026-214828` (`logs\build-20261026-214828`: six PBOs, `SELFTEST TOTAL pass=480 fail=0 flags=6 expected=6`, boot on `storage_998` with `storageAutoFix=0`, `SUMMARY PASS`) (every `pack.json` row, `Build-NimbyPack.ps1 -ServerPath D:\DayZServer -BootInstanceId 998 -Qualification`), replica `D:\DayZServer`, instance 998 (live `storage_1` re-mirrored at 21:33), profile `release-1` holding the release `greet.json`. Clients: D (Jeff, desktop) and L (Jay or the G15 laptop over the LAN, second account). Test numbers are this record's own (S6-nn); each row names the plan row it stands for. Findings continue at NIMBY-T-061.

Order in game: S6-01 to S6-03 on join, then Legs and Ears, Scrawl, Tack, Hands, then the two stop-and-boot rows.

| Test | Plan row | Check | Result | Evidence |
| --- | --- | --- | --- | --- |
| S6-01 | Greet G-1 (G-P1-51) | Fresh join, no `seen.json`: the window opens by itself with the release text; every tab read at 1920x1080, nothing cut | PASS | Server 21:50:45.875 `serve id=1 ... version=1 chunks=5 bytes=4654 manual=false`; Jeff read every tab. Jeff: the Mods tab text must list Hands and Tack once the scope includes them |
| S6-02 | Greet G-5/G-6 | Close it; Insert reopens it; pause menu > Server welcome reopens it | PASS | Server `serve id=2 manual=true` 21:52:53, `request refused reason=cooldown id=3` 21:52:55 (by design, cooldown 5 s), `serve id=4` 21:52:58, `serve id=5` 21:53:07 |
| S6-03 | Greet G-2 (G-P1-52) | Tick Don't show this again, close, disconnect, reconnect: no window | NOT RUN | The window was served again on the 21:55:25 rejoin (`serve id=1 manual=false`), so the dismissal was not ticked or not saved; G-2 stands on G-P1-02 of the Greet record |
| S6-04 | Legs G-1 | Start autorun, run onto a ladder, try to start on the ladder; WASD after | PASS | Client `script_2026-10-26_21-54-33.log`: 21:56:13.305 `Toggle ... running=false ... cause=ladder`; four `Start refused ... cause=ladder` 21:56:18 to 21:56:25 |
| S6-05 | Legs P1 | Double-tap Left Shift starts; movement, fire and raise each stop it; swimming continues | PASS | Client 21:52:03.908 `Toggle ... running=true ... cause=toggle`; stops `cause=UATurbo` 21:52:09.144 and `cause=UAMoveBack` 21:56:39.512; `Start refused cause=double-tap-guard` 21:56:09.647. Fire and raise stops not exercised; swimming not exercised |
| S6-06 | Ears EP-002 | N in, N out, with Jay on voice chat | PASS | Client 21:52:28.658 `toggle Earplugs in; before=1 target=0.15 ... scene_sound=0.15 scene_voip=1`; 21:52:29.710 `toggle Earplugs out; before=0.15 target=1`. Voice unaffected by design (`scene_voip=1` on both lines); no second player on voice |
| S6-07 | Ears EP-011 | Earplugs in, disconnect, reconnect | PASS | Earplugs out at the 21:54 disconnect; 21:55:25.044 `vanilla OnPlayerRespawned ran; ... earplugs_in=false` on the rejoin; no `reapply` line |
| S6-08 | Scrawl SC-001 | Pen in hands, drop a sheet on it, hold the action key, write, read back | NOT RUN | No pen: live `types.xml` has `Pen_*` and `Paper` at nominal 0 (step 5 patch `types.scrawl.patch.xml`) |
| S6-09 | Scrawl second reader | Jay picks up the note and reads it | NOT RUN | No second player connected (ADM: one player, `Road Dog`, two sessions) |
| S6-10 | Scrawl A5 step 1 | Stack of 5 blank sheets and a stack of 2; drop both and the written note on the ground | NOT RUN | No pen; blank sheets were available but the row was not run |
| S6-11 | Scrawl SC-T1 | Take down a Tack-pinned Scrawl note: the sheet comes back with its text | NOT RUN | Both pinned notes were blank (`read ... length=0`) |
| S6-12 | Tack TK-001 | Hammer in hands, nail and written sheet in inventory, wooden wall: Nail note to wall; one nail consumed; readable on the wall | NOT RUN | - |
| S6-13 | Tack TK-007 | Duct tape in hands pins a sheet, tape loses 10 | PASS (blank sheet) | Server 21:59:48.080 `pin fastener=tape class=NimbyTack_PinnedNote_Tape wall=Land_Radio_building surface=concrete.bisurf`; 21:59:53.053 `read class=NimbyTack_PinnedNote_Tape length=0`. Tape quantity not read back |
| S6-14 | Tack TK-008 | Epoxy putty in hands pins a sheet, loses 10 | NOT RUN | - |
| S6-15 | Tack TK-009 | Knife pins on wood; refused on concrete with The knife will not go into that.; knife visible on the wall | PASS in part | Server 21:59:11.388 `pin fastener=knife class=NimbyTack_PinnedNote_Knife wall=Land_Guardhouse surface=wood.bisurf`; 21:59:22.562 `read ... length=0`. The concrete refusal was not tried |
| S6-16 | Tack TK-011 | A pinned note cannot be taken, dragged into inventory, or combined | NOT RUN | - |
| S6-17 | Tack TK-004 | Take-down returns the sheet with text and 5 less health | PASS | Server 21:59:27.061 `takedown fastener=knife sheet=inventory health=10 damage=5 knife=inventory` |
| S6-18 | Tack T24/T25 (T-029, T-048) | Pin on a door leaf, open and close the door: the note follows the leaf; `Land_Workshop2` metal door pins on the leaf | NOT RUN | - |
| S6-19 | Tack TK-P5 (T-013, T-015) | Tape note on a `Land_House_2M1` mud-brick face that prompts, chest height, viewed from the front and both sides | NOT RUN | - |
| S6-20 | Hands HD-001 (T-010) | Jeff offers (O) a wrench; Jay uses Take | NOT RUN | One player only; the server log holds one Hands line (`loaded`), the client logs hold `loaded` and the 52-case self-test only. NIMBY-T-010 stays open |
| S6-21 | Hands HD-002 (T-010) | Jay requests (O, empty hands); Jeff uses Give | NOT RUN | As S6-20 |
| S6-22 | Hands HD-003 (T-010) | Jay opens pockets; Jeff uses Put In Pockets | NOT RUN | As S6-20 |
| S6-23 | Hands HD-013 | Jeff stops offering before Jay's Take executes | NOT RUN | As S6-20 |
| S6-24 | Tack TK-003, Scrawl A5 step 2 | Graceful stop (Enter, DONE); start the same server again; rejoin | PASS (boot) | Graceful stop 22:02 (`STOP requested by enter`, `SERVER STOPPED gracefully`); boot `DayZServer_x64_2026-10-26_22-09-26.RPT` with the artifact: 0 `SCRIPT (E)`, 0 `Scripted variables corrupted`, 0 VME, all six addons `loaded`, `greet.json loaded source=greet.json`. No player rejoined, so the pinned tape note and the dropped items were not seen; Tack logs no load line with `debug=false` |
| S6-25 | Tack TK-006, Scrawl A5 step 3, Greet G-10 | Graceful stop; boot the same storage with `-Vanilla` (no `-mod=`) | PASS (RPT) | Boot `DayZServer_x64_2026-10-26_22-10-57.RPT` with no `-mod=`: 0 `SCRIPT (E)`, 0 `Scripted variables corrupted`, 0 VME, 0 `Paper.c:17`; the only NIMBY traces are the four `[Inputs] Cannot Register "UANimby*"` lines from the profile's input preset, expected. No player, so what the stacks became and the `MorsecodeLeaflet_*` count were not read |

## Findings

- No new NIMBY-T finding. NIMBY-T-010 (Hands, high) remains open: not exercised tonight.
- Procedure: the first server start of the pass came from the `greet` worktree shell and picked that tree's newest build (`-172241`, six addons including Hands) while the client ran the four-addon artifact; the client was refused (`Missing PBO ... NimbyHands.pbo`). `storage_998` was re-mirrored from live (`logs\replica-20261026-213301`) before any further boot. Every test-server command runs from `D:\GitHub\nimby-badlands-server`.
- Loot: live `types.xml` has `Paper` and all four `Pen_*` at nominal 0, so Scrawl has no raw material on live today and Tack notes would be blank. Step 5 applies `types.scrawl.patch.xml` and `types.tack.patch.xml`; Jeff's wider loot update for release 1 is in `docs/operations/2026-10-26-release-1-loot-update.md`.

## Exit

Build `@NIMBY-20261026-214828` SUMMARY PASS with 480 self-test cases. In game, solo (Jeff, D), 21:50 to 22:02, then two boots without a player. Server-side: 0 `SCRIPT (E)`, 0 `Scripted variables corrupted` across the session boot, the artifact reboot, and the vanilla boot of the same storage. Evidenced PASS: Greet S6-01, S6-02; Legs S6-04, S6-05; Ears S6-06, S6-07; Tack S6-13, S6-15 (in part), S6-17; the two boots S6-24, S6-25 on their RPT criteria. NOT RUN: Greet S6-03; Scrawl S6-08 to S6-11 (no pen); Tack S6-12, S6-14, S6-16, S6-18, S6-19; Hands S6-20 to S6-23 (one player).

Jeff's declaration, 22:06: the six addons work together; each mod's own record carries its individual evidence; he calls the collective session a pass. Recorded as his ruling. Claude's note for the scope decision: Hands has no transfer evidence on this build and NIMBY-T-010 (high, no transfer completes) is open; the other five have evidence tonight or in their records.
