# Hands NIMBY-T-010 feasibility study - player-to-player transfer on 1.30

Draft v0.1, October 20, 2026. Owner: Jeff Fabish. Branch `study/hands-t010` (cut from `main` at `1473a3b`). Status: study only. No addon code, build, or test record changed. The diagnostic in section 7 is a proposal for Jeff's ruling and is not built.

Inputs: the "Two-client session, October 20, 2026" section of `docs/testing/2026-10-19-hands-phase1.md`; plan `docs/plans/2026-10-15-hands-plan.md` v0.5 (G4, G5, the signal ranking); `docs/plans/2026-10-15-shared-rules.md` v1.3; the addon under `mods/own/nimby-pack/NimbyHands/`; the server RPT `profiles\main-round1\DayZServer_x64_2026-10-20_15-44-56.RPT` and the D client log `%LOCALAPPDATA%\DayZ\script_2026-10-20_15-59-50.log` (re-read for this study); review `D:\GitHub\_reviews\nimby-badlands-server-review-2026-10-14.md` (CR-001); `git log` of the Hands folder.

Citations: every vanilla reference was read in `vendor/dayz-1.30-scripts/` (build `1.30.164078.38`) on October 20, 2026. Paths are relative to `vendor/dayz-1.30-scripts/scripts/`. Short forms: `DPI` is `4_World/DayZ/Systems/Inventory/DayZPlayerInventory.c`, `INV/` is `3_Game/DayZ/Systems/Inventory/`, `UAC/` is `4_World/DayZ/Classes/UserActionsComponent/`. A declaration is not proof of behavior, and four of the deciding calls are native (engine code that script cannot read); those are marked as such.

## Verdict

The root cause cannot be settled by reading. The dispatch reaches the engine and then passes through four checks that are native and silent on a retail build; any one of them can drop the move with no log line on either side. They are ranked in section 4:

1. **H-B, the animated hide cannot complete while the giver is in an action.** A destination inside any player's inventory makes the hand state machine choose the animated move (a hide animation on the giver), and the item moves only on that animation's event. All three verbs dispatch while the giver is inside an action command. Vanilla never drives a server hand move down the animated branch.
2. **H-A, the action's own target lock.** Take, Give, and Put In Pockets each hold an inventory juncture on the other player for the whole action. The hand event's juncture is requested while that lock is held, its result is ignored, and the server's execution returns silently when it is missing. Every vanilla server move of a locked entity clears its action juncture first.
3. **H-C, the engine's anti-cheat refuses a live receiver.** The juncture's execution runs the native `CheckMoveToDstRequest` on both sides; vanilla's script policy forbids a live, conscious, unrestrained other player as a destination, and the native may enforce the same.

A fourth, H-D (the juncture is never played back), is possible but has no specific evidence.

Recommendation: do not build a fix yet. Run the diagnostic in section 7 (one two-client session, Jeff on both machines), which tells the four apart from server and client log lines. If H-C is false, the fix is P2: dispatch the same `ServerTakeToDst` after both actions end. If H-C is true, the only server-authoritative route is P3: hands to the giver's own inventory, then a synchronous server move to the receiver. For NIMBY-T-012, send a vanilla notification to both players from the server on every terminal state other than `CONFIRMED`.

## What this is not

- Not a fix. Nothing under `mods/`, `scripts/`, or `server/` changed, and no build ran.
- Not a reopening of Phase 1's safety design. The in-flight registry (G5), revalidation, and pockets lifecycle behaved on October 20; every candidate below keeps them.
- Not a test record. The October 20 rows stay as filled.

## 1. What the October 20 run showed (re-verified)

The server RPT lines 6807 to 6835 hold ids 1 to 6 exactly as the record lists them. Each `REQUESTED` line names `dst=CARGO:KameezShirt_Orange@<L>`, and each `UNCONFIRMED` line 5,000 ms later reconciles the item at `HANDS:SurvivorM_Oliver@<D>`. Between them the RPT holds only engine network lines, a character save, and Central Economy loot lines: no `SCRIPT (E)`, no `HCW:` error, no inventory line. The D client log has the Hands load and self-test lines and nothing during the transfers.

Two facts from the logs matter for the trace:

- **The deferred hand event was consumed every time.** `Revalidate` refuses with `giver inventory busy` while the giver's inventory `IsProcessing()` (`moveitemtoplayer.c:204-208`), and `IsProcessing()` is true while a deferred event is queued (`DPI:4639-4642`). The same giver passed that check on every later request (ids 2, 3, 5, 6, 7, and onward), so each queued event left the queue.
- **Neither client completed the move locally.** Hands' item override traces every player-to-player location change on both sides behind the debug flag (`itembase.c:17-22`, `nimbyhands_server.c:418-427`, `:539-548`). No `side=client` line appeared, so the giver's client did not move the item either.

## 2. The dispatch path as 1.30 runs it

| Step | Where | What can stop it | Visible on retail |
| --- | --- | --- | --- |
| 1. `Man.ServerTakeToDst` -> `TakeToDstImpl(SERVER)` | `3_Game/DayZ/Entities/Man.c:757-768` | Nothing | - |
| 2. `HumanInventory.TakeToDst`: a live owner -> `RedirectToHandEvent`; source in hands -> `HandEvent(SERVER, HandEventMoveTo)`; the call's result is dropped | `INV/HumanInventory.c:243-251`, `:200-229` | Nothing at this step | - |
| 3. `DayZPlayerInventory.HandEvent`: refuses while processing, on `CanPerformEventEx` (`LocationCanMoveEntity`), or on reservation; otherwise queues a `DeferredHandEvent` | `DPI:4435-4475`; `INV/Hand_Events.c:306-319`, `:177-190`; on a multiplayer server `AddInventoryReservationEx` returns true without reserving (`INV/Inventory.c:716-721`) | All three are asked by `Revalidate` before dispatch (plan v0.5, signal 3). A refusal sends `SJ_INVENTORY_FAILURE` to the owner (`DPI:4461-4471`), whose handler shows nothing (`DPI:559-651`) | No |
| 4. Next inventory update: `HandleInventoryManipulation` -> `HandleHandEvent`, `SERVER` branch -> `SendServerHandEventViaJuncture` (live player, event not server-side-only) | `INV/HumanInventory.c:528-532`; `DPI:4623-4635`, `:4587-4615` | `ScriptInputUserData.CanStoreInputUserData()` false keeps it queued (native, `3_Game/DayZ/gameplay.c:130-131`) | No, but section 1 shows the queue emptied |
| 5. `SendServerHandEventViaJuncture`: `NeedInventoryJunctureFromServer` is always true in multiplayer, so it requests `AddInventoryJunctureEx(giver, item, dst, test_dst_occupancy=true, 5000)`; the result only selects a log line, and the juncture is sent either way | `DPI:4206-4277` (request `:4230`, send `:4257-4259`); `4_World/DayZ/Entities/ManBase/PlayerBase.c:5344-5356`; natives `3_Game/DayZ/Global/Game.c:741-762` | A denied juncture (native) | No (logs only under `ENABLE_LOGGING`) |
| 6. Playback: `PlayerBase.OnSyncJuncture` `SJ_INVENTORY` -> `OnInventoryJunctureFromServer` stores it -> `OnHandleStoredJunctureData` -> `ExecuteInventoryCommandServer` on the server, `ExecuteInventoryCommandClient` on the giver's client. Vanilla's own comment: "Juncture is only ever Server Req->Client Ack and Perform->Server Perform" | `PlayerBase.c:8496-8497`; `DPI:535-549`, `:657-688` (comment `:668`), `:3501-3567`, `:3569-3631` | When the engine plays it back on each side, and whether the server waits for the client, is native | No |
| 7a. Server: `ExecuteHandEventCommandServer`: `CheckRequestEx` -> `HandEventMoveTo.CheckRequest` -> native `GameInventory.CheckMoveToDstRequest(giver, src, dst, 2.5)`; `CanPerformEventEx`; then `HasInventoryJuncture(giver, item)` or return false; then `ProcessHandEvent` | `DPI:2047-2168` (`:2081`, `:2106`, `:2128-2152`, `:2167`); `INV/Hand_Events.c:95-103`, `:298-304`; `INV/Inventory.c:815-821` | Anti-cheat refusal (native), missing juncture | No |
| 7b. Giver's client: `ExecuteHandEventCommandClient`: clears the destination reservation, the same two checks, no juncture check, then `ProcessHandEvent` | `DPI:2170-2262` (`:2173`, `:2205`, `:2230`, `:2261`) | Same checks against the client's copy | No |
| 8. Hand state machine: `Equipped -MOVETO-> MovingTo` is the animated transition, guarded by `HandSelectAnimationOfMoveFromHandsEvent`; `SlotToAnimType` returns `HIDESHOW_SLOT_INVENTORY` for any cargo destination whose parent belongs to a player, so the guard passes. The non-animated `HandActionMoveTo` is used only when the guard fails. `ProcessHandEvent` returns false silently when no transition fits | `DPI:205-266` (`:238`); `INV/HandAnimated_Guards.c:1-119` (`:7-8`, `:98-117`), `:142-159`, `:206-240`; `INV/HumanInventoryWithFSM.c:62`, `:96-113` | No fitting transition | No |
| 9. Animated move: `HandAnimatedMovingToAtt` starts in `m_Hide`, which starts the `HumanCommandWeapons` HIDE action; the item moves only when the hide animation event moves the state to `m_Show`, whose `OnEntry` calls `LocationSyncMoveEntity`. `HandleInventory` turns a finished weapons action into an abort when the state is not waiting for one, and `m_Hide` aborts back to `Equipped` | `INV/HandAnimatedMovingToAtt.c:90-164`, `:26-88` (move `:58`); `INV/HandStartAction.c:15-48`; `DPI:447-525` (`:493-519`), `:242-243` | The hide never fires its event, or the weapons action finishes first | Only if `StartAction` is refused outright: `HandStartAction.c:37-38` raises `Error("HCW: NOT playing ...")`. None appeared |
| 10. `ItemBase.EEItemLocationChanged` on the server -> Hands `CONFIRMED` (signal 1) | `3_Game/DayZ/Entities/EntityAI.c:976`; `nimbyhands_server.c:429-465` | - | Yes, it never fired |

Vanilla's inventory logging cannot help on retail. The `-doLogs`, `-doInvHFSMLog`, `-doSyncLog`, and `-doInvMoveLog` launch parameters (`3_Game/DayZ/tools/Debug.c:718-737`) only enable calls that print through `hndDebugPrint` and `syncDebugPrint`, which compile to nothing without the `INV_DEBUG` and `LOG_TO_RPT` defines (`INV/HandFSM.c:1-8`, `INV/Debug.c:1-9`), or that sit inside `#ifdef ENABLE_LOGGING`. Any diagnostic has to be Hands' own logging.

## 3. Answers to the six questions

### 3.1. Does the server ever send the juncture?

Very probably yes; the diagnostic confirms it. `CanStoreInputUserData()` is native, and its comment ("the channel is free, AND the InputBuffer is NOT full", `3_Game/DayZ/gameplay.c:130`) describes a client input buffer; its value on a dedicated server cannot be read in script. Two pieces of evidence say it is true often enough:

- `HandleInventoryManipulation` clears `m_DeferredEvent` only inside the `CanStoreInputUserData()` branch (`DPI:4626-4633`). Section 1 shows the queue emptied after every request, and Hands pre-checks every queue-time refusal, so the event most likely went down the `SERVER` branch to `SendServerHandEventViaJuncture` (`DPI:4603-4605`).
- Vanilla depends on the same path. `ActionPlaceObject` (barrels, fireplaces, generators, fences; `UAC/ActionConstructor.c:111`, `4_World/DayZ/Entities/ItemBase/BaseBuildingBase/Fence.c:881`) has no progress (`UAC/Actions/Continuous/ActionPlaceObject.c:18-21`), so its item is still in hands when `MoveEntityToFinalPosition` calls `ServerTakeToDst` (`UAC/Actions/Continuous/DeployActions/ActionDeployBase.c:206-229`, `:225`). That works in vanilla multiplayer.

Correction to the brief (section 2, last bullet): the deploy precedent is weaker than it looked. Deploy actions with progress (tents and most kits) drop the item to the ground when the animation loop starts (`UAC/Actions/Continuous/DeployActions/ActionDeployObject.c:240-252` -> `DropDuringPlacing`, `ActionDeployBase.c:184-200`, a server `LocalDropEntity` plus `SyncDeferredEventToRemotes`), so their `ServerTakeToDst` moves a ground item and never touches the hand state machine. Only `ActionPlaceObject` moves a hand item, and its destination is the ground, which differs from ours in two ways: `SlotToAnimType` returns -1 for a ground destination (`INV/HandAnimated_Guards.c:10-22`), so the move is the instant `HandActionMoveTo` (`INV/HumanInventoryWithFSM.c:62`); and `OnFinishProgressServer` calls `ClearActionJuncture` before the move (`ActionDeployBase.c:70-79`, move at `:123`).

### 3.2. What does the giver's client do with the hand event?

It executes `ExecuteHandEventCommandClient` (`DPI:2170-2262`). Every condition that can drop it:

| Condition | Where | Applies to Hands |
| --- | --- | --- |
| `CheckRequestEx` -> native `CheckMoveToDstRequest(giver, src, dst, 2.5)`; skipped only for remotes | `DPI:2205`; `INV/Hand_Events.c:95-103`, `:298-304` | Yes; `Revalidate` allows the action's reach (`UAMaxDistances.DEFAULT` 2.0 m or `SMALL` 1.3 m, `UAC/Actions/ActionConstants.c:113-114`) plus 0.5 m, so the receiver was inside the 2.5 m radius; whether the native also refuses a live owner is unknown (H-C) |
| `CanPerformEventEx` -> `LocationCanMoveEntity` against the client's copy | `DPI:2230`; `INV/Inventory.c:373` | Revalidate asked it on the server; the client copy could differ, but the receiver was standing still |
| Reservations: `ExecuteHandEventCommandClient` clears only the destination's reservation (`DPI:2173`). The giver's own running action reserved the item in hands on the client when it started (`UAC/ActionBase.c:1053-1118`, hands `:1091-1104`; started from `UAC/ActionManagerClient.c:78-84`; refreshed `:1144-1165`) | Offer, Give, and Put In Pockets keep the default reservation; only Take overrides it (`actiontakeitemfromplayer.c` `InventoryReservation`) | Whether `LocationCanMoveEntity` honors that reservation is native. `ActionPlaceObject` holds the same client reservation and its server move works, so this ranks low |
| The hand state machine and the animated hide | Section 2, steps 8 and 9 | Yes (H-B) |
| Can a client address another player's inventory? | The location names the parent entity; the receiver's entities exist in the giver's network bubble. Vanilla clients do send hand moves into another player's cargo: the inventory screen drops a hand item into a body's cargo through `PredictiveTakeEntityToTargetCargoEx`, which switches to `JUNCTURE` (`5_Mission/DayZ/GUI/InventoryNew/Inherited/ContainerWithCargo.c:443`, `3_Game/DayZ/Entities/Man.c:576-588`), but the screen shows another player's inventory only when that player is dead, unconscious, or restrained (`5_Mission/DayZ/GUI/InventoryNew/Inherited/VicinityContainer.c:497-505`) | Yes for the receiver's entity; the live-player rule is the open question |

The client logs nothing on any of these branches on retail, which matches the October 20 client log.

### 3.3. How does vanilla move an item from one player's inventory into another's?

Only from the looter's side, only into or out of a player who is dead, unconscious, or restrained, and never between two live players.

- **Who initiates and in which mode.** The looter's client, in `JUNCTURE` mode, because `NeedInventoryJunctureFromServer` is true for every player in multiplayer (`PlayerBase.c:5344-5356`; `Man.c:738-747`).
- **Inventory-to-inventory (SYNC_MOVE).** The server validates the request in `ValidateSyncMoveServer` (`DPI:1136-1305`): `PlayerCheckRequestSrc` (`DPI:4644-4680`) and `PlayerCheckRequestDst` (`DPI:4682-4724`) refuse a source or destination whose root is another player who is alive and neither restrained nor unconscious (`:4652-4662`, `:4690-4700`); then `TryAcquireInventoryJunctureFromServer` (`INV/Junctures.c:1-48`). Both sides then run `ExecuteSyncMoveCommandServer` and `...Client`: juncture check, `LocationCanMoveEntity`, `LocationSyncMoveEntity`, no animation (`DPI:1307-1396`).
- **Hand item into a body.** The giver's own `JUNCTURE` hand event, animated, validated by `ValidateHandEventServer` (`DPI:1840-2045`). That validation does not call `PlayerCheckRequestDst`; its only gate on the receiver is the native `CheckMoveToDstRequest`.
- **Permission checks named in the brief.** `PlayerBase.IsInventoryVisible` returns true for every player (`PlayerBase.c:2248-2251`), and `CanManipulateInventory` only blocks the local player's own inventory while that player is restrained (`:2253-2260`). The cross-player rule lives in the vicinity screen (`VicinityContainer.c:497-505`) and in `PlayerCheckRequestSrc` and `PlayerCheckRequestDst`.
- **The server's own authoritative move.** `GameInventory.TakeToDst` in `SERVER` mode is a synchronous `LocationSyncMoveEntity` ("no anims involved", `INV/Inventory.c:562-567`) followed by `SendServerMove` to the clients (`INV/Inventory.c:1057-1066`; `INV/InventoryInputUserData.c:57-66`). `HumanInventory.TakeToDst` reaches it whenever neither end is in hands (`INV/HumanInventory.c:243-251`). It takes no juncture, runs no anti-cheat, plays no animation, and its return value is the real result. Vanilla uses it for non-hand sources, for example `UAC/Actions/Continuous/ActionUnpackGift.c:62` (gift cargo to the ground) and `4_World/DayZ/Classes/ConstructionBase.c:1858` (attachment to the ground).

The looting path is therefore a model only for the receiver-side variant, and that variant is refused by vanilla for a live giver (P4 below).

### 3.4. Candidate dispatch paths

See section 5.

### 3.5. What did the pre-CR-001 code do, and was it ever proven in game?

The verbatim import (`c69278e`, October 14, 16:18) and its prefixed copy (`7d07b7c`, 17:02) moved the item directly on the server: `RemoteObjectTreeDelete(item)`, `LocationCanMoveEntity`, `LocationSyncMoveEntity(src=HANDS, dst)`, then a `LOCAL` `HandEventDestroyed` on the giver to tell its hand state machine the item was gone, a `LOCAL` `HandEventCreated` when the destination was hands, a revert to the source or a drop to the ground when that was refused, and `RemoteObjectTreeCreate(item)` (`git show 7d07b7c:mods/own/nimby-pack/NimbyGiveTake/scripts/4_world/systems/inventory/moveitemtoplayer.c`, `Execute`, lines 93-184). Review CR-001 rated it a high-priority data-integrity defect: the giver's `HandEventDestroyed` can be refused after the native move succeeded, leaving the item in the receiver's hands with neither state machine aware of it, and the ground fallback ignores its own result.

It was never proven in game here. The rewrite (`4c28672`) landed at 17:42 the same day, and the first in-game run, the October 14 solo test, used `@NIMBY-20261014-175505` (17:55), which already carried the rewrite; that run played the offer animation with no second player (`docs/testing/2026-10-14-solo-ingame-test.md`). The upstream Workshop item still ships that method; whether it works on 1.30 is unknown, and its synthetic hand events and revert branches are exactly what the plan forbids (Phase 0 exit; Phase 1 "Never automatically retry, clone, restore, or delete").

### 3.6. NIMBY-T-012: vanilla notification options

| Option | Where | Fit |
| --- | --- | --- |
| `NotificationSystem.SendNotificationToPlayerExtended(player, show_time, title, detail, icon)`, from the server | `3_Game/DayZ/Client/Notifications/NotificationSystem.c:125-151`; the client handler `3_Game/DayZ/DayZGame.c:3192-3206` calls `AddNotificationExtended` (`NotificationSystem.c:215`) | Recommended. A toast in the same style as Hands' pocket notification, sent on vanilla's own RPC (`ERPCs.RPC_SEND_NOTIFICATION_EXTENDED`), so Hands needs no RPC id from its `65502xx` block. Sent by the server, so it reports the server's verdict |
| `NotificationSystem.SendNotificationToPlayer(player, NotificationType, ...)` | `NotificationSystem.c:160-184`; handler `DayZGame.c:3179-3190` | Uses vanilla's fixed notification types (`Notifications.json`); none says "transfer failed" |
| `PlayerBase.MessageStatus`, `MessageImportant`, `MessageAction`, `MessageFriendly`; on the server they go out through `ChatMP` | `PlayerBase.c:6911-6945` | A chat line. Works, but scrolls away and reads as chat |
| `ActionBase.SendMessageToClient` -> `RPC_USER_ACTION_MESSAGE` -> `MessageAction` | `UAC/ActionBase.c:1263-1271`; `PlayerBase.c:5773-5782` | A chat line again, and only from an action instance; the timeout reconcile runs outside any action |

Recommendation: on `REFUSED` and `UNCONFIRMED`, send one toast to each player from the server, about 4 s, with a short title ("Hands") and a plain-language detail built from the existing detail text ("Not transferred: no space in the receiver's inventory"; "Not transferred: the item is still in <giver>'s hands"). One toast per correlation id, at the terminal line, so the lifecycle's one-terminal-line rule also bounds the toasts. Whether `CONFIRMED` gets a toast is Jeff's call (J4); vanilla shows nothing for an ordinary inventory move. Check in game whether a `#STR_` key passed from the server is translated on the client before relying on string-table keys; Phase 2 already uses plain text for the input labels.

## 4. Root cause: ranked hypotheses

| Rank | Hypothesis | Mechanism | Evidence for | Evidence against or unknown |
| --- | --- | --- | --- | --- |
| 1 | H-B: the animated hide cannot complete while the giver is in an action | The hand event executes, enters `MovingTo`, starts the weapons HIDE action; the giver's running action command (offer loop `CMD_ACTIONMOD_GIVER`, Give `CMD_ACTIONMOD_DROPITEM_HANDS`, Put In Pockets full-body `CMD_ACTIONFB_INTERACT`) prevents the hide event; the weapons action reports finished, `HandleInventory` aborts, `m_Hide` returns to `Equipped`, nothing moves (`DPI:493-519`, `:242`) | Explains all three verbs alike, including Give and Put In Pockets where the giver is the actor. A player-owned cargo destination is always the animated branch (section 2, step 8), and no vanilla server hand move takes that branch: every one found targets the ground or a world object (`UAC/Actions/Continuous/DeployActions/ActionDeployBase.c:225`, `4_World/DayZ/Entities/ItemBase/TrapSpawnBase.c:820`, `4_World/DayZ/Entities/ItemBase/FireplaceBase/Fireplace.c:714`, the torch spawned on the ground in `4_World/DayZ/Classes/Recipes/Recipes/CraftTorch.c:55`, `:79`). Vanilla's client refuses a quickbar hand change while any action runs or the hand inventory is processing (`PlayerBase.c:4956-4960`) | Whether `HumanCommandWeapons.StartAction` plays during an action command is native. `ActionPlaceObject` runs its server hand event inside its own full-body action and works, so "during an action" alone is not fatal; only the animated branch is in question. No `HCW: NOT playing` error appeared, so if this is the cause the hide started and then finished or aborted |
| 2 | H-A: the dispatching action's target lock denies the hand event's juncture | Every action with a target locks it at server acceptance: `m_LockTargetOnUse = HasTarget()` (`UAC/ActionBase.c:89`, `HasTarget` true by default `:259-262`), `AddActionJuncture` -> `AddInventoryJunctureEx(actor, target, ..., 10000)` (`UAC/ActionManagerServer.c:168-170`, `UAC/ActionBase.c:1167-1203`, `:1192`), released at action end (`UAC/ActionManagerServer.c:215-230`). Take: L's action locks D, the item's root. Give and Put In Pockets: D's action locks L, the destination's root. The hand event's own juncture (`DPI:4230`) is requested under that lock; if denied, the server returns at "missing juncture" (`DPI:2128-2152`) | Vanilla clears the action juncture before every server move of a locked entity: `UAC/Actions/Continuous/DeployActions/ActionDeployBase.c:79` before `:225`, `UAC/Actions/Continuous/ActionPackGift.c:121`, `UAC/Actions/Continuous/ActionDigGardenPlot.c:132`. Hands never clears it before dispatch | Whether a juncture on a player blocks a juncture on an item inside that player, or on a destination inside it, is native. For Give and Put In Pockets the lock belongs to the same player (D) who requests the new juncture, which may be allowed; then H-A explains only Take |
| 3 | H-C: the native anti-cheat refuses a live receiver | `CheckMoveToDstRequest(giver, src, dst, 2.5)` runs at execution on both sides (`DPI:2081`, `:2205`) and may refuse a destination inside a live, conscious, unrestrained other player | Vanilla's script policy is exactly that rule (`DPI:4690-4700`, the UI at `VicinityContainer.c:497-505`), and the native sits in the "anti-cheats" group (`INV/Inventory.c:814-821`). If the native allowed it, any client could push a hand item into a live player's cargo, because `ValidateHandEventServer` never calls the script rule | `PlayerCheckRequestDst` applies the live-player rule in script and then calls the same native (`DPI:4711-4714`), which suggests the native checks something else, such as distance |
| 4 | H-D: the juncture is never played back | The engine never delivers `SJ_INVENTORY` to the giver's client or never performs it on the server | None specific; L dropped twice that session (`No packets`, 15:50:20 and 16:46:03), but not during the transfers | Native. Settled by one log line per side (DG3) |

These can combine. H-A and H-C stop the move before the hand state machine runs; H-B stops it inside. One diagnostic session separates them (section 7).

## 5. Candidate paths

Every path keeps the G5 registry, revalidation immediately before dispatch, `EEItemLocationChanged` on the server as the completion signal, and the plan's rule against retry, clone, restore, or delete.

| Path | What it does | Exists and works in vanilla | A server action can drive it | What G5 and the signals see | Loss or duplication on each failure branch | Feasible |
| --- | --- | --- | --- | --- | --- | --- |
| P1. As built | `ServerTakeToDst` on the giver, hands to the receiver's cargo, inside the action | No vanilla server hand move into a player's inventory | Yes | `REQUESTED`, then nothing; `UNCONFIRMED` at 5,000 ms | Observed on October 20: none, the item stays in hands. If the client completed while the server refused, the giver's client would show the item moved until the server corrects it; not observed | No (T-010) |
| P2. Dispatch outside the action | The action records the request; the server ends the giver's offer loop for Take (`RequestInterruptAction`, `UAC/ActionManagerServer.c:373-383`); when neither player has a running action (their target locks are then released, `UAC/ActionManagerServer.c:226`), revalidate and run the same `ServerTakeToDst` | Partly: the animated cross-player hand move is what vanilla's inventory screen does for a body (section 3.3), driven by the giver's client; vanilla clears action junctures before server moves (`ActionDeployBase.c:79`) | Yes | Same lifecycle; the registry holds the item in flight during the wait, so a second request is refused. Needs a defined wait cap (for example 3,000 ms) that ends `REFUSED` with "action did not end" | Same as P1: one native move or none. The wait adds a window where the giver can drop or swap the item; revalidation refuses that. No new failure branch moves the item | Unknown: yes if H-B or H-A is the cause and H-C is false |
| P3. Two steps through the giver's own inventory | Step 1: P2 timing, `ServerTakeToDst` from hands to a free location in the giver's own inventory (the commonest vanilla hand move). Step 2: on the server's `EEItemLocationChanged` into the giver's inventory, `ServerTakeToDst` from there to the receiver, which is the synchronous `GameInventory.TakeToDst(SERVER)` (`INV/Inventory.c:1057-1066`) | Step 1: yes for client-driven moves; server-driven is the same open question as P2. Step 2: yes, vanilla's server-authoritative move, used for non-hand sources (`ActionUnpackGift.c:62`) | Yes | Two stages under one id: step 1's signal moves the request on; step 2's return value and its `EEItemLocationChanged` (fired during the call) give `CONFIRMED` or `REFUSED` at once | Step 1 refused: item stays in hands (as P1). Step 1 done, step 2 refused (for example the receiver filled the slot): the item ends in the giver's own inventory, logged `REFUSED` with its location; nothing lost, nothing compensated. No branch creates or deletes an entity. Step 2 bypasses `CheckMoveToDstRequest`, so it is immune to H-C, and the live-player rule then rests entirely on Hands' consent checks | Likely; needs free space in the giver's inventory for the item in hands (J2) |
| P4. Receiver-side looting path | The receiver's client requests the giver's hand item into its own cargo | No: `PlayerCheckRequestSrc` refuses a source in a live, conscious, unrestrained player (`DPI:4652-4662`); a client can send hand events only for its own hands | No | - | - | No |
| P5. Giver-client-initiated juncture | The server tells the giver's client (a new RPC from the `65502xx` block) to issue `PredictiveTakeToDst` of its hand item into the receiver's cargo after the action ends; vanilla then validates it in `ValidateHandEventServer` | Yes for a body (section 3.3); for a live receiver it rests on the native `CheckMoveToDstRequest` (H-C) | Indirectly, through the giver's client | The server sees the juncture request and then signal 1; the request is client-originated, so consent can be checked only before the server sends the RPC | Same as P1. Adds the first Hands RPC and a client authority step | Unknown; same H-C dependency as P2, more moving parts |
| P6. Pre-CR-001 upstream method | Network tree delete and recreate, direct `LocationSyncMoveEntity` from hands, injected `LOCAL` hand events, revert or ground fallback | Not a vanilla pattern; vanilla uses `RemoteObjectTree*` only inside item-replacement lambdas (CR-001 evidence) | Yes | Synchronous result | CR-001: an item can end in the receiver's hands with neither state machine aware; the revert and ground branches are compensation | No, the plan forbids it |
| P7. Server `LOCAL` hand event plus sync to remotes | The `DropDuringPlacing` pattern (`ActionDeployBase.c:184-200`) applied to a cargo destination | Only for a drop, which is non-animated | Yes | - | A cargo destination takes the animated branch on the server alone while the owner's client is never told; desync | No |

Rejected without a row: dropping the item to the ground for the receiver to pick up (another player can take it; loss), and any spawn-and-delete scheme (forbidden).

## 6. Recommendation

1. **Run the diagnostic first (section 7).** Reading cannot settle H-A to H-D, because `AddInventoryJuncture`, `CheckMoveToDstRequest`, `HumanCommandWeapons.StartAction`, and juncture playback are native. The diagnostic is logging behind the existing debug flag plus a server-only variant selector; it changes no outcome in variant V0.
2. **If H-C is false:** build P2. It is one native move, keeps today's code shape, and needs only a change of timing: release the action's lock and leave the action before dispatch. For Take that means the server ends the giver's offer loop when the take executes (J3).
3. **If H-C is true:** a hand-sourced move into a live player is closed on 1.30. Build P3, whose second step is vanilla's synchronous server move and does not pass through the anti-cheat. Its cost is that the giver needs room in their own inventory for the item in hands; Jeff decides whether that is acceptable or whether Hands is rescoped (J2).
4. **NIMBY-T-012:** add the server-sent toast from section 3.6 to whichever path is built; it is independent of T-010 and can land first.
5. **Keep the lifecycle.** Whatever ships, `EEItemLocationChanged` on the server stays the only `CONFIRMED` evidence; P3 adds a synchronous verdict for its second step but not a new kind of evidence.

## 7. Diagnostic proposal (for Jeff's ruling; not built)

### 7.1. Shape

- A separate branch `diag/hands-t010` from `study/hands-t010`, never merged to `main`. Changes only under `mods/own/nimby-pack/NimbyHands/`.
- Every new line is a debug line, written only with `$profile:NimbyHands\debug`, in the plan's format: `[NIMBY][Hands][Debug] diag=<name> side=<server|client> t=<ms> id=<corr|-> ...`. Player names follow the shared rules (both ids, quoted and escaped).
- A server-only variant file, `$profile:NimbyHands\diag-variant`, holding `V0`, `V1`, `V2`, or `V3`, read at each dispatch so a block change needs no restart. Absent or unreadable means `V0`.
- Built with `Build-NimbyPack.ps1` (boot check included) by the implementing session; the test record names the exact `build\@NIMBY-<stamp>`.

### 7.2. What to log, where, on which side

| Line | Side | Where | Fields | Answers |
| --- | --- | --- | --- | --- |
| DG1 `diag=predispatch` | Server | `NimbyHandsMoveItemToPlayer.Execute`, immediately before `Dispatch` | `cmtd=` result of `GameInventory.CheckMoveToDstRequest(giver, src, dst, GameInventory.c_MaxItemDistanceRadius)`; `jItem=HasInventoryJunctureItem(item)`, `jGiverItem=HasInventoryJuncture(giver, item)`, `jGiver=HasInventoryJunctureItem(giver)`, `jReceiver=HasInventoryJunctureItem(receiver)`, `jDst=HasInventoryJunctureDestination(giver, dst)` (`Game.c:757-759`); running action class of each player (`GetActionManager().GetRunningAction()`), `GetCommandModifier_Action() != null`, `GetCommand_Action() != null`; `hcw=` presence of `GetCommandModifier_Weapons()` with `GetRunningAction()`, `GetRunningActionType()`, `IsActionFinished()`; hand state `GetCurrentStateID()`, `IsIdle()` (`INV/HumanInventoryWithFSM.c:169-171`), `IsProcessing()` | H-C directly (`cmtd=false`); the lock picture for H-A; the giver's command state for H-B |
| DG2 `diag=watch` | Server | A per-frame call-later (`CALL_CATEGORY_GAMEPLAY`, repeat) for 3,000 ms after dispatch, writing a line only when a value changes | `processing`, `idle`, `state`, `jGiverItem`, `hcw` running action and finished flag, each player's running action | Whether the queue emptied (step 4), whether the server ever held the juncture (H-A), whether the state machine left `Equipped` and came back without a location change (H-B). The state machine leaving `Equipped` on the server rules out H-A, H-C, and H-D for the server, because `ProcessHandEvent` runs only after all three checks (`DPI:2081-2167`) |
| DG3 `diag=juncture` | Server and both clients | `modded PlayerBase.OnSyncJuncture`, after `super`, only for `SJ_INVENTORY` and `SJ_INVENTORY_FAILURE` (`PlayerBase.c:8496-8500`); the context is not read | Player described, juncture name | H-D, and whether a queue-time refusal happened after all |
| DG4 `diag=watch` | Giver's client | Started by DG3's `SJ_INVENTORY` on the owning client; same fields and cadence as DG2, 3,000 ms | Same as DG2 | Whether the client's state machine moved, and whether its hide aborted |

The existing `signal=` trace stays as it is; it already covers both sides.

### 7.3. Variants

| Variant | Change, server only | Isolates |
| --- | --- | --- |
| V0 | None; today's dispatch | Baseline with the new lines |
| V1 | The Hands action calls `ClearActionJuncture(action_data)` immediately before `move.Execute()`, vanilla's deploy order (`ActionDeployBase.c:79`) | H-A |
| V2 | The action records the request; for Take the server ends the giver's offer loop (`RequestInterruptAction`); dispatch waits until neither player has a running action (cap 3,000 ms, then `REFUSED detail=action did not end`), then full revalidation and today's `ServerTakeToDst` | H-B (and H-A, because ending the action releases its lock) |
| V3 (optional, J5) | V2 timing, then P3: hands to the giver's own free location, then on the server's `EEItemLocationChanged` the server move to the receiver | P3 end to end |

### 7.4. In-game steps (Jeff runs both clients: desktop D, laptop L)

Setup, as on October 20: replica `D:\DayZServer`, instance 999, a profile such as `hands-t010-diag`; `NimbyHands\debug` on the server and on both clients; before starting, confirm no `DayZServer_x64` process is running (another seat may own the port). D carries the Wrench and has one free cargo slot (for V3); L wears the KameezShirt with free cargo. Write the clock time of every attempt on the run order sheet. After the session, copy the laptop's script log for every launch, not only the last one (the October 20 record lost the 16:08 to 16:30 launches).

| Block | Steps | Expected lines |
| --- | --- | --- |
| B0, vanilla control (runs on today's build, no diagnostic code) | D knocks L out (as in HD-019) or cuffs L (HD-020). D opens the inventory and drags the Wrench from hands into L's shirt cargo. Then D takes it back | Server and clients: `signal=EEItemLocationChanged.begin` naming `HANDS:...@<D> -> CARGO:KameezShirt_Orange@<L>`, then `OnInventoryExit`, `OnInventoryEnter`, `.end`. This proves the animated cross-player hand move works on 1.30 when the giver is idle, and records the vanilla signal order for P0-4a |
| B1, V0 | Three Takes (D offers, L takes), three Gives (L requests, D gives), three Put In Pockets (L opens pockets, D deposits) | DG1 to DG4 for each id; `UNCONFIRMED` as on October 20 |
| B2, V1 | Same nine attempts | As B1; any `CONFIRMED` points at H-A |
| B3, V2 | Same nine attempts. For Take, D's offer animation stops when L's take executes | `CONFIRMED` points at H-B (or H-A); `REFUSED detail=action did not end` is a V2 timing finding |
| B4, V3 (only if J5 is yes) | Three Takes, three Gives | Two location changes per id (hands to D's cargo, D's cargo to L's cargo), one `CONFIRMED` |

### 7.5. Reading the result

| Observation | Reading | Next |
| --- | --- | --- |
| B0 fails | The animated cross-player hand move does not work on 1.30 even in vanilla | P3 only |
| DG1 `cmtd=false` | H-C: the engine refuses a live receiver for a hand event | P3 |
| DG2 `processing` never drops | The deferred event never ran; section 3.1 is wrong | Re-study the queue |
| No DG3 `SJ_INVENTORY` on the server or the giver's client | H-D | Re-study playback |
| DG2 `jGiverItem` never true in V0, true in V1 | H-A | P2 |
| DG2 or DG4 `idle=false` then back to idle, the weapons action finished, no location change | H-B | P2 |
| V2 `CONFIRMED` for all three verbs | P2 works | Plan v0.7: build P2 with T-012 |
| V3 `CONFIRMED` | P3 works | Fallback held in reserve, or chosen under J2 |

## 8. Decisions for Jeff

| # | Decision | Options | Study default |
| --- | --- | --- | --- |
| J1 | Approve the diagnostic (section 7): branch `diag/hands-t010`, debug-flag lines, the server variant file, one two-client session | Yes / change / no | Yes |
| J2 | If H-C is confirmed (or B0 fails): accept P3, which needs room in the giver's inventory for the item in hands and briefly puts it there, or rescope Hands | P3 / rescope / stop | P3 |
| J3 | Take under P2: the giver's offer animation ends as soon as someone takes (the server interrupts it) | Accept / keep the loop running and find another way | Accept |
| J4 | NIMBY-T-012: a server-sent toast to both players on `REFUSED` and `UNCONFIRMED`; also on `CONFIRMED`? | Toast on failures only / also on success / chat line instead | Toast on failures only |
| J5 | Include V3 (two-step) in the diagnostic session | Yes / later | Yes, so one session covers both fixes |

NIMBY-T-010 and NIMBY-T-012 stay open until the chosen fix passes HD-001 to HD-003 with the evidence the test record names.
