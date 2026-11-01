// -----------------------------------------------------------------------------
// NimbyShared - the chunked server-to-client codec (Plan 11 v0.2, C7).
// Purpose : A payload larger than one RPC string travels as CHUNK_BYTES chunks plus a done
//           record, each Param3<int, int, string>:
//             chunk  Param3<int requestId, int index, string text>     (text at most CHUNK_BYTES)
//             done   Param3<int requestId, int chunkCount, string doneJson>
//           The done record is the consumer's own type, serialized by the consumer; the codec
//           carries it as an opaque string and reads only the byte count and hash the consumer
//           hands it. Chunk and done RPC ids are the consumer's (shared rules 4.1: NimbyShared
//           has no RPC block).
//           NimbySharedChunks    : the UTF-8 safe cut, engine-free (server and client).
//           NimbySharedChunkReceiver: one request in flight on the client, with the Greet G5
//             validation list:
//             - a message for another requestId is dropped;
//             - a chunk above CHUNK_BYTES, with a negative index, with an index at or beyond
//               chunkCount once done is known, or repeating an index, is dropped;
//             - a chunkCount above the cap is refused (the whole response is dropped);
//             - the reassembled payload must match the byte count and hash and stay under the
//               cap given at construction.
//           NimbySharedChunkSender: the server side, RPCSingleParam to the owning identity
//             (3_Game/DayZ/Global/Game.c:1013); a server-only PBO MAY use it (shared rules 8.1).
// Engine  : string.Length and Substring count bytes (1_Core/DayZ/proto/EnString.c:137,199);
//           string.Get(index) returns the byte at index as a one-byte string (EnString.c:448)
//           and ToAscii (:86) gives its value; string.Hash (EnString.c:223).
// Origin  : NimbyGreetRules.ChunkCount, MaxChunkCount, IsContinuationByte, ChunkEnd, SplitChunks,
//           FillBytes and NimbyGreetFetch (Greet plan v0.4, G5), moved in Plan 11 Phase 1. Every
//           "why" string is unchanged, so the consumer's debug lines read as before. Greet Phase 0
//           carried the 40,359-character Russian payload intact in 74 chunks (G-P0-31) and
//           dropped the first request's chunks in the race (G-P0-32) with this code.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbySharedChunks
{
    // How many chunks of CHUNK_BYTES a payload of the given byte length needs. 0 for 0.
    static int ChunkCount(int bytes)
    {
        if (bytes <= 0)
        {
            return 0;
        }
        return (bytes + NimbySharedRules.CHUNK_BYTES - 1) / NimbySharedRules.CHUNK_BYTES;
    }

    // The most chunks a legitimate response under the consumer's payload cap can carry.
    static int MaxChunkCount(int maxPayloadBytes)
    {
        return ChunkCount(maxPayloadBytes);
    }

    // True when the byte at index is a UTF-8 continuation byte (0x80 to 0xBF). False outside the string.
    static bool IsContinuationByte(string text, int index)
    {
        if (index < 0)
        {
            return false;
        }
        if (index >= text.Length())
        {
            return false;
        }
        int code = text.Get(index).ToAscii();
        if (code < 0)
        {
            code = code + 256;
        }
        if (code < 128)
        {
            return false;
        }
        if (code >= 192)
        {
            return false;
        }
        return true;
    }

    // Exclusive end offset of a chunk that starts at start, holds at most maxBytes bytes, and never
    // ends inside a multi-byte character. Returns the text length when the rest fits.
    static int ChunkEnd(string text, int start, int maxBytes)
    {
        int length = text.Length();
        if (maxBytes <= 0)
        {
            return start;
        }
        int end = start + maxBytes;
        if (end >= length)
        {
            return length;
        }
        while (end > start)
        {
            if (!IsContinuationByte(text, end))
            {
                return end;
            }
            end = end - 1;
        }
        // A run of continuation bytes longer than the chunk is not UTF-8; cut at the byte limit.
        return start + maxBytes;
    }

    // Splits text into chunks of at most maxBytes bytes on character boundaries. Returns the count.
    static int SplitChunks(string text, int maxBytes, out array<string> chunks)
    {
        chunks = new array<string>;
        int length = text.Length();
        int start = 0;
        while (start < length)
        {
            int end = ChunkEnd(text, start, maxBytes);
            if (end <= start)
            {
                break;
            }
            chunks.Insert(text.Substring(start, end - start));
            start = end;
        }
        return chunks.Count();
    }

    // unit repeated until the text holds bytes bytes, then cut on a character boundary at or below
    // bytes. A self-test helper.
    static string FillBytes(string unit, int bytes)
    {
        int unitBytes = unit.Length();
        if (unitBytes <= 0)
        {
            return "";
        }
        if (bytes <= 0)
        {
            return "";
        }
        int count = (bytes + unitBytes - 1) / unitBytes;
        string text = "";
        int i = 0;
        while (i < count)
        {
            text = text + unit;
            i = i + 1;
        }
        int end = ChunkEnd(text, 0, bytes);
        return text.Substring(0, end);
    }
}

class NimbySharedChunkReceiver
{
    int m_RequestId;
    float m_StartTime;
    float m_LastChunkTime;
    float m_DoneTime;
    int m_ChunkCount = -1;
    int m_TotalBytes;
    int m_Hash;
    int m_Received;
    int m_ReceivedBytes;
    bool m_DoneSeen;
    bool m_Refused;
    string m_RefusedWhy;
    protected int m_MaxPayloadBytes;
    protected int m_MaxChunks;
    protected ref map<int, string> m_Chunks;

    // R3: the one cap; the chunk size is the codec's CHUNK_BYTES.
    void NimbySharedChunkReceiver(int maxPayloadBytes)
    {
        m_MaxPayloadBytes = maxPayloadBytes;
        m_MaxChunks = NimbySharedChunks.MaxChunkCount(maxPayloadBytes);
        m_Chunks = new map<int, string>;
    }

    // Starts a request; Begin(0, 0) clears the receiver.
    void Begin(int requestId, float now)
    {
        m_RequestId = requestId;
        m_StartTime = now;
        m_LastChunkTime = now;
        m_DoneTime = 0;
        m_ChunkCount = -1;
        m_TotalBytes = 0;
        m_Hash = 0;
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

    int MaxChunks()
    {
        return m_MaxChunks;
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
        if (index >= m_MaxChunks)
        {
            why = "index beyond the payload cap";
            return false;
        }
        if (text.Length() > NimbySharedRules.CHUNK_BYTES)
        {
            why = "chunk above CHUNK_BYTES";
            return false;
        }
        if (m_Chunks.Contains(index))
        {
            why = "duplicate index";
            return false;
        }
        if (m_ReceivedBytes + text.Length() > m_MaxPayloadBytes)
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

    // False when the done record is for another request or fails validation. readable is false when
    // the consumer could not parse its done record; totalBytes and hash come from that record.
    bool AcceptDone(int requestId, int chunkCount, bool readable, int totalBytes, int hash, float now, out string why)
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
        if (m_DoneSeen)
        {
            why = "duplicate done";
            return false;
        }
        if (!readable)
        {
            why = "done record unreadable";
            Refuse(why);
            return false;
        }
        if (chunkCount < 0 || chunkCount > m_MaxChunks)
        {
            why = "chunkCount out of range";
            Refuse(why);
            return false;
        }
        if (totalBytes < 0 || totalBytes > m_MaxPayloadBytes)
        {
            why = "totalBytes out of range";
            Refuse(why);
            return false;
        }
        m_ChunkCount = chunkCount;
        m_TotalBytes = totalBytes;
        m_Hash = hash;
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
        if (!m_DoneSeen)
        {
            return "no done record";
        }
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

class NimbySharedChunkSender
{
    // Sends every chunk of payload, then the done record, to recipient only; returns the chunk count.
    // target is the object the consumer's OnRPC hook listens on (Greet: the recipient's player).
    // The consumer fills its done record's byte count and hash (payload.Length(), payload.Hash())
    // before it serializes doneJson. The RPC order is chunks, then done.
    static int Send(Object target, PlayerIdentity recipient, int chunkRpc, int doneRpc, int requestId, string payload, string doneJson)
    {
        if (!target)
        {
            return 0;
        }
        array<string> chunks;
        int count = NimbySharedChunks.SplitChunks(payload, NimbySharedRules.CHUNK_BYTES, chunks);
        int i = 0;
        while (i < count)
        {
            Param3<int, int, string> chunk = new Param3<int, int, string>(requestId, i, chunks.Get(i));
            GetGame().RPCSingleParam(target, chunkRpc, chunk, true, recipient);
            i = i + 1;
        }
        SendDone(target, recipient, doneRpc, requestId, count, doneJson);
        return count;
    }

    // A done record alone: chunkCount 0 for a response that carries no payload.
    static void SendDone(Object target, PlayerIdentity recipient, int doneRpc, int requestId, int chunkCount, string doneJson)
    {
        if (!target)
        {
            return;
        }
        Param3<int, int, string> done = new Param3<int, int, string>(requestId, chunkCount, doneJson);
        GetGame().RPCSingleParam(target, doneRpc, done, true, recipient);
    }
}
