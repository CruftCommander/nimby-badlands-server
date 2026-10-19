// -----------------------------------------------------------------------------
// NimbyGiveTakeLog - prefix, debug flag, and escaping for every Hands log line.
// Purpose : One place for the [NIMBY][GiveTake] prefix (shared rules 4.1; it becomes
//           [NIMBY][Hands] at the Phase 2 rename), the $profile:NimbyGiveTake\debug
//           flag read at mission start (shared rules 4.3), and the escaping of
//           player-supplied text (plan Phase 1, Logging).
// Vanilla : PlayerIdentity.GetName/GetId/GetPlainId, gameplay.c:362,368,370.
// -----------------------------------------------------------------------------
class NimbyGiveTakeLog
{
    static const string PREFIX = "[NIMBY][GiveTake]";
    protected static bool s_Debug = false;

    static void ReadDebugFlag()
    {
        s_Debug = FileExist("$profile:NimbyGiveTake\\debug");
    }

    static bool IsDebug()
    {
        return s_Debug;
    }

    static string Bool(bool value)
    {
        if (value)
        {
            return "true";
        }
        return "false";
    }

    static void Info(string line)
    {
        Print(PREFIX + " " + line);
    }

    static void Debug(string line)
    {
        if (!s_Debug)
        {
            return;
        }
        Print(PREFIX + "[Debug] " + line);
    }

    // Player-supplied text (a character name) inside double quotes, with the quote, the
    // backslash, and every control character escaped, so a name cannot end the field, forge a
    // second key=value pair, or break the line.
    static string Quote(string text)
    {
        return "\"" + Escape(text) + "\"";
    }

    static string Escape(string text)
    {
        string result = "";
        int count = text.Length();
        for (int i = 0; i < count; i++)
        {
            string ch = text.Get(i);
            int code = ch.ToAscii();
            if (ch == "\\")
            {
                result = result + "\\\\";
            }
            else if (ch == "\"")
            {
                result = result + "\\\"";
            }
            else if (code == 10)
            {
                result = result + "\\n";
            }
            else if (code == 13)
            {
                result = result + "\\r";
            }
            else if (code == 9)
            {
                result = result + "\\t";
            }
            else if (code >= 0 && code < 32)
            {
                result = result + "\\x" + Hex2(code);
            }
            else if (code == 127)
            {
                result = result + "\\x7f";
            }
            else
            {
                result = result + ch;
            }
        }
        return result;
    }

    protected static string Hex2(int value)
    {
        string digits = "0123456789abcdef";
        int high = (value / 16) % 16;
        int low = value % 16;
        return digits.Get(high) + digits.Get(low);
    }

    // "<name>" (<bi-id>, <steam64>) for an identity; the two ids are the ones shared rules 4.2
    // requires on every line that names a player.
    static string DescribeIdentity(PlayerIdentity identity)
    {
        if (!identity)
        {
            return "no-identity";
        }
        string line = Quote(identity.GetName());
        line = line + " (" + identity.GetId() + ", " + identity.GetPlainId() + ")";
        return line;
    }
}
