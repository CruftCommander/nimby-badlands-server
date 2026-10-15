# Revised plan 1 — NimbyAutorun

**Draft v0.2 — October 15, 2026**  
**Target:** NIMBY | HARDCORE | BADLANDS | 1pp, Host Havoc, DayZ 1.30.164078 / Nasdara  
**Release:** Independent. Neither Gimme nor NimbyAdmin is a prerequisite.

## Purpose and proposed defaults

Provide toggleable forward movement using vanilla movement capabilities and stamina rules. Autorun must stop safely when player state or input makes continued movement inappropriate.

- Retain `NimbyAutorun` as the addon, class prefix, and display name.
- Retain numpad plus as the default toggle; support rebinding.
- Keep lean as an interrupt requirement.
- Stop autorun when inventory, pause, or another input-capturing menu opens, or the game loses focus. Require a fresh toggle afterward.
- Exclude vehicle cruise control and changes to movement speed or stamina balance.
- Add no custom entity serialization or persistent entity classes.

These are proposed defaults. Independent release is confirmed by Jeff.

## Phase 0 — Establish the exact baseline and test environment

1. Record the client/server versions, source revision, and hashes of the vanilla script archives used for verification.
2. Verify relevant movement, input, lifecycle, and RPC APIs against the installed stable 1.30 scripts. Record source locations; exercise native behavior in game rather than treating a declaration as proof.
3. Configure **both** the build check and interactive test server for `dayzOffline.nasdara`.
4. Update the shared test tooling to:
   - Select an explicit successfully built artifact.
   - Record its addon manifest and hashes.
   - Reuse a designated test profile when requested.
   - Support graceful shutdown and restart.
   - Keep all test storage separate from live storage.
5. Separate the quick compile/boot check from release qualification. Readiness alone must not produce a persistence or gameplay verdict.
6. Establish a clean vanilla control on the same build. Investigate existing copied-world save-load errors separately; do not automatically exempt them.

**Exit:** The signed addon boots on the correct build and map, and the test setup can perform reproducible gameplay and restart tests.

## Phase 1 — Correct movement lifecycle and request handling

### Client behavior

- Preserve toggle, movement, fire, weapon-raise, and lean interrupts.
- Detect lean input even when the current animation prevents the lean itself.
- Clear local overrides when controls become unavailable, focus is lost, or the player enters a prohibited state.
- Clear remembered state on player replacement, respawn, disconnect, and mission teardown.
- Preserve the existing change-only RPC behavior; do not restore per-frame guaranteed requests.

### Server behavior

- Continue deriving ownership from the authenticated RPC sender.
- Validate payload readability, movement values, and allowed player state.
- Re-evaluate sprint eligibility while autorun remains active.
- Clear movement overrides on vehicle entry, unconsciousness, restraint, death, and player removal without depending on a client stop.
- Synchronize authoritative cancellation back to the owning client so rejected or cancelled movement does not leave divergent state.
- Add bounded request processing per sender. Keep rejection-log throttling separate from request throttling.
- Ensure an authenticated stop request can clear movement even when start/change requests are being throttled.

Log forced clears and rate-limited refusals. Normal toggle logging remains debug-only and disabled by default.

**Exit:** Client and server consistently reach stopped state after every interruption, with no automatic resumption.

## Phase 2 — Multiplayer and failure testing

Use Jeff and Jay on an isolated server. Supply vehicles through the test mission; do not depend on NimbyAdmin.

| Scenario | Required result |
|---|---|
| Toggle on/off; rebind toggle | Predictable start/stop |
| Movement, fire, raise, lean | Stops |
| Stamina exhaustion or sprint becomes unavailable | Vanilla-legal movement; no sustained prohibited sprint |
| Inventory, pause, input-capturing menu, focus loss | Stops; does not resume on return |
| Car/motorbike entry and exit | Stops and remains stopped |
| Unconsciousness, restraint, death, respawn | Overrides cleared; fresh toggle required |
| Ladder/swimming transitions | Autorun clears and vanilla controls remain usable |
| Disconnect/reconnect | No retained override |
| Server rejects or forcibly clears a request | Client reconciles to stopped state |
| Latency, rapid toggling, malformed requests | No stuck movement or unbounded processing |
| Request targeting another player | Refused; target unaffected |

Use an isolated development harness to exercise forged requests against the production handler. Its addon and signing trust must not enter the live release.

## Phase 3 — Release qualification and independent rollout

- Pass the build, signing, package-content, and signature checks.
- Run the multiplayer cases against the exact release artifact.
- Complete a graceful save/restart cycle and an addon-removal test on disposable storage.
- Require zero unexplained script errors, no new persistence failures, and no stuck movement.
- Update attribution, `NOTICE`, controls instructions, and the test record.
- Publish the approved @NIMBY Workshop version containing only release-qualified addons.
- Verify installation and joining from a normal client, including stale-client rejection or a clear update requirement.
- Deploy at an approved scheduled restart.

Retain the previous package and a verified pre-release backup. Document how clients obtain the matching rollback version.

## Acceptance and estimate

Release requires every Phase 2 case to pass, Jeff’s sign-off, and an exercised deployment/rollback procedure. Any unintended movement after cancellation blocks release.

Estimate after Phase 0. The earlier 2.5-day estimate excludes shared-tool repairs and multiplayer failure testing and should not be treated as a commitment.
