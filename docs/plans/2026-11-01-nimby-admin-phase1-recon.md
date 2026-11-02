# Plan 3 Phase 1 recon - Central Scrutinizer carried items, then the Phase 1 commands

Recon for [Plan 3](2026-10-15-nimby-admin-plan.md) v0.10, Phase 1. Seat: Claude Code in worktree `D:\GitHub\nimby-badlands-server-wt\nimby-admin`, branch `feat/nimby-admin`, from head `648f783` (November 1, 2026). Brief: `D:\GitHub\nimby-badlands-server-wt\brief-nimby-admin-phase1-2026-11-01.md`. Section 0 is Stop A (the items carried out of Phase 0); the Phase 1 recon proper (Stop B) follows after the A3 answer. Vanilla citations are `file:line` inside `vendor/dayz-1.30-scripts/scripts/` (build 164200).

## 0. Carried items

### 0.1. A1 - harness session mirror

Cause (Phase 0 test record, duplicate row): the server keys a session on the `PlayerIdentity` object (`SessionFor`, `nimbyadminserver_core.c:211-231` at `648f783`) and opens a new session, with a cleared high water, when a request's sender object differs from the stored one. The harness mirrored session numbers in its connect hook and forged with `PlayerBase.GetIdentity()`. The `-163708` server script log (`profiles\nimby-admin-p0\script_2026-11-01_16-55-25.log`) shows the effect on both players: L's real hello at 17:15:55 was `session=2`, the step 1 forges at 17:16:04 opened session 3 for L (seen at the burst hello, `role sync ... session=3 reason=hello` 17:16:13), D's step 2 forges opened session 4, and D's own real teleport at 17:18:54 opened session 5 (`session ended number=5` for D at 17:36:27). So on 164200 `PlayerBase.GetIdentity()` returned one object per player across steps 1 to 4 that was not the object of that player's real requests. A3 measures this directly.

Fix (this commit set):

- `nimbydevharness_missionserver.c`: `InvokeOnConnect` passes its `identity` argument to `NimbyDevHarness.OnPlayerConnected(player, identity)`, which passes it to `NimbyDevHarnessAdmin.OnPlayerConnected(player, identity)`.
- `NimbyDevHarnessSessionMirror` (engine-free, `nimbydevharness_admin.c`): one number per identity object per BI id in connect order, by the server's rule (a new number when the object differs from the stored one), and the stored object per BI id, held without `ref` so a destroyed identity reads back as null, as on the server.
- `FindPlayers` finds the two bodies as before, then takes each one's stored `InvokeOnConnect` identity (`ConnectIdentityOf`); a player not seen at `InvokeOnConnect` is a `SKIP`. That object is the sender of every forge, numbers the teleport `session=` argument (`TeleportArgs(PlayerIdentity)`), keys the id base (`IdFor`), and names the actor Steam64 the audit lines are matched on (`AuditFor`, `AuditIn`). `PlayerBase.GetIdentity()` is used only to find a body's BI id, a string.
- Every argument of every call in the admin forge path is a local computed before the call (the 164200 argument-slot rule, `23b4e46`), the `OnRPC` call itself included (`ctx.GetReadContext()` was an argument beside `sender`).
- A run line before step 1: `[NIMBY][DevHarness][Forge] admin run: mirrored sessions owner=<n> other=<n>; senders are the InvokeOnConnect identities`.

Self-test (DevHarness, boot): `admin-mirror-forges-with-the-connect-identity`. Two mirrors stand for the server's `SessionFor` and the harness, with plain script objects as identities, because `PlayerIdentityBase` has a private constructor (`3_Game/DayZ/gameplay.c:383`) and script cannot make two identity objects for one BI id. The case passes only when: the mirror's sender is the connect object; a request sent with it keeps the server number; a respawn (the same object again) keeps the number on both sides; a request sent with a different object for the same BI id, as before A1, opens a new server session; and a reconnect (a new object) moves both sides to the same new number.

Why the case cannot fail on the old harness: the defect was not in the mirror's numbering rule (unchanged) but in which engine object fed it and which object the forges carried, and the old code had no unit that chooses a sender. Telling the two engine objects apart needs a connected client, so the proof that the old wiring fails and the new one passes is the in-game duplicate row: FAIL on `-163708`, rerun in A2. The case's "a different sender opens a new server session" clause shows the failure mode engine-free; a before-fix boot of the case would not compile (no mirror class), so it fails by construction, as the Phase 0 cases for new APIs did.

Build: `build\@NIMBY-20261101-183847` (`Build-NimbyPack.ps1 -Dev`, default boot check, port 2402, fresh `storage_999`), `SUMMARY PASS`, `SELFTEST DevHarness ran=yes cases=20 pass=20 fail=0` (19 before), `SELFTEST TOTAL pass=866 fail=0`; `PASS admin-mirror-forges-with-the-connect-identity detail=sender is the connect object=true kept=true respawn kept=true a different sender opens a new server session=true reconnect moves both=true`. That build also carries the A3 debug check (server half only). `Test-NimbyRepo.ps1`: `PASS: 0 failure(s)` (`logs\static-20261101-184011`). The first try (`-183753`) failed to compile on a local named `debug` in the A3 check, renamed `debugOn`.

Status: done in code; the in-game proof is A2.

### 0.2. A2 - harness rerun on the current build

Status: not started (needs A1, A3 and A4 built, then Jeff's two-client session).

### 0.3. A3 - identity-keyed sessions check

Status: not started.

### 0.4. A4 - argument-slot audit of the shipped code

Status: in progress.
