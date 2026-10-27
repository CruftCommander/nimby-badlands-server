// -----------------------------------------------------------------------------
// NimbyGreet - logging (shared rules 4.2, 4.3).
// Purpose : [NIMBY][Greet] lines. Info is unconditional; Debug prints only when
//           $profile:NimbyGreet\debug exists (read once, at first use). The Phase 0
//           [Probe] level is gone with the probe.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyGreetLog
{
    protected static bool s_DebugRead;
    protected static bool s_Debug;

    static void Info(string message)
    {
        Print("[NIMBY][Greet] " + message);
    }

    static void Debug(string message)
    {
        if (!IsDebug())
        {
            return;
        }
        Print("[NIMBY][Greet][Debug] " + message);
    }

    static bool IsDebug()
    {
        if (!s_DebugRead)
        {
            s_DebugRead = true;
            s_Debug = FileExist(NimbyGreetRules.FILE_DEBUG);
        }
        return s_Debug;
    }

    // "player=(<bi-id>, <steam64>) "<name>"" (shared rules 4.2: both ids on every line that names
    // a player). Ids first and the player-chosen name last, quoted and escaped, since October 27,
    // 2026: the name used to come first and unquoted, so a name holding an id pair forged the
    // line for any first-pair reader, including the first-week watch (review GREET-R2-01).
    static string Who(PlayerIdentity identity)
    {
        if (!identity)
        {
            return "player=<no identity>";
        }
        return WhoFrom(identity.GetName(), identity.GetId(), identity.GetPlainId());
    }

    static string WhoFrom(string name, string biId, string plainId)
    {
        return "player=(" + biId + ", " + plainId + ") \"" + Escape(name) + "\"";
    }

    // Backslash and double quote escaped, line breaks flattened, so the quoted field ends at the
    // first unescaped quote whatever the player typed.
    static string Escape(string text)
    {
        string result = text;
        result.Replace("\\", "\\\\");
        result.Replace("\"", "\\\"");
        result.Replace("\n", " ");
        result.Replace("\r", " ");
        return result;
    }

    // The BI id alone, as a gate key; "none" for no identity.
    static string IdentityKey(PlayerIdentity identity)
    {
        if (!identity)
        {
            return "none";
        }
        return identity.GetId();
    }

    // Shared rules 4.3: an Info line a client can trigger per packet is written at most once
    // per NimbyGreetRules.LOG_GATE_SECONDS per key; suppressed lines are counted and the count
    // rides on the next line written for that key (review GREET-R2-02).
    protected static ref map<string, float> s_GateLast;
    protected static ref map<string, int> s_GateSuppressed;

    static void InfoGated(string key, string message)
    {
        if (!s_GateLast)
        {
            s_GateLast = new map<string, float>();
            s_GateSuppressed = new map<string, int>();
        }
        float now = GetGame().GetTickTime();
        float last = -1000.0;
        if (s_GateLast.Contains(key))
        {
            last = s_GateLast.Get(key);
        }
        if (!NimbyGreetRules.GateAllows(last, now, NimbyGreetRules.LOG_GATE_SECONDS))
        {
            int suppressed = 0;
            if (s_GateSuppressed.Contains(key))
            {
                suppressed = s_GateSuppressed.Get(key);
            }
            s_GateSuppressed.Set(key, suppressed + 1);
            return;
        }
        s_GateLast.Set(key, now);
        int held = 0;
        if (s_GateSuppressed.Contains(key))
        {
            held = s_GateSuppressed.Get(key);
            s_GateSuppressed.Set(key, 0);
        }
        if (held > 0)
        {
            message = message + " suppressed=" + held.ToString();
        }
        Print("[NIMBY][Greet] " + message);
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
