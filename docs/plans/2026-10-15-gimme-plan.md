# Plan: Gimme (formerly `NimbyGiveTake`)

Draft v0.1, October 15, 2026. Owner: Jeff Fabish. Implementer: a Claude Code session, coordinated by the orchestrator session. Status: for Jeff's review.

Gimme lets one player offer an item to another, request an item from another, and allow others to put items into their pockets. The fork exists, its transfer routine was rewritten on October 14 to use the vanilla server path, and it has never moved an item between two real players. This plan renames it to Gimme, fixes the key conflict, proves transfers with two clients on DayZ 1.30, and prepares it for release.

## What this is not

- Not a trading system. No currency, no prices, no trader.
- Not a way to take items without consent. Request and pocket access both need the other player's action or opt-in.
- Not a hands-to-hands transfer. Since October 14, transfers land in the receiver's inventory (cargo or an attachment slot), never their hands.

## Current state (October 15, 2026)

| Item | State | Source |
| --- | --- | --- |
| Source | `mods/own/nimby-pack/NimbyGiveTake/`: actions, inventory move, player, GUI, inputs, string table | Repo |
| Origin and permission | Fork of Give&Take (Workshop 2903112334, author affenb3rt). No license file; the Workshop description grants repack, modify, and redistribute | `NimbyGiveTake/NOTICE` |
| Transfer routine | Rewritten (CR-001): `ServerTakeToDst` through the holder's own hand state machine; logs REQUESTED, then OK or FAILED after the receiver checks | `NOTICE` |
| Logging | One `[NIMBY][GiveTake]` line per transfer; expected refusals logged, not raised (CR-003, CR-006) | `NOTICE` |
| Prefixing | All classes, inputs, string keys, and members prefixed `Nimby` (CR-005) | `NOTICE` |
| Keys | Offer/request `O`; pocket toggle Left Shift + `O` | `inputs.xml` |
| Save footprint | None persisted. Two net-synced variables (`m_NimbyGiveOrTakeState`, `m_NimbyAllowsPocketAccess`), which are not saved | Grep, October 15, 2026 |
| Solo test (1.29) | Offer/request animation: PASS. Shift+O pocket toggle: FAIL, Shift+O also fires offer/request and the toggle writes no log (NIMBY-T-002, high) | `docs/testing/2026-10-14-solo-ingame-test.md` |
| Two-client transfer test | Not run (CR-001 runtime verification) | Review, test doc |
| Permission capture | Workshop permission text not yet captured with the fork (CR-007) | `NOTICE`, review |
| On 1.30 | Never built or booted. 1.30 changed `CreateEntityInCargo`/`CreateInInventory` to check the item flip condition through `CanAddEntityInCargo` (experimental MODDING notes), which touches transfer destinations | Research doc |

## Decisions Jeff owes

| # | Decision | Options | Recommendation |
| --- | --- | --- | --- |
| G1 | Rename depth | Display name only; or rename addon, classes, string keys, and log prefix to `NimbyGimme` | Full rename now: nothing persists, so the rename cannot corrupt saves, and it is cheapest before release. Keep the Give&Take attribution in `NOTICE`. |
| G2 | Pocket toggle key (T-002) | Keep Left Shift + `O` and make offer/request ignore it; or move the toggle to another key | Keep Shift+O and exclude the modifier from offer/request, if the 1.30 input system supports it; otherwise move the toggle to a free key. |
| G3 | Pocket access default | Off by default; or on | Off by default, player opts in, with an on-screen notice each toggle. |

## Work breakdown

Each task ends with `scripts/Build-NimbyPack.ps1` passing, including its boot check.

### Phase 0: 1.30 baseline (about 0.5-1 day)

1. Using the shared 1.30 script reference, re-verify every vanilla symbol the mod touches: the action base classes and `ActionConstructor`, `ServerTakeToDst`, `FindFreeLocationFor`, `CanAddEntityInCargo`, the inventory location types, `ActionTargetsCursor`, and the input API. Record each as VERIFIED (file:line) or CHANGED. Give the `CanAddEntityInCargo` flip-condition change explicit attention.
2. Build and boot on 1.30 Nasdara.

Exit: 0 `SCRIPT (E)` on 1.30; every touched API VERIFIED or fixed.

### Phase 1: rename to Gimme (about 0.5 day)

Per decision G1: rename the folder, PBO, prefix, `CfgPatches`/`CfgMods` classes, script classes and files, input actions (`UANimbyGimme*`), string keys (`STR_NIMBY_GIMME_*`), and the log prefix to `[NIMBY][Gimme]`. Update `NOTICE` (the original attribution stays first) and request the `pack.json` entry change from the orchestrator.

Exit: the build passes; a grep for `GiveTake` outside `NOTICE` and history docs returns nothing.

### Phase 2: defects (about 1 day)

1. **NIMBY-T-002, Shift+O.** Apply decision G2 so the pocket toggle and offer/request never fire together.
2. **Toggle visibility.** The pocket toggle shows a notification to the player and writes one `[NIMBY][Gimme]` log line with player and new state.
3. **CR-007.** Save a dated capture of the Workshop description's permission text next to `NOTICE` (text file; Jeff provides or approves the capture).

### Phase 3: two-client test with Jay (about 1 evening)

The CR-001 runtime verification, on the local test server or a staging server, with two real clients:

| Case | Repetitions | Expected |
| --- | --- | --- |
| Offer from hands, accepted | 20 | Item leaves the giver, lands in the receiver's inventory; one REQUESTED and one OK line |
| Request from the other player, accepted | 20 | Same |
| Put in pockets with access on | 20 | Lands in pockets; refused when access is off |
| Transfer while the giver is mid-action (eating, reloading) | 10 | Refused cleanly or completes; never a ghost item |
| Receiver has no space | 5 | Refused with a FAILED or refusal line; item stays with the giver |
| After every case | - | Both players drop and re-pick the item; both inventories look right after a relog |
| Optional: 200 ms added latency | 10 | Same results |

Any ghost item, duplicated item, or item stuck in hands is a release blocker.

### Phase 4: release prep (about 0.5 day)

Update `NOTICE`, the pack README status, and the player-facing description for `#server-information`.

## Acceptance criteria

- Builds, signs, and boot-checks on 1.30 Nasdara with 0 `SCRIPT (E)`.
- Every Phase 3 case passes with Jay, with log lines matching each transfer.
- Shift+O and O never trigger each other.
- No save footprint, re-checked by grep at release.
- Permission capture stored with the fork.

## Estimate

About 3-3.5 days of implementation plus one two-client evening with Jay. Highest data-integrity risk of the three, because it moves items between players.

## Risks

| Risk | Effect | Mitigation |
| --- | --- | --- |
| 1.30 flip-condition check refuses valid destinations | Transfers fail into roomy inventories | Phase 0 verification; log the refusal reason |
| Desync or ghost items under load | Item loss or duplication exploit | Phase 3 repetitions; block release on any ghost |
| The 1.30 input system cannot exclude a modifier | T-002 fix as designed not possible | Decision G2 fallback: move the toggle key |
| Author changes the permission terms | Redistribution question | CR-007 capture; re-check before each release |

## Shared rules (same in all three plans)

1. **Branches and worktrees.** Each mod works on its own branch in its own git worktree: `feat/autorun`, `feat/gimme`, `feat/nimby-admin`, in `D:\GitHub\nimby-badlands-server-wt\<branch-name>`. Jeff runs merges to `main`; whether Claude Code sessions may commit on their own feature branch is the orchestrator's first question to Jeff.
2. **One 1.30 script reference.** Before any session writes code, the orchestrator unpacks the 1.30 `scripts.pbo` (DayZ Tools or `D:\GitHub\DayZExtract`) to one read-only folder that all three sessions grep. No vanilla API is used without a file:line from that folder.
3. **Shared files are orchestrator-owned.** Only the orchestrator edits `mods/own/nimby-pack/pack.json`, `scripts/Build-NimbyPack.ps1`, `scripts/modules/`, and `scripts/Start-NimbyTestServer.ps1`. A session that needs a change there asks the orchestrator.
4. **Identifiers.** RPC ids by block: Autorun `65501xx` (existing `6550101`), Gimme `65502xx`, NimbyAdmin `65503xx`. Log prefixes `[NIMBY][Autorun]`, `[NIMBY][Gimme]`, `[NIMBY][Admin]`. Class prefixes `NimbyAutorun`, `NimbyGimme`, `NimbyAdmin`.
5. **Key bindings registry.** Autorun: numpad plus. Gimme: `O`, Left Shift + `O`. NimbyAdmin: to be decided (see that plan). No two mods share a default binding, and none shadows a vanilla default.
6. **Zero save footprint.** No `OnStoreSave`/`OnStoreLoad` overrides and no data added to any saved entity, in any of the three mods. Net-synced variables are allowed; they are not persisted.
7. **Quality gate.** A change is done only when `Build-NimbyPack.ps1` exits 0 (build, sign, verify, boot check) and its in-game cases pass.
8. **Live rollout.** All three ship together as one `@NIMBY` release at a scheduled restart, after a local boot against a copy of the live `storage_1` shows 0 "Scripted variables corrupted". The launch line change restarts the server, so it lands in the restart window.
