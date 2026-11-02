// -----------------------------------------------------------------------------
// NimbyAdmin - PlayerBase, client side of the wire (recon 4.1, 8.4).
// Purpose : Sends 6550301 for the controlled player and hands 6550302 (result), 6550303 (push),
//           6550304 (chunk) and 6550305 (done) to MissionGameplay through NimbyAdminClient, and
//           reports the end of OnPlayerLoaded (4_World/DayZ/Entities/ManBase/PlayerBase.c, the
//           Greet bridge) so the client sends session.hello.
// Gates   : recon R11. This PBO also loads on the server, so every handler here runs only when
//           IsClient(). The request id 6550301 is handled only by NimbyAdminServer; on the
//           server this file passes it to super, where the server PBO's override is.
// Vanilla : PlayerBase.OnRPC (PlayerBase.c:5727) reached through DayZGame.OnRPC
//           (3_Game/DayZ/DayZGame.c:3141,3151); CGame.RPCSingleParam (3_Game/DayZ/Global/Game.c:1013).
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
        NimbyAdminClient.Spawned().Invoke(this);
    }

    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        if (rpc_type == NimbyAdminRules.RPC_RESULT || rpc_type == NimbyAdminRules.RPC_PUSH)
        {
            if (GetGame().IsClient())
            {
                NimbyAdminReadTriple(rpc_type, ctx);
            }
            return;
        }
        if (rpc_type == NimbyAdminRules.RPC_CHUNK || rpc_type == NimbyAdminRules.RPC_DONE)
        {
            if (GetGame().IsClient())
            {
                NimbyAdminReadTriple(rpc_type, ctx);
            }
            return;
        }
        super.OnRPC(sender, rpc_type, ctx);
    }

    // The four server-to-client records are all Param3<int, int, string>.
    protected void NimbyAdminReadTriple(int rpc_type, ParamsReadContext ctx)
    {
        Param3<int, int, string> record = new Param3<int, int, string>(0, 0, "");
        if (!ctx.Read(record))
        {
            // Phase 1 A4: the number is a local, not a call inside Info's argument.
            string rpcText = rpc_type.ToString();
            NimbyAdminLog.Info("client: unreadable record rpc=" + rpcText);
            return;
        }
        if (rpc_type == NimbyAdminRules.RPC_RESULT)
        {
            NimbyAdminClient.Result().Invoke(record.param1, record.param2, record.param3);
            return;
        }
        if (rpc_type == NimbyAdminRules.RPC_PUSH)
        {
            NimbyAdminClient.Push().Invoke(record.param1, record.param2, record.param3);
            return;
        }
        if (rpc_type == NimbyAdminRules.RPC_CHUNK)
        {
            NimbyAdminClient.Chunk().Invoke(record.param1, record.param2, record.param3);
            return;
        }
        NimbyAdminClient.Done().Invoke(record.param1, record.param2, record.param3);
    }

    // Client: sends one request for this (controlled) player.
    void NimbyAdminSendRequest(int requestId, string command, string args)
    {
        Param4<int, int, string, string> request = new Param4<int, int, string, string>(NimbyAdminRules.PROTOCOL, requestId, command, args);
        GetGame().RPCSingleParam(this, NimbyAdminRules.RPC_REQUEST, request, true);
        string idText = requestId.ToString();
        NimbyAdminLog.Debug("request sent id=" + idText + " command=" + command);
    }
}
