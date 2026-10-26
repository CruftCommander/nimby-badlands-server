// -----------------------------------------------------------------------------
// NimbyGreet - rules: the constants every other Greet file reads, and the engine-free
// decisions the self-test covers (shared rules 8). No engine call in this file.
// Plan : docs/plans/2026-10-21-greet-plan.md v0.4: G1 (ids), G4 (dismissal), G5
//        (chunking, delays), G9 (caps), G10 (links), G14 (languages), G17 (files).
//        A cap or constant change is a plan change first.
// Phase 0: CHUNK_BYTES is 1,000 because an RPC string arrives intact at 1,000 bytes and
//        throws "String CORRUPTED" on Read at 1,100 and above (NIMBY-T-037); the UTF-8
//        safe cut (ChunkEnd, SplitChunks) keeps every chunk on a character boundary.
//        string.Length and Substring count bytes, LengthUtf8 and SubstringUtf8 count
//        characters (1_Core/DayZ/proto/EnString.c:137,199,206,212); string.Get(index)
//        returns the byte at index as a one-byte string (EnString.c:448) and ToAscii
//        (:86) gives its value, so a continuation byte (10xxxxxx) can be recognized.
// Style: one statement per line.
// -----------------------------------------------------------------------------

class NimbyGreetRules
{
    // G1, G5: the 65511xx block (shared rules v1.4, 4.1).
    static const int RPC_REQUEST = 6551101;   // client to server
    static const int RPC_CHUNK = 6551102;     // server to the owning client
    static const int RPC_DONE = 6551103;      // server to the owning client

    // G9 caps, per language. Characters, not bytes.
    static const int MAX_SECTIONS = 16;
    static const int MAX_BODY_CHARS = 4000;
    static const int MAX_LINKS_PER_SECTION = 8;
    static const int MAX_MEDIA_PER_SECTION = 4;
    static const int MAX_TABLES_PER_SECTION = 2;
    static const int MAX_TOTAL_CHARS = 40000;
    static const int MAX_TITLE_CHARS = 60;
    static const int MAX_TAGLINE_CHARS = 80;
    static const int MAX_HEADING_CHARS = 40;
    static const int MAX_ID_CHARS = 32;

    // NIMBY-T-055 (G-P1-34, G-P1-35): JsonSerializer.ReadFromString keeps at most this many bytes of
    // any one string value and drops the rest without an error (6,000 ASCII characters came back as
    // 1,023; 1,840 bytes of Cyrillic as 1,022, the cut letter dropped whole). A section body is
    // therefore an array of lines, each one string. A line this close to the limit may have been cut.
    static const int ENGINE_STRING_BYTES = 1023;
    static const int ENGINE_STRING_WARN_BYTES = 1021;

    // G5: bytes per chunk (NIMBY-T-037, measured cap in (1,000, 1,100]).
    static const int CHUNK_BYTES = 1000;
    // G5: the reassembled Language JSON may not exceed this many bytes: MAX_TOTAL_CHARS at four
    // bytes a character plus the keys, quotes and escapes of 16 sections. Anything larger is a
    // forged or broken response and is dropped.
    static const int MAX_PAYLOAD_BYTES = 164000;

    // G5: client and server timing. The server values are defaults for greet.json fields.
    static const int FETCH_DELAY_MS = 500;
    static const float DEFAULT_OPEN_DELAY_SECONDS = 1.0;      // Jeff, G-P0-49
    static const float DEFAULT_OPEN_RETRY_SECONDS = 30.0;
    static const float DEFAULT_COOLDOWN_SECONDS = 5.0;
    static const float DEFAULT_REREAD_SECONDS = 10.0;
    static const int FETCH_TIMEOUT_MS = 20000;   // a request with no complete payload by then is given up with one log line (G-21)
    static const int OPEN_RETRY_MS = 250;   // Phase 1 run 1: 1,000 ms made the open feel late behind the respawn countdown
    static const float MIN_OPEN_DELAY_SECONDS = 0.0;
    static const float MAX_OPEN_DELAY_SECONDS = 30.0;
    static const float MAX_OPEN_RETRY_SECONDS = 300.0;

    // G5, G6: UIScriptedMenu id; vanilla's highest is MENU_UNDERGROUND_TRIGGER_EDITOR = 48 (constants.c:223).
    static const int MENU_ID = 65511;

    // G15: frames per clip (ImageWidget.LoadImageFile slots 0 to 7, EnWidgets.c:257).
    static const int MAX_FRAMES = 8;
    static const int MIN_FRAME_MS = 50;

    // G16: text scale steps.
    static const float TEXT_SCALE_NORMAL = 1.0;
    static const float TEXT_SCALE_LARGE = 1.25;
    static const float TEXT_SCALE_LARGER = 1.5;

    // G3, G4, G17: file names under $profile:NimbyGreet\.
    static const string DIR = "$profile:NimbyGreet";
    static const string FILE_TEXT = "$profile:NimbyGreet\\greet.json";
    static const string FILE_TEXT_LKG = "$profile:NimbyGreet\\greet.json.lkg";
    static const string FILE_TEXT_TMP = "$profile:NimbyGreet\\greet.json.tmp";
    static const string FILE_SEEN = "$profile:NimbyGreet\\seen.json";
    static const string FILE_SEEN_TMP = "$profile:NimbyGreet\\seen.json.tmp";
    static const string FILE_PREFS = "$profile:NimbyGreet\\prefs.json";
    static const string FILE_PREFS_TMP = "$profile:NimbyGreet\\prefs.json.tmp";
    static const string FILE_DEBUG = "$profile:NimbyGreet\\debug";
    static const string FILE_SELFTEST = "$profile:NimbyGreet\\selftest";
    static const string FILE_DROPCHUNK = "$profile:NimbyGreet\\dropchunk";
    static const string MEDIA_PREFIX = "NimbyGreet/Media/";
    static const string MANIFEST_PATH = "NimbyGreet/Media/manifest.json";

    // G14: the 14 vanilla stringtable columns (languagecore/stringtable.csv:1), the only language keys.
    static ref array<string> LANGUAGES = {"original", "english", "czech", "german", "russian", "polish", "hungarian", "italian", "spanish", "french", "chinese", "japanese", "portuguese", "chinesesimp"};
    // G14: columns whose glyphs sdf_MetronLight24 lacks (G-P0-27); refused until a font is proven.
    static ref array<string> CJK_LANGUAGES = {"chinese", "japanese", "chinesesimp"};

    // ---- text caps ---------------------------------------------------------------------------

    // Cuts text to at most cap characters (UTF-8 characters, not bytes). Returns the input when it fits.
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

    // ---- chunking (G5) -----------------------------------------------------------------------

    // How many chunks of CHUNK_BYTES a serialized payload of the given byte length needs. 0 for 0.
    static int ChunkCount(int bytes)
    {
        if (bytes <= 0)
        {
            return 0;
        }
        return (bytes + CHUNK_BYTES - 1) / CHUNK_BYTES;
    }

    // The most chunks any legitimate response can carry.
    static int MaxChunkCount()
    {
        return ChunkCount(MAX_PAYLOAD_BYTES);
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

    // unit repeated until the text holds bytes bytes, then cut on a character boundary at or below bytes.
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

    // ---- dismissal (G4) ----------------------------------------------------------------------

    // Whether the window opens by itself for a client in this state. The server applies it to decide
    // between chunks and a bare done; the client applies the same rule to the done record.
    static bool ShouldAutoOpen(bool neverShow, int dismissedVersion, bool forceShow, int version)
    {
        if (!neverShow)
        {
            return true;
        }
        if (!forceShow)
        {
            return false;
        }
        return version > dismissedVersion;
    }

    // ---- open wait (G5) ----------------------------------------------------------------------

    // Whether one deferred open check counts toward openRetrySeconds. Time behind vanilla's pause
    // menu does not (Jeff, October 26, 2026, Phase 1 run 5): a client that loads with the game in the
    // background gets the pause menu at spawn (missionGameplay.c:296-310, IsAppActive), and the window
    // waits for the player to come back however long that takes. Dead or unconscious always counts.
    static bool OpenWaitCounts(bool paused, bool alive, bool unconscious)
    {
        if (!alive || unconscious)
        {
            return true;
        }
        return !paused;
    }

    // ---- languages (G14) ---------------------------------------------------------------------

    static bool IsKnownLanguage(string name)
    {
        return LANGUAGES.Find(name) >= 0;
    }

    static bool IsCjkLanguage(string name)
    {
        return CJK_LANGUAGES.Find(name) >= 0;
    }

    // The language to serve: the requested one when the file has it, else the default.
    static string PickLanguage(string requested, array<string> available, string defaultLanguage)
    {
        if (available.Find(requested) >= 0)
        {
            return requested;
        }
        return defaultLanguage;
    }

    // ---- links (G10) -------------------------------------------------------------------------

    // Only https:// links, with a host, and nothing that could break the row or the markup.
    static bool IsAcceptedUrl(string url)
    {
        if (url.IndexOf("https://") != 0)
        {
            return false;
        }
        if (url.Length() <= 8)
        {
            return false;
        }
        if (url.Length() > 2048)
        {
            return false;
        }
        if (ContainsAny(url, " \t\r\n<>\"'"))
        {
            return false;
        }
        return true;
    }

    // ---- ids (data format) -------------------------------------------------------------------

    // [a-z0-9-], 1 to MAX_ID_CHARS, not starting or ending with -.
    static bool IsValidId(string id)
    {
        int length = id.Length();
        if (length < 1)
        {
            return false;
        }
        if (length > MAX_ID_CHARS)
        {
            return false;
        }
        int i = 0;
        while (i < length)
        {
            int code = id.Get(i).ToAscii();
            bool lower = code >= 97 && code <= 122;
            bool digit = code >= 48 && code <= 57;
            bool dash = code == 45;
            if (!lower && !digit && !dash)
            {
                return false;
            }
            if (dash && (i == 0 || i == length - 1))
            {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    // A clip id may not end in -<digit>, so <id>-0.edds of one clip can never be another still's name.
    static bool IsValidMediaId(string id)
    {
        if (!IsValidId(id))
        {
            return false;
        }
        int length = id.Length();
        if (length < 3)
        {
            return true;
        }
        if (id.Get(length - 2) != "-")
        {
            return true;
        }
        int code = id.Get(length - 1).ToAscii();
        if (code >= 48 && code <= 57)
        {
            return false;
        }
        return true;
    }

    // ---- small string helpers shared by the converter and the server -------------------------

    static bool ContainsAny(string text, string chars)
    {
        int i = 0;
        int length = chars.Length();
        while (i < length)
        {
            if (text.IndexOf(chars.Get(i)) >= 0)
            {
                return true;
            }
            i = i + 1;
        }
        return false;
    }

    // Splits on "\n" and keeps empty lines (string.Split drops them, EnString.c:396). A trailing "\r" is removed.
    static void SplitLines(string text, out array<string> lines)
    {
        lines = new array<string>;
        int length = text.Length();
        int start = 0;
        while (start <= length)
        {
            int next = text.IndexOfFrom(start, "\n");
            if (next < 0)
            {
                next = length;
            }
            string line = text.Substring(start, next - start);
            int lineLength = line.Length();
            if (lineLength > 0)
            {
                if (line.Get(lineLength - 1) == "\r")
                {
                    line = line.Substring(0, lineLength - 1);
                }
            }
            lines.Insert(line);
            start = next + 1;
        }
    }

    static bool StartsWith(string text, string prefix)
    {
        if (prefix.Length() > text.Length())
        {
            return false;
        }
        return text.IndexOf(prefix) == 0;
    }

    static float ClampFloat(float value, float low, float high)
    {
        if (value < low)
        {
            return low;
        }
        if (value > high)
        {
            return high;
        }
        return value;
    }

    static int ClampInt(int value, int low, int high)
    {
        if (value < low)
        {
            return low;
        }
        if (value > high)
        {
            return high;
        }
        return value;
    }
}
