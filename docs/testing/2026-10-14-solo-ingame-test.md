# Solo in-game test - NIMBY pack - October 14, 2026

Build: `build\@NIMBY-20261014-175505`. Server: local test server (`Start-NimbyTestServer.ps1`), log folder `logs\testserver-20261014-183219`. Tester: Jeff, one client.

## Results

| Area | Check | Result |
| --- | --- | --- |
| Autorun | Numpad + starts and stops | PASS |
| Autorun | W, S, A, D, fire, raise weapon interrupt | PASS |
| Autorun | Lean interrupts | FAIL - lean does not stop autorun (NIMBY-T-001) |
| Autorun | Stamina exhaustion drops sprint to run | PASS |
| Autorun | Vehicle entry, CR-008 recovery | NOT RUN - no way to spawn a vehicle (NIMBY-T-003) |
| Give n Take | Offer/request animation on O | PASS (animation only; no second player, so no transfer and no `[NIMBY][GiveTake]` lines) |
| Pocket toggle | Left Shift+O toggles pockets with a notification | FAIL - Shift+O runs offer/request, no notification (NIMBY-T-002) |
| Mod-mismatch refusal | Join without `@NIMBY` | NOT RUN |
| Logs | Script errors | PASS - no `SCRIPT (E)` lines, no `[NIMBY]` lines |

## Findings

- **NIMBY-T-002 (high):** `UANimbyOfferRequestItem` is bound to `kO` with no modifier exclusion, so Shift+O also fires it, and the offer/request action takes the input before `NimbyActionTogglePocketAccess` runs. The toggle action also writes no log line, so the server log cannot confirm whether it ran. Fix next session: resolve the binding conflict and add a `[NIMBY][GiveTake] action=TogglePockets` log line.
- **NIMBY-T-001 (low):** `UALeanLeft` and `UALeanRight` are in the interrupt list but do not stop autorun. Likely cause (unverified): the lean input does not register `LocalPress` while sprinting. Fix next session: verify the input names against the 1.29 script diff, then either fix the check or drop lean from the list and the test plan.
- **NIMBY-T-003 (blocker for CR-008 test):** the local test server has no way to spawn a vehicle. Fix next session: load COT on the local test server only, or add a test-only vehicle spawn.

## Still open

- CR-001 (Give n Take transfer) needs the two-client steps in the pack README, with Jay.
- CR-008 (autorun state after vehicle or unconscious) needs NIMBY-T-003 first.
