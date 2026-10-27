// -----------------------------------------------------------------------------
// NimbyHandsServer - the server side of every transfer: the in-flight registry
// (G5), the lifecycle log lines, the completion signals, the P3 second step, the
// bounded timeout, and the on-screen toast (plan v0.7, P3; NIMBY-T-012, J4).
// Steps   : Step 1 (NimbyHandsMoveItemToPlayer.Execute) asks the holder's own hand
//           state machine to move the item from hands into the holder's cargo
//           (Man.ServerTakeToDst, Man.c:757-760 -> HumanInventory.TakeToDst
//           HumanInventory.c:243-251 -> RedirectToHandEvent :200-229). Its arrival
//           there is signal 1 below, which moves the request to STAGED and defers
//           step 2 by one tick (ScriptCallQueue.CallLater, 2_GameLib/DayZ/tools.c:61),
//           because the hand FSM is mid-transition inside that callback. Step 2
//           (NimbyHandsMoveItemToPlayer.RunStep2) is the same ServerTakeToDst with
//           neither end in hands, which is the synchronous server move
//           GameInventory.TakeToDst(SERVER): LocationSyncMoveEntity plus SendServerMove
//           (Inventory.c:1057-1066, :567). It runs no anti-cheat and takes no juncture,
//           so RevalidateStep2 is the only guard, and its return value is the result.
// Signals : 1. ItemBase.EEItemLocationChanged on the server (EntityAI.c:976): from
//              REQUESTED a landing in the giver's own cargo is STAGED; from STAGED a
//              landing with the receiver is CONFIRMED; anything else refuses.
//           2. OnInventoryExit/OnInventoryEnter (ItemBase.c:1222-1244) fire only from
//              inside signal 1 (EntityAI.c:980), so they are traced behind the debug
//              flag with a sequence number, not wired.
//           3. The holder's hand event result cannot be observed on 1.30 (the class is
//              an engine class; HumanInventory.c:228-229 drops the queue-time result),
//              so step 1's refusal conditions are asked before dispatch.
//           The bounded timeout is the fallback, not a signal: it reconciles where the
//           item is, logs CONFIRMED, REFUSED, or UNCONFIRMED, evicts, and never retries.
// Toast   : NotificationSystem.SendNotificationToPlayerExtended (NotificationSystem.c:125)
//           from the two terminal writers only, to the giver and the receiver, each
//           guarded for a null player and a null identity: the identity overload sends
//           to every player when the identity is null (NotificationSystem.c:141-150).
// Persists: Nothing. Every member is static server memory for the mission.
// -----------------------------------------------------------------------------
class NimbyHandsWorldTransfer : NimbyHandsTransfer
{
    // Weak references: a deleted item or a removed player reads as null, never dangling.
    PlayerBase giver;
    PlayerBase receiver;
    // The player whose action made the request; the only recipient of an unregistered
    // refusal's toast (no item, item in flight).
    PlayerBase requester;
    EntityAI item;
    string itemText;
    string giverText;
    string receiverText;
    // What step 2 needs after the move object has died (plan v0.7, decision H3).
    float reach;
    int destinationFlags;
    // Consent is a snapshot taken at the action's execute; the server ending the giver's
    // offer loop for Take is not a withdrawal (plan v0.7, decision H2).
    bool consent;
    string consentDetail;
    bool hasExpected;
    EntityAI expectedItem;
    string expectedGiverId;
    string expectedReceiverId;
    // The exact step 1 landing, recorded from signal 1: step 2 moves only from here.
    bool staged;
    int stagedType;
    EntityAI stagedParent;
    int stagedSlot;
    int stagedIdx;
    int stagedRow;
    int stagedCol;
}

class NimbyHandsServer
{
    protected static ref NimbyHandsTransfers s_Registry;
    // Depth of traced EEItemLocationChanged calls on the stack (debug ranking). A counter,
    // not a flag, so a nested location change cannot clear the outer trace.
    protected static int s_TraceDepth;
    protected static int s_Sequence;
    // Dev harness only (cost path B): step-2 refusals still to force. Server memory.
    protected static int s_ForcedStep2Refusals;

    static NimbyHandsTransfers Registry()
    {
        if (!s_Registry)
        {
            s_Registry = new NimbyHandsTransfers(NimbyHandsConstants.TRANSFER_TIMEOUT_MS);
        }
        return s_Registry;
    }

    static NimbyHandsWorldTransfer FindTransfer(int id)
    {
        if (!s_Registry)
        {
            return null;
        }
        return NimbyHandsWorldTransfer.Cast(s_Registry.FindById(id));
    }

    // ---- dev harness switch ----------------------------------------------------

    // Arms count forced step-2 refusals (cost path B). Only the dev harness calls this.
    static void ArmStep2Refusal(int count)
    {
        s_ForcedStep2Refusals = count;
    }

    static int ForcedStep2RefusalsArmed()
    {
        return s_ForcedStep2Refusals;
    }

    // Consumes one forced refusal. False when none is armed.
    static bool TakeForcedStep2Refusal()
    {
        if (s_ForcedStep2Refusals <= 0)
        {
            return false;
        }
        s_ForcedStep2Refusals = s_ForcedStep2Refusals - 1;
        return true;
    }

    // ---- descriptions -------------------------------------------------------

    static string DescribePlayer(Man player)
    {
        if (!player)
        {
            return "none";
        }
        return NimbyHandsLog.DescribeIdentity(player.GetIdentity());
    }

    static string DescribeItem(EntityAI item)
    {
        if (!item)
        {
            return "none";
        }
        return item.GetType() + " #" + item.GetNetworkIDString();
    }

    static string ItemKey(EntityAI item)
    {
        if (!item)
        {
            return "";
        }
        return item.GetNetworkIDString();
    }

    // <type>:<parent type>@<owner>, where the owner carries both ids (shared rules 4.2) or
    // reads "world" for a location outside any player.
    static string DescribeLocation(InventoryLocation loc)
    {
        if (!loc)
        {
            return "none";
        }
        string type = typename.EnumToString(InventoryLocationType, loc.GetType());
        EntityAI parent = loc.GetParent();
        if (!parent)
        {
            return type;
        }
        Man root = parent.GetHierarchyRootPlayer();
        string owner = "world";
        if (root)
        {
            owner = NimbyHandsLog.DescribeIdentity(root.GetIdentity());
        }
        return type + ":" + parent.GetType() + "@" + owner;
    }

    static string DescribeItemLocation(EntityAI item)
    {
        if (!item)
        {
            return "none";
        }
        InventoryLocation loc = new InventoryLocation;
        if (!item.GetInventory().GetCurrentInventoryLocation(loc))
        {
            return "unknown";
        }
        return DescribeLocation(loc);
    }

    // "item with giver at ...", "item with receiver at ...", "item at ...", or "item no longer
    // exists". The toast reads the first words (NimbyHandsRules.LocationHint).
    static string DescribeWhere(NimbyHandsWorldTransfer transfer)
    {
        if (!transfer.item)
        {
            return "item no longer exists";
        }
        Man root = transfer.item.GetHierarchyRootPlayer();
        string where = DescribeItemLocation(transfer.item);
        if (root && transfer.giver && root == transfer.giver)
        {
            return "item with giver at " + where;
        }
        if (root && transfer.receiver && root == transfer.receiver)
        {
            return "item with receiver at " + where;
        }
        return "item at " + where;
    }

    // The item's root is the giver and the item is not in the giver's hands.
    static bool ItemInGiverInventory(NimbyHandsWorldTransfer transfer)
    {
        if (!transfer.item || !transfer.giver)
        {
            return false;
        }
        if (transfer.item.GetHierarchyRootPlayer() != transfer.giver)
        {
            return false;
        }
        InventoryLocation loc = new InventoryLocation;
        if (!transfer.item.GetInventory().GetCurrentInventoryLocation(loc))
        {
            return false;
        }
        return loc.GetType() != InventoryLocationType.HANDS;
    }

    protected static int NextSequence()
    {
        s_Sequence = s_Sequence + 1;
        return s_Sequence;
    }

    // ---- lifecycle lines ----------------------------------------------------

    // [NIMBY][Hands] id=<corr> action=<class> state=<STATE> giver=... receiver=... item=... detail=...
    // The description strings are captured when the request is registered, so the terminal line
    // still names the players and the item after a disconnect or a deletion.
    static void LogLifecycle(NimbyHandsWorldTransfer transfer)
    {
        string line = "id=" + transfer.id.ToString() + " action=" + transfer.action;
        line = line + " state=" + NimbyHandsState.Name(transfer.state);
        line = line + " giver=" + transfer.giverText + " receiver=" + transfer.receiverText;
        line = line + " item=" + transfer.itemText + " detail=" + transfer.detail;
        NimbyHandsLog.Info(line);
    }

    static void Describe(NimbyHandsWorldTransfer transfer)
    {
        transfer.giverText = DescribePlayer(transfer.giver);
        transfer.receiverText = DescribePlayer(transfer.receiver);
        transfer.itemText = DescribeItem(transfer.item);
    }

    // A refusal that never entered the registry (no item, or the item already in flight)
    // still gets its own correlation id, exactly one line, and a toast to the requester.
    static void LogUnregisteredRefusal(NimbyHandsWorldTransfer transfer, string detail)
    {
        transfer.id = Registry().NewId();
        transfer.state = NimbyHandsState.REFUSED;
        transfer.detail = detail;
        LogLifecycle(transfer);
        SendToast(transfer.requester, "requester", transfer.state, transfer.detail);
    }

    // Moves the transfer and writes its line. Returns false when the lifecycle forbids the
    // move (it is already terminal); nothing is logged then, so each id has one terminal line.
    // A terminal line also sends the toast to both players (NIMBY-T-012, J4).
    static bool Advance(NimbyHandsWorldTransfer transfer, int state, string detail)
    {
        if (!Registry().Move(transfer, state, detail))
        {
            NimbyHandsLog.Debug("ignored move id=" + transfer.id.ToString() + " from=" + NimbyHandsState.Name(transfer.state) + " to=" + NimbyHandsState.Name(state) + " detail=" + detail);
            return false;
        }
        LogLifecycle(transfer);
        if (NimbyHandsState.IsTerminal(state))
        {
            SendToast(transfer.giver, "giver", transfer.state, transfer.detail);
            SendToast(transfer.receiver, "receiver", transfer.state, transfer.detail);
        }
        return true;
    }

    // [NIMBY][Hands] pockets player=... open=<bool> reason=<toggle|unconscious|restrained|respawn|reconnect>
    // A consent change, so an audit record (shared rules 4.3), not a per-toggle debug line.
    static void LogPockets(Man player, bool open, string reason)
    {
        NimbyHandsLog.Info("pockets player=" + DescribePlayer(player) + " open=" + NimbyHandsLog.Bool(open) + " reason=" + reason);
    }

    // ---- the toast ------------------------------------------------------------

    protected static void SendToast(PlayerBase player, string role, int state, string detail)
    {
        if (!player || !player.GetIdentity())
        {
            // A missing identity must never reach the identity overload: it would broadcast.
            return;
        }
        string text = NimbyHandsRules.ToastText(state, detail, role);
        NotificationSystem.SendNotificationToPlayerExtended(player, NimbyHandsConstants.TOAST_SECONDS, NimbyHandsConstants.TOAST_TITLE, text, "");
    }

    // ---- dispatch -------------------------------------------------------------

    // Hands the request to the holder's own hand state machine (Man.c:757). The return value
    // only says the request was routed; see signal 3 in the header.
    static bool Dispatch(NimbyHandsWorldTransfer transfer, InventoryLocation src, InventoryLocation dst)
    {
        return transfer.giver.ServerTakeToDst(src, dst);
    }

    static void ScheduleTimeout(NimbyHandsWorldTransfer transfer)
    {
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyHandsServer.OnTimeout, Registry().TimeoutMs(), false, transfer.id);
    }

    // Step 2 runs one tick after the step 1 signal, never inside it. The id lookup makes a
    // call for an evicted request harmless.
    static void ScheduleStep2(NimbyHandsWorldTransfer transfer)
    {
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyHandsServer.OnStep2, 0, false, transfer.id);
    }

    static void OnStep2(int id)
    {
        NimbyHandsWorldTransfer transfer = FindTransfer(id);
        if (!transfer || transfer.state != NimbyHandsState.STAGED)
        {
            NimbyHandsLog.Debug("step 2 skipped id=" + id.ToString() + " (request no longer staged)");
            return;
        }
        NimbyHandsMoveItemToPlayer.RunStep2(transfer);
    }

    // ---- the staged location --------------------------------------------------

    static void RecordStaged(NimbyHandsWorldTransfer transfer, InventoryLocation loc)
    {
        transfer.staged = true;
        transfer.stagedType = loc.GetType();
        transfer.stagedParent = loc.GetParent();
        transfer.stagedSlot = loc.GetSlot();
        transfer.stagedIdx = loc.GetIdx();
        transfer.stagedRow = loc.GetRow();
        transfer.stagedCol = loc.GetCol();
    }

    // The location is exactly the one step 1 landed in: type, parent, slot, and cargo cell.
    static bool IsAtStagedLocation(NimbyHandsWorldTransfer transfer, InventoryLocation loc)
    {
        if (!transfer.staged || !loc || !transfer.stagedParent)
        {
            return false;
        }
        if (loc.GetType() != transfer.stagedType || loc.GetParent() != transfer.stagedParent)
        {
            return false;
        }
        if (loc.GetSlot() != transfer.stagedSlot || loc.GetIdx() != transfer.stagedIdx)
        {
            return false;
        }
        return loc.GetRow() == transfer.stagedRow && loc.GetCol() == transfer.stagedCol;
    }

    // ---- signal 1: the item's location changed --------------------------------

    protected static bool ShouldTrace(EntityAI item, InventoryLocation oldLoc, InventoryLocation newLoc)
    {
        if (!NimbyHandsLog.IsDebug())
        {
            return false;
        }
        if (IsPlayerToPlayer(oldLoc, newLoc))
        {
            return true;
        }
        // Any change of an item with a registry entry, so step 1 (same root) leaves a line.
        return s_Registry && s_Registry.Find(ItemKey(item)) != null;
    }

    static void OnItemLocationChangedBegin(EntityAI item, InventoryLocation oldLoc, InventoryLocation newLoc)
    {
        if (!ShouldTrace(item, oldLoc, newLoc))
        {
            return;
        }
        s_TraceDepth = s_TraceDepth + 1;
        TraceSignal("EEItemLocationChanged.begin", item, DescribeLocation(oldLoc) + " -> " + DescribeLocation(newLoc));
    }

    static void OnItemLocationChangedEnd(EntityAI item, InventoryLocation oldLoc, InventoryLocation newLoc)
    {
        if (s_TraceDepth > 0 && ShouldTrace(item, oldLoc, newLoc))
        {
            TraceSignal("EEItemLocationChanged.end", item, DescribeLocation(oldLoc) + " -> " + DescribeLocation(newLoc));
            s_TraceDepth = s_TraceDepth - 1;
        }
        if (!GetGame().IsServer() || !s_Registry || s_Registry.Count() == 0)
        {
            return;
        }
        NimbyHandsWorldTransfer transfer = NimbyHandsWorldTransfer.Cast(s_Registry.Find(ItemKey(item)));
        if (!transfer)
        {
            return;
        }
        if (transfer.state != NimbyHandsState.REQUESTED && transfer.state != NimbyHandsState.STAGED)
        {
            return;
        }
        Man newRoot = null;
        if (newLoc.GetParent())
        {
            newRoot = newLoc.GetParent().GetHierarchyRootPlayer();
        }
        bool atReceiver = newRoot != null && transfer.receiver != null && newRoot == transfer.receiver;
        bool inGiverHands = newLoc.GetType() == InventoryLocationType.HANDS && newLoc.GetParent() == transfer.giver;
        bool atGiverInventory = newRoot != null && transfer.giver != null && newRoot == transfer.giver && !inGiverHands;
        bool isTemporary = newLoc.GetType() == InventoryLocationType.TEMP;
        int from = transfer.state;
        int verdict = NimbyHandsRules.ClassifyMove(from, atReceiver, atGiverInventory, inGiverHands, isTemporary);
        if (verdict == NimbyHandsRules.NO_CHANGE)
        {
            NimbyHandsLog.Debug("id=" + transfer.id.ToString() + " location change ignored new=" + DescribeLocation(newLoc));
            return;
        }
        string detail = "signal=EEItemLocationChanged dst=" + DescribeLocation(newLoc);
        if (verdict == NimbyHandsState.STAGED)
        {
            RecordStaged(transfer, newLoc);
            if (Advance(transfer, verdict, detail))
            {
                ScheduleStep2(transfer);
            }
            return;
        }
        if (verdict == NimbyHandsState.REFUSED)
        {
            if (from == NimbyHandsState.STAGED)
            {
                detail = "item left the staged location before delivery; " + detail;
            }
            else
            {
                detail = "item left the giver for another place before delivery; " + detail;
            }
        }
        Advance(transfer, verdict, detail);
    }

    static void OnInventoryExitSignal(EntityAI item, Man player)
    {
        if (s_TraceDepth > 0)
        {
            TraceSignal("OnInventoryExit", item, "player=" + DescribePlayer(player));
        }
    }

    static void OnInventoryEnterSignal(EntityAI item, Man player)
    {
        if (s_TraceDepth > 0)
        {
            TraceSignal("OnInventoryEnter", item, "player=" + DescribePlayer(player));
        }
    }

    // ---- the bounded timeout ----------------------------------------------------

    static void OnTimeout(int id)
    {
        if (!s_Registry)
        {
            return;
        }
        NimbyHandsWorldTransfer transfer = NimbyHandsWorldTransfer.Cast(s_Registry.FindById(id));
        if (transfer)
        {
            Reconcile(transfer);
        }
        ReconcileOverdue();
    }

    // Anything past its observation period (a timer lost to a mission event) is reconciled,
    // so no entry can block its item for the rest of the mission. Execute calls this before
    // it refuses a request as "item in flight".
    static void ReconcileOverdue()
    {
        if (!s_Registry || s_Registry.Count() == 0)
        {
            return;
        }
        array<ref NimbyHandsTransfer> due = new array<ref NimbyHandsTransfer>;
        s_Registry.Due(GetGame().GetTime(), due);
        foreach (NimbyHandsTransfer entry : due)
        {
            Reconcile(NimbyHandsWorldTransfer.Cast(entry));
        }
    }

    protected static void Reconcile(NimbyHandsWorldTransfer transfer)
    {
        if (!transfer)
        {
            return;
        }
        bool exists = transfer.item != null;
        bool atReceiver = false;
        if (exists)
        {
            Man root = transfer.item.GetHierarchyRootPlayer();
            atReceiver = root != null && transfer.receiver != null && root == transfer.receiver;
        }
        bool atGiverInventory = ItemInGiverInventory(transfer);
        string where = DescribeWhere(transfer);
        int verdict = NimbyHandsRules.ClassifyTimeout(exists, atReceiver, atGiverInventory);
        if (transfer.state == NimbyHandsState.PENDING)
        {
            // Never submitted; only a crash between Begin and dispatch can leave one behind.
            verdict = NimbyHandsState.REFUSED;
        }
        string timeoutText = Registry().TimeoutMs().ToString() + " ms; ";
        string detail = "reconciled at timeout; " + where;
        if (verdict == NimbyHandsState.REFUSED)
        {
            detail = "no delivery within " + timeoutText + where;
        }
        if (verdict == NimbyHandsState.UNCONFIRMED)
        {
            detail = "no completion signal within " + timeoutText + where;
        }
        Advance(transfer, verdict, detail);
    }

    // ---- debug ranking helpers --------------------------------------------------

    static bool IsPlayerToPlayer(InventoryLocation oldLoc, InventoryLocation newLoc)
    {
        if (!oldLoc || !newLoc || !oldLoc.GetParent() || !newLoc.GetParent())
        {
            return false;
        }
        Man oldRoot = oldLoc.GetParent().GetHierarchyRootPlayer();
        Man newRoot = newLoc.GetParent().GetHierarchyRootPlayer();
        return oldRoot != null && newRoot != null && oldRoot != newRoot;
    }

    protected static void TraceSignal(string signal, EntityAI item, string detail)
    {
        if (!NimbyHandsLog.IsDebug())
        {
            return;
        }
        string side = "client";
        if (GetGame().IsServer())
        {
            side = "server";
        }
        string id = "-";
        if (s_Registry && item)
        {
            NimbyHandsTransfer transfer = s_Registry.Find(ItemKey(item));
            if (transfer)
            {
                id = transfer.id.ToString();
            }
        }
        string line = "signal=" + signal + " side=" + side + " seq=" + NextSequence().ToString();
        line = line + " t=" + GetGame().GetTime().ToString() + " id=" + id;
        line = line + " item=" + DescribeItem(item) + " " + detail;
        NimbyHandsLog.Debug(line);
    }
}
