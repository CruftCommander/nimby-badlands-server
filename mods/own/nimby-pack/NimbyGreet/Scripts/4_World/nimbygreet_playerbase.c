// -----------------------------------------------------------------------------
// NimbyGreet - PlayerBase: the RPC plumbing and the spawn hook (plan G5, Phase 0 probe).
// Purpose : Server: answers a client's 6551101 request with 6551102 chunks and a 6551103
//           done record, to the owning identity only (RPCSingleParam with recipient,
//           3_Game/DayZ/Global/Game.c:1013). Client: hands 6551102 and 6551103 to
//           MissionGameplay through NimbyGreetClient's invokers, and reports the end of
//           OnPlayerLoaded (4_World/DayZ/Entities/ManBase/PlayerBase.c:2569) for the
//           controlled player, after vanilla's CloseAll (:2594) has run.
// Security: a client can target any network object with an RPC, so the server derives
//           ownership from the authenticated sender before reading anything (the Legs
//           precedent, nimbyautorun_playerbase.c). A request aimed at another player is
//           refused and logged with both ids (shared rules 4.2).
// Probe   : no cooldown and no dismissal logic; the language field is a probe command
//           (NimbyGreetProbePayload). Phase 1 replaces this file.
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
        NimbyGreetLog.Probe("OnPlayerLoaded: controlled player loaded; vanilla CloseAll has run");
        NimbyGreetClient.Spawned().Invoke(this);
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
        NimbyGreetLog.Probe("request sent id=" + requestId.ToString() + " language=" + language + " manual=" + NimbyGreetLog.Bool(manual));
    }

    protected void NimbyGreetHandleChunk(ParamsReadContext ctx)
    {
        Param3<int, int, string> chunk = new Param3<int, int, string>(0, 0, "");
        if (!ctx.Read(chunk))
        {
            NimbyGreetLog.Probe("chunk unreadable");
            return;
        }
        NimbyGreetClient.Chunk().Invoke(chunk.param1, chunk.param2, chunk.param3);
    }

    protected void NimbyGreetHandleDone(ParamsReadContext ctx)
    {
        Param6<int, int, int, int, string, string> done = new Param6<int, int, int, int, string, string>(0, 0, 0, 0, "", "");
        if (!ctx.Read(done))
        {
            NimbyGreetLog.Probe("done unreadable");
            return;
        }
        NimbyGreetClient.Done().Invoke(done.param1, done.param2, done.param3, done.param4, done.param5, done.param6);
    }

    // ---- server ----------------------------------------------------------------------------

    protected void NimbyGreetHandleRequest(PlayerIdentity sender, ParamsReadContext ctx)
    {
        PlayerIdentity owner = GetIdentity();
        if (!sender)
        {
            NimbyGreetLog.Info("request refused: null sender");
            return;
        }
        if (!owner)
        {
            NimbyGreetLog.Info("request refused: target has no identity; sender " + NimbyGreetLog.Who(sender));
            return;
        }
        if (sender.GetId() != owner.GetId())
        {
            NimbyGreetLog.Info("request refused: sender does not own this player; sender " + NimbyGreetLog.Who(sender) + " target " + NimbyGreetLog.Who(owner));
            return;
        }
        Param5<int, string, int, bool, bool> request = new Param5<int, string, int, bool, bool>(0, "", 0, false, false);
        if (!ctx.Read(request))
        {
            NimbyGreetLog.Info("request refused: unreadable payload; " + NimbyGreetLog.Who(sender));
            return;
        }
        int requestId = request.param1;
        string language = request.param2;
        NimbyGreetLog.Probe("request id=" + requestId.ToString() + " language=" + language + " dismissedVersion=" + request.param3.ToString() + " neverShow=" + NimbyGreetLog.Bool(request.param4) + " manual=" + NimbyGreetLog.Bool(request.param5) + " " + NimbyGreetLog.Who(sender));

        bool singleChunk;
        string info;
        string payload = NimbyGreetProbePayload.Build(language, singleChunk, info);
        array<string> chunks;
        int count;
        if (singleChunk)
        {
            chunks = new array<string>;
            chunks.Insert(payload);
            count = 1;
        }
        else
        {
            count = NimbyGreetRules.SplitChunks(payload, NimbyGreetRules.CHUNK_BYTES, chunks);
        }
        float start = GetGame().GetTickTime();
        int i = 0;
        while (i < count)
        {
            Param3<int, int, string> chunk = new Param3<int, int, string>(requestId, i, chunks.Get(i));
            GetGame().RPCSingleParam(this, NimbyGreetRules.RPC_CHUNK, chunk, true, owner);
            i = i + 1;
        }
        Param6<int, int, int, int, string, string> done = new Param6<int, int, int, int, string, string>(requestId, count, payload.Length(), payload.Hash(), language, info);
        GetGame().RPCSingleParam(this, NimbyGreetRules.RPC_DONE, done, true, owner);
        int elapsedMs = (int)((GetGame().GetTickTime() - start) * 1000.0);
        NimbyGreetLog.Probe("response id=" + requestId.ToString() + " info=" + info + " chunks=" + count.ToString() + " bytes=" + payload.Length().ToString() + " chars=" + payload.LengthUtf8().ToString() + " hash=" + payload.Hash().ToString() + " sendMs=" + elapsedMs.ToString() + " " + NimbyGreetLog.Who(owner));
    }
}
