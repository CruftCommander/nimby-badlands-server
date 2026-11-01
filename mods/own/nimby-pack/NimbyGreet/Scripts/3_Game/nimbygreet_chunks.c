// -----------------------------------------------------------------------------
// NimbyGreet - the done record (plan v0.4, G5).
// Purpose : NimbyGreetDone is the done record (6551103): the fields the client needs
//           besides the chunks, serialized to one JSON string so the RPC stays
//           Param3<int, int, string> however the record grows. totalBytes and hash are what
//           the NimbyShared receiver verifies the reassembled payload against.
// Plan 11 : the chunked delivery itself (the chunk-by-index map, the G5 validation list, the
//           byte-count and hash check, the server send loop) moved to NimbySharedChunkReceiver
//           and NimbySharedChunkSender (NimbyShared/Scripts/3_Game/nimbyshared_chunks.c) in
//           Plan 11 Phase 1, October 31, 2026. The done record stays Greet's type (Plan 11 C7).
// Phase 0 : the done record and the hash check carried the 40,359-character Russian payload
//           intact in 74 chunks (G-P0-31).
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
