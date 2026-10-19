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
