# Hands Phase 1 and Phase 2 - two-client test record (skeleton)

Plan: `docs/plans/2026-10-15-hands-plan.md` v0.5. Branch `feat/hands`. Predecessor record: `docs/testing/2026-10-14-solo-ingame-test.md` (1.29 solo run, NIMBY-T-002).
Build: `@NIMBY-20261019-153354` (commit `8cc681b`, after the Phase 2 rename `e5eb20d` and the reservation pre-check; boot check SUMMARY PASS, Hands self-test 52 PASS, 0 `SCRIPT (E)`). The Phase 1 build under the old name was `@NIMBY-20261019-152248`.
Server: to be filled - local replica `D:\DayZServer`, instance 999 (or 998 for the `storage_1` copy), profile name, `NimbyHands\debug` flag on the server and on both clients for the Phase 0 item 4 rows.
Clients: Jeff and Jay. Logs: to be filled - server `logs\testserver-<stamp>`, both client script logs.
Status: SKELETON. Nothing below has run. The result column is filled only from a run against the build named above.

Finding ids continue the `NIMBY-T-nnn` series; Hands-specific cases are `HD-nnn`.

## Phase 0 item 4 - completion-signal ranking

Run with `$profile:NimbyHands\debug` on the server and both clients. The plan's ranking from the 1.30 reference: EEItemLocationChanged first (wired), OnInventoryExit/OnInventoryEnter second (traced only), the hand event result third (not observable on 1.30).

| Step | Check | Result | Evidence |
| --- | --- | --- | --- |
| P0-4a | Successful Take: which signals fire on the server, and in what order | | Server debug lines for one `id`: `signal=EEItemLocationChanged.begin` (seq n), `signal=OnInventoryExit` naming the giver (n+1), `signal=OnInventoryEnter` naming the receiver (n+2), `signal=EEItemLocationChanged.end` (n+3), then `state=CONFIRMED detail=signal=EEItemLocationChanged ...` with the same `id`. Any other order, a missing line, or more than one begin/end pair per move is a finding |
| P0-4b | Same transfer: which signals fire on each client | | Giver and receiver client debug lines `side=client`: whether `EEItemLocationChanged.begin`/`end` and Exit/Enter appear on either client, and their `t=` against the server's. The plan ranks signals on the server only; a client-only signal is recorded, not wired |
| P0-4c | Latency of the wired signal | | `t=` of `CONFIRMED` minus `t=` of `REQUESTED` for 10 Takes and 10 Gives; every value well under 5,000 ms. A `CONFIRMED` whose detail says `reconciled at timeout` means the signal did not fire |
| P0-4d | Refused move: no signal | | A Take into a full inventory: one `REFUSED detail=no space in receiver inventory` line, and no `signal=` lines for that item |
| P0-4e | Nested items | | Take a backpack with items inside: `OnInventoryEnter` fires once per nested item (expected from `ItemBase.c:1235-1244`), `EEItemLocationChanged` once; one `CONFIRMED` |

## Phase 0 item 6 - native transfer between two real clients

| Step | Check | Result | Evidence |
| --- | --- | --- | --- |
| HD-001 | Offer (Jeff holds O) and Take (Jay) | | One `REQUESTED` and one `CONFIRMED` line with the same `id`; the item is in Jay's inventory (not hands) and gone from Jeff's hands on both clients |
| HD-002 | Request (Jay holds O, empty hands) and Give (Jeff) | | One `REQUESTED` and one `CONFIRMED` line with the same `id`; the item lands in Jay's inventory, not his hands |
| HD-003 | Pocket deposit | | Jay binds "Hands: open or close pockets" and opens pockets (`pockets ... open=true reason=toggle`); Jeff uses Put In Pockets; one `REQUESTED` and one `CONFIRMED` with the same `id` |

## Phase 1 exit conditions

| Step | Check | Result | Evidence |
| --- | --- | --- | --- |
| HD-010 | Self-test passes | | `build.log` for `@NIMBY-20261019-153354`: `SELFTEST Hands pass=104 fail=0` (52 cases; the count is doubled by the RPT and script log) |
| HD-011 | Concurrent requests cannot produce a second transfer | | Phase 3 row "Two takers accept the same offer": one `CONFIRMED`, one `REFUSED detail=item in flight (id=<first>)`; item exists once |
| HD-012 | Stale request refused: item replaced | | Jay starts Take, Jeff swaps the item in hands before the animation event: `REFUSED detail=item replaced since the action started` or `item is not in the giver's hands`; no transfer |
| HD-013 | Stale request refused: consent withdrawn | | Jeff stops offering before Jay's Take executes: `REFUSED detail=no consent: giver is no longer offering` |
| HD-014 | Stale request refused: disconnect | | Jay disconnects mid-action: `REFUSED detail=receiver disconnected` (or `receiver missing`); no item lost |
| HD-015 | Logs distinguish submission from confirmation | | Every `REQUESTED` line is followed by exactly one terminal line with the same `id`; no `id` has two terminal lines |
| HD-016 | Every line naming a player carries both ids | | Each lifecycle and pockets line shows `"<name>" (<BI id>, <Steam64>)`; the Steam64 matches the player's profile |
| HD-017 | Player-supplied text is escaped | | A test character name containing `"` shows as `\"` inside the quotes; the line still parses as one record |
| HD-018 | Debug lines only behind the flag | | Without `$profile:NimbyHands\debug`: no `[NIMBY][Hands][Debug]` lines; with it: the signal trace appears |
| HD-019 | Pockets close on unconsciousness | | Jay with pockets open is knocked unconscious: `pockets player=<Jay> open=false reason=unconscious`; Put In Pockets no longer offered |
| HD-020 | Pockets close on restraint; toggle blocked while restrained | | Jay restrained with pockets open: `pockets ... open=false reason=restrained`; the toggle does nothing while restrained |
| HD-021 | Put In Pockets refuses an unconscious or restrained target | | No prompt on the client; a forged start (dev harness, later) logs `REFUSED detail=receiver unconscious` or `receiver restrained` |
| HD-022 | Pockets close on reconnect and respawn | | Jay with pockets open relogs inside the logout timer: `reason=reconnect`; Jay with pockets open respawns: `reason=respawn`, new character closed |
| HD-023 | Rebind (shared rules 5) | | Options > Controls > Others shows "Hands: offer or request item" on O and "Hands: open or close pockets" unbound; rebinding offer/request to another key works and O stops working; the pocket notification names the bound key |
| HD-024 | Renamed inputs need one rebind | | A client that had changed the old offer/request binding sees the default O again, as the NOTICE warns |

## Phase 3 scenarios (from the plan; run after Phase 0 item 6 passes)

Jeff and Jay for ordinary transfers; a third tester or the dev harness for competing-taker cases. At least 20 repetitions of each normal mode and of the race cases.

| Step | Scenario | Required result | Result | Evidence |
| --- | --- | --- | --- | --- |
| HD-030 | Offer and accept; request and give; permitted pocket deposit | One item delivered, state preserved | | One `REQUESTED` and one `CONFIRMED` line with the same `id`; item inspected once in the receiver's inventory |
| HD-031 | Pocket access off or withdrawn during the action | No deposit | | `REFUSED` line; `pockets ... open=false` precedes it |
| HD-032 | Two takers accept the same offer | At most one transfer | | One `CONFIRMED`, one `REFUSED detail=item in flight`; item exists once |
| HD-033 | Simultaneous transfers compete for space | No duplication or loss | | Each `id` terminal exactly once; inventories match on both clients |
| HD-034 | Giver swaps, drops, or starts another action | Completes or refuses safely | | Terminal line for the `id`; no `UNCONFIRMED` |
| HD-035 | Receiver fills inventory or removes destination clothing | No ghost item | | `REFUSED detail=no space` or `CONFIRMED` with the item visible |
| HD-036 | Disconnect, death, unconsciousness, restraint, or separation | No transfer into the wrong player or state | | `REFUSED` with the state reason; `pockets ... reason=unconscious|restrained` where applicable |
| HD-037 | Recipient immediately moves or drops a received item | No false reversal | | The `id` stays `CONFIRMED`; no second terminal line |
| HD-038 | Rotated items, stacks, attachments, nested cargo, loaded containers | Quantity, condition, contents, ownership preserved | | Manual inspection of contents and quantities, recorded per `id` |
| HD-039 | Added latency and rapid repeats | Same guarantees | | Same evidence under 200 ms added latency |
| HD-040 | Relog and graceful restart | Clients and server agree on ownership | | Inventories inspected after relog and after RCON `#shutdown` restart |

## Solo smoke, October 19, 2026 (before the two-client session)

Jeff solo on the replica `D:\DayZServer`, instance 999, profile `hands-phase1`, build `@NIMBY-20261019-153354`, 16:23-16:27 Central. Server log `logs\testserver-20261019-162308\nimby-events.log` (44 events), client log `%LOCALAPPDATA%\DayZ\script_2026-10-19_16-24-30.log` (0 `SCRIPT (E)`, 0 `GiveTake`). Debug flag on the server and the client; no transfer attempted, so no `[Debug]` signal lines were expected.

| Step | Check | Result | Evidence |
| --- | --- | --- | --- |
| HD-010 | Self-test on the client as well as the build | PASS | Client: 52 `[NIMBY][Hands][SelfTest] PASS`, 0 FAIL, `selftest complete; failures=0` at 16:25:08 (the client line carries no case count; the build log does) |
| HD-016 (pockets lines) | Both ids on every line naming a player | PASS | `pockets player="Road Dog" (h8o5rWLPVptWLIPrMHl9JnGQMT0bM8CfAyoaOMMnH5Q=, 76561198354705597) open=true reason=toggle` at 16:26:28.246, `open=false` at 16:26:28.614, `open=true` at 16:26:46.482; the name is quoted as the escaping rule requires |
| HD-023 | Controls page and rebind | PASS (Jeff) | "Hands: offer or request item" on O and "Hands: open or close pockets" unbound under Others; pockets bound to a free key and used; notification named the key. No screenshot |
| HD-024 | Old binding gone | PASS (Jeff, no log line) | No Give&Take entry listed; client log has 0 `GiveTake` hits |
| Offer animation | Hold O with an item in hands | PASS (Jeff, no log line by design) | The offer animation played; no lifecycle line because no second player could take |
| HD-022 (partial) | Pockets state across restart | NOT RUN | Pockets were left open at 16:26:46 and the server was stopped; the respawn and reconnect cases need the two-client session or a second solo start |

## Findings

None yet.

## Open

- `PERMISSION.md` is a placeholder until Jeff supplies the Workshop text (release blocker, shared rules 9.3).

## Jay session, October 19, 2026 (solo in the event)

Build `@NIMBY-20261019-165705` (`main` `7a295a9`), profile `main-round1`, 17:37-18:16 Central. Jay watched and never connected: the `.ADM` lists one player (Road Dog), the server log holds two Hands lines (`loaded; server=true debug=false` at 17:37:57.450, listed twice), no `REQUESTED`, `pockets`, or `signal=` line. Client log `%LOCALAPPDATA%\DayZ\script_2026-10-19_17-38-28.log`. `profiles\main-round1\NimbyHands\debug` does not exist, so the P0-4 signal trace could not have printed even with a second player.

| Step | Check | Result | Evidence |
| --- | --- | --- | --- |
| HD-010 | Self-test on the release build's client | PASS | `[NIMBY][Hands] selftest complete; failures=0` at 17:39:05.769 and 17:42:33.187; 104 `[NIMBY][Hands][SelfTest] PASS` lines over the two logins (52 each), 0 FAIL |
| P0-4a to P0-4e | Signal ranking | NOT RUN | Needs a second player and the server debug flag |
| HD-001 to HD-003, HD-012 to HD-014, HD-019 to HD-022 | Two-client rows | NOT RUN | Still owed to the two-player session |
| HD-023 | Controls page | CORRECTED | Feedback from the session: the NIMBY bindings sit under UNSORTED, which matches the Ears record (row 4e, E11, October 17, 2026). The solo smoke row above and the comments in all three `Inputs.xml` say "Others"; that wording is wrong. See NIMBY-T-007 |

Findings:

- **NIMBY-T-007 (low, all three bound mods, documentation and packaging):** the `<sorting name="others">` block in `NimbyAutorun`, `NimbyEarplugs`, and `NimbyHands` `Data/Inputs.xml` is a sibling of `<inputs>` under `<modded_inputs>`. Vanilla nests `<sorting>` inside `<inputs>` (`vendor/dayz-1.30-scripts/bin/constants.xml:1`, `:812-937`), and Bohemia tracker T165909 reports the same symptom ("sorting block not recognized") resolved by moving `<sorting>` into the `<inputs>` sub-block. Inference, not yet proven here: the October 17 E11 result ("1.30 ignores `<sorting>` from `modded_inputs`") was the placement, not the engine. Fix candidate: move the block inside `<inputs>` in all three files, rebuild, open Options > Controls. Keep `others` rather than a new `nimby` group: the tracker's reporter notes the tab strip cuts off excess tabs. Reopens E11 if Jeff rules for it.

NIMBY-T-007 resolution, October 19, 2026 evening: proven on `@NIMBY-20261019-183849` (branch `feat/inputs-sorting`, three `Inputs.xml` edits, `<sorting name="nimby" loc="NIMBY">` inside `<inputs>`). Options > Controls shows a NIMBY tab between OTHERS and UNSORTED with all four NIMBY entries, nothing under UNSORTED, nothing clipped; 1.30 merged the three per-addon declarations into one tab. HD-023 reads "Options > Controls > NIMBY" from this build on. The remaining open check from this finding: whether the Controls page accepts a mouse button (`mB4`, `mB5`, wheel) as the binding of a NIMBY input; the PC mouse controller declares them (`bin/constants.xml:605-624`) and a preset can ship one as a default.

## Two-client session, October 20, 2026

Build `@NIMBY-20261019-215218` (`main` `bb59038`), replica `D:\DayZServer`, instance 999, profile `main-round1`, 15:44 to 17:47 Central. Jay is not testing (Jeff, October 20, 2026); Jeff ran both clients: **D** is the desktop on his main account (`Road Dog (2)`, BI id `h8o5r...`, Steam `76561198354705597`), **L** is the G15 laptop on a second account (`Road Dog`, BI id `tYlJF...`, Steam `76561198618429987`), joined over the LAN through a firewall rule scoped to the laptop's address. Logs: server `profiles\main-round1\DayZServer_x64_2026-10-20_15-44-56.RPT` and `.ADM` (run folder `logs\testserver-20261020-154456`, 1856 `[NIMBY]` events); D client `%LOCALAPPDATA%\DayZ\script_2026-10-20_15-59-50.log`; L client `logs\laptop-script-20261020.log` (the laptop's 17:12:52 launch only). Server debug flags on for Autorun, Hands, Notes, Tack; all five client flags on both clients. Run order `logs\runorder-two-player-20261020.md` (git-ignored). L dropped twice unplanned (`No packets`, 15:50:20 and 16:46:03).

| Row | Result | Evidence |
| --- | --- | --- |
| HD-001, P0-4a, P0-4b | FAIL (NIMBY-T-010) | D offering the Wrench #029750 (one press of O starts the giver loop; the rows' "holds O" means that), L uses Take. ids 1, 2, 3, 5, 6 (16:08:20 to 16:09:00), 7 (16:20:22), 14 to 17, 19 (16:27:42 to 16:28:21), 20, 22 (16:30:11 onward): each `state=REQUESTED ... detail=dst=CARGO:KameezShirt_Orange@tYlJF...`, then 5,000 ms later `state=UNCONFIRMED ... detail=no completion signal within 5000 ms; item at HANDS:SurvivorM_Oliver@h8o5r...`. No `signal=` debug line on the server or either client: the item never moved |
| HD-002 | FAIL (NIMBY-T-010) | L requesting, D uses Give with the Hammer #029744: ids 8 (16:22:17), 9 (16:22:30), 12 (16:22:54), each `REQUESTED` then `UNCONFIRMED`, item still in D's hands |
| HD-003 | FAIL (NIMBY-T-010) | `pockets player="Road Dog" (tYlJF...) open=true reason=toggle` 16:23:56.627; D's Put In Pockets with Paper #029739: id 13 `REQUESTED` 16:24:29.752, `UNCONFIRMED` 16:24:34.751 |
| In-flight guard (G5) | PASS | A second request on an item with an unresolved entry is refused: ids 4, 10, 11, 18, 21, each `REFUSED detail=item in flight (id=<n>)` (16:08:47.202, 16:22:32.28, 16:22:33.191, 16:28:13.921, 16:30:15.263); no second dispatch |
| P0-4c, P0-4d, P0-4e, HD-011 | NOT RUN | Need a completed transfer (T-010). P0-4d's refusal path was not attempted |
| HD-012 | NOT REACHABLE in normal play | Vanilla blocks quickbar swaps while D's offer loop runs, so the item in hands cannot change mid-request (Jeff, 16:27 to 16:28). The guard stays as defense; a forged case needs the dev harness |
| HD-013 | NOT PROVEN by hand | Take is an interact action and dispatches at its first animation event; D pressing O to stop offering on the other keyboard never landed inside that window (ids 20, 22 reached `REQUESTED`, no `no consent` refusal). Dev harness |
| HD-014 | NOT RUN | Same dispatch window as HD-013. Dev harness |
| HD-019 | PASS | L pockets open 16:53:21.804; D knocked L out (ADM: three hits, `is unconscious` 16:53:43); `pockets ... open=false reason=unconscious` 16:53:43.665 |
| HD-020 | PASS | L pockets open 16:57:05.620, autorunning pinned to a wall; D cuffed L; `pockets ... open=false reason=restrained` 16:57:35.797; the toggle did nothing while cuffed (Jeff) |
| HD-021 (client half) | PASS | No Put In Pockets prompt on D for the unconscious (C1) or the restrained (C2) L (Jeff). The forged start stays a dev-harness row |
| HD-022 | PASS (respawn); NOT RUN (reconnect) | Respawn: L pockets open 17:14:06.535, killed 17:14:36 (ADM), `pockets ... reason=respawn` 17:14:45.731, new character closed (Jeff). Reconnect: both quick relogs were kicked `Login timed out (WaitAuthPlayerLoginState)` (17:02:14, 17:03:50); a connect takes about 25 s and the replica's `logoutTime` is 15 s, so the character is gone before the client is back. Needs a longer logout timer in the test config |

Findings:

- **NIMBY-T-010 (high, Hands, release blocker):** no Hands verb completes a transfer between two players. Take, Give, and Put In Pockets all reach `REQUESTED` with a valid destination in the receiver's cargo and end `UNCONFIRMED` with the item still in the giver's hands; nothing is lost, duplicated, or retried, so the Phase 1 safety design held. No earlier log on disk has a `CONFIRMED` line; this is the first two-player run of Phase 1. The dispatch is `Man.ServerTakeToDst` on the giver (`nimbyhands_server.c:165`); in 1.30 the server's hand event for a live player is not executed on the server but sent to the giver's own client as a juncture (`DayZPlayerInventory.c`, `HandleHandEvent`, `InventoryMode.SERVER` branch: `SendServerHandEventViaJuncture`), after `HandleInventoryManipulation` gates it on `ScriptInputUserData.CanStoreInputUserData()` (`DayZPlayerInventory.c:4623-4634`). The giver's client logged nothing. Hypothesis, not proven: the giver's client does not execute the juncture, possibly because the giver's own running action (the offer loop, or the Give and Put In Pockets interact actions) holds the item. Next: feasibility study before any build decision.
- **NIMBY-T-012 (low, Hands, UX):** neither player sees anything on screen when a request ends `UNCONFIRMED` or `REFUSED` (Jeff, A8: "I do not see any error"). Fold into the T-010 study.
