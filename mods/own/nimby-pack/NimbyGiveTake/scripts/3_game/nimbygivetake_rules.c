// -----------------------------------------------------------------------------
// NimbyGiveTake - engine-free transfer lifecycle, registry, and validators.
// Purpose : The Phase 1 state machine (REQUESTED, CONFIRMED, REFUSED, UNCONFIRMED),
//           the in-flight registry (G5), and the refusal rules as pure code, so the
//           self-test covers them and the 4_World layer only feeds them facts it
//           reads from the world (plan v0.5, Phase 1; shared rules 8).
// Registry: Keyed by the item's network id string, not the EntityAI reference: an item
//           deleted while in flight would leave a null key that can never be found or
//           evicted. The key still names exactly one item (G5 "keyed by item").
// Persists: Nothing. The registry lives in server memory for the mission.
// -----------------------------------------------------------------------------
class NimbyGiveTakeState
{
    // Registered, validation not finished, nothing submitted. Never logged.
    static const int PENDING = 0;
    // Submitted to the engine (ServerTakeToDst on the holder).
    static const int REQUESTED = 1;
    // The completion signal saw the item arrive with the receiver, or the reconcile at the
    // timeout found it there.
    static const int CONFIRMED = 2;
    // Validation or the engine refused before any transfer, or the item left the giver for
    // somewhere other than the receiver while the request was open.
    static const int REFUSED = 3;
    // No evidence of delivery within the observation period.
    static const int UNCONFIRMED = 4;

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
    static bool CanMove(int from, int to)
    {
        if (from == PENDING)
        {
            return to == REQUESTED || to == REFUSED;
        }
        if (from == REQUESTED)
        {
            return IsTerminal(to);
        }
        return false;
    }
}

class NimbyGiveTakeTransfer
{
    int id;
    string itemKey;
    string action;
    int state;
    int startedMs;
    string detail;
}

class NimbyGiveTakeTransfers
{
    protected ref map<string, ref NimbyGiveTakeTransfer> m_ByItem;
    protected int m_NextId;
    protected int m_TimeoutMs;

    void NimbyGiveTakeTransfers(int timeoutMs)
    {
        m_ByItem = new map<string, ref NimbyGiveTakeTransfer>;
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

    NimbyGiveTakeTransfer Find(string itemKey)
    {
        return m_ByItem.Get(itemKey);
    }

    NimbyGiveTakeTransfer FindById(int id)
    {
        foreach (string key, NimbyGiveTakeTransfer entry : m_ByItem)
        {
            if (entry.id == id)
            {
                return entry;
            }
        }
        return null;
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

    // Registers a request with a new correlation id. Returns false, and leaves the transfer
    // unregistered, when the item has no key or already has an unresolved entry; the caller
    // logs REFUSED "item in flight".
    bool Begin(NimbyGiveTakeTransfer transfer, string itemKey, string action, int nowMs)
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
        transfer.state = NimbyGiveTakeState.PENDING;
        transfer.startedMs = nowMs;
        transfer.detail = "";
        m_ByItem.Set(itemKey, transfer);
        return true;
    }

    // Moves a registered transfer to a new state. Returns false for a move the lifecycle
    // forbids, and the transfer keeps its state. A terminal state evicts the entry, so the
    // caller must hold its own reference to log it.
    bool Move(NimbyGiveTakeTransfer transfer, int state, string detail)
    {
        if (!transfer)
        {
            return false;
        }
        if (!NimbyGiveTakeState.CanMove(transfer.state, state))
        {
            return false;
        }
        transfer.state = state;
        transfer.detail = detail;
        if (NimbyGiveTakeState.IsTerminal(state))
        {
            Evict(transfer);
        }
        return true;
    }

    protected void Evict(NimbyGiveTakeTransfer transfer)
    {
        NimbyGiveTakeTransfer current = m_ByItem.Get(transfer.itemKey);
        if (current == transfer)
        {
            m_ByItem.Remove(transfer.itemKey);
        }
    }

    // Unresolved entries whose observation period has run out. The caller reconciles each with
    // the world and moves it to a terminal state; this never moves or retries anything itself.
    int Due(int nowMs, notnull array<ref NimbyGiveTakeTransfer> due)
    {
        foreach (string key, NimbyGiveTakeTransfer entry : m_ByItem)
        {
            if (nowMs - entry.startedMs >= m_TimeoutMs)
            {
                due.Insert(entry);
            }
        }
        return due.Count();
    }
}

class NimbyGiveTakeRules
{
    // Returned by ClassifyMove when a location change says nothing about the request.
    static const int NO_CHANGE = -1;

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

    // What a location change of a REQUESTED item means for the request: it arrived with the
    // receiver (CONFIRMED); it is still in the giver's hands or in the engine's temporary
    // location mid-move (NO_CHANGE); or it went anywhere else, so this request cannot deliver
    // it (REFUSED, never retried).
    static int ClassifyMove(bool atReceiver, bool inGiverHands, bool isTemporary)
    {
        if (atReceiver)
        {
            return NimbyGiveTakeState.CONFIRMED;
        }
        if (inGiverHands || isTemporary)
        {
            return NO_CHANGE;
        }
        return NimbyGiveTakeState.REFUSED;
    }

    // The reconcile at the timeout: delivered if the item is with the receiver, otherwise no
    // evidence. The timeout never authorizes a retry.
    static int ClassifyTimeout(bool itemExists, bool atReceiver)
    {
        if (itemExists && atReceiver)
        {
            return NimbyGiveTakeState.CONFIRMED;
        }
        return NimbyGiveTakeState.UNCONFIRMED;
    }

    // Engine-free validators for the $profile:NimbyGiveTake\selftest flag (shared rules 8).
    static int SelfTest(out array<string> results)
    {
        int failures = 0;

        failures += Case(results, "pending to requested is legal", NimbyGiveTakeState.CanMove(NimbyGiveTakeState.PENDING, NimbyGiveTakeState.REQUESTED));
        failures += Case(results, "pending to refused is legal", NimbyGiveTakeState.CanMove(NimbyGiveTakeState.PENDING, NimbyGiveTakeState.REFUSED));
        failures += Case(results, "pending cannot confirm before submission", !NimbyGiveTakeState.CanMove(NimbyGiveTakeState.PENDING, NimbyGiveTakeState.CONFIRMED));
        failures += Case(results, "requested to confirmed is legal", NimbyGiveTakeState.CanMove(NimbyGiveTakeState.REQUESTED, NimbyGiveTakeState.CONFIRMED));
        failures += Case(results, "requested to unconfirmed is legal", NimbyGiveTakeState.CanMove(NimbyGiveTakeState.REQUESTED, NimbyGiveTakeState.UNCONFIRMED));
        failures += Case(results, "confirmed never becomes refused", !NimbyGiveTakeState.CanMove(NimbyGiveTakeState.CONFIRMED, NimbyGiveTakeState.REFUSED));
        failures += Case(results, "unconfirmed never becomes confirmed", !NimbyGiveTakeState.CanMove(NimbyGiveTakeState.UNCONFIRMED, NimbyGiveTakeState.CONFIRMED));
        failures += Case(results, "refused never becomes requested", !NimbyGiveTakeState.CanMove(NimbyGiveTakeState.REFUSED, NimbyGiveTakeState.REQUESTED));
        failures += Case(results, "state names match the log format", NimbyGiveTakeState.Name(NimbyGiveTakeState.UNCONFIRMED) == "UNCONFIRMED");

        NimbyGiveTakeTransfers registry = new NimbyGiveTakeTransfers(5000);
        NimbyGiveTakeTransfer first = new NimbyGiveTakeTransfer();
        NimbyGiveTakeTransfer second = new NimbyGiveTakeTransfer();
        NimbyGiveTakeTransfer other = new NimbyGiveTakeTransfer();
        failures += Case(results, "first request registers", registry.Begin(first, "1:100", "Take", 1000));
        failures += Case(results, "second request on the same item is refused in flight", !registry.Begin(second, "1:100", "Take", 1100));
        failures += Case(results, "refused second request gets no id", second.id == 0);
        failures += Case(results, "request on another item registers", registry.Begin(other, "1:200", "Give", 1100));
        failures += Case(results, "correlation ids are distinct", first.id != other.id && first.id > 0);
        failures += Case(results, "new entry starts pending", first.state == NimbyGiveTakeState.PENDING);
        failures += Case(results, "empty item key is refused", !registry.Begin(new NimbyGiveTakeTransfer(), "", "Take", 1000));
        failures += Case(results, "find by id returns the entry", registry.FindById(other.id) == other);

        failures += Case(results, "submit moves to requested", registry.Move(first, NimbyGiveTakeState.REQUESTED, ""));
        failures += Case(results, "requested entry stays in flight", registry.IsInFlight("1:100"));
        failures += Case(results, "confirm moves to confirmed", registry.Move(first, NimbyGiveTakeState.CONFIRMED, "signal"));
        failures += Case(results, "terminal state evicts the entry", !registry.IsInFlight("1:100"));
        failures += Case(results, "second terminal line is refused", !registry.Move(first, NimbyGiveTakeState.REFUSED, "late"));
        failures += Case(results, "confirmed transfer keeps its state", first.state == NimbyGiveTakeState.CONFIRMED);
        failures += Case(results, "evicted item accepts a new request", registry.Begin(second, "1:100", "Give", 2000));

        array<ref NimbyGiveTakeTransfer> due = new array<ref NimbyGiveTakeTransfer>;
        failures += Case(results, "nothing is due before the timeout", registry.Due(5999, due) == 0);
        due.Clear();
        failures += Case(results, "the older entry is due at the timeout", registry.Due(6100, due) == 1 && due[0] == other);
        registry.Move(other, NimbyGiveTakeState.REQUESTED, "");
        registry.Move(other, NimbyGiveTakeState.UNCONFIRMED, "no evidence");
        failures += Case(results, "eviction after the timeout frees the item", !registry.IsInFlight("1:200"));
        due.Clear();
        failures += Case(results, "the newer entry is due later", registry.Due(7000, due) == 1 && due[0] == second);
        registry.Move(second, NimbyGiveTakeState.REFUSED, "no space");
        failures += Case(results, "registry is empty after every terminal state", registry.Count() == 0);

        failures += Case(results, "healthy party passes", CheckParty("giver", true, true, true, true, false, false, false) == "");
        failures += Case(results, "missing party refuses", CheckParty("receiver", false, true, true, true, false, false, false) == "receiver missing");
        failures += Case(results, "disconnected party refuses", CheckParty("giver", true, false, true, true, false, false, false) == "giver disconnected");
        failures += Case(results, "dead party refuses", CheckParty("giver", true, true, false, true, false, false, false) == "giver dead");
        failures += Case(results, "unconscious party refuses", CheckParty("receiver", true, true, true, false, false, false, false) == "receiver unconscious");
        failures += Case(results, "restrained party refuses", CheckParty("receiver", true, true, true, true, true, false, false) == "receiver restrained");
        failures += Case(results, "party in a vehicle refuses", CheckParty("giver", true, true, true, true, false, true, false) == "giver in vehicle");
        failures += Case(results, "swimming party refuses", CheckParty("giver", true, true, true, true, false, false, true) == "giver swimming");
        failures += Case(results, "within reach passes", WithinReach(4.0, 2.0, 0.5));
        failures += Case(results, "inside the drift tolerance passes", WithinReach(6.0, 2.0, 0.5));
        failures += Case(results, "beyond reach refuses", !WithinReach(6.5, 2.0, 0.5));

        failures += Case(results, "arrival at the receiver confirms", ClassifyMove(true, false, false) == NimbyGiveTakeState.CONFIRMED);
        failures += Case(results, "still in the giver's hands changes nothing", ClassifyMove(false, true, false) == NO_CHANGE);
        failures += Case(results, "temporary location changes nothing", ClassifyMove(false, false, true) == NO_CHANGE);
        failures += Case(results, "item going elsewhere refuses", ClassifyMove(false, false, false) == NimbyGiveTakeState.REFUSED);
        failures += Case(results, "timeout with the item at the receiver confirms", ClassifyTimeout(true, true) == NimbyGiveTakeState.CONFIRMED);
        failures += Case(results, "timeout with the item elsewhere is unconfirmed", ClassifyTimeout(true, false) == NimbyGiveTakeState.UNCONFIRMED);
        failures += Case(results, "timeout with the item gone is unconfirmed", ClassifyTimeout(false, false) == NimbyGiveTakeState.UNCONFIRMED);

        failures += Case(results, "plain name is unchanged", NimbyGiveTakeLog.Escape("Trendkill") == "Trendkill");
        failures += Case(results, "quote in a name is escaped", NimbyGiveTakeLog.Escape("a\"b") == "a\\\"b");
        failures += Case(results, "backslash in a name is escaped", NimbyGiveTakeLog.Escape("a\\b") == "a\\\\b");
        failures += Case(results, "newline in a name is escaped", NimbyGiveTakeLog.Escape("a\nb") == "a\\nb");
        failures += Case(results, "forged field stays inside the quotes", NimbyGiveTakeLog.Quote("x\" receiver=\"y") == "\"x\\\" receiver=\\\"y\"");
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
