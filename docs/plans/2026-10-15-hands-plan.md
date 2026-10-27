# Plan 2 - Hands (`NimbyHands`, formerly `NimbyGiveTake`)

Draft v0.7, October 21, 2026. Owner: Jeff Fabish. Implementer: a Claude Code session in worktree `hands-p3` on `study/hands-t010`, coordinated by the orchestrator. Status: P3 built in `@NIMBY-20261021-143500` (boot check SUMMARY PASS with `storageAutoFix = 0`, Hands self-test 156 cases, dev harness 16 cases, 0 `SCRIPT (E)`), unreviewed until Jeff merges it; the two-client re-test of the P3 build is owed (test record, "P3 build, October 21, 2026").

Target: NIMBY | HARDCORE | BADLANDS | 1pp, HostHavoc, DayZ 1.30.164078 / Nasdara.
Release: independent, gated (see [shared rules, 9.2](2026-10-15-shared-rules.md)).

Hands lets one player offer the item in their hands to another, request the item in another player's hands, and open their pockets so others can deposit items. Transfers land in the receiver's cargo, never a worn slot and never their hands (Jeff, October 21, 2026), and travel there in two server steps through the giver's own cargo (P3, ruling J2). The diagnostic of October 20, 2026 moved items between two real players on that path (ids 10 to 13); the P3 build's own two-client run is owed.

Changes since v0.6 (October 21, 2026, `study/hands-t010`, build `@NIMBY-20261021-143500`):

- P3 as built (H1, H3): `STAGED` joins the lifecycle between `REQUESTED` and the terminal states. Step 1 is the giver's own `ServerTakeToDst` from hands into a free cargo location of the giver's inventory (`FindFreeLocationFor` with `ANY_CARGO`, never a worn slot, never inside the moving item); its arrival is signal 1 and writes `STAGED`. Step 2 runs one tick later (`CallLater`, id lookup) after `RevalidateStep2`: consent snapshot, both parties, expected identities, reach, the exact staged location (type, parent, slot, cargo cell) with the giver as root, and the receiver destination recomputed then. It is the synchronous server move (`Inventory.c:1057-1066`); its signal writes `CONFIRMED` and its return value is read (`false` is `REFUSED` with the item's location). Cost path A (no staging room) refuses step 1 with `no space in giver inventory` and the item stays in hands; cost path B (receiver filled between the steps) refuses step 2 and the item stays in the giver's cargo. Nothing is retried, cloned, restored, or deleted.
- Consent is a snapshot taken at the action's execute (H2); the server ending the giver's offer loop is not a withdrawal.
- `REQUESTED -> CONFIRMED` stays a legal move for a direct delivery (H4); the reconcile at the timeout now also writes `REFUSED` when the item is provably still in the giver's inventory, and `UNCONFIRMED` only when it is not.
- NIMBY-T-012 (J4, H5): `NotificationSystem.SendNotificationToPlayerExtended` from the two terminal writers only, to the giver and the receiver on every terminal state (the requester only for `no item` and `item in flight`), each guarded for a null player and a null identity (a null identity would broadcast). The text comes from the pure `NimbyHandsRules.ToastText(state, detail, role)`, covered by the self-test for every terminal state and refusal family; plain text, no string-table key.
- Cargo only (H6, Jeff, October 21, 2026): `DestinationFlags()` is `ANY_CARGO` for all three verbs; an item that fits only a worn slot is a documented `REFUSED`; Put In Pockets can no longer equip a worn slot on the receiver (review item 1). `ReceiverHasSpace` joined the Put In Pockets condition and `GiverHasSpace` (staging room) joined all three conditions, so the prompt does not show for a transfer step 1 would refuse.
- Review items 2, 3, 5, 6, 7, 11, 16: `NimbyHandsCCTManState` sets the two inherited `CCTMan` fields in its constructor; the pocket toggle has a per-player 500 ms server-side gate (silent refusal, `NimbyHandsRules.PocketToggleAllowed`); `DescribeLocation` names the owner with both ids; `Execute` reconciles overdue entries before refusing `item in flight`; `NimbyHandsSetState` marks dirty only on a change; the toggle calls `super.OnStartServer`; Give's `OnStartServer` guards a null target. `Move` refuses an unregistered transfer.
- The dev harness `mods/own/dev/NimbyDevHarness/` exists (H7): 16 boot cases against the real handlers and six online cases that need a connected identity; its NOTICE lists them. The client output is byte-identical with and without `-Dev` (checked on `@NIMBY-20261021-143500` against `@NIMBY-20261021-143633`).
- Self-test: 52 to 156 cases, every new `CanMove` cell, `ClassifyMove` from both open states, `ClassifyStep2`, `ClassifyTimeout`, the registry's staged cases, `CheckRequest` precedence for the 15 step 1 and 11 step 2 refusal details, the toggle gate, and `ToastText`. A deliberate flip of one cell failed the boot (`build-20261021-143655`) and was reverted.

Changes since v0.5 (October 20, 2026, `study/hands-t010`): the first two-client run moved no item (NIMBY-T-010, open, release blocker) and showed players nothing when a request ends `UNCONFIRMED` or `REFUSED` (NIMBY-T-012, open); the feasibility study `docs/design/2026-10-20-hands-t010-feasibility.md` ranks the causes, compares the candidate dispatch paths, and proposes a diagnostic session for Jeff's ruling before any build.

Changes since v0.4 (October 19, 2026, `feat/hands`):

- Phase 0 items 1-5 done from the 1.30 reference: baseline `docs/baselines/2026-10-19-hands-phase0-baseline.md`; the API evidence, validation points, completion-signal ranking, and the G2 answer are in "Phase 0 evidence" below. Item 6 (two clients) is open.
- Phase 1 built under the old name (`@NIMBY-20261019-152248`, commit `945fb6c`) and Phase 2 built after it (`@NIMBY-20261019-152838`, commit `e5eb20d`); the release candidate for the two-client session is `@NIMBY-20261019-153354` (commit `8cc681b`, which adds the reservation pre-check). All three SUMMARY PASS on Nasdara with the Hands self-test at 52 cases, 0 `SCRIPT (E)`.
- G5 as built: the registry is keyed by the item's network id string, not the `EntityAI` reference, so an item deleted in flight cannot leave a null key that is never evicted. It still names exactly one item; G5 is otherwise unchanged. The timeout is a constant (5,000 ms, `NimbyHandsConstants.TRANSFER_TIMEOUT_MS`), not a config file.
- New finding: `DayZPlayerInventory` is an engine class that cannot be modded on 1.30 (boot check `logs/build-20261019-152106`: "Engine class 'DayZPlayerInventory' cannot be modded"). The hand event result (Phase 0 item 4, third candidate) is therefore not observable; the tests it runs are asked before dispatch instead (`GameInventory.LocationCanMoveEntity`, `GameInventory.HasInventoryReservation`), and only a reservation taken after that check can refuse silently, ending `UNCONFIRMED`.
- Revalidation also refuses a stale request whose item or player changed since the action started (the server records them at `OnStartServer`), a disconnected player, a busy giver inventory, a move the engine's own test refuses, and a reserved destination.
- `REFUSED` also covers an item that leaves the giver for somewhere other than the receiver while the request is open (Phase 3, "giver swaps, drops").
- Log format: player names are quoted and escaped, `giver="<name>" (<bi-id>, <steam64>)`, so a name cannot forge a field. Debug lines appear only with `$profile:NimbyHands\debug`.
- Pocket lines: the toggle line (`reason=toggle`) is logged in production as a consent audit record; forced closes are logged only when pockets were actually open.
- Phase 2 as built: the two input labels are plain text (`Hands: offer or request item`, `Hands: open or close pockets`), as Ears does, so their string keys were removed; the pocket notification adds the toggle's key, or where to bind it. `PERMISSION.md` is a marked placeholder until Jeff supplies the text.
- `CLAUDE.md`, `AGENTS.md`, the root `NOTICE`, and the pack `README.md` name `NimbyHands` (commit `a04c6f0`), which the Phase 2 exit grep needs.

Changes since v0.3: display name Hands (Jeff, October 16, 2026; was Gimme), so the rename target is `NimbyHands`, worktree `feat/hands`, inputs `UANimbyHands*`, string keys `STR_NIMBY_HANDS_*`, log prefix `[NIMBY][Hands]`; the pocket-toggle Controls label is "Hands: open or close pockets"; key bindings are player-configurable per shared rules 5.

Changes since v0.2: shared rules moved to their own file; current-state and decisions tables restored; the in-flight registry and completion-signal candidates are specified; pockets close on unconsciousness and restraint (G4); the pocket toggle is unbound with a discoverable label (G2); every test row names its evidence; dead `#ifdef DIAG` code is scheduled for removal; the stale-PBO check moves to deployment.

## What this is not

- Not a trading system. No currency, prices, or trader.
- Not a way to take items without consent. Request and pocket access both need the other player's action or opt-in.
- Not a hands-to-hands transfer.
- Not a persistence change. No custom entity serialization and no persistent entity classes.

## Current state (verified October 15, 2026)

The rows below describe `main` before Phase 1. On `feat/hands` the transfer routine, concurrency, logging, keys, pocket, and dead-code rows are superseded by "Changes since v0.4".

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
| G3 | Pocket access default | Off; resets on reconnect and respawn; opening permits deposits only, into cargo only (H6) | Decided |
| G4 | Pockets auto-close | Close on unconsciousness and on restraint. "Put in pockets" refuses an unconscious or restrained target. The toggle cannot be used while restrained | Decided October 15, 2026 |
| G5 | In-flight transfer registry | Server-side, mission-lifetime singleton `NimbyHandsTransfers`: `map<EntityAI, ref NimbyHandsTransfer>` keyed by item, evicted on terminal state or after a bounded timeout (default 5,000 ms) once inventory state is reconciled. Nothing persisted | Decided (Claude default) |
| H1 | Transfer path after NIMBY-T-010 (J2) | P3: two server steps through the giver's own cargo under one id, with `STAGED` between `REQUESTED` and the terminal states; `CanMove` allows `REQUESTED -> STAGED` and `STAGED -> CONFIRMED, REFUSED, UNCONFIRMED`, never back to `REQUESTED`. Step 2 bypasses the engine's live-player rule and 2.5 m anti-cheat, so `RevalidateStep2` is the only guard | Decided by Jeff, October 20, 2026; built October 21, 2026 |
| H2 | Consent at step 2 | The snapshot taken at the action's execute; step 2 does not re-read it. The server ending the giver's offer loop for Take is not a withdrawal | Decided October 21, 2026 |
| H3 | What the registry entry carries | `NimbyHandsWorldTransfer` holds reach, destination flags, the consent snapshot, the expected item and identities, the step 1 landing (type, parent, slot, cargo cell), and the giver as `PlayerBase`; the move object dies when the action returns | Decided October 21, 2026 |
| H4 | Direct delivery cell | `REQUESTED -> CONFIRMED` stays legal: the reconcile can find the item with the receiver without a `STAGED` line, and a forbidden move there would leave the entry unevictable. The reconcile writes `REFUSED` when the item is provably in the giver's inventory and `UNCONFIRMED` only when it is not | Decided October 21, 2026 (session; the build brief listed the cell as absent) |
| H5 | Toast (NIMBY-T-012, J4) | `SendNotificationToPlayerExtended` for 4 s with the title `Hands`, from `Advance` and `LogUnregisteredRefusal` only, to giver and receiver (requester only when unregistered), null player and null identity guarded; text from the pure `ToastText`, plain text | Decided by Jeff, October 20, 2026 (J4); built October 21, 2026 |
| H6 | Destinations | Cargo only for Take, Give, and Put In Pockets (`FindInventoryLocationType.ANY_CARGO`); step 1's staging location is cargo only and never inside the moving item. Cost: an item that fits only a worn slot (a backpack with a full cargo, a rifle) is `REFUSED`, with `no space in giver inventory` when the giver cannot stage it; a documented capability loss, not a bug | Decided by Jeff, October 21, 2026 |
| H7 | Dev harness | `mods/own/dev/NimbyDevHarness/`, built with `-Dev` under `nimby_dev`, never an `addons` row; boot cases at mission start with the `selftest` flag, online cases on connect when `$profile:NimbyDevHarness\forge` says `online`, and `step2-refuse` in that file arms one forced step-2 refusal (cost path B) | Decided by Jeff, October 21, 2026; built the same day |

## Phase 0 - prove the transfer path before renaming

1. Record the client and server build, source revision, and the shared 1.30 reference hashes in `docs/baselines/`.
2. Verify against the shared reference and record file:line: the action base classes and `ActionConstructor`, `ContinuousDefaultActionInput`/`StandardActionInput`, `FindFreeLocationFor`, `CanAddEntityInCargo` (flip-condition change), `ServerTakeToDst`, `HumanInventory.RedirectToHandEvent`, `InventoryLocation`, `ActionTargetsCursor.GetItemDesc`/`GetItemHealth`/`GetItemCargoCount`, `NotificationSystem.AddNotificationExtended`, and the input API.
3. Trace where vanilla validates consent, distance, source ownership, destination availability, and action continuation, and record it.
4. Find the completion signal. Candidates to test, in order: `ItemBase.EEItemLocationChanged`, `OnInventoryEnter`/`OnInventoryExit` on the receiver, and the hand-event result on the holder. Record which fire, on which side, and in what order, for a successful move and for a refused one. A successful `ServerTakeToDst` return is not proof of completion.
5. Settle the G2 question: does the input system let `UANimbyOfferRequestItem` ignore a press while Left Shift is held? Record yes or no with file:line.
6. Run offer, request, and pocket deposit between Jeff and Jay on the local test server with the shared tooling.

Exit: the native transfer path moves an item between two real clients; its validation points and completion signal are documented. If reliable transfer cannot be shown, stop and revise. Never compensate by deleting and recreating items or injecting synthetic hand-state changes.

### Phase 0 evidence (October 19, 2026)

Items 1-5, from the shared 1.30 reference (build `1.30.164078.38`, `scripts.pbo` hash re-checked in `docs/baselines/2026-10-19-hands-phase0-baseline.md`). Paths are relative to `vendor/dayz-1.30-scripts/`; `S/` stands for `scripts/`, `UAC/` for `scripts/4_World/DayZ/Classes/UserActionsComponent/`, `INV/` for `scripts/3_Game/DayZ/Systems/Inventory/`. A declaration is not proof of behavior; item 6 and Phase 3 exercise these in game.

#### Item 2 - API evidence

| API | Where (file:line) | What the reference shows | Used by Hands |
| --- | --- | --- | --- |
| `ActionBase` | `UAC/ActionBase.c:51`; `Can` `:929-973`; `CanContinue` `:975-999`; `CreateActionData` `:240`; `Start` calls `OnStartServer` `:775-800` (`:788`); `CanBeUsedInRestrain` default false `:356` | The server evaluates `Can` once when it accepts the action (`UAC/ActionManagerServer.c:168`); the condition mask (vehicle, restrained, swimming, ...) is computed there (`:875-927`) | `NimbyHandsActionData` via `CreateActionData`; `OnStartServer` records item and identities |
| `AnimatedActionBase` | `UAC/AnimatedActionBase.c:188-210` (animation event runs `OnExecuteServer`, no `Can` re-check); `Do` `:395-455` (`CanContinue` per progress tick `:419`, at initialize `:443`) | An interact action executes on its animation event without re-checking its condition | Revalidation at dispatch closes this gap |
| `ActionContinuousBase` | `UAC/Actions/ActionContinuousBase.c:120`; `OnFinishProgressServer` `:289`, called `:257` | Continuous actions re-check `ActionCondition` every tick through `CanContinue` | Put In Pockets, offer, request |
| `ActionInteractBase`, `ActionInstantBase` | `UAC/Actions/ActionInteractBase.c:38`; `UAC/Actions/ActionInstantBase.c:1` | | Give, Take; pocket toggle |
| `ActionConstructor` | `UAC/ActionConstructor.c:1`; `RegisterActions` `:27` | | All six actions registered |
| `StandardActionInput`, `ContinuousDefaultActionInput` | `UAC/ActionInput.c:292`, `:627` (`ContinuousInteractActionInput` `:545`); `SetInput` `:77`; `AIT_SINGLE` `:4`, `AIT_CLICKCONTINUOUS` `:7` | | Offer/request input (click-continuous), toggle input (single) |
| `FindFreeLocationFor` | `INV/Inventory.c:276` (`FindFreeLocationForEx` `:283`) | Native; returns a location, flip included | Destination at dispatch and in the action conditions (`ANY_CARGO` only since v0.7, H6; the giver's staging location is checked not to lie inside the moving item through `GetHierarchyParent`, `EntityAI.c:885`) |
| `CanAddEntityInCargo` and the flip change | `INV/Inventory.c:154` (native); `CreateEntityInCargo` `:143`, `CreateEntityInCargoEx` `:150`; `CreateInInventory` `:876-894` passes `loc.GetFlip()` `:886` | No script in `3_Game` or `4_World` calls `CanAddEntityInCargo`; the 1.30 flip-condition check is engine-side and cannot be read in script. The research doc cited in the current-state table has no text on it. Hands creates no entity; it moves one to a location the engine itself returned | Covered in game by the Phase 3 "rotated items" row |
| `ServerTakeToDst` | `S/3_Game/DayZ/Entities/Man.c:757-760` -> `TakeToDstImpl` `:762-768` -> `HumanInventory.TakeToDst` `INV/HumanInventory.c:243-251`; `EntityAI.ServerTakeToDst` `S/3_Game/DayZ/Entities/EntityAI.c:2063` | A dead holder skips the hand event and falls through to a direct inventory move (`HumanInventory.c:247-250`); Hands refuses a dead giver before dispatch | The transfer path |
| `HumanInventory.RedirectToHandEvent` | `INV/HumanInventory.c:200-241` | With the source in hands it clears the item's user-reserved location, calls the holder's `HandEvent`, and returns true without looking at that call's result (`:228-229`). A `ServerTakeToDst` true is not proof of anything | |
| `DayZPlayerInventory.HandEvent` | `S/4_World/DayZ/Systems/Inventory/DayZPlayerInventory.c:4435-4475`: refuses while `IsProcessing` (`:4437`, `:4639-4642`), on `CanPerformEventEx` (`:4446`), or on reservation (`:4449`); then defers. `HandleInventoryManipulation` `:4623-4635` runs it next update; SERVER mode sends it as juncture `SJ_INVENTORY` (`:4587-4615`, `:4206-4277`, `:4259`); the execution result is discarded (`:1830`) | Class at `:147` is an engine class: modding it fails the boot (`logs/build-20261019-152106`) | Busy check and `LocationCanMoveEntity` before dispatch |
| `HandEventMoveTo` | `INV/Hand_Events.c:268-338`; `CanPerformEvent` `:306-319` uses `GameInventory.LocationCanMoveEntity` (`INV/Inventory.c:373`) | | Same engine test asked before dispatch |
| `InventoryLocation` | `INV/InventoryLocation.c:29`; types `:3-13` (`TEMP` "client side only" `:12`); `FindInventoryLocationType` `:17`; `IsValid` `:36`, `GetType` `:43`, `GetParent` `:52`, `GetItem` `:62`; `GetCurrentInventoryLocation` `INV/Inventory.c:268` | | Source check, destination text, signal classification |
| `ActionTargetsCursor` | `S/5_Mission/DayZ/GUI/ActionTargetsCursor.c:53`; `GetItemDesc` `:742`, `GetItemHealth` `:815`, `GetItemCargoCount` `:950`, `AttachmentsWithInventoryOrCargoCount` `:1317` (all protected) | | Offered item's name, health, cargo count on the cursor |
| `NotificationSystem.AddNotificationExtended` | `S/3_Game/DayZ/Client/Notifications/NotificationSystem.c:215` | | Pocket notification; text joined with `Widget.TranslateString` (`S/1_Core/DayZ/proto/EnWidgets.c:112`) |
| Input API | `UAInput` `S/3_Game/DayZ/InputAPI/UAInput.c:33-103` (`LocalPress` `:60`, `BindCombo` `:43`, event limits `:71-81`, `Supress` `:84`, `Lock` `:88`, `ForceDisable` `:94`); `UAInputAPI` `:205-277` (`GetInputByName` `:218`, `SortingCount` `:258`); `GetUApi` `:279`; `InputUtils.GetButtonNameFromInput` `S/3_Game/DayZ/tools/InputUtils.c:17-30`; `bin/constants.xml:482` (`kO`), `:500` (`kLShift`), `:915` (Others sorting) | | Bindings in `Data/Inputs.xml`; notification names the toggle key |
| `PlayerIdentity.GetPlainId()` | `S/3_Game/DayZ/gameplay.c:370` (`GetId` `:368`, `GetName` `:362`); vanilla reads it as the Steam UID in `S/3_Game/DayZ/Client/SyncPlayerList.c:18` and `ClientData.c:46` | The declaration's comment says "plaintext unique id of player (cannot be used in database or logs)". Shared rules 4.2 decides both ids go in the log; recorded here so the choice is visible | Second id on every line naming a player |

#### Item 3 - where vanilla validates

| Check | Vanilla point | Gap Hands closes |
| --- | --- | --- |
| Consent | None in vanilla. Hands' consent is the other player's state: offering (`NimbyHandsCCTManState`), requesting, or pockets open | Re-read at dispatch (`SetConsent`) |
| Distance | `CCTMan.Can`: facing and `DistanceSq <= max^2` (`UAC/TargetConditionsComponents/CCTMan.c:12-22`); checked at server acceptance (`ActionManagerServer.c:168`) and, for continuous actions, every tick (`CCTBase.CanContinue` calls `Can`, `CCTBase.c:8-11`). Client-sent hand events are also bounded by `c_MaxItemDistanceRadius` 2.5 m (`INV/Hand_Events.c:298-304`, `INV/Inventory.c:815`); a server-mode event is not | Interact actions: rechecked at dispatch with the action's reach plus 0.5 m |
| Source ownership | `ActionBase.Can` refuses a target item in another player's inventory unless the target is a player (`ActionBase.c:938-944`); `RedirectToHandEvent` acts only for a source in hands (`HumanInventory.c:202`); `CheckRequestSrc` for client requests (`Hand_Events.c:288-296`) | Item must be in the giver's hands and be the item seen at action start |
| Destination availability | `FindFreeLocationFor` (`Inventory.c:276`); `LocationCanMoveEntity` in `CanPerformEvent` (`Hand_Events.c:306-319`); reservation (`DayZPlayerInventory.c:4449` -> `DeferredHandEvent.ReserveInventory` `:133-136` -> `INV/Hand_Events.c:177`; short timeout 3,000 ms `Inventory.c:715`); hand guard `HandGuardHasRoomForItem` (`INV/Hand_Guards.c:255`) | Both script-visible tests asked at dispatch |
| Action continuation | `CanContinue` (`ActionBase.c:975-999`) -> `ActionConditionContinue` (`:436`; `AnimatedActionBase.c:239`) -> `ActionCondition`; `Do` interrupts on failure (`AnimatedActionBase.c:418-430`). The condition mask is only computed at acceptance | Player state (G4) rechecked at dispatch |

#### Item 4 - completion signal

Order for a successful move, read from the reference (server side): `ServerTakeToDst` queues the hand event (`DayZPlayerInventory.c:4448`); the next inventory update sends it as juncture `SJ_INVENTORY` (`:4604`, `:4259`); when the juncture is processed the hand state machine moves the item and the engine calls `EEItemLocationChanged` (`EntityAI.c:976`), which calls `OnItemLocationChanged` (`:980`; `ItemBase.c:1162`), which calls `OnInventoryExit` on the giver (`ItemBase.c:1231`) and then `OnInventoryEnter` on the receiver (`:1242`) for the item and each nested item; back in `EEItemLocationChanged` the giver's `OnItemInHandsChanged` runs (`EntityAI.c:1005-1010`). A refused move changes no location, so none of these fire. Which side fires each event is engine behavior the scripts do not state; P0-4b in the test record measures it.

| Rank | Candidate | Reading | Wired as |
| --- | --- | --- | --- |
| 1 | `ItemBase.EEItemLocationChanged`, server | Carries the old and new location, fires once per move, before the other two. All 21 vanilla overrides call super | `CONFIRMED` when the new location's root player is the receiver; `REFUSED` when the item went anywhere else; ignored for the giver's hands and `TEMP` |
| 2 | `OnInventoryExit` / `OnInventoryEnter` | Fire only from inside rank 1, once per nested item, and carry only the player | Traced behind the debug flag with a sequence number |
| 3 | Hand event result on the holder | Not observable on 1.30: the queue-time result is dropped (`HumanInventory.c:228-229`), the execution result is discarded (`DayZPlayerInventory.c:1830`), and the class cannot be modded | Its refusal tests (busy inventory, `LocationCanMoveEntity`, `HasInventoryReservation` at `INV/Inventory.c:787`) are asked before dispatch; a reservation taken after that check ends `UNCONFIRMED` at the 5,000 ms reconcile. Action reservations are not a conflict on the server: `ActionBase.InventoryReservation` returns at once on a dedicated server (`UAC/ActionBase.c:1061-1062`) |

The two-client run confirms the ranking when, for every successful transfer, the server debug trace shows `EEItemLocationChanged.begin`, `OnInventoryExit` (giver), `OnInventoryEnter` (receiver), `EEItemLocationChanged.end` in that order and the `CONFIRMED` line names `signal=EEItemLocationChanged`, well inside 5,000 ms of `REQUESTED`; and a refused transfer shows no signal lines (test record P0-4a to P0-4e).

#### Item 5 - can `UANimbyHandsOfferRequest` ignore a press while Left Shift is held?

No. The 1.30 input API has no per-input "not while another button is held" condition: `UAInput` offers bindings and combos, event limits, suppression, locking, and force-disable (`S/3_Game/DayZ/InputAPI/UAInput.c:33-103`); `modificator` entries are sensitivity modifiers (`bin/specific.xml:3-13`); exclude groups disable whole input sets by context (`bin/specific.xml:81` onward); and the vanilla Shift binding `UAADSToggle` is a plain click on `kLShift` (`bin/preset_X1MouseKey.xml:169-176`), not an exclusion. A combo (nested `btn`) fires its own input on Shift+O but nothing stops the plain-O input firing on the same press, which is NIMBY-T-002. A script could compare two inputs in the same frame, but that is unproven and frame-order dependent. G2 stands: the toggle ships unbound.

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
| `REQUESTED` | Step 1 submitted to the engine (hands to the giver's own cargo) |
| `STAGED` | Step 1 landed at the recorded location in the giver's cargo; step 2 scheduled one tick later (v0.7, H1). Not terminal |
| `CONFIRMED` | The item arrived with the receiver: step 2's completion signal, or the reconcile found it there |
| `REFUSED` | No delivery and the item's place is known: validation or engine refusal, the item left the giver or the staged location for somewhere other than the receiver, or the reconcile found it still in the giver's inventory |
| `UNCONFIRMED` | No evidence within the bounded observation period and the item not in the giver's inventory |

A recipient moving the item afterward must not turn `CONFIRMED` into a failure. Never automatically retry, clone, restore, or delete an item to compensate for an uncertain result.

### Pockets (G4)

- `NimbyActionTogglePocketAccess.CanBeUsedInRestrain()` returns false.
- The server closes pockets when the owner becomes unconscious or restrained, and on reconnect and respawn.
- `NimbyActionPutInPockets.ActionCondition` refuses an unconscious or restrained target.

### Logging

| Event | Line |
| --- | --- |
| Lifecycle | `[NIMBY][Hands] id=<corr> action=<class> state=<REQUESTED|CONFIRMED|REFUSED|UNCONFIRMED> giver="<name>" (<bi-id>, <steam64>) receiver="<name>" (<bi-id>, <steam64>) item=<type> #<netid> detail=<text>` |
| Pocket toggle | `[NIMBY][Hands] pockets player="<name>" (<bi-id>, <steam64>) open=<bool> reason=<toggle|unconscious|restrained|respawn|reconnect>` |
| Debug (flag only) | `[NIMBY][Hands][Debug] signal=<name> side=<server|client> seq=<n> t=<ms> id=<corr|-> item=<type> #<netid> <detail>` |
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
