// -----------------------------------------------------------------------------
// NimbyGreet - PlayerBase: the RPC plumbing and the spawn hook (plan v0.4, G5).
// Purpose : The only file that touches the wire. Client: sends 6551101 for the controlled
//           player, hands 6551102 chunks and the 6551103 done record to MissionGameplay
//           through NimbyGreetClient's invokers, and reports the end of OnPlayerLoaded
//           (4_World/DayZ/Entities/ManBase/PlayerBase.c:2569) for the controlled player,
//           after vanilla's CloseAll (:2594) has run. Server: checks that the sender owns
//           this player, reads the request, and hands it to MissionServer through the
//           Request invoker; MissionServer answers through NimbyGreetSendChunk and
//           NimbyGreetSendDone, to the owning identity only (RPCSingleParam with
//           recipient, 3_Game/DayZ/Global/Game.c:1013).
// Security: a client can target any network object with an RPC, so the server derives
//           ownership from the authenticated sender before reading anything (the Legs
//           precedent, nimbyautorun_playerbase.c). A request aimed at another player is
//           refused and logged with both ids (shared rules 4.2).
// Wire    : request  Param5<int requestId, string language, int dismissedVersion, bool neverShow, bool manual>
//           chunk    Param3<int requestId, int index, string text>          (text at most CHUNK_BYTES)
//           done     Param3<int requestId, int chunkCount, string doneJson> (NimbyGreetDone)
// Phase 0 : this plumbing carried both runs; only the done shape changed (JSON record).
// Style   : One statement per line.
// -----------------------------------------------------------------------------

modded class PlayerBase extends ManBase
{
    override void OnPlayerLoaded()
    {
        super.OnPlayerLoaded();
        if (!GetGame().IsClient())
        {
            return;
        }
        if (!IsControlledPlayer())
        {
            return;
        }
        NimbyGreetLog.Debug("OnPlayerLoaded: controlled player loaded; vanilla CloseAll has run");
        NimbyGreetClient.Spawned().Invoke(this);
    }

    // G5: a hit on the controlled player closes the window (OnReceivedHit, PlayerBase.c:1413, client only;
    // EEHitByRemote, :1382, on every machine). A health value drifting down fires neither.
    override void OnReceivedHit(ImpactEffectsData hitData)
    {
        super.OnReceivedHit(hitData);
        NimbyGreetReportHit("OnReceivedHit");
    }

    override void EEHitByRemote(int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos)
    {
        super.EEHitByRemote(damageType, source, component, dmgZone, ammo, modelPos);
        NimbyGreetReportHit("EEHitByRemote");
    }

    protected void NimbyGreetReportHit(string hook)
    {
        if (!GetGame().IsClient())
        {
            return;
        }
        if (!IsControlledPlayer())
        {
            return;
        }
        NimbyGreetLog.Debug("hit on the controlled player via " + hook);
        NimbyGreetClient.Hit().Invoke();
    }

    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        if (rpc_type == NimbyGreetRules.RPC_REQUEST)
        {
            if (GetGame().IsServer())
            {
                NimbyGreetHandleRequest(sender, ctx);
            }
            return;
        }
        if (rpc_type == NimbyGreetRules.RPC_CHUNK)
        {
            if (GetGame().IsClient())
            {
                NimbyGreetHandleChunk(ctx);
            }
            return;
        }
        if (rpc_type == NimbyGreetRules.RPC_DONE)
        {
            if (GetGame().IsClient())
            {
                NimbyGreetHandleDone(ctx);
            }
            return;
        }
        super.OnRPC(sender, rpc_type, ctx);
    }

    // ---- client ----------------------------------------------------------------------------

    // Sends 6551101 for this (controlled) player. Called by MissionGameplay.
    void NimbyGreetSendRequest(int requestId, string language, int dismissedVersion, bool neverShow, bool manual)
    {
        Param5<int, string, int, bool, bool> request = new Param5<int, string, int, bool, bool>(requestId, language, dismissedVersion, neverShow, manual);
        GetGame().RPCSingleParam(this, NimbyGreetRules.RPC_REQUEST, request, true);
        NimbyGreetLog.Debug("request sent id=" + requestId.ToString() + " language=" + language + " dismissedVersion=" + dismissedVersion.ToString() + " neverShow=" + NimbyGreetLog.Bool(neverShow) + " manual=" + NimbyGreetLog.Bool(manual));
    }

    protected void NimbyGreetHandleChunk(ParamsReadContext ctx)
    {
        Param3<int, int, string> chunk = new Param3<int, int, string>(0, 0, "");
        if (!ctx.Read(chunk))
        {
            NimbyGreetLog.Info("chunk unreadable (an oversize or corrupt RPC string; NIMBY-T-037)");
            return;
        }
        NimbyGreetClient.Chunk().Invoke(chunk.param1, chunk.param2, chunk.param3);
    }

    protected void NimbyGreetHandleDone(ParamsReadContext ctx)
    {
        Param3<int, int, string> done = new Param3<int, int, string>(0, 0, "");
        if (!ctx.Read(done))
        {
            NimbyGreetLog.Info("done unreadable");
            return;
        }
        NimbyGreetClient.Done().Invoke(done.param1, done.param2, done.param3);
    }

    // ---- server ----------------------------------------------------------------------------

    protected void NimbyGreetHandleRequest(PlayerIdentity sender, ParamsReadContext ctx)
    {
        PlayerIdentity owner = GetIdentity();
        if (!sender)
        {
            NimbyGreetLog.Info("request refused reason=null-sender");
            return;
        }
        if (!owner)
        {
            NimbyGreetLog.Info("request refused reason=target-has-no-identity sender " + NimbyGreetLog.Who(sender));
            return;
        }
        if (sender.GetId() != owner.GetId())
        {
            NimbyGreetLog.Info("request refused reason=sender-not-owner sender " + NimbyGreetLog.Who(sender) + " target " + NimbyGreetLog.Who(owner));
            return;
        }
        Param5<int, string, int, bool, bool> request = new Param5<int, string, int, bool, bool>(0, "", 0, false, false);
        if (!ctx.Read(request))
        {
            NimbyGreetLog.Info("request refused reason=unreadable " + NimbyGreetLog.Who(sender));
            return;
        }
        NimbyGreetClient.Request().Invoke(this, sender, request.param1, request.param2, request.param3, request.param4, request.param5);
    }

    // One chunk to the owning identity. Called by MissionServer.
    void NimbyGreetSendChunk(PlayerIdentity recipient, int requestId, int index, string text)
    {
        Param3<int, int, string> chunk = new Param3<int, int, string>(requestId, index, text);
        GetGame().RPCSingleParam(this, NimbyGreetRules.RPC_CHUNK, chunk, true, recipient);
    }

    // The done record to the owning identity. Called by MissionServer.
    void NimbyGreetSendDone(PlayerIdentity recipient, int requestId, int chunkCount, string doneJson)
    {
        Param3<int, int, string> done = new Param3<int, int, string>(requestId, chunkCount, doneJson);
        GetGame().RPCSingleParam(this, NimbyGreetRules.RPC_DONE, done, true, recipient);
    }
}
