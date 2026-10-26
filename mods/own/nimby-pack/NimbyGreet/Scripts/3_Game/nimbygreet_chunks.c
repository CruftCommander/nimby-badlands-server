// -----------------------------------------------------------------------------
// NimbyGreet - chunked delivery, client side (plan v0.4, G5).
// Purpose : NimbyGreetDone is the done record (6551103): the fields the client needs
//           besides the chunks, serialized to one JSON string so the RPC stays
//           Param3<int, int, string> however the record grows. NimbyGreetFetch holds one
//           request in flight on the client: the requestId, the chunks by index, the done
//           record and the timing, and applies the G5 validation list:
//             - a message for another requestId is dropped;
//             - a chunk above CHUNK_BYTES, with a negative index, with an index at or beyond
//               chunkCount once done is known, or repeating an index, is dropped;
//             - a chunkCount above MaxChunkCount is refused (the whole response is dropped);
//             - the reassembled JSON must match the byte count and hash in done and stay
//               under MAX_PAYLOAD_BYTES.
//           Everything here is engine-free except string.Hash (EnString.c:223), so the
//           self-test drives it with made-up chunks.
// Phase 0 : the chunk-by-index map, the done record and the hash check carried the
//           40,359-character Russian payload intact in 74 chunks (G-P0-31) and dropped
//           the first request's chunks in the race (G-P0-32).
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyGreetDone
{
    int version = 0;
    bool forceShow = false;
    string servedLanguage = "";
    ref array<string> languages = new array<string>;
    float openDelaySeconds = 1.0;
    float openRetrySeconds = 30.0;
    int totalBytes = 0;
    int hash = 0;
    string reason = "";   // why nothing was sent when chunkCount is 0: "dismissed", "no-text", "cooldown"
    float retryAfterSeconds = 0;   // with reason "cooldown": how long until the server will answer again

    static string ToJson(NimbyGreetDone done)
    {
        string text;
        JsonSerializer serializer = new JsonSerializer();
        serializer.WriteToString(done, false, text);
        return text;
    }

    static NimbyGreetDone FromJson(string text, out string error)
    {
        NimbyGreetDone done = new NimbyGreetDone();
        JsonSerializer serializer = new JsonSerializer();
        if (!serializer.ReadFromString(done, text, error))
        {
            return null;
        }
        if (!done.languages)
        {
            done.languages = new array<string>;
        }
        return done;
    }
}

class NimbyGreetFetch
{
    int m_RequestId;
    string m_Language;
    bool m_Manual;
    float m_StartTime;
    float m_LastChunkTime;
    float m_DoneTime;
    int m_ChunkCount = -1;
    ref NimbyGreetDone m_Done;
    int m_Received;
    int m_ReceivedBytes;
    bool m_DoneSeen;
    bool m_Refused;
    string m_RefusedWhy;
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
        m_Done = null;
        m_Received = 0;
        m_ReceivedBytes = 0;
        m_DoneSeen = false;
        m_Refused = false;
        m_RefusedWhy = "";
        m_Chunks.Clear();
    }

    bool IsActive()
    {
        return m_RequestId != 0;
    }

    // False when the chunk fails the validation list; why says which rule.
    bool AcceptChunk(int requestId, int index, string text, float now, out string why)
    {
        why = "";
        if (requestId != m_RequestId)
        {
            why = "other request";
            return false;
        }
        if (m_Refused)
        {
            why = "response refused";
            return false;
        }
        if (index < 0)
        {
            why = "negative index";
            return false;
        }
        if (m_DoneSeen && index >= m_ChunkCount)
        {
            why = "index beyond chunkCount";
            return false;
        }
        if (index >= NimbyGreetRules.MaxChunkCount())
        {
            why = "index beyond the payload cap";
            return false;
        }
        if (text.Length() > NimbyGreetRules.CHUNK_BYTES)
        {
            why = "chunk above CHUNK_BYTES";
            return false;
        }
        if (m_Chunks.Contains(index))
        {
            why = "duplicate index";
            return false;
        }
        if (m_ReceivedBytes + text.Length() > NimbyGreetRules.MAX_PAYLOAD_BYTES)
        {
            why = "payload above MAX_PAYLOAD_BYTES";
            Refuse(why);
            return false;
        }
        m_Chunks.Insert(index, text);
        m_Received = m_Received + 1;
        m_ReceivedBytes = m_ReceivedBytes + text.Length();
        m_LastChunkTime = now;
        return true;
    }

    // False when the done record is for another request or fails validation.
    bool AcceptDone(int requestId, int chunkCount, NimbyGreetDone done, float now, out string why)
    {
        why = "";
        if (requestId != m_RequestId)
        {
            why = "other request";
            return false;
        }
        if (m_Refused)
        {
            why = "response refused";
            return false;
        }
        if (!done)
        {
            why = "done record unreadable";
            Refuse(why);
            return false;
        }
        if (chunkCount < 0 || chunkCount > NimbyGreetRules.MaxChunkCount())
        {
            why = "chunkCount out of range";
            Refuse(why);
            return false;
        }
        if (done.totalBytes < 0 || done.totalBytes > NimbyGreetRules.MAX_PAYLOAD_BYTES)
        {
            why = "totalBytes out of range";
            Refuse(why);
            return false;
        }
        if (m_DoneSeen)
        {
            why = "duplicate done";
            return false;
        }
        m_ChunkCount = chunkCount;
        m_Done = done;
        m_DoneSeen = true;
        m_DoneTime = now;
        // Chunks that arrived before done and sit beyond chunkCount are discarded now.
        array<int> drop = new array<int>;
        foreach (int index, string chunk : m_Chunks)
        {
            if (index >= chunkCount)
            {
                drop.Insert(index);
            }
        }
        foreach (int dropIndex : drop)
        {
            string gone;
            if (m_Chunks.Find(dropIndex, gone))
            {
                m_ReceivedBytes = m_ReceivedBytes - gone.Length();
                m_Received = m_Received - 1;
                m_Chunks.Remove(dropIndex);
            }
        }
        return true;
    }

    protected void Refuse(string why)
    {
        m_Refused = true;
        m_RefusedWhy = why;
        m_Chunks.Clear();
        m_Received = 0;
        m_ReceivedBytes = 0;
    }

    bool IsRefused()
    {
        return m_Refused;
    }

    bool IsComplete()
    {
        if (m_Refused)
        {
            return false;
        }
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
        if (!m_Done)
        {
            return "no done record";
        }
        if (assembled.Length() != m_Done.totalBytes)
        {
            return "length mismatch got=" + assembled.Length().ToString() + " expected=" + m_Done.totalBytes.ToString();
        }
        if (assembled.Hash() != m_Done.hash)
        {
            return "hash mismatch got=" + assembled.Hash().ToString() + " expected=" + m_Done.hash.ToString();
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
