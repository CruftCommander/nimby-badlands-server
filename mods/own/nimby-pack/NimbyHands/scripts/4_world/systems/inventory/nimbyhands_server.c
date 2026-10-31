// -----------------------------------------------------------------------------
// NimbyHandsServer - the server side of every transfer: the in-flight registry
// (G5), the lifecycle log lines, the completion signals, the P3 second step, the
// bounded timeout, and the on-screen toast (plan v0.7, P3; NIMBY-T-012, J4).
// Steps   : Step 1 for a connected giver, since October 31, 2026 (NIMBY-T-063): the server sends
//           NimbyHandsRpc.STAGE to the giver's client, which posts PredictiveTakeToDst from hands
//           into its own cargo (see "the client-driven step 1" below); the landing is signal 1 as
//           for the server path. Step 1 for a dummy giver (dev harness; the path every connected
//           giver took through October 30, crashes 1 to 10):
//           (NimbyHandsMoveItemToPlayer.Poll -> Submit -> Dispatch, run from the
//           giver's own CommandHandler for a connected giver since build 10, October 30,
//           2026, NIMBY-T-063 probe) asks the holder's own hand state machine to move
//           the item from hands into the holder's cargo
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
//           Sent through the call queue, never from inside the writer: a terminal state
//           is often written inside EEItemLocationChanged, that is, inside the engine's
//           own move (NIMBY-T-063, October 29, 2026).
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
    // The staged location as step 1's signal described it (type:parent@owner) and the
    // parent entity it landed in. The cargo cell is not recorded: step 2 moves from the
    // item's current location, and the two October 28 crashes sat on the cell compare.
    string stagedText;
    EntityAI stagedParent;
    // GetGame().GetTime() when the STAGED signal was read; the step 2 poll measures from it.
    int stagedAtMs;
    // Identity ids captured at registration; the terminal toast resolves its targets from the
    // engine's player list by these, never through the stored references (see MoveStep2).
    string giverId;
    string receiverId;
}

class NimbyHandsServer
{
    protected static ref NimbyHandsTransfers s_Registry;
    // Depth of traced EEItemLocationChanged calls on the stack (debug ranking). A counter,
    // not a flag, so a nested location change cannot clear the outer trace.
    protected static int s_TraceDepth;
    protected static int s_Sequence;
    // The step 2 move in flight (NIMBY-T-063, October 29, 2026): while s_Step2MoveId is set,
    // every inventory signal handler returns at once and the end handler only raises
    // s_Step2Landed. Crash 8 (crash_2026-10-29_19-42-55) died inside the engine's move callback
    // on a pure-script static call in the CONFIRMED bookkeeping; MoveStep2 does that bookkeeping
    // after LocationSyncMoveEntity returns, outside the callback.
    protected static int s_Step2MoveId;
    protected static bool s_Step2Landed;
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

    // Dev harness only (NIMBY-T-063 bot case): while on, a party with no identity (a spawned
    // dummy) passes the party check as connected, so the boot check can run a real two-step
    // transfer between two dummies with no client. Off by default; the harness turns it on for
    // that one case and off again after it. A real player always has an identity, so this
    // changes nothing for a connected player.
    protected static bool s_DummyPartiesAllowed;

    static void AllowDummyParties(bool allow)
    {
        s_DummyPartiesAllowed = allow;
    }

    static bool DummyPartiesAllowed()
    {
        return s_DummyPartiesAllowed;
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
        transfer.giverId = NimbyHandsActionData.IdentityId(transfer.giver);
        transfer.receiverId = NimbyHandsActionData.IdentityId(transfer.receiver);
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
        SendToastTo(transfer.requester, "requester", transfer.state, transfer.detail);
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
            // On the next call-queue pass, not here: CONFIRMED and the signal refusals are
            // written inside EEItemLocationChanged, inside the native move. An RPC from there
            // has never run to completion (NIMBY-T-063). A deleted player reads as null then.
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyHandsServer.SendToast, 0, false, transfer.giverId, "giver", transfer.state, transfer.detail);
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyHandsServer.SendToast, 0, false, transfer.receiverId, "receiver", transfer.state, transfer.detail);
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

    // The terminal toast: the target is resolved from the engine's player list by the identity
    // id captured at registration (NIMBY-T-063, October 29, 2026: the stored giver reference is
    // not handed to the engine after the hand event; a player who left is simply not found).
    static void SendToast(string identityId, string role, int state, string detail)
    {
        PlayerBase player = FindPlayerByIdentityId(identityId);
        NimbyHandsLog.Debug("toast role=" + role + " target=" + DescribePlayer(player));
        SendToastTo(player, role, state, detail);
    }

    static void SendToastTo(PlayerBase player, string role, int state, string detail)
    {
        if (!player || !player.GetIdentity())
        {
            // A missing identity must never reach the identity overload: it would broadcast.
            return;
        }
        string text = NimbyHandsRules.ToastText(state, detail, role);
        NotificationSystem.SendNotificationToPlayerExtended(player, NimbyHandsConstants.TOAST_SECONDS, NimbyHandsConstants.TOAST_TITLE, text, "");
    }

    // The connected player with this identity id, from GetGame().GetPlayers, or null.
    static PlayerBase FindPlayerByIdentityId(string identityId)
    {
        if (identityId == "")
        {
            return null;
        }
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        foreach (Man man : players)
        {
            if (man && man.GetIdentity() && man.GetIdentity().GetId() == identityId)
            {
                return PlayerBase.Cast(man);
            }
        }
        return null;
    }

    // ---- dispatch -------------------------------------------------------------

    // Hands the request to the holder's own hand state machine (Man.c:757). The return value
    // only says the request was routed; see signal 3 in the header.
    // Before the hand event the debug lines read the giver's junctures (the October 20
    // diagnostic's predispatch read, clean on 1.30.164078): crashes 4 and 5 (October 29, 2026)
    // died in HasInventoryJuncture(giver, item) after the hand event, so a crash here says
    // the native or the giver reference is at fault, a clean read says the hand event is.
    static bool Dispatch(NimbyHandsWorldTransfer transfer, InventoryLocation src, InventoryLocation dst)
    {
        if (NimbyHandsLog.IsDebug())
        {
            LogJunctures("step 1", transfer.id.ToString(), transfer.giver, transfer.receiver, transfer.item, dst);
        }
        return transfer.giver.ServerTakeToDst(src, dst);
    }

    // ---- the client-driven step 1 (NIMBY-T-063, October 31, 2026) -----------------------
    // Crashes 4 to 10 on 1.30.164200 all followed a server-driven hand event on a connected
    // giver (Dispatch above, from the call queue or from the giver's own CommandHandler); the
    // same step 2 with no hand event before it confirmed between two connected players (S6-D,
    // run testserver-20261031-153005). So a connected giver stages the item from its own client:
    // the server sends NimbyHandsRpc.STAGE to the giver's identity, the client posts
    // PredictiveTakeToDst (NimbyHandsClient.OnStageRequest), the engine carries it to the server
    // as any player-made hands-to-cargo move and applies it in the giver's CommandHandler;
    // signal 1 writes STAGED as before and the step 2 poll takes it from there. A client that
    // cannot post the move answers STAGE_REFUSED (OnClientStageRefused); a client that answers
    // nothing is caught by the bounded timeout. Dispatch above stays for a dummy giver.

    // The dispatch path for this giver (NimbyHandsRules.StageVia): a giver with an identity has
    // a client; a dev harness dummy has none.
    static string StageVia(PlayerBase giver)
    {
        bool hasIdentity = giver != null && giver.GetIdentity() != null;
        return NimbyHandsRules.StageVia(hasIdentity);
    }

    // Sends the stage request to the giver's own client. The item travels as its network id;
    // the client finds its own staging cell (the same FindStagingLocation) and the server records
    // whatever cell the landing reports, so the stage computed here is written only to the log.
    static void RequestClientStage(NimbyHandsWorldTransfer transfer, InventoryLocation src, InventoryLocation stage)
    {
        int low = 0;
        int high = 0;
        transfer.item.GetNetworkID(low, high);
        Param3<int, int, int> request = new Param3<int, int, int>(transfer.id, low, high);
        GetGame().RPCSingleParam(transfer.giver, NimbyHandsRpc.STAGE, request, true, transfer.giver.GetIdentity());
        NimbyHandsLog.Debug("id=" + transfer.id.ToString() + " step 1 stage rpc sent to giver=" + transfer.giverText + " item=" + transfer.itemText + " net=" + low.ToString() + ":" + high.ToString() + " stage=" + DescribeLocation(stage));
    }

    // STAGE_REFUSED from a client (PlayerBase.OnRPC on the server). Accepted only from the
    // giver's own identity for a request still REQUESTED (NimbyHandsRules.ClientStageRefusalAccepted);
    // anything else is logged and ignored. The reason text is the client's, escaped.
    static void OnClientStageRefused(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param2<int, string> reply = new Param2<int, string>(0, "");
        if (!ctx.Read(reply))
        {
            NimbyHandsLog.Info("stage refused: unreadable reply from " + DescribeIdentity(sender));
            return;
        }
        NimbyHandsWorldTransfer transfer = FindTransfer(reply.param1);
        string senderId = "";
        if (sender)
        {
            senderId = sender.GetId();
        }
        if (!transfer)
        {
            NimbyHandsLog.Info("stage refused: unknown id=" + reply.param1.ToString() + " from " + DescribeIdentity(sender));
            return;
        }
        if (!NimbyHandsRules.ClientStageRefusalAccepted(senderId, transfer.giverId, transfer.state))
        {
            NimbyHandsLog.Info("stage refused: ignored id=" + transfer.id.ToString() + " state=" + NimbyHandsState.Name(transfer.state) + " from " + DescribeIdentity(sender) + " giver=" + transfer.giverText);
            return;
        }
        Advance(transfer, NimbyHandsState.REFUSED, NimbyHandsRules.ClientStageRefusalDetail(NimbyHandsLog.Escape(reply.param2)));
    }

    static string DescribeIdentity(PlayerIdentity identity)
    {
        if (!identity)
        {
            return "(no identity)";
        }
        return NimbyHandsLog.Quote(identity.GetName()) + " (" + identity.GetId() + ", " + identity.GetPlainId() + ")";
    }

    // ---- the step 2 move (NIMBY-T-063 instrumentation) ---------------------------
    // What Man.ServerTakeToDst does for a move with neither end in hands, one native per
    // debug line, so a crash log names the native that died (October 28, 2026: three access
    // violations in step 2, the last inside ServerTakeToDst itself):
    //   Man.TakeToDstImpl (Man.c:762-767) -> HumanInventory.TakeToDst (HumanInventory.c:243-251;
    //   RedirectToHandEvent is false without a HANDS end) -> GameInventory.TakeToDst(SERVER)
    //   (Inventory.c:1057-1066): LocationSyncMoveEntity (:567), then SendServerMove
    //   (InventoryInputUserData.c:57) when it moved and dst is valid; then Man.UpdateInventoryMenu.
    // This function never touches the giver object (NIMBY-T-063, October 29, 2026). On game
    // build 1.30.164200 seven server crashes in a row died here, each on the first operation
    // that handed transfer.giver to the engine or to a script call after the step 1 hand event:
    // ServerTakeToDst on it (crash 3), HasInventoryJuncture(giver, item) (4, 5),
    // DescribePlayer(giver) (6, 7), at 0, 49, 93, 999 ms after the landing, scheduled from the
    // signal or from the poll alike, while every item-only and location-only read passed and the
    // same reads on the giver inside CollectStep2Facts passed moments earlier. The same code
    // confirmed on 1.30.164078 (October 20) and confirms every boot between two dummies. So the
    // debug lines read the item and the two locations only, the reservation lines read the item
    // only, and Man.UpdateInventoryMenu (a menu refresh hint the vanilla TakeToDstImpl sends) is
    // not called. The junctures are read by Dispatch, before the hand event.
    static bool MoveStep2(NimbyHandsWorldTransfer transfer, InventoryLocation src, InventoryLocation dst)
    {
        string idText = transfer.id.ToString();
        if (NimbyHandsLog.IsDebug())
        {
            NimbyHandsLog.Debug("id=" + idText + " step 2 move: src=" + DescribeLocation(src) + " dst=" + DescribeLocation(dst));
            LogReservations(idText, transfer.item, src, dst);
        }
        s_Step2MoveId = transfer.id;
        s_Step2Landed = false;
        bool moved = GameInventory.LocationSyncMoveEntity(src, dst);
        bool landed = s_Step2Landed;
        s_Step2MoveId = 0;
        s_Step2Landed = false;
        NimbyHandsLog.Debug("id=" + idText + " step 2 move: LocationSyncMoveEntity=" + NimbyHandsLog.Bool(moved) + " signal=" + NimbyHandsLog.Bool(landed));
        if (moved && dst.IsValid())
        {
            InventoryInputUserData.SendServerMove(null, InventoryCommandType.SYNC_MOVE, src, dst);
            NimbyHandsLog.Debug("id=" + idText + " step 2 move: SendServerMove sent");
        }
        // The bookkeeping the signal handler used to do, now outside the engine's callback:
        // CONFIRMED when the move reported success, the signal fired, and the item's hierarchy
        // root is the receiver (a pointer compare, no read through either player).
        if (moved && landed && transfer.item && transfer.receiver && transfer.item.GetHierarchyRootPlayer() == transfer.receiver)
        {
            Advance(transfer, NimbyHandsState.CONFIRMED, "signal=EEItemLocationChanged dst=" + DescribeItemLocation(transfer.item));
        }
        return moved;
    }

    // The engine's juncture locks (Game.c:757-759): on the item, between the giver and the
    // item, on either player as an item, and on the giver for the destination. Server only.
    // One native per debug line, so a crash in one of them leaves the values read before it.
    // stage names the caller ("step 1" before the hand event, "step 2" before the move).
    static void LogJunctures(string stage, string idText, PlayerBase giver, PlayerBase receiver, EntityAI item, InventoryLocation dst)
    {
        string prefix = "id=" + idText + " " + stage + " junctures: ";
        if (!GetGame().IsServer())
        {
            NimbyHandsLog.Debug(prefix + "j=client");
            return;
        }
        if (item)
        {
            NimbyHandsLog.Debug(prefix + "jItem=" + NimbyHandsLog.Bool(GetGame().HasInventoryJunctureItem(item)));
            if (giver)
            {
                NimbyHandsLog.Debug(prefix + "jGiverItem=" + NimbyHandsLog.Bool(GetGame().HasInventoryJuncture(giver, item)));
            }
            else
            {
                NimbyHandsLog.Debug(prefix + "jGiverItem=none");
            }
        }
        else
        {
            NimbyHandsLog.Debug(prefix + "jItem=none jGiverItem=none");
        }
        if (giver)
        {
            NimbyHandsLog.Debug(prefix + "jGiver=" + NimbyHandsLog.Bool(GetGame().HasInventoryJunctureItem(giver)));
        }
        if (receiver)
        {
            NimbyHandsLog.Debug(prefix + "jReceiver=" + NimbyHandsLog.Bool(GetGame().HasInventoryJunctureItem(receiver)));
        }
        if (giver && dst)
        {
            NimbyHandsLog.Debug(prefix + "jDst=" + NimbyHandsLog.Bool(GetGame().HasInventoryJunctureDestination(giver, dst)));
        }
    }

    // The script-side reservation (Inventory.c:787) on the item for each end. One native per
    // debug line, as LogJunctures. Reads the item only (see MoveStep2 for why not the giver).
    static void LogReservations(string idText, EntityAI item, InventoryLocation src, InventoryLocation dst)
    {
        string prefix = "id=" + idText + " step 2 reservations: ";
        if (!item)
        {
            NimbyHandsLog.Debug(prefix + "rSrc=none rDst=none");
            return;
        }
        NimbyHandsLog.Debug(prefix + "rSrc=" + NimbyHandsLog.Bool(GameInventory.HasInventoryReservation(item, src)));
        NimbyHandsLog.Debug(prefix + "rDst=" + NimbyHandsLog.Bool(GameInventory.HasInventoryReservation(item, dst)));
    }

    // ---- the step 1 wait (NIMBY-T-063) ------------------------------------------
    // Step 1 is a hand event on the giver, and the giver's hand state machine drops it
    // while the giver is inside an action command (feasibility study H-B); every request
    // dispatched from inside Take, Give or Put In Pockets ended UNCONFIRMED with the item
    // still in hands (S6-30, October 28, 2026). The October 20 diagnostic's V3, the only
    // P3 that confirmed in game, ended both consent loops the way vanilla ends a continuous
    // action on the server (ActionManagerServer.RequestInterruptAction), then waited until
    // both players were free before dispatching. This is that wait, bounded by
    // NimbyHandsConstants.WAIT_CAP_MS. The request stays PENDING in the registry meanwhile,
    // so a second request for the item is refused in flight.
    protected static ref array<ref NimbyHandsMoveItemToPlayer> s_Waiting;
    protected static bool s_Ticking;

    static void Defer(NimbyHandsMoveItemToPlayer move, NimbyHandsWorldTransfer transfer)
    {
        if (!s_Waiting)
        {
            s_Waiting = new array<ref NimbyHandsMoveItemToPlayer>;
        }
        s_Waiting.Insert(move);
        NimbyHandsLog.Debug("id=" + transfer.id.ToString() + " wait start");
        EndConsentLoop(transfer.giver, transfer.id);
        EndConsentLoop(transfer.receiver, transfer.id);
        if (!s_Ticking)
        {
            s_Ticking = true;
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyHandsServer.Tick, NimbyHandsConstants.WAIT_TICK_MS, true);
        }
    }

    // Ends the offer or request loop (CAContinuousTime(-1), which never ends on its own) so
    // the player becomes free. Consent was read when the request was made.
    protected static void EndConsentLoop(PlayerBase player, int id)
    {
        if (!player || !player.GetActionManager())
        {
            return;
        }
        ActionBase action = player.GetActionManager().GetRunningAction();
        if (!action)
        {
            return;
        }
        if (!NimbyHandsActionOfferItem.Cast(action) && !NimbyHandsActionRequestItem.Cast(action))
        {
            return;
        }
        player.GetActionManager().RequestInterruptAction();
        NimbyHandsLog.Debug("id=" + id.ToString() + " interrupt player=" + DescribePlayer(player) + " action=" + action.ClassName());
    }

    static void Tick()
    {
        int nowMs = GetGame().GetTime();
        if (s_Waiting)
        {
            for (int i = s_Waiting.Count() - 1; i >= 0; i--)
            {
                NimbyHandsMoveItemToPlayer move = s_Waiting.Get(i);
                if (!move || move.Poll(nowMs, false))
                {
                    s_Waiting.RemoveOrdered(i);
                }
            }
        }
        StopTickWhenIdle();
    }

    // ---- the giver's frame (NIMBY-T-063 probe, build 10, October 30, 2026) ----------
    // Called by PlayerBase.ModCommandHandlerBefore on the server for every player, every
    // frame. Polls the waiting requests whose giver is this player, so for a connected giver
    // the step 1 hand event (Dispatch -> ServerTakeToDst) is posted from inside the giver's
    // own CommandHandler, right after vanilla's HandleInventory, instead of from the global
    // call queue. The wait tick keeps running for the cap and for dummy givers.
    static void OnCommandHandler(PlayerBase player)
    {
        if (!s_Waiting || s_Waiting.Count() == 0 || !player)
        {
            return;
        }
        int nowMs = GetGame().GetTime();
        for (int i = s_Waiting.Count() - 1; i >= 0; i--)
        {
            NimbyHandsMoveItemToPlayer move = s_Waiting.Get(i);
            if (!move)
            {
                s_Waiting.RemoveOrdered(i);
                continue;
            }
            if (!move.GiverIs(player))
            {
                continue;
            }
            if (move.Poll(nowMs, true))
            {
                s_Waiting.RemoveOrdered(i);
            }
        }
        StopTickWhenIdle();
    }

    // Build 10 probe: a giver with an identity (a connected player) dispatches step 1 only
    // from its own CommandHandler. A dummy (no identity) has no client to desync and never runs
    // a hand event (build 115044), so it keeps dispatching from the wait tick, which is what
    // the dev harness boot cases rely on. Read before the hand event, where GetIdentity on the
    // giver has always passed.
    static bool DispatchesFromCommandHandler(PlayerBase giver)
    {
        if (!giver)
        {
            return false;
        }
        return giver.GetIdentity() != null;
    }

    protected static void StopTickWhenIdle()
    {
        if (s_Ticking && (!s_Waiting || s_Waiting.Count() == 0))
        {
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(NimbyHandsServer.Tick);
            s_Ticking = false;
        }
    }

    // No running action; for the holder also no action command and an idle hand inventory.
    // A missing player counts as free: the revalidation refuses it with its own detail.
    static bool IsFree(PlayerBase player, bool holder)
    {
        if (!player)
        {
            return true;
        }
        if (player.GetActionManager() && player.GetActionManager().GetRunningAction())
        {
            return false;
        }
        if (!holder)
        {
            return true;
        }
        if (player.GetCommand_Action() != null || player.GetCommandModifier_Action() != null)
        {
            return false;
        }
        DayZPlayerInventory inventory = DayZPlayerInventory.Cast(player.GetInventory());
        if (inventory && inventory.IsProcessing())
        {
            return false;
        }
        return true;
    }

    static void ScheduleTimeout(NimbyHandsWorldTransfer transfer)
    {
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyHandsServer.OnTimeout, Registry().TimeoutMs(), false, transfer.id);
    }

    // ---- the step 2 poll (NIMBY-T-063, October 29, 2026) --------------------------
    // Step 2 is never scheduled from inside the step 1 signal. Six server crashes on game
    // build 1.30.164200 (October 28 and 29, 2026) all ran step 2 from a CallLater posted
    // inside EEItemLocationChanged, that is inside the engine's hand-event move, and died in
    // the script VM on the first touch of the giver object (six different sites: a location
    // compare, a script method call, a juncture native, GetIdentity). Every step 2 that ever
    // confirmed ran from somewhere else: the dev harness calls OnStep2 directly, and the
    // October 20 diagnostic polled for the landing from its own timer (hands-t010, confirmed
    // four of four on 1.30.164078). This poll is that timer: Submit starts it after the
    // dispatch, from the wait tick, and it runs step 2 for every STAGED transfer at least
    // STEP2_DELAY_MS past its stagedAtMs. It stops when the registry is empty (terminal
    // entries are evicted; a stuck entry is evicted by OnTimeout).
    protected static bool s_Step2Ticking;

    static void EnsureStep2Poll()
    {
        if (s_Step2Ticking)
        {
            return;
        }
        s_Step2Ticking = true;
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyHandsServer.Step2Tick, NimbyHandsConstants.WAIT_TICK_MS, true);
    }

    static void Step2Tick()
    {
        if (!s_Registry || s_Registry.Count() == 0)
        {
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(NimbyHandsServer.Step2Tick);
            s_Step2Ticking = false;
            return;
        }
        int nowMs = GetGame().GetTime();
        array<NimbyHandsTransfer> staged = new array<NimbyHandsTransfer>;
        s_Registry.CollectByState(NimbyHandsState.STAGED, staged);
        foreach (NimbyHandsTransfer entry : staged)
        {
            NimbyHandsWorldTransfer transfer = NimbyHandsWorldTransfer.Cast(entry);
            if (!transfer)
            {
                continue;
            }
            int agoMs = nowMs - transfer.stagedAtMs;
            if (agoMs >= NimbyHandsConstants.STEP2_DELAY_MS)
            {
                NimbyHandsLog.Debug("id=" + transfer.id.ToString() + " step 2 poll: staged " + agoMs.ToString() + " ms ago");
                OnStep2(transfer.id);
            }
        }
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
        transfer.stagedAtMs = GetGame().GetTime();
        transfer.stagedType = loc.GetType();
        transfer.stagedParent = loc.GetParent();
        transfer.stagedText = DescribeLocation(loc);
    }

    // The item is still where step 1 put it: same location type and the same parent entity,
    // read one native at a time with a debug line after each (October 28: two server crashes
    // in this check, access violation in one native; the lines name it if it recurs).
    static bool IsAtStagedLocation(NimbyHandsWorldTransfer transfer, InventoryLocation loc)
    {
        if (!transfer.staged || !loc)
        {
            return false;
        }
        string idText = transfer.id.ToString();
        int type = loc.GetType();
        NimbyHandsLog.Debug("id=" + idText + " staged check: type=" + type.ToString() + " staged=" + transfer.stagedType.ToString());
        if (type != transfer.stagedType || type == InventoryLocationType.HANDS)
        {
            return false;
        }
        EntityAI parent = loc.GetParent();
        NimbyHandsLog.Debug("id=" + idText + " staged check: parent read " + NimbyHandsLog.Bool(parent != null));
        if (!parent)
        {
            return false;
        }
        bool same = parent == transfer.stagedParent;
        NimbyHandsLog.Debug("id=" + idText + " staged check: parent same=" + NimbyHandsLog.Bool(same) + " staged=" + transfer.stagedText);
        return same;
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
        if (s_Step2MoveId != 0)
        {
            return;
        }
        if (!ShouldTrace(item, oldLoc, newLoc))
        {
            return;
        }
        s_TraceDepth = s_TraceDepth + 1;
        TraceSignal("EEItemLocationChanged.begin", item, DescribeLocation(oldLoc) + " -> " + DescribeLocation(newLoc));
    }

    static void OnItemLocationChangedEnd(EntityAI item, InventoryLocation oldLoc, InventoryLocation newLoc)
    {
        if (s_Step2MoveId != 0)
        {
            // Inside the step 2 move: one static write, nothing else (see s_Step2MoveId).
            s_Step2Landed = true;
            return;
        }
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
            // Nothing is scheduled from inside the engine's move callback; the step 2 poll
            // (started by Submit, outside it) reads stagedAtMs on its next tick.
            RecordStaged(transfer, newLoc);
            Advance(transfer, verdict, detail);
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
        if (s_Step2MoveId != 0)
        {
            return;
        }
        if (s_TraceDepth > 0)
        {
            TraceSignal("OnInventoryExit", item, "player=" + DescribePlayer(player));
        }
    }

    static void OnInventoryEnterSignal(EntityAI item, Man player)
    {
        if (s_Step2MoveId != 0)
        {
            return;
        }
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
