// -----------------------------------------------------------------------------
// NimbyHandsServer - the server side of every transfer: the in-flight registry
// (G5), the lifecycle log lines, and the completion signals (plan v0.5, Phase 1).
// Signals : Wired, strongest first (Phase 0 item 4, ranked from the 1.30 reference):
//           1. ItemBase.EEItemLocationChanged on the server (EntityAI.c:976) moves a
//              REQUESTED transfer to CONFIRMED when the new location's root player is
//              the receiver, or to REFUSED when the item went anywhere else.
//           2. OnInventoryExit/OnInventoryEnter (ItemBase.c:1222-1244) fire only from
//              inside signal 1 (EntityAI.c:980), so they are traced behind the debug
//              flag with a sequence number for the in-game ranking, not wired.
//           3. The holder's hand event result cannot be observed on 1.30:
//              DayZPlayerInventory is an engine class that cannot be modded (boot
//              check build-20261019-152106), RedirectToHandEvent drops the queue-time
//              result (HumanInventory.c:228-229), and vanilla discards the execution
//              result (DayZPlayerInventory.c:1830). ServerTakeToDst returning true is
//              therefore not proof of anything. Its refusal conditions are checked
//              before dispatch instead (NimbyHandsMoveItemToPlayer.Revalidate): the busy
//              inventory (DayZPlayerInventory.c:4437), the engine move test the
//              event itself runs (Hand_Events.c:306-319), and the reservation test
//              (Hand_Events.c:185). Only a reservation taken after that check
//              (DayZPlayerInventory.c:4449) stays invisible; it ends UNCONFIRMED.
//           The bounded timeout is the fallback, not a signal: it reconciles where the
//           item is, logs CONFIRMED or UNCONFIRMED, evicts, and never retries.
// Persists: Nothing. Every member is static server memory for the mission.
// -----------------------------------------------------------------------------
class NimbyHandsWorldTransfer : NimbyHandsTransfer
{
    Man giver;
    PlayerBase receiver;
    EntityAI item;
    string itemText;
    string giverText;
    string receiverText;
    // Diagnostic only (diag/hands-t010, NimbyHandsDiag): the variant read at dispatch, the
    // V3 stage (1 = hands to the giver's own inventory in flight, 2 = server move to the
    // receiver scheduled), and the action's reach for the V3 step 2 revalidation.
    int diagVariant;
    int diagStage;
    float diagReach;
}

class NimbyHandsServer
{
    protected static ref NimbyHandsTransfers s_Registry;
    // True while a player-to-player EEItemLocationChanged is on the stack (debug ranking).
    protected static bool s_TracingMove;
    protected static int s_Sequence;

    static NimbyHandsTransfers Registry()
    {
        if (!s_Registry)
        {
            s_Registry = new NimbyHandsTransfers(NimbyHandsConstants.TRANSFER_TIMEOUT_MS);
        }
        return s_Registry;
    }

    // Diagnostic lookups (NimbyHandsDiag).
    static NimbyHandsWorldTransfer FindTransfer(int id)
    {
        if (!s_Registry)
        {
            return null;
        }
        return NimbyHandsWorldTransfer.Cast(s_Registry.FindById(id));
    }

    static int FindIdByItem(EntityAI item)
    {
        if (!s_Registry || !item)
        {
            return 0;
        }
        NimbyHandsTransfer transfer = s_Registry.Find(ItemKey(item));
        if (!transfer)
        {
            return 0;
        }
        return transfer.id;
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
        if (root && root.GetIdentity())
        {
            owner = root.GetIdentity().GetId();
        }
        return type + ":" + parent.GetType() + "@" + owner;
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
    // still gets its own correlation id and exactly one line.
    static void LogUnregisteredRefusal(NimbyHandsWorldTransfer transfer, string detail)
    {
        transfer.id = Registry().NewId();
        transfer.state = NimbyHandsState.REFUSED;
        transfer.detail = detail;
        LogLifecycle(transfer);
    }

    // Moves the transfer and writes its line. Returns false when the lifecycle forbids the
    // move (it is already terminal); nothing is logged then, so each id has one terminal line.
    static bool Advance(NimbyHandsWorldTransfer transfer, int state, string detail)
    {
        if (!Registry().Move(transfer, state, detail))
        {
            NimbyHandsLog.Debug("ignored move id=" + transfer.id.ToString() + " from=" + NimbyHandsState.Name(transfer.state) + " to=" + NimbyHandsState.Name(state) + " detail=" + detail);
            return false;
        }
        LogLifecycle(transfer);
        return true;
    }

    // [NIMBY][Hands] pockets player=... open=<bool> reason=<toggle|unconscious|restrained|respawn|reconnect>
    // A consent change, so an audit record (shared rules 4.3), not a per-toggle debug line.
    static void LogPockets(Man player, bool open, string reason)
    {
        NimbyHandsLog.Info("pockets player=" + DescribePlayer(player) + " open=" + NimbyHandsLog.Bool(open) + " reason=" + reason);
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

    // ---- signal 1: the item's location changed --------------------------------

    static void OnItemLocationChangedBegin(EntityAI item, InventoryLocation oldLoc, InventoryLocation newLoc)
    {
        s_TracingMove = false;
        if (!NimbyHandsLog.IsDebug() || !IsPlayerToPlayer(oldLoc, newLoc))
        {
            return;
        }
        s_TracingMove = true;
        TraceSignal("EEItemLocationChanged.begin", item, DescribeLocation(oldLoc) + " -> " + DescribeLocation(newLoc));
    }

    static void OnItemLocationChangedEnd(EntityAI item, InventoryLocation oldLoc, InventoryLocation newLoc)
    {
        if (s_TracingMove)
        {
            TraceSignal("EEItemLocationChanged.end", item, DescribeLocation(oldLoc) + " -> " + DescribeLocation(newLoc));
            s_TracingMove = false;
        }
        if (!GetGame().IsServer() || !s_Registry || s_Registry.Count() == 0)
        {
            return;
        }
        NimbyHandsWorldTransfer transfer = NimbyHandsWorldTransfer.Cast(s_Registry.Find(ItemKey(item)));
        if (!transfer || transfer.state != NimbyHandsState.REQUESTED)
        {
            return;
        }
        Man newRoot = null;
        if (newLoc.GetParent())
        {
            newRoot = newLoc.GetParent().GetHierarchyRootPlayer();
        }
        bool atReceiver = newRoot != null && newRoot == transfer.receiver;
        bool inGiverHands = newLoc.GetType() == InventoryLocationType.HANDS && newLoc.GetParent() == transfer.giver;
        bool isTemporary = newLoc.GetType() == InventoryLocationType.TEMP;
        // Diagnostic V3: step 1 landed in the giver's own inventory; schedule step 2 for the
        // next frame instead of classifying this as the item going elsewhere.
        if (transfer.diagStage == 1 && newRoot != null && newRoot == transfer.giver && !inGiverHands && !isTemporary)
        {
            transfer.diagStage = 2;
            NimbyHandsDiag.Line("step1", transfer.id.ToString(), "result=arrived dst=" + DescribeLocation(newLoc));
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyHandsDiag.RunStep2, 0, false, transfer.id);
            return;
        }
        int verdict = NimbyHandsRules.ClassifyMove(atReceiver, inGiverHands, isTemporary);
        if (verdict == NimbyHandsRules.NO_CHANGE)
        {
            NimbyHandsLog.Debug("id=" + transfer.id.ToString() + " location change ignored new=" + DescribeLocation(newLoc));
            return;
        }
        string detail = "signal=EEItemLocationChanged dst=" + DescribeLocation(newLoc);
        if (verdict == NimbyHandsState.REFUSED)
        {
            detail = "item left the giver for another place before delivery; " + detail;
        }
        Advance(transfer, verdict, detail);
    }

    static void OnInventoryExitSignal(EntityAI item, Man player)
    {
        if (s_TracingMove)
        {
            TraceSignal("OnInventoryExit", item, "player=" + DescribePlayer(player));
        }
    }

    static void OnInventoryEnterSignal(EntityAI item, Man player)
    {
        if (s_TracingMove)
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
        // Anything else past its observation period (a timer lost to a mission event) is
        // reconciled now too, so no entry can block its item for the rest of the mission.
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
        string where = "item no longer exists";
        if (exists)
        {
            Man root = transfer.item.GetHierarchyRootPlayer();
            atReceiver = root != null && root == transfer.receiver;
            InventoryLocation loc = new InventoryLocation;
            transfer.item.GetInventory().GetCurrentInventoryLocation(loc);
            where = "item at " + DescribeLocation(loc);
        }
        int verdict = NimbyHandsRules.ClassifyTimeout(exists, atReceiver);
        if (transfer.state == NimbyHandsState.PENDING)
        {
            // Never submitted; only a crash between Begin and dispatch can leave one behind.
            verdict = NimbyHandsState.REFUSED;
        }
        string detail = "reconciled at timeout; " + where;
        if (verdict == NimbyHandsState.UNCONFIRMED)
        {
            detail = "no completion signal within " + Registry().TimeoutMs().ToString() + " ms; " + where;
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
