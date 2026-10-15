# Plan 1 - Legs (`NimbyAutorun`)

Draft v0.3, October 15, 2026. Owner: Jeff Fabish. Implementer: a Claude Code session in worktree `feat/autorun`, coordinated by the orchestrator. Status: ready for implementation once Plan 0 (shared tooling) is on `main`.

Target: NIMBY | HARDCORE | BADLANDS | 1pp, HostHavoc, DayZ 1.30.164078 / Nasdara.
Release: independent, gated (see [shared rules, 9.2](2026-10-15-shared-rules.md)).

Legs lets a player toggle continuous forward movement with one key, using vanilla movement and stamina rules. It must stop on its own whenever player state or input makes continued movement inappropriate, and a stopped state must be the same on the client and the server.

Changes since v0.2: shared rules moved to their own file; current-state and decisions tables restored; every test row names its evidence; lean reverts to a conditional requirement (A2); the server cancellation RPC is reserved; the debug-log mechanism and self-test are specified; display name set to Legs (A1).

## What this is not

- Not a movement cheat. The server validates every request; autorun never moves faster than the player could on foot and never runs in a vehicle, unconscious, restrained, or dead.
- Not vehicle cruise control.
- Not a change to movement speed or stamina balance.
- Not a rewrite. The Apache-2.0 fork, its attribution, and its server-side validation stay.

## Current state (verified October 15, 2026)

| Item | State | Source |
| --- | --- | --- |
| Source | `mods/own/nimby-pack/NimbyAutorun/`: `config.cpp`, `Data/Inputs.xml`, two scripts | Repo |
| Origin and license | Fork of SchanaModAutorun commit 1048f71, Apache-2.0; `LICENSE` and `NOTICE` present | `NimbyAutorun/NOTICE` |
| Server validation | Ownership, movement index and angle range, sprint permission; in-vehicle, unconscious, restrained, or dead requests are quietly downgraded to stop | `nimbyautorun_playerbase.c:40-88` |
| Server rate limiting | None on request processing; only rejection logging is rate-limited (10 s per player) | `nimbyautorun_playerbase.c:96-111` |
| Server-to-client cancellation | None; the server changes its own override and the client is not told | `nimbyautorun_playerbase.c:68-72` |
| Client interrupts | Toggle, WASD, fire, raise, lean, plus vehicle, unconscious, restrained, dead; `IsControlDisabled()` checked only on start, not as an interrupt | `nimbyautorun_missiongameplay.c:27-80` |
| Default key | numpad plus (`kAdd`) | `Data/Inputs.xml` |
| RPC id | `6550101` | `nimbyautorun_playerbase.c:15` |
| Save footprint | None; no `OnStoreSave`/`OnStoreLoad`, no net-synced variables | Grep |
| Player id in logs | `GetId()` (BI hash) only | `nimbyautorun_playerbase.c:108` |
| Last build | `build\@NIMBY-20261014-175505`, 1.29 Chernarus | Pack README, test doc |
| Solo test (1.29) | Toggle, WASD, fire, raise, stamina drop: PASS. Lean: FAIL (NIMBY-T-001). Vehicle and unconscious recovery: NOT RUN (NIMBY-T-003) | `docs/testing/2026-10-14-solo-ingame-test.md` |
| On 1.30 | Never built or booted | - |

## Decisions

| # | Decision | Choice | Status |
| --- | --- | --- | --- |
| A1 | Display name | Legs. Code prefix stays `NimbyAutorun`; Controls entry reads "Legs: toggle autorun" | Decided October 15, 2026 |
| A2 | Lean interrupt (T-001) | Lean interrupts if Phase 0 shows `UALeanLeft`/`UALeanRight` report a press while sprinting; otherwise lean is dropped from the interrupt list and documented | Decided, conditional |
| A3 | Default key | numpad plus; players rebind | Decided |
| A4 | Server cancellation RPC | `6550102`, server to owning client; on receipt the client clears its override and remembered state and does not re-send start | Decided (shared rules 4.1) |

## Phase 0 - baseline and test environment

Depends on Plan 0 (shared tooling) being on `main`.

1. Record the client and server build, source revision, and the hashes of the shared 1.30 script reference in `docs/baselines/`.
2. Verify every vanilla symbol the mod touches against the shared reference and record file:line: `MissionGameplay.OnInit`/`OnUpdate`, `IsControlDisabled`, `PlayerBase.OnRPC`, `GetUApi().GetInputByName`, `UAInput.LocalPress`, the `UA*` input names in the interrupt list, `HumanInputController.OverrideMovementSpeed`/`OverrideMovementAngle`, `CanSprint`, `GetStaminaHandler`, `IsInVehicle`, `IsUnconscious`, `IsRestrained`, and the vehicle-entry, unconsciousness, restraint, death, and player-removal hooks the server will use. Exercise each in game; a declaration is not proof.
3. Settle A2: on the test server, hold lean while sprinting and record whether `LocalPress` fires. Record the result in this plan.
4. Build and boot on 1.30 Nasdara with the shared tooling; confirm the quick boot check and the qualification run are separate invocations.
5. Establish the same-build vanilla control. Investigate existing copied-world save-load errors separately; do not exempt them automatically.

Exit: the signed addon boots on 1.30 Nasdara with 0 `SCRIPT (E)`; every touched API is recorded as VERIFIED (file:line) or CHANGED with its fix; A2 is settled.

## Phase 1 - movement lifecycle and request handling

### Client

- Preserve the toggle, movement, fire, weapon-raise, and lean (per A2) interrupts.
- Treat `IsControlDisabled()`, inventory, pause, any input-capturing menu, and focus loss as interrupts, not only as start blockers. Require a fresh toggle afterward.
- Clear local overrides and remembered state on player replacement, respawn, disconnect, and mission teardown.
- Handle RPC `6550102` per A4.
- Preserve the change-only RPC behavior; do not restore per-frame requests.

### Server

- Continue deriving ownership from the authenticated RPC sender.
- Validate payload readability, movement values, and allowed player state.
- Re-evaluate sprint eligibility while autorun remains active.
- Clear movement overrides on vehicle entry, unconsciousness, restraint, death, and player removal without depending on a client stop, and send `6550102` to the owning client on every forced clear or rejection that changes state.
- Add bounded request processing per sender (default 10 requests per second, in `$profile:NimbyAutorun\config.json`). Keep rejection-log throttling separate from request throttling. An authenticated stop request bypasses the request throttle.
- Move argument validation and the throttle into engine-free classes and cover them with the self-test (shared rules 8).

### Logging

| Event | Line | Level |
| --- | --- | --- |
| Forced clear | `[NIMBY][Autorun] Forced clear player=<name> (<bi-id>, <steam64>) reason=<vehicle|unconscious|restrained|dead|removed>` | Always |
| Rejected request | `[NIMBY][Autorun] Rejected request from <bi-id>, <steam64>: <reason>` (rate-limited) | Always |
| Request throttled | `[NIMBY][Autorun] Throttled player=... count=<n>` (rate-limited) | Always |
| Toggle on or off | `[NIMBY][Autorun] Toggle player=... running=<bool> idx=<n>` | Debug flag only |
| Self-test | `[NIMBY][Autorun][SelfTest] PASS|FAIL <case>` | Self-test flag only |

Exit: client and server reach the same stopped state after every interruption, with no automatic resumption, and the self-test passes.

## Phase 2 - multiplayer and failure testing

Jeff and Jay on the local test server. Vehicles come from the test mission's `init.c`, never the live mission; do not depend on Golden God.

| Scenario | Required result | Evidence |
| --- | --- | --- |
| Toggle on and off; rebind the toggle | Predictable start and stop | Debug toggle lines show matching `running` values |
| Movement, fire, raise, lean (per A2) | Stops | Debug toggle line `running=false`; no further movement |
| Stamina exhaustion or sprint unavailable | Vanilla-legal movement; no sustained prohibited sprint | Debug toggle line `idx=` changes to RUN; no sprint animation at zero stamina |
| Inventory, pause, input-capturing menu, focus loss | Stops; does not resume on return | `running=false` line at open; no `running=true` line without a key press |
| Car and motorbike entry and exit | Stops and stays stopped | `Forced clear ... reason=vehicle` on the server; player stands still after exit |
| Unconsciousness, restraint, death, respawn | Overrides cleared; fresh toggle required | `Forced clear` with the matching reason |
| Ladder and swimming transitions | Autorun clears; vanilla controls usable | `running=false`; player responds to WASD |
| Disconnect and reconnect | No retained override | No `running=true` line after reconnect until a key press |
| Server rejects or forcibly clears | Client reconciles to stopped | Server `Forced clear` or `Rejected` line, then no client re-send within 5 s |
| Latency, rapid toggling, malformed requests | No stuck movement; bounded processing | `Throttled` lines appear; player stops on toggle |
| Request targeting another player (dev harness) | Refused; target unaffected | `Rejected request ... sender does not own this player`; target's state unchanged |

Forged requests run only from the dev harness (shared rules 8). Its addon and key never enter a release.

## Phase 3 - release qualification and rollout

Follow shared rules 9. In addition:

- Apply A1: `mod.cpp` and Controls text read Legs; `NOTICE` lists every change since October 14, newest first.
- Update `mods/own/nimby-pack/README.md`: status "Built and tested in game on 1.30 Nasdara".
- Record the run against the exact artifact in `docs/testing/`.

## Acceptance criteria

- Builds, signs, boot-checks, and self-tests on 1.30 Nasdara with 0 `SCRIPT (E)`.
- Every Phase 2 row passes with the named evidence, witnessed by Jeff.
- No save footprint, re-checked by grep at release.
- The `storage_1`-copy boot in shared rules 9.2 shows 0 "Scripted variables corrupted".
- Any unintended movement after cancellation blocks release.

## Estimate

Re-estimate after Phase 0. The earlier 2.5-day figure excluded shared-tool repairs and multiplayer failure testing and is not a commitment.

## Risks

| Risk | Effect | Mitigation |
| --- | --- | --- |
| 1.30 input or animation changes break the movement override | Autorun does nothing or moves oddly | Phase 0 verification before other work |
| Lean not observable while sprinting | T-001 unfixable as designed | A2 drops lean and documents it |
| Vehicle test blocked | CR-008 unproven | Test-only spawn in the local mission `init.c` |
| Server and client disagree after a forced clear | Player stuck moving or unable to restart | A4 cancellation RPC; Phase 2 reconciliation row |
