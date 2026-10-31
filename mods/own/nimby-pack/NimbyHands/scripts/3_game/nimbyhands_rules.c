// -----------------------------------------------------------------------------
// NimbyHands - engine-free transfer lifecycle, registry, and validators.
// Purpose : The state machine (REQUESTED, STAGED, CONFIRMED, REFUSED, UNCONFIRMED),
//           the in-flight registry (G5), the refusal rules, the pocket-toggle gate,
//           and the toast text as pure code, so the self-test covers them and the
//           4_World layer only feeds them facts it reads from the world (plan v0.7,
//           P3; shared rules 8).
// P3      : A transfer is two server moves under one id (ruling J2, October 21,
//           2026). Step 1 moves the item from the giver's hands into the giver's own
//           cargo; its arrival there is STAGED. Step 2 moves it from that staged
//           location to the receiver; its arrival there is CONFIRMED. Nothing is
//           retried, cloned, restored, or deleted on any branch.
// Registry: Keyed by the item's network id string, not the EntityAI reference: an item
//           deleted while in flight would leave a null key that can never be found or
//           evicted. The key still names exactly one item (G5 "keyed by item").
// Persists: Nothing. The registry lives in server memory for the mission.
// -----------------------------------------------------------------------------
class NimbyHandsState
{
    // Registered, validation not finished, nothing submitted. Never logged.
    static const int PENDING = 0;
    // Step 1 submitted to the engine (ServerTakeToDst on the holder, into the holder's cargo).
    static const int REQUESTED = 1;
    // The item arrived with the receiver (step 2's signal, or the reconcile found it there).
    static const int CONFIRMED = 2;
    // No delivery: validation or the engine refused, the item went somewhere other than the
    // receiver, or the reconcile found it still in the giver's inventory.
    static const int REFUSED = 3;
    // No evidence of delivery within the observation period and the item is not in the
    // giver's inventory.
    static const int UNCONFIRMED = 4;
    // Step 1 landed: the item is in the giver's own cargo at the recorded location and step 2
    // is scheduled. Not terminal; logged once per request. Never goes back to REQUESTED.
    static const int STAGED = 5;

    static string Name(int state)
    {
        if (state == PENDING)
        {
            return "PENDING";
        }
        if (state == REQUESTED)
        {
            return "REQUESTED";
        }
        if (state == STAGED)
        {
            return "STAGED";
        }
        if (state == CONFIRMED)
        {
            return "CONFIRMED";
        }
        if (state == REFUSED)
        {
            return "REFUSED";
        }
        if (state == UNCONFIRMED)
        {
            return "UNCONFIRMED";
        }
        return "UNKNOWN";
    }

    static bool IsTerminal(int state)
    {
        return state == CONFIRMED || state == REFUSED || state == UNCONFIRMED;
    }

    // The only legal moves. A terminal state never changes again, so one request has exactly
    // one terminal line and a later move of the item cannot turn CONFIRMED into a failure.
    // REQUESTED -> CONFIRMED stays legal for a direct delivery (the reconcile finds the item
    // with the receiver without a STAGED line); without it such an entry could never be evicted.
    static bool CanMove(int from, int to)
    {
        if (from == PENDING)
        {
            return to == REQUESTED || to == REFUSED;
        }
        if (from == REQUESTED)
        {
            return to == STAGED || IsTerminal(to);
        }
        if (from == STAGED)
        {
            return IsTerminal(to);
        }
        return false;
    }
}

class NimbyHandsTransfer
{
    int id;
    string itemKey;
    string action;
    int state;
    int startedMs;
    string detail;
}

class NimbyHandsTransfers
{
    protected ref map<string, ref NimbyHandsTransfer> m_ByItem;
    protected int m_NextId;
    protected int m_TimeoutMs;

    void NimbyHandsTransfers(int timeoutMs)
    {
        m_ByItem = new map<string, ref NimbyHandsTransfer>;
        m_NextId = 1;
        m_TimeoutMs = timeoutMs;
    }

    int Count()
    {
        return m_ByItem.Count();
    }

    int TimeoutMs()
    {
        return m_TimeoutMs;
    }

    NimbyHandsTransfer Find(string itemKey)
    {
        return m_ByItem.Get(itemKey);
    }

    NimbyHandsTransfer FindById(int id)
    {
        foreach (string key, NimbyHandsTransfer entry : m_ByItem)
        {
            if (entry.id == id)
            {
                return entry;
            }
        }
        return null;
    }

    // Appends every registered transfer in the given state to into (a snapshot: the caller
    // may advance or evict entries while it walks the array). Returns how many were added.
    int CollectByState(int state, notnull array<NimbyHandsTransfer> into)
    {
        int added = 0;
        foreach (string key, NimbyHandsTransfer entry : m_ByItem)
        {
            if (entry.state == state)
            {
                into.Insert(entry);
                added = added + 1;
            }
        }
        return added;
    }

    // A fresh correlation id. Begin uses it; a refusal that never enters the registry (no
    // item, item in flight) takes one too, so every logged request has its own id.
    int NewId()
    {
        int id = m_NextId;
        m_NextId = m_NextId + 1;
        return id;
    }

    // An entry exists only while it is unresolved: a terminal state evicts it.
    bool IsInFlight(string itemKey)
    {
        return m_ByItem.Contains(itemKey);
    }

    // True when this transfer is the registered entry for its item.
    bool IsRegistered(NimbyHandsTransfer transfer)
    {
        if (!transfer || transfer.itemKey == "")
        {
            return false;
        }
        return m_ByItem.Get(transfer.itemKey) == transfer;
    }

    // Registers a request with a new correlation id. Returns false, and leaves the transfer
    // unregistered, when the item has no key or already has an unresolved entry; the caller
    // logs REFUSED "item in flight".
    bool Begin(NimbyHandsTransfer transfer, string itemKey, string action, int nowMs)
    {
        if (!transfer || itemKey == "")
        {
            return false;
        }
        if (IsInFlight(itemKey))
        {
            return false;
        }
        transfer.id = NewId();
        transfer.itemKey = itemKey;
        transfer.action = action;
        transfer.state = NimbyHandsState.PENDING;
        transfer.startedMs = nowMs;
        transfer.detail = "";
        m_ByItem.Set(itemKey, transfer);
        return true;
    }

    // Moves a registered transfer to a new state. Returns false for an unregistered transfer
    // or a move the lifecycle forbids, and the transfer keeps its state. A terminal state
    // evicts the entry, so the caller must hold its own reference to log it.
    bool Move(NimbyHandsTransfer transfer, int state, string detail)
    {
        if (!IsRegistered(transfer))
        {
            return false;
        }
        if (!NimbyHandsState.CanMove(transfer.state, state))
        {
            return false;
        }
        transfer.state = state;
        transfer.detail = detail;
        if (NimbyHandsState.IsTerminal(state))
        {
            Evict(transfer);
        }
        return true;
    }

    protected void Evict(NimbyHandsTransfer transfer)
    {
        NimbyHandsTransfer current = m_ByItem.Get(transfer.itemKey);
        if (current == transfer)
        {
            m_ByItem.Remove(transfer.itemKey);
        }
    }

    // Unresolved entries whose observation period has run out. The caller reconciles each with
    // the world and moves it to a terminal state; this never moves or retries anything itself.
    int Due(int nowMs, notnull array<ref NimbyHandsTransfer> due)
    {
        foreach (string key, NimbyHandsTransfer entry : m_ByItem)
        {
            if (nowMs - entry.startedMs >= m_TimeoutMs)
            {
                due.Insert(entry);
            }
        }
        return due.Count();
    }
}

// What the world layer found immediately before a dispatch. Every positive fact starts false,
// so a fact the world layer forgot to record refuses instead of passing.
class NimbyHandsRequestFacts
{
    // Dev harness only (cost path B): one forced step-2 refusal. Read first.
    bool forcedRefusal;
    bool consent;
    string consentDetail;
    // "" when the party may take part, otherwise the CheckParty detail.
    string giverParty;
    string receiverParty;
    bool samePlayer;
    bool giverReplaced;
    bool receiverReplaced;
    bool itemExists;
    bool withinReach;
    string distanceText;
    bool locationKnown;
    // Step 1: the item is in the giver's hands. Step 2: at the exact staged location.
    bool itemAtSource;
    bool itemReplaced;
    // Step 1 only: the holder's inventory is still processing a hand event.
    bool giverBusy;
    bool receiverHasSpace;
    // Step 1 only: a cargo location in the giver's own inventory for the staging step.
    bool giverHasSpace;
    bool engineAllows;
    string destinationText;
    // Step 1 only.
    bool reserved;

    void NimbyHandsRequestFacts()
    {
        forcedRefusal = false;
        consent = false;
        consentDetail = "not evaluated";
        giverParty = "";
        receiverParty = "";
        samePlayer = false;
        giverReplaced = false;
        receiverReplaced = false;
        itemExists = false;
        withinReach = false;
        distanceText = "?";
        locationKnown = false;
        itemAtSource = false;
        itemReplaced = false;
        giverBusy = false;
        receiverHasSpace = false;
        giverHasSpace = false;
        engineAllows = false;
        destinationText = "none";
        reserved = false;
    }
}

class NimbyHandsRules
{
    // Returned by the classifiers when an observation says nothing about the request.
    static const int NO_CHANGE = -1;

    static const int STEP1 = 1;
    static const int STEP2 = 2;

    // Refusal families, for the toast text.
    static const int FAMILY_NONE = 0;
    static const int FAMILY_NO_ITEM = 1;
    static const int FAMILY_IN_FLIGHT = 2;
    static const int FAMILY_NO_CONSENT = 3;
    static const int FAMILY_SAME_PLAYER = 4;
    static const int FAMILY_GIVER_STATE = 5;
    static const int FAMILY_RECEIVER_STATE = 6;
    static const int FAMILY_REACH = 7;
    static const int FAMILY_ITEM_CHANGED = 8;
    static const int FAMILY_GIVER_BUSY = 9;
    static const int FAMILY_RECEIVER_SPACE = 10;
    static const int FAMILY_GIVER_SPACE = 11;
    static const int FAMILY_ENGINE = 12;
    static const int FAMILY_NO_DELIVERY = 13;
    static const int FAMILY_OTHER = 14;

    static const string DETAIL_FORCED = "no space in receiver inventory (forced by dev harness)";

    // Revalidation of one party immediately before dispatch (plan Phase 1; G4). Returns "" when
    // the party may take part, otherwise the refusal detail.
    static string CheckParty(string role, bool present, bool connected, bool alive, bool conscious, bool restrained, bool inVehicle, bool swimming)
    {
        if (!present)
        {
            return role + " missing";
        }
        if (!connected)
        {
            return role + " disconnected";
        }
        if (!alive)
        {
            return role + " dead";
        }
        if (!conscious)
        {
            return role + " unconscious";
        }
        if (restrained)
        {
            return role + " restrained";
        }
        if (inVehicle)
        {
            return role + " in vehicle";
        }
        if (swimming)
        {
            return role + " swimming";
        }
        return "";
    }

    // The players are still within the action's reach plus the drift tolerance.
    static bool WithinReach(float distanceSq, float reach, float tolerance)
    {
        float limit = reach + tolerance;
        return distanceSq <= limit * limit;
    }

    // Every check that must hold immediately before a dispatch, in precedence order. Returns ""
    // or the refusal detail. Step 1 runs the whole list; step 2 skips the busy test, the
    // staging-room test, and the reservation test, which belong to the hand event.
    static string CheckRequest(NimbyHandsRequestFacts f, int step)
    {
        if (f.forcedRefusal)
        {
            return DETAIL_FORCED;
        }
        if (!f.consent)
        {
            return "no consent: " + f.consentDetail;
        }
        if (f.giverParty != "")
        {
            return f.giverParty;
        }
        if (f.receiverParty != "")
        {
            return f.receiverParty;
        }
        if (f.samePlayer)
        {
            return "giver and receiver are the same player";
        }
        if (f.giverReplaced)
        {
            return "giver replaced since the action started";
        }
        if (f.receiverReplaced)
        {
            return "receiver replaced since the action started";
        }
        if (!f.itemExists)
        {
            return "item no longer exists";
        }
        if (!f.withinReach)
        {
            return "out of reach (" + f.distanceText + " m)";
        }
        if (!f.locationKnown)
        {
            return "item location unavailable";
        }
        if (!f.itemAtSource)
        {
            if (step == STEP2)
            {
                return "item is not at the staged location";
            }
            return "item is not in the giver's hands";
        }
        if (f.itemReplaced)
        {
            return "item replaced since the action started";
        }
        if (step == STEP1 && f.giverBusy)
        {
            return "giver inventory busy";
        }
        if (!f.receiverHasSpace)
        {
            return "no space in receiver inventory";
        }
        if (step == STEP1 && !f.giverHasSpace)
        {
            return "no space in giver inventory";
        }
        if (!f.engineAllows)
        {
            return "engine refuses the move to " + f.destinationText;
        }
        if (step == STEP1 && f.reserved)
        {
            return "destination reserved";
        }
        return "";
    }

    // What a location change of an open request means. From REQUESTED: a landing in the
    // giver's own inventory is step 1 done (STAGED); the giver's hands or the engine's
    // temporary location mean nothing (NO_CHANGE); the receiver is a direct delivery
    // (CONFIRMED); anywhere else refuses. From STAGED: only the receiver confirms; the
    // temporary location means nothing; any other move (back to hands, another pocket, the
    // ground) refuses, because step 2 moves only from the recorded staged location.
    static int ClassifyMove(int from, bool atReceiver, bool atGiverInventory, bool inGiverHands, bool isTemporary)
    {
        if (from != NimbyHandsState.REQUESTED && from != NimbyHandsState.STAGED)
        {
            return NO_CHANGE;
        }
        if (isTemporary)
        {
            return NO_CHANGE;
        }
        if (atReceiver)
        {
            return NimbyHandsState.CONFIRMED;
        }
        if (from == NimbyHandsState.STAGED)
        {
            return NimbyHandsState.REFUSED;
        }
        if (inGiverHands)
        {
            return NO_CHANGE;
        }
        if (atGiverInventory)
        {
            return NimbyHandsState.STAGED;
        }
        return NimbyHandsState.REFUSED;
    }

    // Step 2's synchronous verdict, read after the server move returns. The signal inside the
    // call already wrote CONFIRMED when the item arrived (the caller writes nothing more). A
    // move the engine refused, or one that left the item with the giver, is REFUSED. A move
    // reported done with no signal and the item not with the giver is left to the reconcile.
    static int ClassifyStep2(bool moved, bool signalConfirmed, bool atGiverInventory)
    {
        if (signalConfirmed)
        {
            return NimbyHandsState.CONFIRMED;
        }
        if (!moved || atGiverInventory)
        {
            return NimbyHandsState.REFUSED;
        }
        return NO_CHANGE;
    }

    // The reconcile at the timeout: delivered if the item is with the receiver; refused if it
    // is provably still in the giver's inventory; otherwise no evidence. Never a retry.
    static int ClassifyTimeout(bool itemExists, bool atReceiver, bool atGiverInventory)
    {
        if (itemExists && atReceiver)
        {
            return NimbyHandsState.CONFIRMED;
        }
        if (itemExists && atGiverInventory)
        {
            return NimbyHandsState.REFUSED;
        }
        return NimbyHandsState.UNCONFIRMED;
    }

    // NIMBY-T-063: what the step 1 wait does on each poll. Both players free dispatches at
    // once, however long the wait has been; otherwise the request holds until the cap, then
    // refuses. A capped request is logged as the giver being busy.
    static const int WAIT_HOLD = 0;
    static const int WAIT_DISPATCH = 1;
    static const int WAIT_CAP = 2;

    static int WaitVerdict(bool giverFree, bool receiverFree, int waitedMs, int capMs)
    {
        if (giverFree && receiverFree)
        {
            return WAIT_DISPATCH;
        }
        if (waitedMs < capMs)
        {
            return WAIT_HOLD;
        }
        return WAIT_CAP;
    }

    static string WaitCapDetail(int capMs)
    {
        return "giver inventory busy; players did not become free within " + capMs.ToString() + " ms";
    }

    // ---- the client-driven step 1 (NIMBY-T-063, October 31, 2026) -------------------------
    // Ten server crashes on 1.30.164200 followed a server-driven hand event on a connected giver
    // (ServerTakeToDst from the call queue and from the giver's own CommandHandler alike); the
    // same step 2 between two connected players with no hand event confirmed (S6-D). So a giver
    // with a client stages the item itself: the server sends NimbyHandsRpc.STAGE and the client
    // runs PredictiveTakeToDst from hands into its own cargo, the move every player-made
    // hands-to-cargo drag takes. A giver without a client (a dev harness dummy) keeps the server
    // path; a dummy never runs a hand event.
    static const string VIA_CLIENT = "client";
    static const string VIA_SERVER = "server";

    static string StageVia(bool giverHasIdentity)
    {
        if (giverHasIdentity)
        {
            return VIA_CLIENT;
        }
        return VIA_SERVER;
    }

    // What the giver's client found when the stage request arrived, in precedence order; ""
    // when the move can be posted. The text travels back in STAGE_REFUSED and becomes the detail.
    static string ClientStageRefusal(bool itemFound, bool inHands, bool canStoreInput, bool hasRoom)
    {
        if (!itemFound)
        {
            return "item not found on the giver's client";
        }
        if (!inHands)
        {
            return "item not in the giver's hands on the client";
        }
        if (!canStoreInput)
        {
            return "giver's client has inventory input pending";
        }
        if (!hasRoom)
        {
            return "no space in giver inventory on the client";
        }
        return "";
    }

    // The REFUSED detail written by the server from a STAGE_REFUSED reply.
    static string ClientStageRefusalDetail(string reason)
    {
        return "client could not stage: " + reason;
    }

    // A STAGE_REFUSED reply counts only from the giver's own identity for a request that is
    // still REQUESTED; anything else is ignored and logged (another client cannot refuse a
    // transfer it is not the giver of).
    static bool ClientStageRefusalAccepted(string senderId, string giverId, int state)
    {
        if (senderId == "" || giverId == "" || senderId != giverId)
        {
            return false;
        }
        return state == NimbyHandsState.REQUESTED;
    }

    // The server-side minimum interval between two pocket toggles of one player (review item
    // 3). A clock that moved backwards (a new mission) opens the gate.
    static bool PocketToggleAllowed(int lastMs, int nowMs, int minMs)
    {
        if (nowMs < lastMs)
        {
            return true;
        }
        return nowMs - lastMs >= minMs;
    }

    // ---- toast text (NIMBY-T-012, J4) ----------------------------------------------------

    static int RefusalFamily(string detail)
    {
        if (detail == "no item")
        {
            return FAMILY_NO_ITEM;
        }
        if (StartsWith(detail, "item in flight"))
        {
            return FAMILY_IN_FLIGHT;
        }
        if (StartsWith(detail, "no consent"))
        {
            return FAMILY_NO_CONSENT;
        }
        if (StartsWith(detail, "giver and receiver are the same"))
        {
            return FAMILY_SAME_PLAYER;
        }
        if (StartsWith(detail, "giver inventory busy"))
        {
            return FAMILY_GIVER_BUSY;
        }
        if (StartsWith(detail, "giver "))
        {
            return FAMILY_GIVER_STATE;
        }
        if (StartsWith(detail, "receiver "))
        {
            return FAMILY_RECEIVER_STATE;
        }
        if (StartsWith(detail, "out of reach"))
        {
            return FAMILY_REACH;
        }
        if (StartsWith(detail, "item "))
        {
            return FAMILY_ITEM_CHANGED;
        }
        if (StartsWith(detail, "no space in receiver"))
        {
            return FAMILY_RECEIVER_SPACE;
        }
        if (StartsWith(detail, "no space in giver"))
        {
            return FAMILY_GIVER_SPACE;
        }
        if (StartsWith(detail, "engine refuses") || StartsWith(detail, "server move") || StartsWith(detail, "destination reserved"))
        {
            return FAMILY_ENGINE;
        }
        if (StartsWith(detail, "no delivery") || StartsWith(detail, "no completion signal") || StartsWith(detail, "reconciled"))
        {
            return FAMILY_NO_DELIVERY;
        }
        // The giver's client could not post the stage move (client-driven step 1): the giver
        // retries, so it reads as the giver being busy.
        if (StartsWith(detail, "client could not stage"))
        {
            return FAMILY_GIVER_BUSY;
        }
        return FAMILY_OTHER;
    }

    // Where the item ended, from the "item with giver at ..." clause the server appends to a
    // terminal detail. "" when the detail carries no such clause.
    static string LocationHint(string detail, string role)
    {
        bool giver = role == "giver";
        if (detail.Contains("item with giver at HANDS"))
        {
            if (giver)
            {
                return " It is still in your hands.";
            }
            return " It is still in the giver's hands.";
        }
        if (detail.Contains("item with giver at"))
        {
            if (giver)
            {
                return " It is in your inventory.";
            }
            return " It stayed in the giver's inventory.";
        }
        if (detail.Contains("item with receiver at"))
        {
            if (giver)
            {
                return " It is with the other player.";
            }
            return " It is in your inventory.";
        }
        if (detail.Contains("item no longer exists"))
        {
            return " The item no longer exists.";
        }
        if (detail.Contains("item at "))
        {
            return " The item is with neither player; check the ground.";
        }
        return "";
    }

    // The on-screen text for one terminal state, for the giver, the receiver, or the
    // requester of an unregistered refusal. Never empty.
    static string ToastText(int state, string detail, string role)
    {
        bool giver = role == "giver";
        bool receiver = role == "receiver";
        if (state == NimbyHandsState.CONFIRMED)
        {
            if (giver)
            {
                return "Item handed over.";
            }
            if (receiver)
            {
                return "Item received.";
            }
            return "Item transferred.";
        }
        if (state == NimbyHandsState.UNCONFIRMED)
        {
            if (giver)
            {
                return "Transfer outcome unknown; check your hands and inventory." + LocationHint(detail, role);
            }
            return "Transfer outcome unknown; check your inventory." + LocationHint(detail, role);
        }
        if (state != NimbyHandsState.REFUSED)
        {
            return "Transfer " + NimbyHandsState.Name(state) + ".";
        }
        int family = RefusalFamily(detail);
        string text = "Transfer refused.";
        if (family == FAMILY_NO_ITEM)
        {
            text = "Nothing in hand to transfer.";
        }
        else if (family == FAMILY_IN_FLIGHT)
        {
            text = "That item is already being transferred.";
        }
        else if (family == FAMILY_NO_CONSENT)
        {
            text = ConsentText(detail, giver);
        }
        else if (family == FAMILY_SAME_PLAYER)
        {
            text = "You cannot transfer to yourself.";
        }
        else if (family == FAMILY_GIVER_STATE)
        {
            text = StateText(detail, "giver", giver, receiver);
        }
        else if (family == FAMILY_RECEIVER_STATE)
        {
            text = StateText(detail, "receiver", giver, receiver);
        }
        else if (family == FAMILY_REACH)
        {
            text = "Too far apart.";
        }
        else if (family == FAMILY_ITEM_CHANGED)
        {
            text = "The item moved before it could be transferred.";
        }
        else if (family == FAMILY_GIVER_BUSY)
        {
            text = BusyText(giver);
        }
        else if (family == FAMILY_RECEIVER_SPACE)
        {
            text = ReceiverSpaceText(receiver);
        }
        else if (family == FAMILY_GIVER_SPACE)
        {
            text = GiverSpaceText(giver);
        }
        else if (family == FAMILY_ENGINE)
        {
            text = "The game refused the move.";
        }
        else if (family == FAMILY_NO_DELIVERY)
        {
            text = "Transfer did not complete.";
        }
        return text + LocationHint(detail, role);
    }

    protected static string BusyText(bool giver)
    {
        if (giver)
        {
            return "Your hands are busy; try again.";
        }
        return "The giver's hands are busy; try again.";
    }

    protected static string ReceiverSpaceText(bool receiver)
    {
        if (receiver)
        {
            return "You have no room for it.";
        }
        return "The receiver has no room for it.";
    }

    protected static string GiverSpaceText(bool giver)
    {
        if (giver)
        {
            return "You need a free cargo slot to hand the item over; it stays in your hands.";
        }
        return "The giver has no free cargo slot to hand the item over.";
    }

    protected static string ConsentText(string detail, bool giver)
    {
        if (detail.Contains("pockets"))
        {
            if (giver)
            {
                return "Their pockets are closed.";
            }
            return "Your pockets are closed.";
        }
        if (detail.Contains("requesting"))
        {
            if (giver)
            {
                return "They are no longer requesting.";
            }
            return "You are no longer requesting.";
        }
        if (detail.Contains("offering"))
        {
            if (giver)
            {
                return "You are no longer offering.";
            }
            return "They are no longer offering.";
        }
        return "Consent withdrawn.";
    }

    // "<party> <state>" details: "giver dead", "receiver restrained", "receiver disconnected".
    protected static string StateText(string detail, string party, bool giver, bool receiver)
    {
        string state = detail;
        int cut = party.Length() + 1;
        if (detail.Length() > cut)
        {
            state = detail.Substring(cut, detail.Length() - cut);
        }
        bool self = (party == "giver" && giver) || (party == "receiver" && receiver);
        if (self)
        {
            return "You cannot transfer right now (" + state + ").";
        }
        if (party == "giver")
        {
            return "The giver cannot transfer right now (" + state + ").";
        }
        return "The receiver cannot take it right now (" + state + ").";
    }

    static bool StartsWith(string text, string prefix)
    {
        int length = prefix.Length();
        if (text.Length() < length)
        {
            return false;
        }
        return text.Substring(0, length) == prefix;
    }

    // ---- self-test -------------------------------------------------------------------------

    // Engine-free validators for the $profile:NimbyHands\selftest flag (shared rules 8).
    static int SelfTest(out array<string> results)
    {
        int failures = 0;
        int PENDING = NimbyHandsState.PENDING;
        int REQUESTED = NimbyHandsState.REQUESTED;
        int STAGED = NimbyHandsState.STAGED;
        int CONFIRMED = NimbyHandsState.CONFIRMED;
        int REFUSED = NimbyHandsState.REFUSED;
        int UNCONFIRMED = NimbyHandsState.UNCONFIRMED;

        failures += Case(results, "pending to requested is legal", NimbyHandsState.CanMove(PENDING, REQUESTED));
        failures += Case(results, "pending to refused is legal", NimbyHandsState.CanMove(PENDING, REFUSED));
        failures += Case(results, "pending cannot confirm before submission", !NimbyHandsState.CanMove(PENDING, CONFIRMED));
        failures += Case(results, "pending cannot stage before submission", !NimbyHandsState.CanMove(PENDING, STAGED));
        failures += Case(results, "requested to staged is legal", NimbyHandsState.CanMove(REQUESTED, STAGED));
        failures += Case(results, "requested to refused is legal", NimbyHandsState.CanMove(REQUESTED, REFUSED));
        failures += Case(results, "requested to unconfirmed is legal", NimbyHandsState.CanMove(REQUESTED, UNCONFIRMED));
        failures += Case(results, "requested to confirmed stays legal for a direct delivery", NimbyHandsState.CanMove(REQUESTED, CONFIRMED));
        failures += Case(results, "requested never goes back to pending", !NimbyHandsState.CanMove(REQUESTED, PENDING));
        failures += Case(results, "staged to confirmed is legal", NimbyHandsState.CanMove(STAGED, CONFIRMED));
        failures += Case(results, "staged to refused is legal", NimbyHandsState.CanMove(STAGED, REFUSED));
        failures += Case(results, "staged to unconfirmed is legal", NimbyHandsState.CanMove(STAGED, UNCONFIRMED));
        failures += Case(results, "staged never goes back to requested", !NimbyHandsState.CanMove(STAGED, REQUESTED));
        failures += Case(results, "staged cannot stage again", !NimbyHandsState.CanMove(STAGED, STAGED));
        failures += Case(results, "confirmed never becomes refused", !NimbyHandsState.CanMove(CONFIRMED, REFUSED));
        failures += Case(results, "confirmed never becomes staged", !NimbyHandsState.CanMove(CONFIRMED, STAGED));
        failures += Case(results, "unconfirmed never becomes confirmed", !NimbyHandsState.CanMove(UNCONFIRMED, CONFIRMED));
        failures += Case(results, "refused never becomes requested", !NimbyHandsState.CanMove(REFUSED, REQUESTED));
        failures += Case(results, "staged is not terminal", !NimbyHandsState.IsTerminal(STAGED));
        failures += Case(results, "state names match the log format", NimbyHandsState.Name(UNCONFIRMED) == "UNCONFIRMED");
        failures += Case(results, "staged name matches the log format", NimbyHandsState.Name(STAGED) == "STAGED");

        NimbyHandsTransfers registry = new NimbyHandsTransfers(5000);
        NimbyHandsTransfer first = new NimbyHandsTransfer();
        NimbyHandsTransfer second = new NimbyHandsTransfer();
        NimbyHandsTransfer other = new NimbyHandsTransfer();
        NimbyHandsTransfer stray = new NimbyHandsTransfer();
        failures += Case(results, "move on an unregistered transfer is refused", !registry.Move(stray, REQUESTED, ""));
        failures += Case(results, "unregistered transfer keeps pending", stray.state == PENDING);
        failures += Case(results, "first request registers", registry.Begin(first, "1:100", "Take", 1000));
        failures += Case(results, "second request on the same item is refused in flight", !registry.Begin(second, "1:100", "Take", 1100));
        failures += Case(results, "refused second request gets no id", second.id == 0);
        failures += Case(results, "request on another item registers", registry.Begin(other, "1:200", "Give", 1100));
        failures += Case(results, "correlation ids are distinct", first.id != other.id && first.id > 0);
        failures += Case(results, "new entry starts pending", first.state == PENDING);
        failures += Case(results, "empty item key is refused", !registry.Begin(new NimbyHandsTransfer(), "", "Take", 1000));
        failures += Case(results, "find by id returns the entry", registry.FindById(other.id) == other);

        failures += Case(results, "submit moves to requested", registry.Move(first, REQUESTED, ""));
        failures += Case(results, "requested entry stays in flight", registry.IsInFlight("1:100"));
        failures += Case(results, "step 1 landing moves to staged", registry.Move(first, STAGED, "signal"));
        failures += Case(results, "staged entry stays in flight", registry.IsInFlight("1:100"));
        failures += Case(results, "second request on a staged item is refused", !registry.Begin(second, "1:100", "Take", 1200));
        failures += Case(results, "staged entry keeps its detail", first.detail == "signal");
        failures += Case(results, "confirm moves to confirmed", registry.Move(first, CONFIRMED, "signal"));
        failures += Case(results, "terminal state evicts the entry", !registry.IsInFlight("1:100"));
        failures += Case(results, "second terminal line is refused", !registry.Move(first, REFUSED, "late"));
        failures += Case(results, "confirmed transfer keeps its state", first.state == CONFIRMED);
        failures += Case(results, "evicted item accepts a new request", registry.Begin(second, "1:100", "Give", 2000));

        array<ref NimbyHandsTransfer> due = new array<ref NimbyHandsTransfer>;
        failures += Case(results, "nothing is due before the timeout", registry.Due(5999, due) == 0);
        due.Clear();
        failures += Case(results, "the older entry is due at the timeout", registry.Due(6100, due) == 1 && due[0] == other);
        registry.Move(other, REQUESTED, "");
        registry.Move(other, UNCONFIRMED, "no evidence");
        failures += Case(results, "eviction after the timeout frees the item", !registry.IsInFlight("1:200"));
        due.Clear();
        failures += Case(results, "the newer entry is due later", registry.Due(7000, due) == 1 && due[0] == second);
        registry.Move(second, REFUSED, "no space");
        failures += Case(results, "registry is empty after every terminal state", registry.Count() == 0);

        failures += Case(results, "healthy party passes", CheckParty("giver", true, true, true, true, false, false, false) == "");
        failures += Case(results, "missing party refuses", CheckParty("receiver", false, true, true, true, false, false, false) == "receiver missing");
        failures += Case(results, "disconnected party refuses", CheckParty("giver", true, false, true, true, false, false, false) == "giver disconnected");
        failures += Case(results, "dead party refuses", CheckParty("giver", true, true, false, true, false, false, false) == "giver dead");
        failures += Case(results, "unconscious party refuses", CheckParty("receiver", true, true, true, false, false, false, false) == "receiver unconscious");
        failures += Case(results, "restrained party refuses", CheckParty("receiver", true, true, true, true, true, false, false) == "receiver restrained");
        failures += Case(results, "party in a vehicle refuses", CheckParty("giver", true, true, true, true, false, true, false) == "giver in vehicle");
        failures += Case(results, "swimming party refuses", CheckParty("giver", true, true, true, true, false, false, true) == "giver swimming");
        failures += Case(results, "dead and restrained party logs dead", CheckParty("giver", true, true, false, true, true, false, false) == "giver dead");
        failures += Case(results, "disconnected and unconscious party logs disconnected", CheckParty("receiver", true, false, true, false, false, false, false) == "receiver disconnected");
        failures += Case(results, "within reach passes", WithinReach(4.0, 2.0, 0.5));
        failures += Case(results, "inside the drift tolerance passes", WithinReach(6.0, 2.0, 0.5));
        failures += Case(results, "beyond reach refuses", !WithinReach(6.5, 2.0, 0.5));

        failures += SelfTestCheckRequest(results);

        failures += Case(results, "requested: landing in the giver's inventory stages", ClassifyMove(REQUESTED, false, true, false, false) == STAGED);
        failures += Case(results, "requested: still in the giver's hands changes nothing", ClassifyMove(REQUESTED, false, false, true, false) == NO_CHANGE);
        failures += Case(results, "requested: temporary location changes nothing", ClassifyMove(REQUESTED, false, false, false, true) == NO_CHANGE);
        failures += Case(results, "requested: item going elsewhere refuses", ClassifyMove(REQUESTED, false, false, false, false) == REFUSED);
        failures += Case(results, "requested: direct arrival at the receiver confirms", ClassifyMove(REQUESTED, true, false, false, false) == CONFIRMED);
        failures += Case(results, "staged: arrival at the receiver confirms", ClassifyMove(STAGED, true, false, false, false) == CONFIRMED);
        failures += Case(results, "staged: back in the giver's hands refuses", ClassifyMove(STAGED, false, false, true, false) == REFUSED);
        failures += Case(results, "staged: moved within the giver's inventory refuses", ClassifyMove(STAGED, false, true, false, false) == REFUSED);
        failures += Case(results, "staged: item going elsewhere refuses", ClassifyMove(STAGED, false, false, false, false) == REFUSED);
        failures += Case(results, "staged: temporary location changes nothing", ClassifyMove(STAGED, false, false, false, true) == NO_CHANGE);
        failures += Case(results, "pending: a location change changes nothing", ClassifyMove(PENDING, true, false, false, false) == NO_CHANGE);
        failures += Case(results, "confirmed: a later move changes nothing", ClassifyMove(CONFIRMED, false, false, false, false) == NO_CHANGE);

        failures += Case(results, "step 2 signal confirms", ClassifyStep2(true, true, false) == CONFIRMED);
        failures += Case(results, "step 2 engine refusal refuses", ClassifyStep2(false, false, true) == REFUSED);
        failures += Case(results, "step 2 moved but still with the giver refuses", ClassifyStep2(true, false, true) == REFUSED);
        failures += Case(results, "step 2 moved without a signal waits for the reconcile", ClassifyStep2(true, false, false) == NO_CHANGE);

        failures += Case(results, "timeout with the item at the receiver confirms", ClassifyTimeout(true, true, false) == CONFIRMED);
        failures += Case(results, "timeout with the item in the giver's inventory refuses", ClassifyTimeout(true, false, true) == REFUSED);
        failures += Case(results, "timeout with the item elsewhere is unconfirmed", ClassifyTimeout(true, false, false) == UNCONFIRMED);
        failures += Case(results, "timeout with the item gone is unconfirmed", ClassifyTimeout(false, false, false) == UNCONFIRMED);
        failures += Case(results, "timeout with the item gone never refuses on a stale flag", ClassifyTimeout(false, false, true) == UNCONFIRMED);

        failures += Case(results, "wait: both players free dispatches", WaitVerdict(true, true, 0, 3000) == WAIT_DISPATCH);
        failures += Case(results, "wait: busy giver holds inside the cap", WaitVerdict(false, true, 2999, 3000) == WAIT_HOLD);
        failures += Case(results, "wait: busy receiver holds inside the cap", WaitVerdict(true, false, 10, 3000) == WAIT_HOLD);
        failures += Case(results, "wait: still busy at the cap refuses", WaitVerdict(false, false, 3000, 3000) == WAIT_CAP);
        failures += Case(results, "wait: free at the cap still dispatches", WaitVerdict(true, true, 4000, 3000) == WAIT_DISPATCH);
        failures += Case(results, "wait: cap detail is the busy family", RefusalFamily(WaitCapDetail(3000)) == FAMILY_GIVER_BUSY);
        failures += Case(results, "wait: cap detail names the cap", WaitCapDetail(3000) == "giver inventory busy; players did not become free within 3000 ms");

        failures += Case(results, "first pocket toggle is allowed", PocketToggleAllowed(-500, 0, 500));
        failures += Case(results, "pocket toggle inside the interval is refused", !PocketToggleAllowed(1000, 1300, 500));
        failures += Case(results, "pocket toggle at the interval is allowed", PocketToggleAllowed(1000, 1500, 500));
        failures += Case(results, "pocket toggle after a clock reset is allowed", PocketToggleAllowed(90000, 100, 500));

        failures += SelfTestToast(results);

        failures += Case(results, "plain name is unchanged", NimbyHandsLog.Escape("Trendkill") == "Trendkill");
        failures += Case(results, "quote in a name is escaped", NimbyHandsLog.Escape("a\"b") == "a\\\"b");
        failures += Case(results, "backslash in a name is escaped", NimbyHandsLog.Escape("a\\b") == "a\\\\b");
        failures += Case(results, "newline in a name is escaped", NimbyHandsLog.Escape("a\nb") == "a\\nb");
        failures += Case(results, "forged field stays inside the quotes", NimbyHandsLog.Quote("x\" receiver=\"y") == "\"x\\\" receiver=\\\"y\"");
        return failures;
    }

    // The refusal details and their precedence: start with every fact failing and clear them
    // one at a time in the order CheckRequest consults them.
    protected static int SelfTestCheckRequest(out array<string> results)
    {
        int failures = 0;
        NimbyHandsRequestFacts f = new NimbyHandsRequestFacts();
        f.consentDetail = "giver is no longer offering";
        f.giverParty = "giver dead";
        f.receiverParty = "receiver restrained";
        f.samePlayer = true;
        f.giverReplaced = true;
        f.receiverReplaced = true;
        f.distanceText = "3.1";
        f.itemReplaced = true;
        f.giverBusy = true;
        f.destinationText = "CARGO:Test";
        f.reserved = true;

        failures += Case(results, "fresh facts refuse on consent", CheckRequest(f, STEP1) == "no consent: giver is no longer offering");
        f.consent = true;
        failures += Case(results, "giver state precedes receiver state", CheckRequest(f, STEP1) == "giver dead");
        f.giverParty = "";
        failures += Case(results, "receiver state precedes the same-player test", CheckRequest(f, STEP1) == "receiver restrained");
        f.receiverParty = "";
        failures += Case(results, "same player precedes the replaced tests", CheckRequest(f, STEP1) == "giver and receiver are the same player");
        f.samePlayer = false;
        failures += Case(results, "giver replaced precedes receiver replaced", CheckRequest(f, STEP1) == "giver replaced since the action started");
        f.giverReplaced = false;
        failures += Case(results, "receiver replaced precedes the item tests", CheckRequest(f, STEP1) == "receiver replaced since the action started");
        f.receiverReplaced = false;
        failures += Case(results, "missing item precedes reach", CheckRequest(f, STEP1) == "item no longer exists");
        f.itemExists = true;
        failures += Case(results, "reach precedes the location tests", CheckRequest(f, STEP1) == "out of reach (3.1 m)");
        f.withinReach = true;
        failures += Case(results, "unknown location precedes the source test", CheckRequest(f, STEP1) == "item location unavailable");
        f.locationKnown = true;
        failures += Case(results, "step 1 source must be the giver's hands", CheckRequest(f, STEP1) == "item is not in the giver's hands");
        failures += Case(results, "step 2 source must be the staged location", CheckRequest(f, STEP2) == "item is not at the staged location");
        f.itemAtSource = true;
        failures += Case(results, "item replaced precedes the busy test", CheckRequest(f, STEP1) == "item replaced since the action started");
        f.itemReplaced = false;
        failures += Case(results, "busy giver precedes the space tests", CheckRequest(f, STEP1) == "giver inventory busy");
        failures += Case(results, "step 2 ignores the busy test", CheckRequest(f, STEP2) == "no space in receiver inventory");
        f.giverBusy = false;
        failures += Case(results, "receiver space precedes giver space", CheckRequest(f, STEP1) == "no space in receiver inventory");
        f.receiverHasSpace = true;
        failures += Case(results, "step 1 needs staging room in the giver's inventory", CheckRequest(f, STEP1) == "no space in giver inventory");
        failures += Case(results, "step 2 ignores the staging-room test", CheckRequest(f, STEP2) == "engine refuses the move to CARGO:Test");
        f.giverHasSpace = true;
        failures += Case(results, "engine refusal precedes the reservation test", CheckRequest(f, STEP1) == "engine refuses the move to CARGO:Test");
        f.engineAllows = true;
        failures += Case(results, "reservation is the last step 1 refusal", CheckRequest(f, STEP1) == "destination reserved");
        failures += Case(results, "step 2 ignores the reservation test", CheckRequest(f, STEP2) == "");
        f.reserved = false;
        failures += Case(results, "every fact passing dispatches step 1", CheckRequest(f, STEP1) == "");
        f.forcedRefusal = true;
        failures += Case(results, "forced refusal precedes every other fact", CheckRequest(f, STEP2) == DETAIL_FORCED);
        f.forcedRefusal = false;
        failures += Case(results, "forced refusal is a receiver-space family", RefusalFamily(DETAIL_FORCED) == FAMILY_RECEIVER_SPACE);
        return failures;
    }

    // Every terminal state and refusal family yields a non-empty, role-correct text.
    protected static int SelfTestToast(out array<string> results)
    {
        int failures = 0;
        int CONFIRMED = NimbyHandsState.CONFIRMED;
        int REFUSED = NimbyHandsState.REFUSED;
        int UNCONFIRMED = NimbyHandsState.UNCONFIRMED;
        array<string> details = new array<string>;
        details.Insert("no item");
        details.Insert("item in flight (id=3)");
        details.Insert("no consent: giver is no longer offering");
        details.Insert("no consent: receiver is no longer requesting");
        details.Insert("no consent: receiver's pockets are closed");
        details.Insert("giver and receiver are the same player");
        details.Insert("giver dead");
        details.Insert("receiver disconnected");
        details.Insert("giver replaced since the action started");
        details.Insert("out of reach (3.2 m)");
        details.Insert("item no longer exists");
        details.Insert("item location unavailable");
        details.Insert("item is not in the giver's hands");
        details.Insert("item is not at the staged location; item with giver at CARGO:Shirt@x");
        details.Insert("item replaced since the action started");
        details.Insert("item left the giver for another place before delivery; signal=EEItemLocationChanged dst=GROUND");
        details.Insert("giver inventory busy");
        details.Insert("no space in receiver inventory");
        details.Insert("no space in receiver inventory; item with giver at CARGO:Shirt@x");
        details.Insert(DETAIL_FORCED + "; item with giver at CARGO:Shirt@x");
        details.Insert("no space in giver inventory");
        details.Insert("engine refuses the move to CARGO:Shirt@x");
        details.Insert("server move refused; item with giver at CARGO:Shirt@x");
        details.Insert("destination reserved");
        details.Insert("no delivery within 5000 ms; item with giver at CARGO:Shirt@x");
        details.Insert("something new");
        foreach (string detail : details)
        {
            string giverText = ToastText(REFUSED, detail, "giver");
            string receiverText = ToastText(REFUSED, detail, "receiver");
            string requesterText = ToastText(REFUSED, detail, "requester");
            bool ok = giverText != "" && receiverText != "" && requesterText != "";
            failures += Case(results, "toast for refusal '" + detail + "' is never empty", ok);
        }
        failures += Case(results, "confirmed toast differs by role", ToastText(CONFIRMED, "signal", "giver") != ToastText(CONFIRMED, "signal", "receiver"));
        failures += Case(results, "confirmed toast is never empty", ToastText(CONFIRMED, "", "requester") != "");
        failures += Case(results, "unconfirmed toast names the giver's hands", ToastText(UNCONFIRMED, "no completion signal within 5000 ms; item with giver at HANDS:X@y", "giver").Contains("hands"));
        failures += Case(results, "unconfirmed toast for the receiver is never empty", ToastText(UNCONFIRMED, "no completion signal within 5000 ms; item no longer exists", "receiver") != "");
        failures += Case(results, "giver-state toast addresses the giver as you", ToastText(REFUSED, "giver dead", "giver").Contains("You cannot"));
        failures += Case(results, "giver-state toast tells the receiver about the giver", ToastText(REFUSED, "giver dead", "receiver").Contains("giver"));
        failures += Case(results, "receiver-state toast addresses the receiver as you", ToastText(REFUSED, "receiver restrained", "receiver").Contains("You cannot"));
        failures += Case(results, "receiver-state toast tells the giver about the receiver", ToastText(REFUSED, "receiver restrained", "giver").Contains("receiver"));
        failures += Case(results, "staging-room toast tells the giver the item stays in hand", ToastText(REFUSED, "no space in giver inventory", "giver").Contains("stays in your hands"));
        failures += Case(results, "step 2 refusal toast tells the giver where the item is", ToastText(REFUSED, "no space in receiver inventory; item with giver at CARGO:Shirt@x", "giver").Contains("in your inventory"));
        failures += Case(results, "step 2 refusal toast tells the receiver where the item is", ToastText(REFUSED, "no space in receiver inventory; item with giver at CARGO:Shirt@x", "receiver").Contains("giver's inventory"));
        failures += Case(results, "pocket consent toast addresses the receiver", ToastText(REFUSED, "no consent: receiver's pockets are closed", "receiver").Contains("Your pockets"));
        failures += Case(results, "offer consent toast addresses the giver", ToastText(REFUSED, "no consent: giver is no longer offering", "giver").Contains("You are no longer offering"));
        failures += Case(results, "no-item toast is role neutral", ToastText(REFUSED, "no item", "requester") == "Nothing in hand to transfer.");
        failures += Case(results, "in-flight toast is role neutral", ToastText(REFUSED, "item in flight (id=2)", "requester") == "That item is already being transferred.");
        failures += Case(results, "refusal families are distinct for the main details", RefusalFamily("giver dead") == FAMILY_GIVER_STATE && RefusalFamily("giver inventory busy") == FAMILY_GIVER_BUSY);
        failures += Case(results, "unknown detail falls back to other", RefusalFamily("something new") == FAMILY_OTHER);
        failures += Case(results, "stage rpc ids are fixed", NimbyHandsRpc.STAGE == 6552101 && NimbyHandsRpc.STAGE_REFUSED == 6552102);
        failures += Case(results, "stage via: a giver with an identity stages from its client", StageVia(true) == VIA_CLIENT);
        failures += Case(results, "stage via: a dummy giver stages from the server", StageVia(false) == VIA_SERVER);
        failures += Case(results, "client stage: every check true posts the move", ClientStageRefusal(true, true, true, true) == "");
        failures += Case(results, "client stage: missing item refuses first", ClientStageRefusal(false, false, false, false) == "item not found on the giver's client");
        failures += Case(results, "client stage: item not in hands refuses", ClientStageRefusal(true, false, true, true) == "item not in the giver's hands on the client");
        failures += Case(results, "client stage: pending input refuses", ClientStageRefusal(true, true, false, true) == "giver's client has inventory input pending");
        failures += Case(results, "client stage: no room refuses", ClientStageRefusal(true, true, true, false) == "no space in giver inventory on the client");
        failures += Case(results, "client stage refusal detail names the client", ClientStageRefusalDetail("x") == "client could not stage: x");
        failures += Case(results, "client stage refusal detail is the giver-busy family", RefusalFamily(ClientStageRefusalDetail("giver's client has inventory input pending")) == FAMILY_GIVER_BUSY);
        failures += Case(results, "stage refused: accepted from the giver while requested", ClientStageRefusalAccepted("a", "a", NimbyHandsState.REQUESTED));
        failures += Case(results, "stage refused: another identity is ignored", !ClientStageRefusalAccepted("b", "a", NimbyHandsState.REQUESTED));
        failures += Case(results, "stage refused: empty ids are ignored", !ClientStageRefusalAccepted("", "", NimbyHandsState.REQUESTED));
        failures += Case(results, "stage refused: a staged request is not refused by the client", !ClientStageRefusalAccepted("a", "a", NimbyHandsState.STAGED));
        return failures;
    }

    protected static int Case(out array<string> results, string name, bool passed)
    {
        if (passed)
        {
            results.Insert("PASS " + name);
            return 0;
        }
        results.Insert("FAIL " + name);
        return 1;
    }
}
