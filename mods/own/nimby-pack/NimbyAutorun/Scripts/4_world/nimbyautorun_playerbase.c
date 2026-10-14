// -----------------------------------------------------------------------------
// NimbyAutorun - server side.
// Purpose : Applies an autorun request from the owning client to the server's
//           copy of the player, after validating it.
// Origin  : Fork of SchanaModAutorun playerbase.c (Apache-2.0). See NOTICE.
// Security: The original trusted any client RPC. A client can target any
//           network object with an RPC, so without the ownership check one
//           player could force another into autorun. Every field is validated.
// -----------------------------------------------------------------------------

class NimbyAutorunRpc
{
    // Distinct from the original (-44201) so both mods can coexist. Arbitrary;
    // change only together with the client in 5_mission.
    static const int TOGGLE = 6550101;

    // Minimum seconds between rejection log lines for one player.
    static const float REJECT_LOG_INTERVAL = 10.0;
}

modded class PlayerBase extends ManBase
{
    protected bool m_NimbyAutorunning;
    protected float m_NimbyAutorunLastRejectLog = -1000.0;

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

        super.OnRPC(sender, rpc_type, ctx);
    }

    protected void NimbyAutorunHandleRequest(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param3<bool, int, int> request;
        if (!ctx.Read(request))
        {
            NimbyAutorunReject(sender, "unreadable payload");
            return;
        }

        bool isRunning = request.param1;
        int movementIdx = request.param2;
        int angle = request.param3;

        PlayerIdentity owner = GetIdentity();
        if (!sender || !owner || sender.GetId() != owner.GetId())
        {
            NimbyAutorunReject(sender, "sender does not own this player");
            return;
        }

        if (isRunning)
        {
            if (!NimbyAutorunIsValidMovement(movementIdx) || angle != 1)
            {
                NimbyAutorunReject(sender, "movement out of range: idx=" + movementIdx + " angle=" + angle);
                return;
            }

            if (IsInVehicle() || IsUnconscious() || IsRestrained() || !IsAlive())
            {
                // Expected during races between client and server state; stop quietly.
                isRunning = false;
            }
            else if (movementIdx == DayZPlayerConstants.MOVEMENTIDX_SPRINT && !CanSprint())
            {
                movementIdx = DayZPlayerConstants.MOVEMENTIDX_RUN;
            }
        }

        if (!isRunning)
        {
            movementIdx = DayZPlayerConstants.MOVEMENTIDX_IDLE;
            angle = 0;
        }

        m_NimbyAutorunning = isRunning;
        GetInputController().OverrideMovementSpeed(isRunning, movementIdx);
        GetInputController().OverrideMovementAngle(isRunning, angle);
    }

    protected bool NimbyAutorunIsValidMovement(int movementIdx)
    {
        return movementIdx == DayZPlayerConstants.MOVEMENTIDX_WALK
            || movementIdx == DayZPlayerConstants.MOVEMENTIDX_RUN
            || movementIdx == DayZPlayerConstants.MOVEMENTIDX_SPRINT;
    }

    protected void NimbyAutorunReject(PlayerIdentity sender, string reason)
    {
        float now = GetGame().GetTickTime();
        if (now - m_NimbyAutorunLastRejectLog < NimbyAutorunRpc.REJECT_LOG_INTERVAL)
        {
            return;
        }
        m_NimbyAutorunLastRejectLog = now;

        string senderId = "unknown";
        if (sender)
        {
            senderId = sender.GetId();
        }
        Print("[NIMBY][Autorun] Rejected request from " + senderId + ": " + reason);
    }

    void NimbySetAutorunning(bool isRunning)
    {
        m_NimbyAutorunning = isRunning;
    }

    bool NimbyIsAutorunning()
    {
        return m_NimbyAutorunning;
    }
}
