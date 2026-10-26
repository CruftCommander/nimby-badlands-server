// -----------------------------------------------------------------------------
// NimbyGreet - client fetch state and the 4_World to 5_Mission bridge (plan G5).
// Purpose : NimbyGreetFetch holds one request in flight on the client: the requestId,
//           the chunks by index, the done record, and the timing, and checks the
//           reassembled payload against the byte count and hash the server sent.
//           NimbyGreetClient carries three ScriptInvokers (2_GameLib/DayZ/tools.c:115)
//           so PlayerBase (4_World), where OnPlayerLoaded and OnRPC arrive, can hand the
//           events to MissionGameplay (5_Mission), which owns the window and the
//           once-per-connection state; 4_World cannot name a 5_Mission class.
// Engine  : string.Hash (EnString.c:219) is the integrity check; string.Length is bytes.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyGreetFetch
{
    int m_RequestId;
    string m_Language;
    bool m_Manual;
    float m_StartTime;
    float m_LastChunkTime;
    float m_DoneTime;
    int m_ChunkCount = -1;
    int m_TotalBytes;
    int m_Hash;
    string m_ServedLanguage;
    string m_Info;
    int m_Received;
    int m_ReceivedBytes;
    bool m_DoneSeen;
    ref map<int, string> m_Chunks;

    void NimbyGreetFetch()
    {
        m_Chunks = new map<int, string>;
    }

    void Begin(int requestId, string language, bool manual, float now)
    {
        m_RequestId = requestId;
        m_Language = language;
        m_Manual = manual;
        m_StartTime = now;
        m_LastChunkTime = now;
        m_DoneTime = 0;
        m_ChunkCount = -1;
        m_TotalBytes = 0;
        m_Hash = 0;
        m_ServedLanguage = "";
        m_Info = "";
        m_Received = 0;
        m_ReceivedBytes = 0;
        m_DoneSeen = false;
        m_Chunks.Clear();
    }

    // False when the chunk belongs to another request or repeats an index.
    bool AcceptChunk(int requestId, int index, string text, float now)
    {
        if (requestId != m_RequestId)
        {
            return false;
        }
        if (index < 0)
        {
            return false;
        }
        if (m_Chunks.Contains(index))
        {
            return false;
        }
        m_Chunks.Insert(index, text);
        m_Received = m_Received + 1;
        m_ReceivedBytes = m_ReceivedBytes + text.Length();
        m_LastChunkTime = now;
        return true;
    }

    bool AcceptDone(int requestId, int chunkCount, int totalBytes, int hash, string servedLanguage, string info, float now)
    {
        if (requestId != m_RequestId)
        {
            return false;
        }
        m_ChunkCount = chunkCount;
        m_TotalBytes = totalBytes;
        m_Hash = hash;
        m_ServedLanguage = servedLanguage;
        m_Info = info;
        m_DoneSeen = true;
        m_DoneTime = now;
        return true;
    }

    bool IsComplete()
    {
        if (!m_DoneSeen)
        {
            return false;
        }
        if (m_ChunkCount < 0)
        {
            return false;
        }
        return m_Received >= m_ChunkCount;
    }

    string Assemble()
    {
        string result = "";
        int i = 0;
        while (i < m_ChunkCount)
        {
            string part;
            if (m_Chunks.Find(i, part))
            {
                result = result + part;
            }
            i = i + 1;
        }
        return result;
    }

    // "intact" when the byte count and hash match what the server sent; otherwise the mismatch.
    string Verify(string assembled)
    {
        if (assembled.Length() != m_TotalBytes)
        {
            return "length mismatch got=" + assembled.Length().ToString() + " expected=" + m_TotalBytes.ToString();
        }
        if (assembled.Hash() != m_Hash)
        {
            return "hash mismatch got=" + assembled.Hash().ToString() + " expected=" + m_Hash.ToString();
        }
        return "intact";
    }

    // Milliseconds from the request to the later of the last chunk and the done record.
    int ElapsedMs()
    {
        float end = m_LastChunkTime;
        if (m_DoneTime > end)
        {
            end = m_DoneTime;
        }
        return (int)((end - m_StartTime) * 1000.0);
    }
}

class NimbyGreetClient
{
    protected static ref ScriptInvoker s_Spawned;
    protected static ref ScriptInvoker s_Chunk;
    protected static ref ScriptInvoker s_Done;
    protected static int s_NextRequestId = 1;

    // Invoke(PlayerBase player): the controlled player finished OnPlayerLoaded.
    static ScriptInvoker Spawned()
    {
        if (!s_Spawned)
        {
            s_Spawned = new ScriptInvoker();
        }
        return s_Spawned;
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

    // Invoke(int requestId, int chunkCount, int totalBytes, int hash, string servedLanguage, string info).
    static ScriptInvoker Done()
    {
        if (!s_Done)
        {
            s_Done = new ScriptInvoker();
        }
        return s_Done;
    }

    static int NextRequestId()
    {
        int id = s_NextRequestId;
        s_NextRequestId = s_NextRequestId + 1;
        return id;
    }
}
