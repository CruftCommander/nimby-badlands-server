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

    void NimbyHandsMoveItemToPlayer(PlayerBase dst_player, EntityAI item, string action_name, float reach)
    {
        m_DstPlayer = dst_player;
        m_Item = item;
        m_ActionName = action_name;
        m_Reach = reach;
        m_Consent = false;
        m_ConsentDetail = "not evaluated";
        m_HasExpected = false;
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

        InventoryLocation src = new InventoryLocation;
        InventoryLocation dst = new InventoryLocation;
        string refusal = Revalidate(transfer, src, dst);
        if (refusal != "")
        {
            NimbyHandsServer.Advance(transfer, NimbyHandsState.REFUSED, refusal);
            return false;
        }

        NimbyHandsServer.Advance(transfer, NimbyHandsState.REQUESTED, "dst=" + NimbyHandsServer.DescribeLocation(dst));
        bool accepted = NimbyHandsServer.Dispatch(transfer, src, dst);
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

    // Every check that must hold immediately before dispatch. Fills src and dst. Returns "" or
    // the refusal detail.
    protected string Revalidate(NimbyHandsWorldTransfer transfer, InventoryLocation src, InventoryLocation dst)
    {
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
