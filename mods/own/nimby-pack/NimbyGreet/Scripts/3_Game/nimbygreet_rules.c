// -----------------------------------------------------------------------------
// NimbyGreet - rules: the constants every other Greet file reads, and the engine-free
// validators the self-test runs (shared rules 8). No engine call in this file.
// Plan : docs/plans/2026-10-21-greet-plan.md v0.3, G1 (ids), G5 (chunking), G9 (caps),
//        G17 (files). A cap change is a plan change first.
// Phase 0 additions (October 2026): the UTF-8 safe chunk cut (ChunkEnd, SplitChunks) and
// the byte-exact string builders the item 8 ceiling test uses on both machines (Repeat,
// FillBytes). string.Length and Substring count bytes, LengthUtf8 and SubstringUtf8
// count characters (1_Core/DayZ/proto/EnString.c:137,199,206,212); string.Get(index)
// returns the byte at index as a one-byte string (EnString.c:448) and ToAscii (:86)
// gives its value, so a UTF-8 continuation byte (10xxxxxx) can be recognized and a
// chunk cut backed off to a character boundary.
// Style: one statement per line.
// -----------------------------------------------------------------------------

class NimbyGreetRules
{
    // G1, G5: the 65511xx block (shared rules 4.1, row pending the v1.4 amendment).
    static const int RPC_REQUEST = 6551101;   // client to server
    static const int RPC_CHUNK = 6551102;     // server to the owning client
    static const int RPC_DONE = 6551103;      // server to the owning client

    // G9 caps, per language.
    static const int MAX_SECTIONS = 16;
    static const int MAX_BODY_CHARS = 4000;
    static const int MAX_LINKS_PER_SECTION = 8;
    static const int MAX_MEDIA_PER_SECTION = 4;
    static const int MAX_TOTAL_CHARS = 40000;
    static const int MAX_TITLE_CHARS = 60;
    static const int MAX_TAGLINE_CHARS = 80;
    static const int MAX_HEADING_CHARS = 40;
    static const int MAX_ID_CHARS = 32;

    // G5: bytes per chunk, set from the Phase 0 item 8 measurement; 1500 is the plan's starting value.
    static const int CHUNK_BYTES = 1500;

    // G5, G6: UIScriptedMenu id; vanilla's highest is MENU_UNDERGROUND_TRIGGER_EDITOR = 48 (constants.c:223).
    static const int MENU_ID = 65511;

    // G3, G4, G17: file names under $profile:NimbyGreet\.
    static const string DIR = "$profile:NimbyGreet";
    static const string FILE_TEXT = "$profile:NimbyGreet\\greet.json";
    static const string FILE_TEXT_LKG = "$profile:NimbyGreet\\greet.json.lkg";
    static const string FILE_SEEN = "$profile:NimbyGreet\\seen.json";
    static const string FILE_PREFS = "$profile:NimbyGreet\\prefs.json";
    static const string FILE_DEBUG = "$profile:NimbyGreet\\debug";
    static const string FILE_SELFTEST = "$profile:NimbyGreet\\selftest";

    // Cuts text to at most cap characters (UTF-8 characters, not bytes: EnString.c:137,206).
    // Returns the input when it fits.
    static string CapText(string text, int cap)
    {
        if (cap < 0)
        {
            return "";
        }
        if (text.LengthUtf8() <= cap)
        {
            return text;
        }
        return text.SubstringUtf8(0, cap);
    }

    // How many chunks of CHUNK_BYTES a serialized payload of the given byte length needs (G5). 0 for 0.
    static int ChunkCount(int bytes)
    {
        if (bytes <= 0)
        {
            return 0;
        }
        return (bytes + CHUNK_BYTES - 1) / CHUNK_BYTES;
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

    // unit repeated count times.
    static string Repeat(string unit, int count)
    {
        string result = "";
        int i = 0;
        while (i < count)
        {
            result = result + unit;
            i = i + 1;
        }
        return result;
    }

    // unit repeated until the text holds bytes bytes, then cut on a character boundary at or below
    // bytes. Exact when bytes is a multiple of the unit's byte length. Used by the item 8 ceiling
    // test on both machines, so the client can rebuild what the server sent.
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
        string text = Repeat(unit, count);
        int end = ChunkEnd(text, 0, bytes);
        return text.Substring(0, end);
    }

    // Returns the number of failures; the caller prints one PASS|FAIL line per case.
    static int SelfTest(out array<string> results)
    {
        int failures = 0;
        failures += SelfTestCase(results, "cap: short text unchanged", CapText("abc", 10) == "abc");
        failures += SelfTestCase(results, "cap: long text cut to cap", CapText("abcdef", 3) == "abc");
        failures += SelfTestCase(results, "cap: zero cap gives empty", CapText("abc", 0) == "");
        failures += SelfTestCase(results, "chunks: 0 chars is 0 chunks", ChunkCount(0) == 0);
        failures += SelfTestCase(results, "chunks: one char is 1 chunk", ChunkCount(1) == 1);
        failures += SelfTestCase(results, "chunks: exact multiple", ChunkCount(CHUNK_BYTES * 2) == 2);
        failures += SelfTestCase(results, "chunks: one over rounds up", ChunkCount(CHUNK_BYTES * 2 + 1) == 3);
        failures += SelfTestCase(results, "chunks: 40000 bytes is 27 chunks", ChunkCount(40000) == 27);
        failures += SelfTestCase(results, "cap: multibyte text cut by characters", CapText("дайз", 2).LengthUtf8() == 2);
        failures += SelfTestCase(results, "utf8: ascii byte is not a continuation byte", IsContinuationByte("abc", 1) == false);
        failures += SelfTestCase(results, "utf8: second byte of a cyrillic char is a continuation byte", IsContinuationByte("дайз", 1) == true);
        failures += SelfTestCase(results, "utf8: lead byte of a cyrillic char is not a continuation byte", IsContinuationByte("дайз", 2) == false);
        failures += SelfTestCase(results, "utf8: chunk end backs off a split 2-byte char", ChunkEnd("дайз", 0, 3) == 2);
        failures += SelfTestCase(results, "utf8: chunk end keeps an exact boundary", ChunkEnd("дайз", 0, 4) == 4);
        failures += SelfTestCase(results, "utf8: chunk end past the end is the length", ChunkEnd("abc", 0, 10) == 3);
        failures += SelfTestCase(results, "fill: ascii gives exact bytes", FillBytes("a", 10).Length() == 10);
        failures += SelfTestCase(results, "fill: cyrillic rounds down to a whole char", FillBytes("д", 5).Length() == 4);
        failures += SelfTestCase(results, "fill: cjk exact multiple of 3 bytes", FillBytes("字", 9).Length() == 9);
        array<string> parts;
        int count = SplitChunks(FillBytes("a", 80), 32, parts);
        failures += SelfTestCase(results, "split: 80 ascii bytes at 32 is 3 chunks", count == 3);
        failures += SelfTestCase(results, "split: chunks rejoin to the input", Rejoin(parts) == FillBytes("a", 80));
        count = SplitChunks(FillBytes("д", 20), 7, parts);
        failures += SelfTestCase(results, "split: cyrillic at 7 bytes is 4 chunks of 6 bytes", count == 4 && parts.Get(0).Length() == 6);
        failures += SelfTestCase(results, "split: cyrillic chunks never split a char", ChunksAreWholeChars(parts, 2));
        return failures;
    }

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

    protected static int SelfTestCase(out array<string> results, string name, bool passed)
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
