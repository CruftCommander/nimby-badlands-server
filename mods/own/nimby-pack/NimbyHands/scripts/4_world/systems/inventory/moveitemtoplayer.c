// -----------------------------------------------------------------------------
// NimbyHandsMoveItemToPlayer - moves an item from one player's hands into another
// player's cargo (never a worn slot, never hands) in two server steps (P3).
//
// Rewritten October 14, 2026 (review CR-001) to use the vanilla server path:
// Man.ServerTakeToDst on the player holding the item (Man.c:757-760), which routes a
// HandEventMoveTo through that player's own hand state machine
// (HumanInventory.RedirectToHandEvent, HumanInventory.c:200-229).
//
// Phase 1, October 19, 2026 (plan v0.5): every request is registered in the
// in-flight registry with a correlation id before dispatch (G5) and refused when
// its item already has an unresolved entry; consent, both players' state (G4),
// proximity, the source item, and the destination are revalidated immediately
// before dispatch; the outcome comes from the completion signals in
// NimbyHandsServer. Nothing is retried, cloned, restored, or deleted.
//
// P3, October 21, 2026 (plan v0.7, ruling J2): the engine refuses every
// hand-sourced move into a live player (NIMBY-T-010, H-C), so step 1 moves the
// item from the giver's hands into a free cargo location of the giver's own
// inventory (FindFreeLocationFor with ANY_CARGO, Inventory.c:276) and step 2, one
// tick after that landing, moves it from the recorded staged location to the
// receiver with the synchronous server move (Inventory.c:1057-1066). Step 1's
// return value is meaningless for a hands source; step 2's is the result. Step 2
// bypasses the engine's live-player rule and 2.5 m anti-cheat, so RevalidateStep2
// is the only guard. Cost paths: step 1 refused (no staging room) leaves the item
// in hands; step 2 refused (receiver filled between the steps) leaves it in the
// giver's cargo. Jeff ruled on October 21, 2026 that all three verbs deliver into
// cargo only, so an item that fits only a worn slot is a documented REFUSED.
//
// NIMBY-T-063, October 28, 2026: step 1 is not dispatched from inside the action.
// The request is registered (PENDING), NimbyHandsServer ends both consent loops and
// polls Poll until both players are free (cap NimbyHandsConstants.WAIT_CAP_MS), and
// only then Submit revalidates, writes REQUESTED and dispatches. The October 21
// production dispatched at once, from inside Take, Give or Put In Pockets, and the
// giver's hand state machine dropped every hand event (feasibility study H-B): 12 of
// 12 transfers UNCONFIRMED with the item still in hands. The October 20 diagnostic's
// V3 waited, and confirmed.
//
// NIMBY-T-063, October 29, 2026: step 2 no longer calls ServerTakeToDst as one native. It
// runs the same natives one at a time (NimbyHandsServer.MoveStep2) with a debug line after
// each, after three server crashes in step 2 on October 28; a crash log now names the native.
//
// NIMBY-T-063 probe, October 30, 2026 (build 10): for a connected giver (one with an identity)
// the wait tick only holds and enforces the cap; the dispatch runs from the giver's own
// CommandHandler (PlayerBase.ModCommandHandlerBefore -> NimbyHandsServer.OnCommandHandler ->
// Poll with inGiverFrame), the frame in which vanilla itself posts server hand events
// (HandleInventory, DayZPlayerImplement.c:2294). A dummy giver (no identity, dev harness)
// still dispatches from the wait tick: a dummy never runs a hand event, and the harness
// contract (Tick after Execute, then read the verdict) holds. Jeff ruled this one-build probe
// on October 30 over the client-driven step 1; if it crashes, the client-driven step 1 follows.
// -----------------------------------------------------------------------------

class NimbyHandsMoveItemToPlayer
{
    // Destinations a transfer may use: cargo only (Jeff, October 21, 2026). A function
    // rather than a static const: vanilla has no example of a const built from enum flags.
    static int DestinationFlags()
    {
        return FindInventoryLocationType.ANY_CARGO;
    }

    protected PlayerBase m_Requester;
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
    // NIMBY-T-063: the registered request while it waits, and when the wait began.
    protected ref NimbyHandsWorldTransfer m_Transfer;
    protected int m_WaitStartMs;
    // Build 10 probe: CommandHandler visits by the giver while this request waited (zero at a
    // wait cap says the hook never ran for this giver), and whether the hold line was written.
    protected int m_GiverTicks;
    protected bool m_HeldForGiverLogged;

    // requester is the player whose action made the request (the taker for Take, the
    // holder for Give and Put In Pockets); dst_player is the receiver.
    void NimbyHandsMoveItemToPlayer(PlayerBase requester, PlayerBase dst_player, EntityAI item, string action_name, float reach)
    {
        m_Requester = requester;
        m_DstPlayer = dst_player;
        m_Item = item;
        m_ActionName = action_name;
        m_Reach = reach;
        m_Consent = false;
        m_ConsentDetail = "not evaluated";
        m_HasExpected = false;
    }

    // The consenting party's state, read by the action at dispatch time. A snapshot: step 2
    // does not read it again (plan v0.7, decision H2).
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

    // True when the receiver has cargo room for the item. Used by action conditions so the
    // prompt only shows when the transfer can succeed.
    static bool ReceiverHasSpace(PlayerBase receiver, EntityAI item)
    {
        if (!receiver || !item)
        {
            return false;
        }
        InventoryLocation dst = new InventoryLocation;
        return receiver.GetInventory().FindFreeLocationFor(item, DestinationFlags(), dst);
    }

    // True when the giver has cargo room to stage the item in hands (cost path A). Used by
    // the action conditions so the prompt does not show for a transfer step 1 would refuse.
    static bool GiverHasSpace(PlayerBase giver, EntityAI item)
    {
        InventoryLocation dst = new InventoryLocation;
        return FindStagingLocation(giver, item, dst);
    }

    // A free cargo location in the giver's own inventory, never inside the moving item
    // itself: the engine walks every entity the giver carries, the item in hands included.
    static bool FindStagingLocation(PlayerBase giver, EntityAI item, InventoryLocation dst)
    {
        if (!giver || !item)
        {
            return false;
        }
        if (!giver.GetInventory().FindFreeLocationFor(item, DestinationFlags(), dst))
        {
            return false;
        }
        return !LocationInside(dst, item);
    }

    // The location's parent is the item or one of its descendants (EntityAI.GetHierarchyParent,
    // EntityAI.c:885).
    protected static bool LocationInside(InventoryLocation loc, EntityAI item)
    {
        EntityAI parent = loc.GetParent();
        int guard = 0;
        while (parent && guard < 32)
        {
            if (parent == item)
            {
                return true;
            }
            parent = parent.GetHierarchyParent();
            guard = guard + 1;
        }
        return false;
    }

    // Registers, revalidates, and dispatches step 1. Returns true when it was submitted and
    // not refused; the terminal outcome is logged by NimbyHandsServer.
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
        transfer.requester = m_Requester;
        transfer.receiver = m_DstPlayer;
        transfer.item = m_Item;
        if (m_Item)
        {
            transfer.giver = PlayerBase.Cast(m_Item.GetHierarchyRootPlayer());
        }
        transfer.reach = m_Reach;
        transfer.destinationFlags = DestinationFlags();
        transfer.consent = m_Consent;
        transfer.consentDetail = m_ConsentDetail;
        transfer.hasExpected = m_HasExpected;
        transfer.expectedItem = m_ExpectedItem;
        transfer.expectedGiverId = m_ExpectedGiverId;
        transfer.expectedReceiverId = m_ExpectedReceiverId;
        NimbyHandsServer.Describe(transfer);

        if (!m_Item)
        {
            NimbyHandsServer.LogUnregisteredRefusal(transfer, "no item");
            return false;
        }

        // A lost timer must not block an item until an unrelated timeout (review item 6).
        NimbyHandsServer.ReconcileOverdue();
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

        // NIMBY-T-063: nothing is validated or dispatched inside the action. The request holds
        // until both players are free; Poll revalidates and dispatches then.
        m_Transfer = transfer;
        m_WaitStartMs = GetGame().GetTime();
        NimbyHandsServer.Defer(this, transfer);
        return true;
    }

    // The registered request, or null before Execute or when Execute refused it unregistered.
    // The registry evicts a terminal entry, so this is how the dev harness reads a request's
    // state and detail after the wait tick.
    NimbyHandsWorldTransfer Transfer()
    {
        return m_Transfer;
    }

    // True when player is the giver this request waits on (the hierarchy root of the item at
    // registration). Used by the giver's CommandHandler to find its own requests.
    bool GiverIs(PlayerBase player)
    {
        if (!m_Transfer || !player)
        {
            return false;
        }
        return m_Transfer.giver == player;
    }

    // Polled every NimbyHandsConstants.WAIT_TICK_MS from the wait tick (inGiverFrame false) and
    // every frame from the giver's CommandHandler (inGiverFrame true) while the request waits.
    // Returns true when the wait is over: dispatched or refused. Build 10 probe: a connected
    // giver dispatches only from its own frame; the wait tick holds it and enforces the cap.
    bool Poll(int nowMs, bool inGiverFrame)
    {
        NimbyHandsWorldTransfer transfer = m_Transfer;
        if (!transfer || transfer.state != NimbyHandsState.PENDING)
        {
            return true;
        }
        if (inGiverFrame)
        {
            m_GiverTicks++;
        }
        int waited = nowMs - m_WaitStartMs;
        int capMs = NimbyHandsConstants.WAIT_CAP_MS;
        bool giverFree = NimbyHandsServer.IsFree(transfer.giver, true);
        bool receiverFree = NimbyHandsServer.IsFree(m_DstPlayer, false);
        int verdict = NimbyHandsRules.WaitVerdict(giverFree, receiverFree, waited, capMs);
        if (verdict == NimbyHandsRules.WAIT_HOLD)
        {
            return false;
        }
        if (verdict == NimbyHandsRules.WAIT_CAP)
        {
            NimbyHandsLog.Debug("id=" + transfer.id.ToString() + " wait cap waited=" + waited.ToString() + " giverFree=" + NimbyHandsLog.Bool(giverFree) + " receiverFree=" + NimbyHandsLog.Bool(receiverFree) + " giverTicks=" + m_GiverTicks.ToString());
            NimbyHandsServer.Advance(transfer, NimbyHandsState.REFUSED, NimbyHandsRules.WaitCapDetail(capMs));
            return true;
        }
        bool giverFrameOnly = NimbyHandsServer.DispatchesFromCommandHandler(transfer.giver);
        if (giverFrameOnly && !inGiverFrame)
        {
            if (!m_HeldForGiverLogged)
            {
                m_HeldForGiverLogged = true;
                NimbyHandsLog.Debug("id=" + transfer.id.ToString() + " wait free waited=" + waited.ToString() + " held for the giver's CommandHandler");
            }
            return false;
        }
        string site = "callQueue";
        if (inGiverFrame)
        {
            site = "commandHandler";
        }
        NimbyHandsLog.Debug("id=" + transfer.id.ToString() + " wait free waited=" + waited.ToString() + " site=" + site + " giverTicks=" + m_GiverTicks.ToString());
        // Consent was read when the request was made and the loop that carried it has ended;
        // open pockets are a standing state, so they are read again.
        if (m_ActionName == "NimbyHandsActionPutInPockets")
        {
            bool open = false;
            if (m_DstPlayer)
            {
                open = m_DstPlayer.NimbyHandsGetPocketsOpen();
            }
            SetConsent(open, "receiver's pockets are closed");
        }
        return Submit(transfer, waited, site);
    }

    // Revalidates, writes REQUESTED, dispatches step 1, and schedules the timeout. The
    // observation period starts here, not at registration. site names the dispatch frame
    // (build 10 probe): "commandHandler" or "callQueue".
    protected bool Submit(NimbyHandsWorldTransfer transfer, int waitedMs, string site)
    {
        InventoryLocation src = new InventoryLocation;
        InventoryLocation stage = new InventoryLocation;
        InventoryLocation dst = new InventoryLocation;
        NimbyHandsRequestFacts facts = CollectStep1Facts(transfer, src, stage, dst);
        string refusal = NimbyHandsRules.CheckRequest(facts, NimbyHandsRules.STEP1);
        if (refusal != "")
        {
            NimbyHandsServer.Advance(transfer, NimbyHandsState.REFUSED, refusal);
            return false;
        }

        transfer.startedMs = GetGame().GetTime();
        string requested = "stage=" + NimbyHandsServer.DescribeLocation(stage) + " dst=" + NimbyHandsServer.DescribeLocation(dst) + " waited=" + waitedMs.ToString() + " site=" + site;
        NimbyHandsServer.Advance(transfer, NimbyHandsState.REQUESTED, requested);
        bool accepted = NimbyHandsServer.Dispatch(transfer, src, stage);
        // A server hand event is deferred to the holder's next inventory update
        // (DayZPlayerInventory.c:4448, 4623-4634), so no signal is expected inside the call;
        // if one did land the item, its line is already written. Step 2 is run by the poll
        // started here, outside the engine's move callback (NimbyHandsServer.Step2Tick).
        if (NimbyHandsState.IsTerminal(transfer.state))
        {
            return transfer.state == NimbyHandsState.CONFIRMED;
        }
        if (transfer.state == NimbyHandsState.STAGED)
        {
            NimbyHandsServer.ScheduleTimeout(transfer);
            NimbyHandsServer.EnsureStep2Poll();
            return true;
        }
        if (!accepted)
        {
            NimbyHandsServer.Advance(transfer, NimbyHandsState.REFUSED, "server move rejected");
            return false;
        }
        NimbyHandsServer.ScheduleTimeout(transfer);
        NimbyHandsServer.EnsureStep2Poll();
        return true;
    }

    // Every fact step 1 needs, read from the world in precedence order, stopping at the first
    // fact that refuses (the rest keep their refusing defaults, which CheckRequest never
    // reaches). Fills src (the giver's hands), stage (the giver's own cargo), and dst (the
    // receiver's cargo).
    protected NimbyHandsRequestFacts CollectStep1Facts(NimbyHandsWorldTransfer transfer, InventoryLocation src, InventoryLocation stage, InventoryLocation dst)
    {
        NimbyHandsRequestFacts f = new NimbyHandsRequestFacts();
        PlayerBase giver = transfer.giver;
        f.consent = m_Consent;
        f.consentDetail = m_ConsentDetail;
        if (!f.consent)
        {
            return f;
        }
        f.giverParty = CheckPlayer("giver", giver);
        f.receiverParty = CheckPlayer("receiver", m_DstPlayer);
        if (f.giverParty != "" || f.receiverParty != "")
        {
            return f;
        }
        f.samePlayer = giver == m_DstPlayer;
        f.giverReplaced = m_HasExpected && m_ExpectedGiverId != "" && NimbyHandsActionData.IdentityId(giver) != m_ExpectedGiverId;
        f.receiverReplaced = m_HasExpected && m_ExpectedReceiverId != "" && NimbyHandsActionData.IdentityId(m_DstPlayer) != m_ExpectedReceiverId;
        if (f.samePlayer || f.giverReplaced || f.receiverReplaced)
        {
            return f;
        }
        f.itemExists = m_Item != null;
        if (!f.itemExists)
        {
            return f;
        }
        FillReach(f, giver, m_DstPlayer, m_Reach);
        if (!f.withinReach)
        {
            return f;
        }
        f.locationKnown = m_Item.GetInventory().GetCurrentInventoryLocation(src);
        if (!f.locationKnown)
        {
            return f;
        }
        f.itemAtSource = src.GetType() == InventoryLocationType.HANDS && src.GetParent() == giver;
        f.itemReplaced = m_HasExpected && m_ExpectedItem != m_Item;
        if (!f.itemAtSource || f.itemReplaced)
        {
            return f;
        }
        DayZPlayerInventory giverInventory = DayZPlayerInventory.Cast(giver.GetInventory());
        if (giverInventory && giverInventory.IsProcessing())
        {
            f.giverBusy = true;
            return f;
        }
        f.receiverHasSpace = m_DstPlayer.GetInventory().FindFreeLocationFor(m_Item, DestinationFlags(), dst);
        if (!f.receiverHasSpace)
        {
            return f;
        }
        f.giverHasSpace = FindStagingLocation(giver, m_Item, stage);
        f.destinationText = NimbyHandsServer.DescribeLocation(stage);
        if (!f.giverHasSpace)
        {
            return f;
        }
        // The engine test HandEventMoveTo.CanPerformEvent runs at queue time (Hand_Events.c:306-319,
        // Inventory.c:373). Its result cannot be observed after dispatch on 1.30, so it is asked here.
        f.engineAllows = GameInventory.LocationCanMoveEntity(src, stage);
        if (!f.engineAllows)
        {
            return f;
        }
        // The reservation test HandEventBase.ReserveInventory makes (Hand_Events.c:185,
        // Inventory.c:787). A reservation taken between here and the holder's next inventory
        // update can still refuse silently; that request ends at the reconcile.
        f.reserved = GameInventory.HasInventoryReservation(m_Item, stage);
        return f;
    }

    // ---- step 2 ----------------------------------------------------------------

    // Runs one tick after step 1 landed (NimbyHandsServer.OnStep2). Revalidates, then moves
    // the item from the staged location to the receiver with the synchronous server move and
    // reads its result. Only a STAGED transfer reaches here.
    static void RunStep2(NimbyHandsWorldTransfer transfer)
    {
        InventoryLocation src = new InventoryLocation;
        InventoryLocation dst = new InventoryLocation;
        NimbyHandsRequestFacts facts = CollectStep2Facts(transfer, src, dst);
        string refusal = NimbyHandsRules.CheckRequest(facts, NimbyHandsRules.STEP2);
        if (refusal != "")
        {
            NimbyHandsServer.Advance(transfer, NimbyHandsState.REFUSED, refusal + "; " + NimbyHandsServer.DescribeWhere(transfer));
            return;
        }

        // The natives ServerTakeToDst runs with neither end in hands, one per debug line
        // (NimbyHandsServer.MoveStep2; HumanInventory.c:243-251, Inventory.c:1057-1066). The
        // EEItemLocationChanged inside the first writes CONFIRMED; its return value is the result.
        bool moved = NimbyHandsServer.MoveStep2(transfer, src, dst);
        if (NimbyHandsState.IsTerminal(transfer.state))
        {
            return;
        }
        bool atGiverInventory = NimbyHandsServer.ItemInGiverInventory(transfer);
        int verdict = NimbyHandsRules.ClassifyStep2(moved, false, atGiverInventory);
        if (verdict == NimbyHandsState.REFUSED)
        {
            NimbyHandsServer.Advance(transfer, NimbyHandsState.REFUSED, "server move refused; " + NimbyHandsServer.DescribeWhere(transfer));
            return;
        }
        // Moved without a signal and not with the giver: left to the reconcile, which finds it.
        NimbyHandsLog.Debug("id=" + transfer.id.ToString() + " step 2 moved=" + NimbyHandsLog.Bool(moved) + " without a signal; left to the reconcile");
    }

    // Every fact step 2 needs, read from the world in precedence order, stopping at the first
    // fact that refuses: the consent snapshot, both parties, the expected identities, reach
    // with the action's own value, the exact staged location with the giver as root, and the
    // receiver destination recomputed now (the receiver may have filled it since step 1). No
    // busy test: it belongs to the hand event and may still read true for a frame after step 1.
    protected static NimbyHandsRequestFacts CollectStep2Facts(NimbyHandsWorldTransfer transfer, InventoryLocation src, InventoryLocation dst)
    {
        NimbyHandsRequestFacts f = new NimbyHandsRequestFacts();
        PlayerBase giver = transfer.giver;
        PlayerBase receiver = transfer.receiver;
        f.forcedRefusal = NimbyHandsServer.TakeForcedStep2Refusal();
        if (f.forcedRefusal)
        {
            return f;
        }
        f.consent = transfer.consent;
        f.consentDetail = transfer.consentDetail;
        if (!f.consent)
        {
            return f;
        }
        f.giverParty = CheckPlayer("giver", giver);
        f.receiverParty = CheckPlayer("receiver", receiver);
        if (f.giverParty != "" || f.receiverParty != "")
        {
            return f;
        }
        f.samePlayer = giver == receiver;
        f.giverReplaced = transfer.hasExpected && transfer.expectedGiverId != "" && NimbyHandsActionData.IdentityId(giver) != transfer.expectedGiverId;
        f.receiverReplaced = transfer.hasExpected && transfer.expectedReceiverId != "" && NimbyHandsActionData.IdentityId(receiver) != transfer.expectedReceiverId;
        if (f.samePlayer || f.giverReplaced || f.receiverReplaced)
        {
            return f;
        }
        f.itemExists = transfer.item != null;
        if (!f.itemExists)
        {
            return f;
        }
        FillReach(f, giver, receiver, transfer.reach);
        if (!f.withinReach)
        {
            return f;
        }
        f.locationKnown = transfer.item.GetInventory().GetCurrentInventoryLocation(src);
        if (!f.locationKnown)
        {
            return f;
        }
        // NIMBY-T-063 (two server crashes here on October 28, access violation in a native on
        // the first staged transfer, 0 ms and 50 ms after the signal): no hierarchy walk and no
        // cargo-cell compare in step 2. The item is at its source when its current location has
        // the staged type and the staged parent entity; the giver still owns that parent
        // because the request was revalidated 50 ms ago and the reach check above holds.
        NimbyHandsLog.Debug("id=" + transfer.id.ToString() + " step 2 facts: location read");
        f.itemAtSource = NimbyHandsServer.IsAtStagedLocation(transfer, src);
        NimbyHandsLog.Debug("id=" + transfer.id.ToString() + " step 2 facts: at source=" + NimbyHandsLog.Bool(f.itemAtSource));
        f.itemReplaced = transfer.hasExpected && transfer.expectedItem != transfer.item;
        if (!f.itemAtSource || f.itemReplaced)
        {
            return f;
        }
        f.giverBusy = false;
        f.receiverHasSpace = receiver.GetInventory().FindFreeLocationFor(transfer.item, transfer.destinationFlags, dst);
        f.destinationText = NimbyHandsServer.DescribeLocation(dst);
        if (!f.receiverHasSpace)
        {
            return f;
        }
        f.giverHasSpace = true;
        f.engineAllows = GameInventory.LocationCanMoveEntity(src, dst);
        f.reserved = false;
        return f;
    }

    protected static void FillReach(NimbyHandsRequestFacts f, PlayerBase giver, PlayerBase receiver, float reach)
    {
        if (!giver || !receiver)
        {
            return;
        }
        float distanceSq = vector.DistanceSq(giver.GetPosition(), receiver.GetPosition());
        f.withinReach = NimbyHandsRules.WithinReach(distanceSq, reach, NimbyHandsConstants.PROXIMITY_TOLERANCE);
        f.distanceText = Math.Sqrt(distanceSq).ToString();
    }

    static string CheckPlayer(string role, PlayerBase player)
    {
        if (!player)
        {
            return NimbyHandsRules.CheckParty(role, false, false, false, false, false, false, false);
        }
        // A dummy with no identity counts as connected only while the dev harness says so.
        bool connected = player.GetIdentity() != null || NimbyHandsServer.DummyPartiesAllowed();
        bool alive = player.IsAlive();
        bool conscious = !player.IsUnconscious();
        return NimbyHandsRules.CheckParty(role, true, connected, alive, conscious, player.IsRestrained(), player.IsInVehicle(), player.IsSwimming());
    }
}
