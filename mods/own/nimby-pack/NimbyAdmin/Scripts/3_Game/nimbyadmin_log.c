// -----------------------------------------------------------------------------
// NimbyAdmin - logging (shared rules 4.2, 4.3), used by both halves (one log prefix, N1).
// Purpose : [NIMBY][Admin] lines. Info is unconditional; Debug prints only when
//           $profile:NimbyAdmin\debug exists (read once, at first use). Who() puts both ids first
//           and the escaped, quoted name last (the Greet GREET-R2-01 format,
//           nimbygreet_log.c:42-58). Gated() writes a line a client can cause at most once per
//           LOG_GATE_SECONDS per key and counts what it held back (GREET-R2-02). Safe() makes
//           any client-supplied text fit for an ordinary log line (review finding 7): WhoFrom
//           applies it to the name and both ids, so every path that names a player is covered.
// Never   : a webhook URL, a password, or any value of config.json other than its bounds.
// Engine  : PlayerIdentity.GetName, GetId, GetPlainId (3_Game/DayZ/gameplay.c:362,368,370);
//           CGame.GetTickTime (3_Game/DayZ/Global/Game.c:914).
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyAdminLog
{
    protected static bool s_DebugRead;
    protected static bool s_Debug;
    protected static ref map<string, float> s_GateLast;
    protected static ref map<string, int> s_GateHeld;

    static void Info(string message)
    {
        Print("[NIMBY][Admin] " + message);
    }

    static void Debug(string message)
    {
        if (!IsDebug())
        {
            return;
        }
        Print("[NIMBY][Admin][Debug] " + message);
    }

    static bool IsDebug()
    {
        if (!s_DebugRead)
        {
            s_DebugRead = true;
            s_Debug = FileExist(NimbyAdminRules.FILE_DEBUG);
        }
        return s_Debug;
    }

    static string Who(PlayerIdentity identity)
    {
        if (!identity)
        {
            return "player=<no identity>";
        }
        return WhoFrom(identity.GetName(), identity.GetId(), identity.GetPlainId());
    }

    static string WhoFrom(string name, string biId, string steam64)
    {
        return "player=(" + Safe(biId) + ", " + Safe(steam64) + ") \"" + Escape(Safe(name)) + "\"";
    }

    // Review finding 7 (October 31, 2026): at most SAFE_BYTES bytes (on a UTF-8 boundary), every
    // byte below 0x20 and 0x7F replaced with '?', so a client cannot write a line break or a
    // control sequence into the server log. Also replaced, each sequence by one '?' (security
    // review of f8d99c0): the UTF-8 C1 controls U+0080 to U+009F (NEL among them), the line and
    // paragraph separators U+2028 and U+2029, and the bidirectional marks, embeddings, overrides
    // and isolates U+200E, U+200F, U+202A to U+202E, U+2066 to U+2069. Engine-free; self-tested.
    static const int SAFE_BYTES = 64;

    static string Safe(string text)
    {
        string cut = NimbyAdminRules.CutBytes(text, SAFE_BYTES);
        string result = "";
        int i = 0;
        int length = cut.Length();
        while (i < length)
        {
            int b0 = ByteAt(cut, i);
            int b1 = -1;
            int b2 = -1;
            if (i + 1 < length)
            {
                b1 = ByteAt(cut, i + 1);
            }
            if (i + 2 < length)
            {
                b2 = ByteAt(cut, i + 2);
            }
            int unsafeLength = UnsafeSequenceLength(b0, b1, b2);
            if (unsafeLength > 0)
            {
                result = result + "?";
                i = i + unsafeLength;
            }
            else
            {
                result = result + cut.Get(i);
                i = i + 1;
            }
        }
        return result;
    }

    // The byte at index as 0 to 255 (ToAscii may report a byte above 0x7F as negative).
    static int ByteAt(string text, int index)
    {
        int c = text.Get(index).ToAscii();
        if (c < 0)
        {
            c = c + 256;
        }
        return c;
    }

    // How many bytes from b0 form one unsafe sequence (1, 2 or 3), or 0. b1 and b2 are -1 past
    // the end of the text.
    static int UnsafeSequenceLength(int b0, int b1, int b2)
    {
        if (IsUnsafeByte(b0))
        {
            return 1;
        }
        if (b0 == 0xC2 && b1 >= 0x80 && b1 <= 0x9F)
        {
            return 2;
        }
        if (b0 != 0xE2)
        {
            return 0;
        }
        if (b1 == 0x80 && (b2 == 0x8E || b2 == 0x8F))
        {
            return 3;
        }
        if (b1 == 0x80 && b2 >= 0xA8 && b2 <= 0xAE)
        {
            return 3;
        }
        if (b1 == 0x81 && b2 >= 0xA6 && b2 <= 0xA9)
        {
            return 3;
        }
        return 0;
    }

    static bool IsUnsafeByte(int c)
    {
        if (c >= 0 && c < 32)
        {
            return true;
        }
        return c == 127;
    }

    // Backslash and double quote escaped, line breaks flattened.
    static string Escape(string text)
    {
        string result = text;
        result.Replace("\\", "\\\\");
        result.Replace("\"", "\\\"");
        result.Replace("\n", " ");
        result.Replace("\r", " ");
        return result;
    }

    static string Key(PlayerIdentity identity)
    {
        if (!identity)
        {
            return "none";
        }
        return identity.GetId();
    }

    // True when a gated line for key may be written at now (engine-free; self-tested).
    static bool GateAllows(float last, float now, float seconds)
    {
        return now - last >= seconds;
    }

    static void Gated(string key, string message)
    {
        if (!s_GateLast)
        {
            s_GateLast = new map<string, float>;
            s_GateHeld = new map<string, int>;
        }
        float now = GetGame().GetTickTime();
        float last = -1000.0;
        if (s_GateLast.Contains(key))
        {
            last = s_GateLast.Get(key);
        }
        if (!GateAllows(last, now, NimbyAdminRules.LOG_GATE_SECONDS))
        {
            int held = 0;
            if (s_GateHeld.Contains(key))
            {
                held = s_GateHeld.Get(key);
            }
            s_GateHeld.Set(key, held + 1);
            return;
        }
        s_GateLast.Set(key, now);
        int suppressed = 0;
        if (s_GateHeld.Contains(key))
        {
            suppressed = s_GateHeld.Get(key);
            s_GateHeld.Set(key, 0);
        }
        if (suppressed > 0)
        {
            message = message + " suppressed=" + suppressed.ToString();
        }
        Info(message);
    }

    static string Bool(bool value)
    {
        if (value)
        {
            return "true";
        }
        return "false";
    }
}
