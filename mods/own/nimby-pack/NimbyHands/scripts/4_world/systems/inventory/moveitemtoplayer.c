// -----------------------------------------------------------------------------
// NimbyHandsMoveItemToPlayer - moves an item from one player's hands into another
// player's inventory (cargo or an attachment slot, never hands).
//
// Rewritten October 14, 2026 (review CR-001) to use the vanilla server path:
// Man.ServerTakeToDst on the player holding the item (Man.c:757), which routes a
// HandEventMoveTo through that player's own hand state machine
// (HumanInventory.RedirectToHandEvent, HumanInventory.c:200-229). Because the item
// never lands in the receiver's hands, no second hand state machine is involved.
//
// Phase 1, October 19, 2026 (plan v0.5): every request is registered in the
// in-flight registry with a correlation id before dispatch (G5) and refused when
// its item already has an unresolved entry; consent, both players' state (G4),
// proximity, the source item, and the destination are revalidated immediately
// before dispatch; the outcome comes from the completion signals in
// NimbyHandsServer, not from the ServerTakeToDst return value, which is true
// even when the holder's inventory refuses the hand event. Nothing is retried,
// cloned, restored, or deleted to compensate for an uncertain result.
//
// Diagnostic, branch diag/hands-t010 only (never merged): the variant switch of
// NimbyHandsDiag. V0 and V1 dispatch as above; V2 and V3 keep the request PENDING
// in the registry until both players are free, revalidate, then dispatch; V3
// dispatches to the giver's own inventory first (DiagStep2 does the rest).
// -----------------------------------------------------------------------------

class NimbyHandsMoveItemToPlayer
{
    // Destinations a transfer may use. Hands are excluded on purpose (see header). A function
    // rather than a static const: vanilla has no example of a const built from enum flags.
    static int DestinationFlags()
    {
        return FindInventoryLocationType.ANY_CARGO | FindInventoryLocationType.ATTACHMENT;
    }

    protected PlayerBase m_DstPlayer;
    protected EntityAI m_Item;
    protected string m_ActionName;
    protected float m_Reach;
    protected bool m_Consent;
    protected string m_ConsentDetail;
    protected bool m_HasExpected;
    protected EntityAI m_ExpectedItem;
    protected string m_ExpectedGiverId;
    protected string m_ExpectedReceiverId;
    // Diagnostic: the variant the action read, and the request held while V2 or V3 waits.
    protected int m_DiagVariant;
    protected ref NimbyHandsWorldTransfer m_Transfer;
    protected int m_WaitStartMs;

    void NimbyHandsMoveItemToPlayer(PlayerBase dst_player, EntityAI item, string action_name, float reach)
    {
        m_DstPlayer = dst_player;
        m_Item = item;
        m_ActionName = action_name;
        m_Reach = reach;
        m_Consent = false;
        m_ConsentDetail = "not evaluated";
        m_HasExpected = false;
        m_DiagVariant = NimbyHandsDiag.V0;
    }

    void SetDiagVariant(int variant)
    {
        m_DiagVariant = variant;
    }

    // The consenting party's state, read by the action at dispatch time.
    void SetConsent(bool consent, string detail)
    {
        m_Consent = consent;
        m_ConsentDetail = detail;
    }

    // What the action saw when it started (NimbyHandsActionData).
    void SetExpected(EntityAI item, string giverId, string receiverId)
    {
        m_HasExpected = true;
        m_ExpectedItem = item;
        m_ExpectedGiverId = giverId;
        m_ExpectedReceiverId = receiverId;
    }

    // True when the receiver has room for the item outside their hands. Used by action
    // conditions so the prompt only shows when the transfer can succeed.
    static bool ReceiverHasSpace(PlayerBase receiver, EntityAI item)
    {
        if (!receiver || !item)
        {
            return false;
        }
        InventoryLocation dst = new InventoryLocation;
        return receiver.GetInventory().FindFreeLocationFor(item, DestinationFlags(), dst);
    }

    // Registers, revalidates, and dispatches the request. Returns true when it was submitted
    // and not refused; the terminal outcome is logged by NimbyHandsServer.
    bool Execute()
    {
        if (GetGame().IsClient())
        {
            // A client calling this is a programming error, not a player outcome.
            Error("[NIMBY][Hands] NimbyHandsMoveItemToPlayer can only run on the server");
            return false;
        }

        NimbyHandsWorldTransfer transfer = new NimbyHandsWorldTransfer();
        transfer.action = m_ActionName;
        transfer.receiver = m_DstPlayer;
        transfer.item = m_Item;
        if (m_Item)
        {
            transfer.giver = m_Item.GetHierarchyRootPlayer();
        }
        NimbyHandsServer.Describe(transfer);

        if (!m_Item)
        {
            NimbyHandsServer.LogUnregisteredRefusal(transfer, "no item");
            return false;
        }

        NimbyHandsTransfers registry = NimbyHandsServer.Registry();
        string key = NimbyHandsServer.ItemKey(m_Item);
        if (!registry.Begin(transfer, key, m_ActionName, GetGame().GetTime()))
        {
            string inFlight = "item in flight";
            NimbyHandsTransfer other = registry.Find(key);
            if (other)
            {
                inFlight = inFlight + " (id=" + other.id.ToString() + ")";
            }
            NimbyHandsServer.LogUnregisteredRefusal(transfer, inFlight);
            return false;
        }
        transfer.diagVariant = m_DiagVariant;
        transfer.diagReach = m_Reach;

        InventoryLocation src = new InventoryLocation;
        InventoryLocation dst = new InventoryLocation;
        string refusal = Revalidate(transfer, src, dst);
        if (refusal != "")
        {
            NimbyHandsServer.Advance(transfer, NimbyHandsState.REFUSED, refusal);
            return false;
        }

        if (m_DiagVariant == NimbyHandsDiag.V2 || m_DiagVariant == NimbyHandsDiag.V3)
        {
            // Stays PENDING (registered, so a second request for the item is refused in flight)
            // until NimbyHandsDiag.Tick finds both players free and calls DiagPoll.
            m_Transfer = transfer;
            m_WaitStartMs = GetGame().GetTime();
            NimbyHandsDiag.Defer(this, transfer);
            return true;
        }
        return Submit(transfer, src, dst);
    }

    // Writes REQUESTED, dispatches, and schedules the timeout. In V3 the dispatch goes to a
    // free location in the giver's own inventory (step 1); dst stays the receiver's.
    protected bool Submit(NimbyHandsWorldTransfer transfer, InventoryLocation src, InventoryLocation dst)
    {
        InventoryLocation dispatchDst = dst;
        string requested = "dst=" + NimbyHandsServer.DescribeLocation(dst);
        if (transfer.diagVariant == NimbyHandsDiag.V3)
        {
            InventoryLocation ownDst = new InventoryLocation;
            if (!transfer.giver.GetInventory().FindFreeLocationFor(m_Item, DestinationFlags(), ownDst))
            {
                NimbyHandsServer.Advance(transfer, NimbyHandsState.REFUSED, "diag V3: no space in the giver's own inventory");
                return false;
            }
            if (!GameInventory.LocationCanMoveEntity(src, ownDst))
            {
                NimbyHandsServer.Advance(transfer, NimbyHandsState.REFUSED, "diag V3: engine refuses the move to " + NimbyHandsServer.DescribeLocation(ownDst));
                return false;
            }
            dispatchDst = ownDst;
            transfer.diagStage = 1;
            requested = "dst=" + NimbyHandsServer.DescribeLocation(ownDst) + "; diag V3 step 1 to the giver's own inventory, then " + NimbyHandsServer.DescribeLocation(dst);
        }

        NimbyHandsDiag.PreDispatch(transfer, src, dispatchDst, dst);
        NimbyHandsServer.Advance(transfer, NimbyHandsState.REQUESTED, requested);
        // The observation period starts at dispatch, not at registration (V2 and V3 waited).
        transfer.startedMs = GetGame().GetTime();
        bool accepted = NimbyHandsServer.Dispatch(transfer, src, dispatchDst);
        NimbyHandsDiag.StartWatch(PlayerBase.Cast(transfer.giver), transfer.receiver, m_Item, transfer.id);
        // A server hand event is deferred to the holder's next inventory update
        // (DayZPlayerInventory.c:4448, 4623-4634), so no signal is expected inside the call;
        // if one did end the request, its terminal line is already written.
        if (transfer.state == NimbyHandsState.CONFIRMED)
        {
            return true;
        }
        if (transfer.state != NimbyHandsState.REQUESTED)
        {
            return false;
        }
        if (!accepted)
        {
            NimbyHandsServer.Advance(transfer, NimbyHandsState.REFUSED, "server move rejected");
            return false;
        }
        NimbyHandsServer.ScheduleTimeout(transfer);
        return true;
    }

    // Diagnostic V2 and V3: polled every NimbyHandsDiag.TICK_MS while the request waits.
    // Returns true when the wait is over, dispatched or refused.
    bool DiagPoll(int nowMs)
    {
        NimbyHandsWorldTransfer transfer = m_Transfer;
        if (!transfer || transfer.state != NimbyHandsState.PENDING)
        {
            return true;
        }
        string idText = transfer.id.ToString();
        PlayerBase giver = PlayerBase.Cast(transfer.giver);
        int waited = nowMs - m_WaitStartMs;
        if (!NimbyHandsDiag.IsFree(giver, true) || !NimbyHandsDiag.IsFree(m_DstPlayer, false))
        {
            if (waited < NimbyHandsDiag.WAIT_CAP_MS)
            {
                return false;
            }
            string capLine = "result=cap waited=" + waited.ToString();
            capLine = capLine + " giver{" + NimbyHandsDiag.DescribeHands(giver) + " " + NimbyHandsDiag.DescribeCommands(giver) + "}";
            capLine = capLine + " receiver{" + NimbyHandsDiag.DescribeCommands(m_DstPlayer) + "}";
            NimbyHandsDiag.Line("wait", idText, capLine);
            int capMs = NimbyHandsDiag.WAIT_CAP_MS;
            NimbyHandsServer.Advance(transfer, NimbyHandsState.REFUSED, "action did not end within " + capMs.ToString() + " ms (diag " + NimbyHandsDiag.VariantName(transfer.diagVariant) + ")");
            return true;
        }
        NimbyHandsDiag.Line("wait", idText, "result=free waited=" + waited.ToString());

        // Consent was read when the request was made and the loop that carried it has been
        // interrupted; open pockets are a standing state, so they are read again.
        if (m_ActionName == "NimbyHandsActionPutInPockets")
        {
            bool open = false;
            if (m_DstPlayer)
            {
                open = m_DstPlayer.NimbyHandsGetPocketsOpen();
            }
            SetConsent(open, "receiver's pockets are closed");
        }

        InventoryLocation src = new InventoryLocation;
        InventoryLocation dst = new InventoryLocation;
        string refusal = Revalidate(transfer, src, dst);
        if (refusal != "")
        {
            NimbyHandsServer.Advance(transfer, NimbyHandsState.REFUSED, refusal + " (after the diag wait)");
            return true;
        }
        Submit(transfer, src, dst);
        return true;
    }

    // Diagnostic V3, step 2: the item is in the giver's own inventory, so the giver's
    // ServerTakeToDst is the synchronous server move (HumanInventory.c:243-251 ->
    // Inventory.c:1057-1066) and its return value is the result. Its EEItemLocationChanged
    // fires inside the call and writes CONFIRMED. A refusal leaves the item where step 1 put it.
    static void DiagStep2(NimbyHandsWorldTransfer transfer)
    {
        string idText = transfer.id.ToString();
        PlayerBase giver = PlayerBase.Cast(transfer.giver);
        PlayerBase receiver = transfer.receiver;
        InventoryLocation src = new InventoryLocation;
        InventoryLocation dst = new InventoryLocation;
        string refusal = CheckPlayer("giver", giver);
        if (refusal == "")
        {
            refusal = CheckPlayer("receiver", receiver);
        }
        if (refusal == "" && !transfer.item)
        {
            refusal = "item no longer exists";
        }
        if (refusal == "")
        {
            float distanceSq = vector.DistanceSq(giver.GetPosition(), receiver.GetPosition());
            if (!NimbyHandsRules.WithinReach(distanceSq, transfer.diagReach, NimbyHandsConstants.PROXIMITY_TOLERANCE))
            {
                refusal = "out of reach (" + Math.Sqrt(distanceSq).ToString() + " m)";
            }
        }
        if (refusal == "" && !transfer.item.GetInventory().GetCurrentInventoryLocation(src))
        {
            refusal = "item location unavailable";
        }
        if (refusal == "")
        {
            bool inGiverInventory = src.GetParent() != null && src.GetParent().GetHierarchyRootPlayer() == giver;
            if (!inGiverInventory || src.GetType() == InventoryLocationType.HANDS)
            {
                refusal = "item is not in the giver's inventory";
            }
        }
        if (refusal == "" && !receiver.GetInventory().FindFreeLocationFor(transfer.item, DestinationFlags(), dst))
        {
            refusal = "no space in receiver inventory";
        }
        if (refusal == "" && !GameInventory.LocationCanMoveEntity(src, dst))
        {
            refusal = "engine refuses the move to " + NimbyHandsServer.DescribeLocation(dst);
        }
        if (refusal != "")
        {
            NimbyHandsDiag.Line("step2", idText, "result=refused detail=" + refusal);
            NimbyHandsServer.Advance(transfer, NimbyHandsState.REFUSED, "diag V3 step 2: " + refusal + "; item at " + NimbyHandsDiag.DescribeItemLocation(transfer.item));
            return;
        }

        string line = "src=" + NimbyHandsServer.DescribeLocation(src) + " dst=" + NimbyHandsServer.DescribeLocation(dst);
        line = line + " " + NimbyHandsDiag.DescribeJunctures(giver, receiver, transfer.item, dst);
        NimbyHandsDiag.Line("step2", idText, line);
        bool moved = giver.ServerTakeToDst(src, dst);
        NimbyHandsDiag.Line("step2", idText, "result=" + NimbyHandsLog.Bool(moved) + " state=" + NimbyHandsState.Name(transfer.state));
        if (transfer.state != NimbyHandsState.REQUESTED)
        {
            // CONFIRMED (or REFUSED) was written by the location-change signal inside the call.
            return;
        }
        if (!moved)
        {
            NimbyHandsServer.Advance(transfer, NimbyHandsState.REFUSED, "diag V3 step 2: server move refused; item at " + NimbyHandsDiag.DescribeItemLocation(transfer.item));
        }
        // Moved without a signal: left to the timeout, which reconciles where the item is.
    }

    // Every check that must hold immediately before dispatch. Fills src and dst. Returns "" or
    // the refusal detail.
    protected string Revalidate(NimbyHandsWorldTransfer transfer, InventoryLocation src, InventoryLocation dst)
    {
        // Only a diagnostic wait (V2, V3) can reach here after the item was deleted.
        if (!m_Item)
        {
            return "item no longer exists";
        }
        if (!m_Consent)
        {
            return "no consent: " + m_ConsentDetail;
        }

        PlayerBase giver = PlayerBase.Cast(transfer.giver);
        string party = CheckPlayer("giver", giver);
        if (party != "")
        {
            return party;
        }
        party = CheckPlayer("receiver", m_DstPlayer);
        if (party != "")
        {
            return party;
        }
        if (giver == m_DstPlayer)
        {
            return "giver and receiver are the same player";
        }
        if (m_HasExpected && m_ExpectedGiverId != "" && NimbyHandsActionData.IdentityId(giver) != m_ExpectedGiverId)
        {
            return "giver replaced since the action started";
        }
        if (m_HasExpected && m_ExpectedReceiverId != "" && NimbyHandsActionData.IdentityId(m_DstPlayer) != m_ExpectedReceiverId)
        {
            return "receiver replaced since the action started";
        }

        float distanceSq = vector.DistanceSq(giver.GetPosition(), m_DstPlayer.GetPosition());
        if (!NimbyHandsRules.WithinReach(distanceSq, m_Reach, NimbyHandsConstants.PROXIMITY_TOLERANCE))
        {
            return "out of reach (" + Math.Sqrt(distanceSq).ToString() + " m)";
        }

        if (!m_Item.GetInventory().GetCurrentInventoryLocation(src))
        {
            return "item location unavailable";
        }
        if (src.GetType() != InventoryLocationType.HANDS || src.GetParent() != giver)
        {
            return "item is not in the giver's hands";
        }
        if (m_HasExpected && m_ExpectedItem != m_Item)
        {
            return "item replaced since the action started";
        }

        DayZPlayerInventory giverInventory = DayZPlayerInventory.Cast(giver.GetInventory());
        if (giverInventory && giverInventory.IsProcessing())
        {
            return "giver inventory busy";
        }

        if (!m_DstPlayer.GetInventory().FindFreeLocationFor(m_Item, DestinationFlags(), dst))
        {
            return "no space in receiver inventory";
        }

        // The engine test HandEventMoveTo.CanPerformEvent runs at queue time (Hand_Events.c:306-319,
        // Inventory.c:373). Its result cannot be observed after dispatch on 1.30, so it is asked here.
        if (!GameInventory.LocationCanMoveEntity(src, dst))
        {
            return "engine refuses the move to " + NimbyHandsServer.DescribeLocation(dst);
        }

        // The reservation test HandEventBase.ReserveInventory makes (Hand_Events.c:185,
        // Inventory.c:787). A reservation taken between here and the holder's next inventory
        // update can still refuse silently; that request ends UNCONFIRMED at the timeout.
        if (GameInventory.HasInventoryReservation(m_Item, dst))
        {
            return "destination reserved";
        }
        return "";
    }

    protected static string CheckPlayer(string role, PlayerBase player)
    {
        if (!player)
        {
            return NimbyHandsRules.CheckParty(role, false, false, false, false, false, false, false);
        }
        bool connected = player.GetIdentity() != null;
        bool alive = player.IsAlive();
        bool conscious = !player.IsUnconscious();
        return NimbyHandsRules.CheckParty(role, true, connected, alive, conscious, player.IsRestrained(), player.IsInVehicle(), player.IsSwimming());
    }
}
