# Revised plan 2 — Gimme

**Draft v0.2 — October 15, 2026**  
**Target:** NIMBY | HARDCORE | BADLANDS | 1pp, Host Havoc, DayZ 1.30.164078 / Nasdara  
**Release:** Independent. Neither Autorun nor NimbyAdmin is a prerequisite.

## Purpose and proposed defaults

Allow consensual item transfers from one player’s hands into another player’s inventory.

- Rename the addon and its identifiers to `NimbyGimme`; display name: **Gimme**.
- Preserve Give&Take attribution and permission evidence.
- Deliver to cargo or eligible attachment slots, never directly into hands.
- Pocket access defaults to off and resets on reconnect/respawn.
- Opening pockets permits deposits only.
- Retain `O` for offer/request.
- Give pocket access its own rebindable action, **unbound by default**. Explain binding it in the player instructions. This removes the current Shift+O collision without choosing another potentially conflicting default.
- Add no custom entity serialization or persistent entity classes.

These are proposed defaults. Independent release is confirmed by Jeff.

## Phase 0 — Prove the transfer path before renaming

1. Record the exact client/server build, source revision, and vanilla script archive hashes.
2. Verify the action framework, inventory-location APIs, `ServerTakeToDst`, hand-event processing, and destination checks against stable 1.30.
3. Trace where vanilla validates consent, distance, source ownership, destination availability, and action continuation.
4. Determine how the actual move reports completion or failure. A successful request return is not proof of completed transfer.
5. Run basic offer, request, and pocket-deposit cases with Jeff and Jay before investing in the rename.
6. Use a pinned build, Nasdara mission, reusable test profile, and graceful shutdown. Require the shared test-tool prerequisites described in this plan; another addon need not be ready.

**Exit:** The native transfer path works between two real clients, and its validation and completion behavior are documented.

Stop and revise the approach if reliable transfer behavior cannot be demonstrated. Do not compensate by deleting/recreating items or injecting synthetic hand-state changes.

## Phase 1 — Make transfers safe to validate and diagnose

### Validation and concurrency

- Associate every request with its original giver, receiver, item, and action.
- Revalidate the applicable consent, player state, proximity, source item, and destination immediately before dispatch.
- Reject stale requests after item replacement, consent withdrawal, disconnect, or player replacement.
- Use the engine’s supported inventory coordination mechanisms, verified during Phase 0.
- Prevent overlapping mod requests from operating on the same item while its outcome remains unresolved.
- Reconcile actual inventory state before clearing an unresolved operation; a timeout must not itself authorize a retry.

### Transfer outcomes

Replace the fixed one-second “current holder means success/failure” conclusion with correlated lifecycle records:

- `REQUESTED`: submitted to the engine.
- `CONFIRMED`: supported completion evidence or a server observation of the expected delivery.
- `REFUSED`: validation or engine refusal before a completed transfer.
- `UNCONFIRMED`: insufficient evidence within the bounded observation period.

Prefer verified engine completion/failure signals. If those are unavailable, use bounded observation and report uncertainty honestly. A recipient moving the item afterward must not turn a confirmed success into a failure.

Never automatically retry, clone, restore, or delete an item to compensate for an uncertain result.

### Logging and feedback

- Give each transfer a correlation identifier.
- Record participant identities, item identity/type, action, outcome, and reason.
- Escape player-supplied text.
- Keep local logs bounded and avoid duplicate terminal reports.
- Show useful refusal feedback without claiming that a merely submitted transfer has completed.

**Exit:** Concurrent or stale requests cannot produce an unsupported second transfer, and logs distinguish submission from confirmation.

## Phase 2 — Rename, controls, and permission evidence

- Rename folder, PBO, config classes, custom classes/members, input actions, string keys, and log prefix consistently.
- Update the pack manifest and resource paths.
- Explicitly remove the obsolete GiveTake PBO from assembled releases; do not overlay the renamed addon onto an old installation.
- Document that renamed input actions may require users to rebind controls.
- Ensure pocket toggles produce one server event and clear on-screen state feedback.
- Store a dated capture of the author’s redistribution permission beside `NOTICE`.
- Preserve original attribution and the fork’s change history.

**Exit:** The packaged renamed addon loads, its controls work, and neither old PBOs nor stale resource references remain.

## Phase 3 — Required integrity and adversarial tests

Use Jeff and Jay for ordinary transfers. Obtain a third tester or a controlled harness for competing-taker cases.

| Scenario | Required result |
|---|---|
| Offer, accept; request, give; permitted pocket deposit | One item delivered, original state preserved |
| Pocket access off or withdrawn during action | No unauthorized deposit |
| Two takers accept the same offer | At most one successful transfer |
| Simultaneous transfers compete for inventory space | Valid outcomes without duplication or loss |
| Giver swaps/drops item or begins another action | Original request safely completes or is refused |
| Receiver fills inventory or removes destination clothing | No ghost item or invalid placement |
| Disconnect, death, unconsciousness, or separation | No stale transfer into the wrong player/state |
| Recipient immediately moves or drops a received item | No false reversal of a confirmed result |
| Rotated items, stacks, attachments, nested cargo, loaded containers | Quantity, condition, contents, and ownership preserved |
| Added latency and rapid repeated attempts | Same integrity guarantees |
| Relog and graceful restart | Both clients and server agree on ownership |

Run at least 20 repetitions of each normal transfer mode and repeated race/failure cases. Latency testing is mandatory. Record outcomes against the exact artifact.

For every completed transfer, verify that the item exists once and remains usable. Inspect contents and quantities, not merely its icon.

## Phase 4 — Independent release and rollback

- Pass build/signature checks and all Phase 3 cases.
- Test installation, update, and removal using disposable storage, including saved characters.
- Verify a clean Workshop install and an update from the prior package.
- Confirm client/server version compatibility and absence of the obsolete PBO.
- Publish the approved @NIMBY version and deploy at an approved scheduled restart.
- Retain the previous package and pre-release backup.

Any unexplained item loss, duplication, ghost item, stuck hand state, or consent violation blocks release. Disabling the addon stops further transfers but does not undo completed transfers; restoring a backup is a separate operator decision.

## Acceptance and estimate

Release requires the full integrity matrix, Jeff’s approval, usable controls, permission evidence, and a tested deployment path.

Estimate after Phase 0. The previous 3–3.5-day estimate does not cover transfer-outcome redesign, three-player contention, and restart qualification.
