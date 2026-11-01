// -----------------------------------------------------------------------------
// NimbyShared - self-test (shared rules 8, Plan 11 v0.2 C8).
// Purpose : The engine-free validators, run at mission start when $profile:NimbyShared\selftest
//           exists (MissionBase hook in 5_Mission, which appends the 5_Mission registry cases).
//           The chunk and receiver cases moved here from the Greet self-test with their names
//           unchanged (Plan 11 recon, section 1.6).
//           The boot check reads "[NIMBY][Shared] selftest complete; cases=N failures=M".
// Style   : One statement per line.
// -----------------------------------------------------------------------------

// Plan 11 risk 1: the window base releases the zeroing guard in its destructor, and a subclass
// keeps a destructor of its own. These two classes prove at boot that both destructors run.
class NimbySharedDtorProbeBase
{
    static int s_Order;

    void ~NimbySharedDtorProbeBase()
    {
        s_Order = s_Order * 10 + 2;
    }
}

class NimbySharedDtorProbe extends NimbySharedDtorProbeBase
{
    void ~NimbySharedDtorProbe()
    {
        s_Order = s_Order * 10 + 1;
    }
}

class NimbySharedSelfTest
{
    // Returns the number of failures; the caller prints one PASS|FAIL line per result.
    static int Run(out array<string> results)
    {
        results = new array<string>;
        int failures = 0;
        failures += RuleCases(results);
        failures += ChunkCases(results);
        failures += ReceiverCases(results);
        failures += LanguageCases(results);
        return failures;
    }

    // ---- rules (the October 31 stub cases, then the text-size steps) -------------------------

    protected static int RuleCases(array<string> results)
    {
        int failures = 0;
        failures += Case(results, "rules: CHUNK_BYTES is 1000 (NIMBY-T-037)", NimbySharedRules.CHUNK_BYTES == 1000);
        failures += Case(results, "rules: debug flag lives under $profile:NimbyShared", NimbySharedRules.FILE_DEBUG.IndexOf(NimbySharedRules.DIR) == 0);
        failures += Case(results, "rules: selftest flag lives under $profile:NimbyShared", NimbySharedRules.FILE_SELFTEST.IndexOf(NimbySharedRules.DIR) == 0);
        failures += Case(results, "scale: normal steps to large", NimbySharedRules.NextTextScale(NimbySharedRules.TEXT_SCALE_NORMAL) == NimbySharedRules.TEXT_SCALE_LARGE);
        failures += Case(results, "scale: large steps to larger", NimbySharedRules.NextTextScale(NimbySharedRules.TEXT_SCALE_LARGE) == NimbySharedRules.TEXT_SCALE_LARGER);
        failures += Case(results, "scale: larger wraps to normal", NimbySharedRules.NextTextScale(NimbySharedRules.TEXT_SCALE_LARGER) == NimbySharedRules.TEXT_SCALE_NORMAL);
        return failures;
    }

    // ---- chunk math and the UTF-8 cut (Greet Phase 0 cases) ----------------------------------

    protected static int ChunkCases(array<string> results)
    {
        int failures = 0;
        failures += Case(results, "chunks: 0 chars is 0 chunks", NimbySharedChunks.ChunkCount(0) == 0);
        failures += Case(results, "chunks: one char is 1 chunk", NimbySharedChunks.ChunkCount(1) == 1);
        failures += Case(results, "chunks: exact multiple", NimbySharedChunks.ChunkCount(NimbySharedRules.CHUNK_BYTES * 2) == 2);
        failures += Case(results, "chunks: one over rounds up", NimbySharedChunks.ChunkCount(NimbySharedRules.CHUNK_BYTES * 2 + 1) == 3);
        failures += Case(results, "chunks: 40000 bytes is 40 chunks", NimbySharedChunks.ChunkCount(40000) == 40);
        failures += Case(results, "chunks: cap of 164000 bytes is 164 chunks", NimbySharedChunks.MaxChunkCount(164000) == 164);
        failures += Case(results, "chunks: cap of 1001 bytes is 2 chunks", NimbySharedChunks.MaxChunkCount(1001) == 2);
        failures += Case(results, "utf8: ascii byte is not a continuation byte", NimbySharedChunks.IsContinuationByte("abc", 1) == false);
        failures += Case(results, "utf8: second byte of a cyrillic char is a continuation byte", NimbySharedChunks.IsContinuationByte("дайз", 1) == true);
        failures += Case(results, "utf8: lead byte of a cyrillic char is not a continuation byte", NimbySharedChunks.IsContinuationByte("дайз", 2) == false);
        failures += Case(results, "utf8: chunk end backs off a split 2-byte char", NimbySharedChunks.ChunkEnd("дайз", 0, 3) == 2);
        failures += Case(results, "utf8: chunk end keeps an exact boundary", NimbySharedChunks.ChunkEnd("дайз", 0, 4) == 4);
        failures += Case(results, "utf8: chunk end past the end is the length", NimbySharedChunks.ChunkEnd("abc", 0, 10) == 3);
        failures += Case(results, "utf8: chunk end backs off a split 3-byte char", NimbySharedChunks.ChunkEnd("字字", 0, 4) == 3);
        failures += Case(results, "fill: ascii gives exact bytes", NimbySharedChunks.FillBytes("a", 10).Length() == 10);
        failures += Case(results, "fill: cyrillic rounds down to a whole char", NimbySharedChunks.FillBytes("д", 5).Length() == 4);
        failures += Case(results, "fill: cjk exact multiple of 3 bytes", NimbySharedChunks.FillBytes("字", 9).Length() == 9);
        array<string> parts;
        int count = NimbySharedChunks.SplitChunks(NimbySharedChunks.FillBytes("a", 80), 32, parts);
        failures += Case(results, "split: 80 ascii bytes at 32 is 3 chunks", count == 3);
        failures += Case(results, "split: chunks rejoin to the input", Rejoin(parts) == NimbySharedChunks.FillBytes("a", 80));
        count = NimbySharedChunks.SplitChunks(NimbySharedChunks.FillBytes("д", 20), 7, parts);
        failures += Case(results, "split: cyrillic at 7 bytes is 4 chunks of 6 bytes", count == 4 && parts.Get(0).Length() == 6);
        failures += Case(results, "split: cyrillic chunks never split a char", ChunksAreWholeChars(parts, 2));
        return failures;
    }

    // ---- the receiver (Greet G5 fetch cases) -------------------------------------------------

    protected static int ReceiverCases(array<string> results)
    {
        int failures = 0;
        string why;
        int cap = 164000;
        string payload = NimbySharedChunks.FillBytes("д", 2500);
        array<string> chunks;
        int count = NimbySharedChunks.SplitChunks(payload, NimbySharedRules.CHUNK_BYTES, chunks);
        failures += Case(results, "fetch: 2500 cyrillic bytes are 3 chunks", count == 3);
        int total = payload.Length();
        int hash = payload.Hash();

        NimbySharedChunkReceiver fetch = new NimbySharedChunkReceiver(cap);
        fetch.Begin(7, 0.0);
        failures += Case(results, "fetch: chunk for another request dropped", !fetch.AcceptChunk(6, 0, chunks.Get(0), 0.1, why) && why == "other request");
        failures += Case(results, "fetch: oversize chunk dropped", !fetch.AcceptChunk(7, 0, NimbySharedChunks.FillBytes("a", NimbySharedRules.CHUNK_BYTES + 1), 0.1, why) && why == "chunk above CHUNK_BYTES");
        failures += Case(results, "fetch: negative index dropped", !fetch.AcceptChunk(7, -1, chunks.Get(0), 0.1, why));
        failures += Case(results, "fetch: index at the payload cap dropped before done", !fetch.AcceptChunk(7, fetch.MaxChunks(), "x", 0.1, why) && why == "index beyond the payload cap");
        failures += Case(results, "fetch: chunk 2 accepted before done", fetch.AcceptChunk(7, 2, chunks.Get(2), 0.2, why));
        failures += Case(results, "fetch: duplicate index dropped", !fetch.AcceptChunk(7, 2, chunks.Get(2), 0.2, why) && why == "duplicate index");
        failures += Case(results, "fetch: stray chunk 9 accepted before done", fetch.AcceptChunk(7, 9, "stray", 0.2, why));
        failures += Case(results, "fetch: not complete without done", !fetch.IsComplete());
        failures += Case(results, "fetch: done accepted and drops the stray chunk", fetch.AcceptDone(7, 3, true, total, hash, 0.3, why) && fetch.m_Received == 1);
        failures += Case(results, "fetch: index beyond chunkCount dropped after done", !fetch.AcceptChunk(7, 3, "late", 0.3, why) && why == "index beyond chunkCount");
        fetch.AcceptChunk(7, 0, chunks.Get(0), 0.4, why);
        failures += Case(results, "fetch: still incomplete at 2 of 3", !fetch.IsComplete());
        fetch.AcceptChunk(7, 1, chunks.Get(1), 0.5, why);
        failures += Case(results, "fetch: complete at 3 of 3 in any order", fetch.IsComplete());
        string assembled = fetch.Assemble();
        failures += Case(results, "fetch: assembled payload intact", assembled == payload && fetch.Verify(assembled) == "intact");
        failures += Case(results, "fetch: elapsed from request to done", fetch.ElapsedMs() == 500);

        NimbySharedChunkReceiver bad = new NimbySharedChunkReceiver(cap);
        bad.Begin(8, 0.0);
        failures += Case(results, "fetch: chunkCount above the cap refuses the response", !bad.AcceptDone(8, bad.MaxChunks() + 1, true, total, hash, 0.1, why) && bad.IsRefused());
        failures += Case(results, "fetch: refused response takes no chunk", !bad.AcceptChunk(8, 0, "x", 0.1, why) && why == "response refused");

        NimbySharedChunkReceiver empty = new NimbySharedChunkReceiver(cap);
        empty.Begin(9, 0.0);
        failures += Case(results, "fetch: done with zero chunks is complete and empty", empty.AcceptDone(9, 0, true, 0, 0, 0.1, why) && empty.IsComplete() && empty.Assemble() == "");

        failures += Case(results, "fetch: duplicate done dropped without refusing", !fetch.AcceptDone(7, 3, true, total, hash, 0.6, why) && why == "duplicate done" && !fetch.IsRefused());

        NimbySharedChunkReceiver unreadable = new NimbySharedChunkReceiver(cap);
        unreadable.Begin(10, 0.0);
        failures += Case(results, "fetch: unreadable done record refuses the response", !unreadable.AcceptDone(10, 1, false, 0, 0, 0.1, why) && why == "done record unreadable" && unreadable.IsRefused());

        NimbySharedChunkReceiver small = new NimbySharedChunkReceiver(1500);
        small.Begin(11, 0.0);
        small.AcceptChunk(11, 0, NimbySharedChunks.FillBytes("a", 1000), 0.1, why);
        failures += Case(results, "fetch: bytes above the cap refuse the response", !small.AcceptChunk(11, 1, NimbySharedChunks.FillBytes("a", 600), 0.1, why) && why == "payload above MAX_PAYLOAD_BYTES" && small.IsRefused());
        return failures;
    }

    // ---- language behaviour the window shell relies on (Plan 11 risk 1) ----------------------

    protected static int LanguageCases(array<string> results)
    {
        int failures = 0;
        NimbySharedDtorProbeBase.s_Order = 0;
        NimbySharedDtorProbe probe = new NimbySharedDtorProbe();
        delete probe;
        failures += Case(results, "lang: subclass destructor runs, then the base destructor", NimbySharedDtorProbeBase.s_Order == 12);
        return failures;
    }

    // ---- helpers -----------------------------------------------------------------------------

    protected static string Rejoin(array<string> parts)
    {
        string result = "";
        foreach (string part : parts)
        {
            result = result + part;
        }
        return result;
    }

    protected static bool ChunksAreWholeChars(array<string> parts, int bytesPerChar)
    {
        foreach (string part : parts)
        {
            if (part.Length() != part.LengthUtf8() * bytesPerChar)
            {
                return false;
            }
        }
        return true;
    }

    static int Case(array<string> results, string name, bool passed)
    {
        if (passed)
        {
            results.Insert("PASS " + name);
            return 0;
        }
        results.Insert("FAIL " + name);
        return 1;
    }
}
