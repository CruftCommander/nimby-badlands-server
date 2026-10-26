// -----------------------------------------------------------------------------
// NimbyGreet - the 4_World to 5_Mission bridge (plan v0.4, G5).
// Purpose : PlayerBase (4_World) is where OnPlayerLoaded and OnRPC arrive, but the window,
//           the once-per-connection state and the server's text live in 5_Mission, which
//           4_World cannot name. These ScriptInvokers (2_GameLib/DayZ/tools.c:115) carry
//           the events upward on both machines:
//             client: Spawned, Chunk, Done, Hit
//             server: Request
//           The request-id counter lives here too so every request in one client session
//           gets a fresh id (G5).
// Phase 0 : the same three client invokers carried every event of both runs.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyGreetClient
{
    protected static ref ScriptInvoker s_Spawned;
    protected static ref ScriptInvoker s_Chunk;
    protected static ref ScriptInvoker s_Done;
    protected static ref ScriptInvoker s_Request;
    protected static ref ScriptInvoker s_Hit;
    protected static int s_NextRequestId = 1;

    // Client. Invoke(PlayerBase player): the controlled player finished OnPlayerLoaded.
    static ScriptInvoker Spawned()
    {
        if (!s_Spawned)
        {
            s_Spawned = new ScriptInvoker();
        }
        return s_Spawned;
    }

    // Client. Invoke(int requestId, int index, string text).
    static ScriptInvoker Chunk()
    {
        if (!s_Chunk)
        {
            s_Chunk = new ScriptInvoker();
        }
        return s_Chunk;
    }

    // Client. Invoke(int requestId, int chunkCount, string doneJson).
    static ScriptInvoker Done()
    {
        if (!s_Done)
        {
            s_Done = new ScriptInvoker();
        }
        return s_Done;
    }

    // Client. Invoke(): the controlled player took a hit (G5 close rule).
    static ScriptInvoker Hit()
    {
        if (!s_Hit)
        {
            s_Hit = new ScriptInvoker();
        }
        return s_Hit;
    }

    // Server. Invoke(PlayerBase player, PlayerIdentity sender, int requestId, string language, int dismissedVersion, bool neverShow, bool manual).
    static ScriptInvoker Request()
    {
        if (!s_Request)
        {
            s_Request = new ScriptInvoker();
        }
        return s_Request;
    }

    static int NextRequestId()
    {
        int id = s_NextRequestId;
        s_NextRequestId = s_NextRequestId + 1;
        return id;
    }
}

// NIMBY-T-052: Greet's deferred calls. The 5_Mission script module is compiled afresh for every
// mission (the "Module: Mission; loaded ..." line at each mission start), so a call queued from
// 5_Mission code, instance or static, that fires after the player leaves points into a discarded
// module: an access violation in DayZGame.OnUpdate (dayzgame.c:3044, the GUI call queue tick), in
// Phase 1 run 5 three times, about 20 s after a request (the fetch give-up). ScriptCallQueue.Remove
// did not prevent it. Every Greet CallLater therefore queues Fire, which lives in 3_Game (never
// reloaded), and reaches the mission through the vanilla Mission class (3_Game, gameplay.c:685);
// MissionGameplay overrides NimbyGreetDeferredCall and drops calls for an earlier connection.
class NimbyGreetDeferred
{
    static const int AUTO_REQUEST = 1;
    static const int FETCH_TIMEOUT = 2;
    static const int RETRY_REQUEST = 3;
    static const int TRY_OPEN = 4;

    // Bumped by MissionGameplay in OnInit and OnMissionFinish; kept here so it survives the reload.
    static int s_Connection;

    static void Fire(int kind, int connection, int param)
    {
        if (!g_Game)
        {
            return;
        }
        Mission mission = g_Game.GetMission();
        if (mission)
        {
            mission.NimbyGreetDeferredCall(kind, connection, param);
        }
    }
}

modded class Mission
{
    // Overridden by Greet's MissionGameplay; every other mission (the main menu included) ignores it.
    void NimbyGreetDeferredCall(int kind, int connection, int param)
    {
    }
}
