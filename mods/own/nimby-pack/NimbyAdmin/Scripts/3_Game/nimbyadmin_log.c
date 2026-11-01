// -----------------------------------------------------------------------------
// NimbyAdmin - logging (shared rules 4.2, 4.3), used by both halves (one log prefix, N1).
// Purpose : [NIMBY][Admin] lines. Info is unconditional; Debug prints only when
//           $profile:NimbyAdmin\debug exists (read once, at first use). Who() puts both ids first
//           and the escaped, quoted name last (the Greet GREET-R2-01 format,
//           nimbygreet_log.c:42-58). Gated() writes a line a client can cause at most once per
//           LOG_GATE_SECONDS per key and counts what it held back (GREET-R2-02).
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
        return "player=(" + biId + ", " + steam64 + ") \"" + Escape(name) + "\"";
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
