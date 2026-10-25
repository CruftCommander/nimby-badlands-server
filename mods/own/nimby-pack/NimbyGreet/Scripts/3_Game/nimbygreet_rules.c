// -----------------------------------------------------------------------------
// NimbyGreet - rules: the constants every other Greet file reads, and the engine-free
// validators the self-test runs (shared rules 8). No engine call in this file.
// Plan : docs/plans/2026-10-21-greet-plan.md v0.3, G1 (ids), G5 (chunking), G9 (caps),
//        G17 (files). A cap change is a plan change first.
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
        return failures;
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
