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
        // Phase 1 A4: the three identity strings are locals, not calls inside WhoFrom's
        // arguments (the 164200 argument-slot rule, 23b4e46).
        string name = identity.GetName();
        string biId = identity.GetId();
        string steam64 = identity.GetPlainId();
        return WhoFrom(name, biId, steam64);
    }

    static string WhoFrom(string name, string biId, string steam64)
    {
        string safeBiId = Safe(biId);
        string safeSteam64 = Safe(steam64);
        string safeName = Safe(name);
        string escapedName = Escape(safeName);
        return "player=(" + safeBiId + ", " + safeSteam64 + ") \"" + escapedName + "\"";
    }

    // Review finding 7 (October 31, 2026): at most SAFE_BYTES bytes (on a UTF-8 boundary), and
    // nothing that can break or disguise a log line. Security reviews of f8d99c0 and 4935842: the
    // text is decoded as UTF-8 and allowed by code point (NextUnit). Every malformed byte (a stray
    // continuation, 0xC0, 0xC1, 0xF5 to 0xFF, an overlong form, a surrogate, a sequence cut short)
    // becomes '?', and so does every unsafe code point (IsUnsafeCodePoint: all of Cc, which holds
    // every byte below 0x20, 0x7F and NEL, and the format characters that hide or reorder text).
    // Every other character is kept whole, so a Cyrillic name still reads. Engine-free;
    // self-tested.
    static const int SAFE_BYTES = 64;

    static string Safe(string text)
    {
        string cut = NimbyAdminRules.CutBytes(text, SAFE_BYTES);
        array<int> bytes = new array<int>;
        int b = 0;
        while (b < cut.Length())
        {
            int value = ByteAt(cut, b);
            bytes.Insert(value);
            b = b + 1;
        }
        string result = "";
        int i = 0;
        while (i < bytes.Count())
        {
            bool keep;
            int unit = NextUnit(bytes, i, keep);
            if (keep)
            {
                result = result + cut.Substring(i, unit);
            }
            else
            {
                result = result + "?";
            }
            i = i + unit;
        }
        return result;
    }

    // The unit at index: its length in bytes (1 to 4) and whether it is kept. A malformed unit is
    // one byte and never kept.
    static int NextUnit(array<int> bytes, int index, out bool keep)
    {
        keep = false;
        int b0 = bytes.Get(index);
        if (b0 < 0x80)
        {
            keep = !IsUnsafeCodePoint(b0);
            return 1;
        }
        int need = 0;
        int codePoint = 0;
        int lowest = 0;
        if (b0 >= 0xC2 && b0 <= 0xDF)
        {
            need = 1;
            codePoint = b0 & 0x1F;
            lowest = 0x80;
        }
        else if (b0 >= 0xE0 && b0 <= 0xEF)
        {
            need = 2;
            codePoint = b0 & 0x0F;
            lowest = 0x800;
        }
        else if (b0 >= 0xF0 && b0 <= 0xF4)
        {
            need = 3;
            codePoint = b0 & 0x07;
            lowest = 0x10000;
        }
        else
        {
            return 1;
        }
        if (index + need >= bytes.Count())
        {
            return 1;
        }
        int k = 1;
        while (k <= need)
        {
            int next = bytes.Get(index + k);
            if (next < 0x80 || next > 0xBF)
            {
                return 1;
            }
            codePoint = (codePoint << 6) | (next & 0x3F);
            k = k + 1;
        }
        if (codePoint < lowest || codePoint > 0x10FFFF)
        {
            return 1;
        }
        if (codePoint >= 0xD800 && codePoint <= 0xDFFF)
        {
            return 1;
        }
        keep = !IsUnsafeCodePoint(codePoint);
        return need + 1;
    }

    // Cc (U+0000 to U+001F, U+007F to U+009F) and the format characters that hide, join or
    // reorder text: U+061C, U+180E, U+200B to U+200F, U+2028 to U+202E, U+2060 to U+206F,
    // U+FEFF, U+FFF9 to U+FFFB.
    static bool IsUnsafeCodePoint(int codePoint)
    {
        if (IsUnsafeByte(codePoint))
        {
            return true;
        }
        if (codePoint >= 0x80 && codePoint <= 0x9F)
        {
            return true;
        }
        if (codePoint == 0x061C || codePoint == 0x180E || codePoint == 0xFEFF)
        {
            return true;
        }
        if (codePoint >= 0x200B && codePoint <= 0x200F)
        {
            return true;
        }
        if (codePoint >= 0x2028 && codePoint <= 0x202E)
        {
            return true;
        }
        if (codePoint >= 0x2060 && codePoint <= 0x206F)
        {
            return true;
        }
        return codePoint >= 0xFFF9 && codePoint <= 0xFFFB;
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
