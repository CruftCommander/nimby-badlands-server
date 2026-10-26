// -----------------------------------------------------------------------------
// NimbyGreet - the 4_World to 5_Mission bridge (plan v0.4, G5).
// Purpose : PlayerBase (4_World) is where OnPlayerLoaded and OnRPC arrive, but the window,
//           the once-per-connection state and the server's text live in 5_Mission, which
//           4_World cannot name. These ScriptInvokers (2_GameLib/DayZ/tools.c:115) carry
//           the events upward on both machines:
//             client: Spawned, Chunk, Done
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
