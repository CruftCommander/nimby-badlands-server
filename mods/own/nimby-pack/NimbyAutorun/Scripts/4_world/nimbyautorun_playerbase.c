// -----------------------------------------------------------------------------
// NimbyAutorun - Legs. Player state, server request handling, forced clears.
// Purpose : Holds the autorun state of one player on both machines, applies a
//           validated request from the owning client on the server, clears the
//           override from server-side hooks when the player enters a vehicle,
//           falls unconscious, is restrained, dies, or is removed, and tells the
//           owning client with RPC 6550102 (A4). Sends the server's stopOnSwim to
//           the owning client with RPC 6550103 on every start, and caches the
//           player's name and ids for lines written after the identity is gone.
// Origin  : Fork of SchanaModAutorun playerbase.c (Apache-2.0). See NOTICE.
// Security: A client can target any network object with an RPC, so ownership is
//           derived from the authenticated sender before anything is read, and
//           every field is validated in NimbyAutorunRules. A request aimed at
//           another player changes nothing on that player.
// Plan    : docs/plans/2026-10-15-autorun-plan.md, Phase 1.
// -----------------------------------------------------------------------------

modded class PlayerBase extends ManBase
{
    // State on this machine. On the owning client it is what the client applied; on the server
    // it is what the server applied. Both reset with the player object on respawn.
    protected bool m_NimbyAutorunning;
    protected int m_NimbyAutorunIdx;
    // Server only.
    protected ref NimbyAutorunThrottle m_NimbyAutorunThrottle;
    protected ref NimbyAutorunLogGate m_NimbyAutorunRejectGate;
    protected ref NimbyAutorunLogGate m_NimbyAutorunThrottleGate;
    // Server only, NIMBY-T-005: the name and both ids, cached while the identity is attached, so
    // a forced clear after the identity is gone still names the player (shared rules 4.2). In
    // memory only; nothing is saved.
    protected string m_NimbyAutorunWhoName;
    protected string m_NimbyAutorunWhoId;
    protected string m_NimbyAutorunWhoPlain;
    // Client only: the server's stopOnSwim, from RPC 6550103. Starts at the shipped default
    // (false, swim carries on) until the first accepted start brings the server value.
    protected bool m_NimbyAutorunStopOnSwim = false;

    bool NimbyIsAutorunning()
    {
        return m_NimbyAutorunning;
    }

    int NimbyGetAutorunIdx()
    {
        return m_NimbyAutorunIdx;
    }

    bool NimbyAutorunStopOnSwim()
    {
        return m_NimbyAutorunStopOnSwim;
    }

    // Caches the name and both ids. Called wherever the identity is known to be attached:
    // connect, reconnect, every request, and every forced clear that still has it.
    protected void NimbyAutorunRememberIdentity(PlayerIdentity identity)
    {
        if (!identity)
        {
            return;
        }
        m_NimbyAutorunWhoName = identity.GetName();
        m_NimbyAutorunWhoId = identity.GetId();
        m_NimbyAutorunWhoPlain = identity.GetPlainId();
    }

    // "player=<name> (<bi-id>, <steam64>)" from the live identity, or from the cache once the
    // identity is gone (NIMBY-T-005: the removal hooks run after it is detached).
    string NimbyAutorunWho()
    {
        PlayerIdentity identity = GetIdentity();
        if (identity)
        {
            NimbyAutorunRememberIdentity(identity);
            return NimbyAutorunLog.Who(identity);
        }
        return NimbyAutorunLog.WhoFrom(m_NimbyAutorunWhoName, m_NimbyAutorunWhoId, m_NimbyAutorunWhoPlain);
    }

    // Applies the state to this machine's input controller (human.c:253-256, enum at :7-12).
    void NimbyAutorunApply(bool running, int movementIdx)
    {
        if (!running)
        {
            movementIdx = DayZPlayerConstants.MOVEMENTIDX_IDLE;
        }
        m_NimbyAutorunning = running;
        m_NimbyAutorunIdx = movementIdx;
        HumanInputController controller = GetInputController();
        if (!controller)
        {
            return;
        }
        if (running)
        {
            controller.OverrideMovementSpeed(HumanInputControllerOverrideType.ENABLED, movementIdx);
            controller.OverrideMovementAngle(HumanInputControllerOverrideType.ENABLED, 1);
            return;
        }
        controller.OverrideMovementSpeed(HumanInputControllerOverrideType.DISABLED, 0);
        controller.OverrideMovementAngle(HumanInputControllerOverrideType.DISABLED, 0);
    }

    // The state that forbids autorun, or "" when none applies.
    string NimbyAutorunBlockedReason()
    {
        if (!IsAlive())
        {
            return "dead";
        }
        if (IsUnconscious())
        {
            return "unconscious";
        }
        if (IsRestrained())
        {
            return "restrained";
        }
        if (IsInVehicle())
        {
            return "vehicle";
        }
        return "";
    }

    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        if (rpc_type == NimbyAutorunRpc.TOGGLE)
        {
            if (GetGame().IsServer())
            {
                NimbyAutorunHandleRequest(sender, ctx);
            }
            return;
        }
        if (rpc_type == NimbyAutorunRpc.CANCEL)
        {
            if (GetGame().IsClient())
            {
                NimbyAutorunHandleCancel(ctx);
            }
            return;
        }
        if (rpc_type == NimbyAutorunRpc.SETTINGS)
        {
            if (GetGame().IsClient())
            {
                NimbyAutorunHandleSettings(ctx);
            }
            return;
        }

        super.OnRPC(sender, rpc_type, ctx);
    }

    // ---- server -------------------------------------------------------------------------

    protected NimbyAutorunThrottle NimbyAutorunGetThrottle()
    {
        if (!m_NimbyAutorunThrottle)
        {
            m_NimbyAutorunThrottle = new NimbyAutorunThrottle(NimbyAutorunConfigLoader.Get().requestsPerSecond);
        }
        return m_NimbyAutorunThrottle;
    }

    protected void NimbyAutorunHandleRequest(PlayerIdentity sender, ParamsReadContext ctx)
    {
        // Ownership first: a request aimed at someone else is refused before it is read, so it
        // can change nothing on the target and earns the target's owner no cancellation.
        PlayerIdentity owner = GetIdentity();
        if (!sender || !owner || sender.GetId() != owner.GetId())
        {
            NimbyAutorunLogReject(sender, "sender does not own this player");
            return;
        }
        NimbyAutorunRememberIdentity(owner);

        float now = GetGame().GetTickTime();
        Param3<bool, int, int> request = new Param3<bool, int, int>(false, 0, 0);
        if (!ctx.Read(request))
        {
            // Counted against the budget, because the stop bypass needs a readable payload.
            if (!NimbyAutorunGetThrottle().Admit(now, false))
            {
                NimbyAutorunOnThrottled(sender, now);
                return;
            }
            NimbyAutorunRefuse(sender, "unreadable payload");
            return;
        }

        bool running = request.param1;
        int movementIdx = request.param2;
        int angle = request.param3;

        if (!running)
        {
            // An authenticated stop bypasses the request throttle and always applies.
            if (m_NimbyAutorunning)
            {
                NimbyAutorunApply(false, DayZPlayerConstants.MOVEMENTIDX_IDLE);
                NimbyAutorunLogToggle(owner, "server", "client-stop");
            }
            return;
        }

        if (!NimbyAutorunGetThrottle().Admit(now, false))
        {
            NimbyAutorunOnThrottled(sender, now);
            return;
        }

        string invalid = NimbyAutorunRules.ValidateRequest(running, movementIdx, angle);
        if (invalid != "")
        {
            NimbyAutorunRefuse(sender, invalid);
            return;
        }

        string blocked = NimbyAutorunBlockedReason();
        if (blocked != "")
        {
            // Expected in races between client and server state; the client is told to stop.
            NimbyAutorunRefuse(sender, "player is " + blocked);
            return;
        }

        int resolved = NimbyAutorunRules.ResolveServerIdx(movementIdx, CanSprint());
        if (m_NimbyAutorunning && resolved == m_NimbyAutorunIdx)
        {
            return;
        }
        if (!m_NimbyAutorunning)
        {
            // A start, not a speed change: the client learns the swim rule before it can matter.
            NimbyAutorunSendSettings();
        }
        NimbyAutorunApply(true, resolved);
        NimbyAutorunLogToggle(owner, "server", "client-request");
    }

    // A refused running request: the client already applied it locally, so the server clears
    // its own override if it was on and sends 6550102 so both sides stop.
    protected void NimbyAutorunRefuse(PlayerIdentity sender, string reason)
    {
        NimbyAutorunLogReject(sender, reason);
        if (m_NimbyAutorunning)
        {
            NimbyAutorunApply(false, DayZPlayerConstants.MOVEMENTIDX_IDLE);
        }
        NimbyAutorunSendCancel("rejected");
    }

    // A dropped running request: the client applied it locally and the server did not, so the
    // first drop in a window stops both sides; later drops in the window are only counted.
    protected void NimbyAutorunOnThrottled(PlayerIdentity sender, float now)
    {
        NimbyAutorunThrottle throttle = NimbyAutorunGetThrottle();
        if (throttle.TakeCancel())
        {
            if (m_NimbyAutorunning)
            {
                NimbyAutorunApply(false, DayZPlayerConstants.MOVEMENTIDX_IDLE);
            }
            NimbyAutorunSendCancel("throttled");
        }
        if (!m_NimbyAutorunThrottleGate)
        {
            m_NimbyAutorunThrottleGate = new NimbyAutorunLogGate();
        }
        if (m_NimbyAutorunThrottleGate.Allow(now, NimbyAutorunRules.LOG_INTERVAL_SECONDS))
        {
            NimbyAutorunLog.Info("Throttled " + NimbyAutorunLog.Who(sender) + " count=" + throttle.TakeDropped().ToString());
        }
    }

    protected void NimbyAutorunLogReject(PlayerIdentity sender, string reason)
    {
        if (!m_NimbyAutorunRejectGate)
        {
            m_NimbyAutorunRejectGate = new NimbyAutorunLogGate();
        }
        if (!m_NimbyAutorunRejectGate.Allow(GetGame().GetTickTime(), NimbyAutorunRules.LOG_INTERVAL_SECONDS))
        {
            return;
        }
        NimbyAutorunLog.Info("Rejected request from " + NimbyAutorunLog.Ids(sender) + ": " + reason);
    }

    // Game.c:1013, with the owner as the only recipient.
    protected void NimbyAutorunSendCancel(string reason)
    {
        PlayerIdentity owner = GetIdentity();
        if (!owner)
        {
            return;
        }
        Param1<string> payload = new Param1<string>(reason);
        GetGame().RPCSingleParam(this, NimbyAutorunRpc.CANCEL, payload, true, owner);
    }

    // RPC 6550103 to the owner only, carrying the server's stopOnSwim.
    protected void NimbyAutorunSendSettings()
    {
        PlayerIdentity owner = GetIdentity();
        if (!owner)
        {
            return;
        }
        Param1<bool> payload = new Param1<bool>(NimbyAutorunConfigLoader.Get().stopOnSwim);
        GetGame().RPCSingleParam(this, NimbyAutorunRpc.SETTINGS, payload, true, owner);
    }

    // A server-side clear that does not wait for the client. via is "hook" for the state hooks
    // below and "tick" for the CommandHandler backstop. A hook that fires while autorun is off
    // does nothing and logs nothing.
    void NimbyAutorunForceClear(string reason, string via)
    {
        if (!GetGame().IsServer() || !m_NimbyAutorunning)
        {
            return;
        }
        NimbyAutorunApply(false, DayZPlayerConstants.MOVEMENTIDX_IDLE);
        NimbyAutorunLog.Info("Forced clear " + NimbyAutorunWho() + " reason=" + reason + " via=" + via);
        NimbyAutorunSendCancel(reason);
    }

    void NimbyAutorunLogToggle(PlayerIdentity identity, string side, string cause)
    {
        if (!NimbyAutorunLog.IsDebug())
        {
            return;
        }
        string line = "Toggle " + NimbyAutorunLog.Who(identity) + " running=" + NimbyAutorunLog.Bool(m_NimbyAutorunning);
        line = line + " idx=" + m_NimbyAutorunIdx.ToString() + " side=" + side + " cause=" + cause;
        NimbyAutorunLog.Debug(line);
    }

    // Server-side hooks. Vehicle: PlayerBase.c:4342. Unconscious: PlayerBase.c:3756, called from
    // CommandHandler at :3381 on both machines. Restraint: PlayerBase.c:2206, called by the
    // server-side restrain actions and HandcuffsLocked. Death: PlayerBase.c:1214. Removal:
    // PlayerBase.c:7647 (MissionServer.InvokeOnDisconnect, missionServer.c:442-447) and
    // PlayerBase.c:1393 (EEDelete). NimbyAutorunForceClear returns at once on a client.
    override void OnCommandVehicleStart()
    {
        super.OnCommandVehicleStart();
        NimbyAutorunForceClear("vehicle", "hook");
    }

    override void OnUnconsciousStart()
    {
        super.OnUnconsciousStart();
        NimbyAutorunForceClear("unconscious", "hook");
    }

    override void SetRestrained(bool is_restrained)
    {
        super.SetRestrained(is_restrained);
        if (is_restrained)
        {
            NimbyAutorunForceClear("restrained", "hook");
        }
    }

    override void EEKilled(Object killer)
    {
        NimbyAutorunForceClear("dead", "hook");
        super.EEKilled(killer);
    }

    // NIMBY-T-005: cache the identity as soon as the server attaches the player to it
    // (PlayerBase.c:7623 and :7638, called from MissionServer.InvokeOnConnect,
    // missionServer.c:435-440, and OnClientReconnectEvent, :637). Server only.
    override void OnConnect()
    {
        super.OnConnect();
        NimbyAutorunRememberIdentity(GetIdentity());
    }

    override void OnReconnect()
    {
        super.OnReconnect();
        NimbyAutorunRememberIdentity(GetIdentity());
    }

    // Before super. The October 19 kill test (R2-7b) showed the identity can already be gone
    // here; the line then names the player from the cache (NIMBY-T-005).
    override void OnDisconnect()
    {
        NimbyAutorunForceClear("removed", "hook");
        super.OnDisconnect();
    }

    override void EEDelete(EntityAI parent)
    {
        NimbyAutorunForceClear("removed", "hook");
        super.EEDelete(parent);
    }

    // Backstop and sprint re-evaluation while autorun stays on (PlayerBase.c:3206). Costs one
    // bool test per frame when autorun is off.
    override void CommandHandler(float pDt, int pCurrentCommandID, bool pCurrentCommandFinished)
    {
        super.CommandHandler(pDt, pCurrentCommandID, pCurrentCommandFinished);
        if (!m_NimbyAutorunning || !GetGame().IsServer())
        {
            return;
        }
        string blocked = NimbyAutorunBlockedReason();
        if (blocked != "")
        {
            NimbyAutorunForceClear(blocked, "tick");
            return;
        }
        int resolved = NimbyAutorunRules.ResolveServerIdx(m_NimbyAutorunIdx, CanSprint());
        if (resolved != m_NimbyAutorunIdx)
        {
            NimbyAutorunApply(true, resolved);
            NimbyAutorunLogToggle(GetIdentity(), "server", "cannot-sprint");
        }
    }

    // ---- client -------------------------------------------------------------------------

    // A4: clear the override and the remembered state. Nothing is re-sent, so autorun stays off
    // until the player double-taps again.
    protected void NimbyAutorunHandleCancel(ParamsReadContext ctx)
    {
        Param1<string> payload = new Param1<string>("");
        string reason = "unknown";
        if (ctx.Read(payload))
        {
            reason = payload.param1;
        }
        bool wasRunning = m_NimbyAutorunning;
        NimbyAutorunApply(false, DayZPlayerConstants.MOVEMENTIDX_IDLE);
        NimbyAutorunLog.Debug("Cancel received " + NimbyAutorunLog.Who(GetIdentity()) + " reason=" + reason + " wasRunning=" + NimbyAutorunLog.Bool(wasRunning));
    }

    // RPC 6550103: the server's swim rule. Changes no movement state; a later swim start reads it.
    protected void NimbyAutorunHandleSettings(ParamsReadContext ctx)
    {
        Param1<bool> payload = new Param1<bool>(true);
        if (!ctx.Read(payload))
        {
            return;
        }
        bool changed = m_NimbyAutorunStopOnSwim != payload.param1;
        m_NimbyAutorunStopOnSwim = payload.param1;
        if (changed)
        {
            NimbyAutorunLog.Debug("Settings received " + NimbyAutorunLog.Who(GetIdentity()) + " stopOnSwim=" + NimbyAutorunLog.Bool(m_NimbyAutorunStopOnSwim));
        }
    }
}
