// -----------------------------------------------------------------------------
// NimbyAdmin - the 4_World to 5_Mission bridge on the client (the Greet G5 pattern,
// nimbygreet_client.c). PlayerBase (4_World) receives the RPCs and OnPlayerLoaded; the menu and
// the role live in 5_Mission, which 4_World cannot name. ScriptInvoker: 2_GameLib/DayZ/tools.c:115.
// Client only. The server half never uses these.
// Style : One statement per line.
// -----------------------------------------------------------------------------

class NimbyAdminClient
{
    protected static ref ScriptInvoker s_Spawned;
    protected static ref ScriptInvoker s_Result;
    protected static ref ScriptInvoker s_Push;
    protected static ref ScriptInvoker s_Chunk;
    protected static ref ScriptInvoker s_Done;

    // Invoke(PlayerBase player): the controlled player finished OnPlayerLoaded.
    static ScriptInvoker Spawned()
    {
        if (!s_Spawned)
        {
            s_Spawned = new ScriptInvoker();
        }
        return s_Spawned;
    }

    // Invoke(int requestId, int code, string detail).
    static ScriptInvoker Result()
    {
        if (!s_Result)
        {
            s_Result = new ScriptInvoker();
        }
        return s_Result;
    }

    // Invoke(int kind, int sequence, string payload).
    static ScriptInvoker Push()
    {
        if (!s_Push)
        {
            s_Push = new ScriptInvoker();
        }
        return s_Push;
    }

    // Invoke(int requestId, int index, string text).
    static ScriptInvoker Chunk()
    {
        if (!s_Chunk)
        {
            s_Chunk = new ScriptInvoker();
        }
        return s_Chunk;
    }

    // Invoke(int requestId, int chunkCount, string doneJson).
    static ScriptInvoker Done()
    {
        if (!s_Done)
        {
            s_Done = new ScriptInvoker();
        }
        return s_Done;
    }
}
