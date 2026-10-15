# Plan: Autorun (working name `NimbyAutorun`)

Draft v0.1, October 15, 2026. Owner: Jeff Fabish. Implementer: a Claude Code session, coordinated by the orchestrator session. Status: for Jeff's review.

Autorun lets a player toggle continuous forward movement with one key. The fork exists and passed most of a solo test on DayZ 1.29; this plan brings it to a releasable state on DayZ 1.30: fix the two open defects, add defense-in-depth on the server, prove it on 1.30 and Nasdara, and rename it once Jeff picks a name.

## What this is not

- Not a movement cheat. The server keeps validating every request; autorun never moves faster than the player could on foot, and never runs while in a vehicle, unconscious, restrained, or dead.
- Not a vehicle cruise control.
- Not a rewrite. The Apache-2.0 fork, its attribution, and its server-side validation stay.

## Current state (October 15, 2026)

| Item | State | Source |
| --- | --- | --- |
| Source | `mods/own/nimby-pack/NimbyAutorun/`, 2 scripts (271 lines), `config.cpp`, `Data/Inputs.xml` | Repo |
| Origin and license | Fork of SchanaModAutorun commit 1048f71, Apache-2.0; `LICENSE` and `NOTICE` present | `NimbyAutorun/NOTICE` |
| Server validation | Ownership, movement index and angle range, sprint permission, refused in vehicle, unconscious, restrained, or dead; rejects logged, rate-limited | `NOTICE`, `nimbyautorun_playerbase.c` |
| Review fixes done | CR-004 (RPC every frame) fixed | `NOTICE` |
| Default key | Numpad plus (`kAdd`) | `Data/Inputs.xml` |
| RPC id | `6550101` | `nimbyautorun_playerbase.c:15` |
| Save footprint | None (no `OnStoreSave`/`OnStoreLoad`, no net-synced variables) | Grep, October 15, 2026 |
| Last build | `build\@NIMBY-20261014-175505`, built and boot-checked on 1.29 Chernarus | Pack README, test doc |
| Solo test (1.29) | Toggle, WASD/fire/raise interrupts, stamina drop: PASS. Lean interrupt: FAIL (NIMBY-T-001). Vehicle and unconscious recovery: NOT RUN (NIMBY-T-003) | `docs/testing/2026-10-14-solo-ingame-test.md` |
| On 1.30 | Never built or booted | - |

## Decisions Jeff owes

| # | Decision | Options | Recommendation |
| --- | --- | --- | --- |
| A1 | Final name | Keep `NimbyAutorun`; or a short name such as "Cruise" or "Trek" | Keep the class prefix `NimbyAutorun` in code whatever the display name is; renaming code is churn with no player benefit. Only the display name and `mod.cpp` text change. |
| A2 | Lean behavior (T-001) | Make lean interrupt autorun; or drop lean from the interrupt list | Make it interrupt if 1.30 exposes a usable lean input event; otherwise drop it and document. |
| A3 | Default key | Keep numpad plus; or add a second default | Keep numpad plus. Players can rebind. |

## Work breakdown

Each task ends with the pack build (`scripts/Build-NimbyPack.ps1`) passing, including its boot check.

### Phase 0: 1.30 baseline (about 0.5 day)

1. Use the shared 1.30 script reference (see "Shared rules", item 2). Re-verify every vanilla symbol the mod touches: `MissionGameplay.OnInit`/`OnUpdate`, `PlayerBase.OnRPC`, `GetUApi().GetInputByName`, the `UA*` input names, the movement override call, and `CanSprint` or its equivalent. Record each as VERIFIED (file:line in 1.30) or CHANGED.
2. Build and boot on 1.30 with `bootCheck.mission` set to `dayzOffline.nasdara` (the orchestrator owns that change).
3. Re-run the passing solo checks on 1.30 Nasdara: toggle, WASD, fire, raise, stamina drop.

Exit: 0 `SCRIPT (E)` on 1.30; every touched API recorded as VERIFIED or fixed.

### Phase 1: defects (about 1 day)

1. **NIMBY-T-001, lean.** Find why `UALeanLeft`/`UALeanRight` never register while sprinting. Fix the check, or drop lean per decision A2, and update the interrupt list and its comment.
2. **CR-008, stale server override.** Clear the server-side override when the player enters a vehicle, falls unconscious, is restrained, or dies, without waiting for a client stop. Use the vanilla state hooks found in Phase 0; log one `[NIMBY][Autorun]` line per forced clear.
3. Add a log line for toggle on and off at debug level only, off by default, so production logs stay quiet.

Exit: the Phase 2 test cases for T-001 and CR-008 pass.

### Phase 2: in-game test (about 0.5 day with Jeff)

Test on the local test server with the 1.30 Nasdara mission. The vehicle cases need a vehicle: use the NimbyAdmin spawn once Phase 0 of that plan lands, or a test-only spawn line in the local test mission's `init.c`, never in the live mission.

| Case | Expected |
| --- | --- |
| Toggle on, toggle off | Starts and stops |
| W, S, A, D, fire, raise weapon, lean (or lean removed) | Stops |
| Sprint until exhausted | Drops to run, keeps moving |
| Autorun, then get in a car or on a motorbike | Autorun stops; after exiting, the player stands still |
| Autorun, then knocked unconscious, then wake | Player stands still |
| Autorun, then restrained | Stops; cannot restart |
| Forged request from a modified client | Not testable in game; covered by the existing server validation review |

### Phase 3: release prep (about 0.5 day)

1. Apply decision A1 (display name, `mod.cpp`, README, `NOTICE` change list).
2. Update `NOTICE` with every change since October 14, newest first.
3. Update `mods/own/nimby-pack/README.md`: status "Built and tested in game on 1.30".

## Acceptance criteria

- Builds, signs, and boot-checks on 1.30 Nasdara with 0 `SCRIPT (E)`.
- Every Phase 2 case passes, witnessed by Jeff.
- No save footprint (unchanged rule, re-checked by grep at release).
- `NOTICE` and the pack README are current.

## Estimate

About 2.5 days of implementation plus one test evening with Jeff. Lowest risk of the three plans.

## Risks

| Risk | Effect | Mitigation |
| --- | --- | --- |
| 1.30 animation or input changes break the movement override | Autorun does nothing or moves oddly | Phase 0 verification before any other work |
| Lean input not observable while sprinting | T-001 cannot be fixed as designed | Decision A2 allows dropping lean |
| Vehicle test blocked | CR-008 unproven | Test-only spawn in the local mission if NimbyAdmin is not ready |

## Shared rules (same in all three plans)

1. **Branches and worktrees.** Each mod works on its own branch in its own git worktree: `feat/autorun`, `feat/gimme`, `feat/nimby-admin`, in `D:\GitHub\nimby-badlands-server-wt\<branch-name>`. Jeff runs merges to `main`; whether Claude Code sessions may commit on their own feature branch is the orchestrator's first question to Jeff.
2. **One 1.30 script reference.** Before any session writes code, the orchestrator unpacks the 1.30 `scripts.pbo` (DayZ Tools or `D:\GitHub\DayZExtract`) to one read-only folder that all three sessions grep. No vanilla API is used without a file:line from that folder.
3. **Shared files are orchestrator-owned.** Only the orchestrator edits `mods/own/nimby-pack/pack.json`, `scripts/Build-NimbyPack.ps1`, `scripts/modules/`, and `scripts/Start-NimbyTestServer.ps1`. A session that needs a change there asks the orchestrator.
4. **Identifiers.** RPC ids by block: Autorun `65501xx` (existing `6550101`), Gimme `65502xx`, NimbyAdmin `65503xx`. Log prefixes `[NIMBY][Autorun]`, `[NIMBY][Gimme]`, `[NIMBY][Admin]`. Class prefixes `NimbyAutorun`, `NimbyGimme`, `NimbyAdmin`.
5. **Key bindings registry.** Autorun: numpad plus. Gimme: `O`, Left Shift + `O`. NimbyAdmin: to be decided (see that plan). No two mods share a default binding, and none shadows a vanilla default.
6. **Zero save footprint.** No `OnStoreSave`/`OnStoreLoad` overrides and no data added to any saved entity, in any of the three mods. Net-synced variables are allowed; they are not persisted.
7. **Quality gate.** A change is done only when `Build-NimbyPack.ps1` exits 0 (build, sign, verify, boot check) and its in-game cases pass.
8. **Live rollout.** All three ship together as one `@NIMBY` release at a scheduled restart, after a local boot against a copy of the live `storage_1` shows 0 "Scripted variables corrupted". The launch line change restarts the server, so it lands in the restart window.
