# Plan 2 - Hands (`NimbyHands`, formerly `NimbyGiveTake`)

Draft v0.4, October 16, 2026. Owner: Jeff Fabish. Implementer: a Claude Code session in worktree `feat/hands`, coordinated by the orchestrator. Status: ready for implementation once Plan 0 (shared tooling) is on `main`.

Target: NIMBY | HARDCORE | BADLANDS | 1pp, HostHavoc, DayZ 1.30.164078 / Nasdara.
Release: independent, gated (see [shared rules, 9.2](2026-10-15-shared-rules.md)).

Hands lets one player offer the item in their hands to another, request the item in another player's hands, and open their pockets so others can deposit items. Transfers land in the receiver's inventory (cargo or an eligible attachment slot), never their hands. It has never moved an item between two real players; proving that comes before the rename.

Changes since v0.3: display name Hands (Jeff, October 16, 2026; was Gimme), so the rename target is `NimbyHands`, worktree `feat/hands`, inputs `UANimbyHands*`, string keys `STR_NIMBY_HANDS_*`, log prefix `[NIMBY][Hands]`; the pocket-toggle Controls label is "Hands: open or close pockets"; key bindings are player-configurable per shared rules 5.

Changes since v0.2: shared rules moved to their own file; current-state and decisions tables restored; the in-flight registry and completion-signal candidates are specified; pockets close on unconsciousness and restraint (G4); the pocket toggle is unbound with a discoverable label (G2); every test row names its evidence; dead `#ifdef DIAG` code is scheduled for removal; the stale-PBO check moves to deployment.

## What this is not

- Not a trading system. No currency, prices, or trader.
- Not a way to take items without consent. Request and pocket access both need the other player's action or opt-in.
- Not a hands-to-hands transfer.
- Not a persistence change. No custom entity serialization and no persistent entity classes.

## Current state (verified October 15, 2026)

| Item | State | Source |
| --- | --- | --- |
| Source | `mods/own/nimby-pack/NimbyGiveTake/`: six actions, inventory move, player, GUI, inputs, string table | Repo |
| Origin and permission | Fork of Give&Take (Workshop 2903112334, author affenb3rt). No license file; the Workshop description grants repack, modify, and redistribute. Capture not yet stored (CR-007) | `NimbyGiveTake/NOTICE` |
| Transfer routine | `ServerTakeToDst` through the holder's hand state machine; logs `REQUESTED`, then `OK` or `FAILED` from a fixed 1,000 ms `CallLater` on the receiver | `moveitemtoplayer.c:68-130`, `playerbase.c:59-80` |
| Concurrency | None. No in-flight registry; two takers on one offer both reach `Execute()` | `moveitemtoplayer.c` |
| Logging | One `[NIMBY][GiveTake]` line per outcome; no correlation id; player id is `GetId()` (BI hash) | `nimbygivetakelog.c:13-40` |
| Keys | Offer/request `O`; pocket toggle Left Shift + `O`, which also fires offer/request (NIMBY-T-002) | `inputs.xml`, test doc |
| Pocket toggle while restrained | Allowed (`CanBeUsedInRestrain` returns true) | `actiontogglepocketaccess.c:14` |
| Deposit into unconscious or restrained player | Not refused; only swimming and in-vehicle are checked | `actionputinpockets.c:33-57` |
| Dead debug code | `#ifdef DIAG` blocks set the target's give/take state and never clear it | `actiongiveiteminhandstoplayer.c:52-55`, `actiontakeitemfromplayer.c:56-59` |
| Save footprint | None persisted. Two net-synced variables (`m_NimbyGiveOrTakeState`, `m_NimbyAllowsPocketAccess`) | `playerbase.c:3-11` |
| RPC ids | None; the action framework and net-sync carry everything. Block `65502xx` reserved | Shared rules 4.1 |
| Solo test (1.29) | Offer/request animation: PASS. Shift+O toggle: FAIL (T-002) | `docs/testing/2026-10-14-solo-ingame-test.md` |
| Two-client transfer | Never run | - |
| On 1.30 | Never built or booted. 1.30 changed `CreateEntityInCargo`/`CreateInInventory` to check the flip condition through `CanAddEntityInCargo` | Research doc |

## Decisions

| # | Decision | Choice | Status |
| --- | --- | --- | --- |
| G1 | Rename depth | Full rename to `NimbyHands`: folder, PBO, config classes, scripts, input actions, string keys, log prefix. Attribution stays first in `NOTICE` | Decided |
| G2 | Pocket toggle key (T-002) | Unbound by default. Controls label "Hands: open or close pockets"; the pocket notification names the binding. Phase 0 still records whether `Shift+O` can exclude a plain `O` press; if it can, Jeff may revisit | Decided October 15, 2026 |
| G3 | Pocket access default | Off; resets on reconnect and respawn; opening permits deposits only | Decided |
| G4 | Pockets auto-close | Close on unconsciousness and on restraint. "Put in pockets" refuses an unconscious or restrained target. The toggle cannot be used while restrained | Decided October 15, 2026 |
| G5 | In-flight transfer registry | Server-side, mission-lifetime singleton `NimbyHandsTransfers`: `map<EntityAI, ref NimbyHandsTransfer>` keyed by item, evicted on terminal state or after a bounded timeout (default 5,000 ms) once inventory state is reconciled. Nothing persisted | Decided (Claude default) |

## Phase 0 - prove the transfer path before renaming

1. Record the client and server build, source revision, and the shared 1.30 reference hashes in `docs/baselines/`.
2. Verify against the shared reference and record file:line: the action base classes and `ActionConstructor`, `ContinuousDefaultActionInput`/`StandardActionInput`, `FindFreeLocationFor`, `CanAddEntityInCargo` (flip-condition change), `ServerTakeToDst`, `HumanInventory.RedirectToHandEvent`, `InventoryLocation`, `ActionTargetsCursor.GetItemDesc`/`GetItemHealth`/`GetItemCargoCount`, `NotificationSystem.AddNotificationExtended`, and the input API.
3. Trace where vanilla validates consent, distance, source ownership, destination availability, and action continuation, and record it.
4. Find the completion signal. Candidates to test, in order: `ItemBase.EEItemLocationChanged`, `OnInventoryEnter`/`OnInventoryExit` on the receiver, and the hand-event result on the holder. Record which fire, on which side, and in what order, for a successful move and for a refused one. A successful `ServerTakeToDst` return is not proof of completion.
5. Settle the G2 question: does the input system let `UANimbyOfferRequestItem` ignore a press while Left Shift is held? Record yes or no with file:line.
6. Run offer, request, and pocket deposit between Jeff and Jay on the local test server with the shared tooling.

Exit: the native transfer path moves an item between two real clients; its validation points and completion signal are documented. If reliable transfer cannot be shown, stop and revise. Never compensate by deleting and recreating items or injecting synthetic hand-state changes.

## Phase 1 - make transfers safe to validate and diagnose

### Validation and concurrency

- Register every request in `NimbyHandsTransfers` (G5) with giver, receiver, item, action, and a correlation id before dispatch; refuse a request whose item already has an unresolved entry.
- Revalidate consent, player state (alive, conscious, not restrained per G4, not in a vehicle, not swimming), proximity, source item, and destination immediately before dispatch.
- Reject stale requests after item replacement, consent withdrawal, disconnect, or player replacement.
- Reconcile actual inventory state before evicting an unresolved entry; a timeout must not itself authorize a retry.
- Move the state machine and validators into engine-free classes covered by the self-test (shared rules 8).

### Transfer outcomes

Replace the fixed one-second holder check with correlated lifecycle records:

| State | Meaning |
| --- | --- |
| `REQUESTED` | Submitted to the engine |
| `CONFIRMED` | Completion signal observed (Phase 0 item 4), or a server observation of the expected delivery |
| `REFUSED` | Validation or engine refusal before a transfer |
| `UNCONFIRMED` | No evidence within the bounded observation period |

A recipient moving the item afterward must not turn `CONFIRMED` into a failure. Never automatically retry, clone, restore, or delete an item to compensate for an uncertain result.

### Pockets (G4)

- `NimbyActionTogglePocketAccess.CanBeUsedInRestrain()` returns false.
- The server closes pockets when the owner becomes unconscious or restrained, and on reconnect and respawn.
- `NimbyActionPutInPockets.ActionCondition` refuses an unconscious or restrained target.

### Logging

| Event | Line |
| --- | --- |
| Lifecycle | `[NIMBY][Hands] id=<corr> action=<class> state=<REQUESTED|CONFIRMED|REFUSED|UNCONFIRMED> giver=<name> (<bi-id>, <steam64>) receiver=<name> (<bi-id>, <steam64>) item=<type> #<netid> detail=<text>` |
| Pocket toggle | `[NIMBY][Hands] pockets player=<name> (<bi-id>, <steam64>) open=<bool> reason=<toggle|unconscious|restrained|respawn|reconnect>` |
| Self-test | `[NIMBY][Hands][SelfTest] PASS|FAIL <case>` |

Escape player-supplied text. Keep logs bounded; one terminal line per request.

### Cleanup

Delete the `#ifdef DIAG` blocks in `actiongiveiteminhandstoplayer.c` and `actiontakeitemfromplayer.c`.

Exit: concurrent or stale requests cannot produce a second transfer; logs distinguish submission from confirmation; the self-test passes.

## Phase 2 - rename, controls, and permission evidence

- Rename folder, PBO, config classes, custom classes and members, input actions (`UANimbyHands*`), string keys (`STR_NIMBY_HANDS_*`), and the log prefix. Ask the orchestrator for the `pack.json` row change.
- Apply G2: the toggle ships unbound; Controls label and notification text per the decision.
- Document that renamed input actions require players to rebind `O` once.
- Store a dated capture of the Workshop permission text beside `NOTICE` (Jeff provides or approves it). Public Workshop distribution makes this a release blocker.
- Preserve attribution and the fork's change history in `NOTICE`.

Exit: the renamed addon builds and loads; a grep for `GiveTake` outside `NOTICE` and history docs returns nothing.

## Phase 3 - integrity and adversarial tests

Jeff and Jay for ordinary transfers; a third tester or the dev harness for competing-taker cases.

| Scenario | Required result | Evidence |
| --- | --- | --- |
| Offer and accept; request and give; permitted pocket deposit | One item delivered, state preserved | One `REQUESTED` and one `CONFIRMED` line with the same `id`; item inspected once in the receiver's inventory |
| Pocket access off or withdrawn during the action | No deposit | `REFUSED` line; `pockets ... open=false` precedes it |
| Two takers accept the same offer | At most one transfer | One `CONFIRMED`, one `REFUSED detail=item in flight`; item exists once |
| Simultaneous transfers compete for space | No duplication or loss | Each `id` terminal exactly once; inventories match on both clients |
| Giver swaps, drops, or starts another action | Completes or refuses safely | Terminal line for the `id`; no `UNCONFIRMED` |
| Receiver fills inventory or removes destination clothing | No ghost item | `REFUSED detail=no space` or `CONFIRMED` with the item visible |
| Disconnect, death, unconsciousness, restraint, or separation | No transfer into the wrong player or state | `REFUSED` with the state reason; `pockets ... reason=unconscious|restrained` where applicable |
| Recipient immediately moves or drops a received item | No false reversal | The `id` stays `CONFIRMED`; no second terminal line |
| Rotated items, stacks, attachments, nested cargo, loaded containers | Quantity, condition, contents, ownership preserved | Manual inspection of contents and quantities, recorded per `id` |
| Added latency and rapid repeats | Same guarantees | Same evidence under 200 ms added latency |
| Relog and graceful restart | Clients and server agree on ownership | Inventories inspected after relog and after RCON `#shutdown` restart |

Run at least 20 repetitions of each normal mode and repeated race cases. Record outcomes against the exact artifact in `docs/testing/`.

## Phase 4 - release and rollback

Follow shared rules 9. In addition:

- Confirm that `@NIMBY/addons` on the host contains no `NimbyGiveTake.pbo` after upload (the build never produces one; a stale file on the host is the risk).
- Any unexplained item loss, duplication, ghost item, stuck hand state, or consent violation blocks release.
- Disabling the addon stops further transfers but does not undo completed ones; restoring a backup is a separate operator decision.

## Acceptance criteria

- Builds, signs, boot-checks, and self-tests on 1.30 Nasdara with 0 `SCRIPT (E)`.
- The full Phase 3 matrix passes with the named evidence, witnessed by Jeff.
- Usable controls per G2; permission capture stored.
- No save footprint, re-checked by grep; the `storage_1`-copy boot shows 0 "Scripted variables corrupted".

## Estimate

Re-estimate after Phase 0. The earlier 3-3.5-day figure did not cover the outcome redesign, three-player contention, or restart qualification.

## Risks

| Risk | Effect | Mitigation |
| --- | --- | --- |
| 1.30 flip-condition check refuses valid destinations | Transfers fail into roomy inventories | Phase 0 item 2; log the refusal reason |
| No usable completion signal | Outcomes stay `UNCONFIRMED` | Phase 0 item 4 tests three candidates; bounded observation as the fallback, reported honestly |
| Registry entry leaks | Item blocked from transfer | G5 timeout plus reconciliation; self-test covers eviction |
| Author changes permission terms | Redistribution question on a public Workshop item | Dated capture; re-check before each release |
