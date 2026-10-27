// -----------------------------------------------------------------------------
// NimbyNotes - logging (shared rules 4.1 and 4.3).
// Purpose : One place for the [NIMBY][Notes] prefix and the debug flag. Info lines
//           are the production record (refusals, failures, the load line); Debug
//           lines are per-use and per-frame detail, written only when
//           $profile:NimbyNotes\debug exists at mission start (on a client,
//           $profile: is %LOCALAPPDATA%\DayZ, the same place as the Ears flag).
// Style   : One statement per line.
// -----------------------------------------------------------------------------
class NimbyNotesLog
{
    static const string PREFIX = "[NIMBY][Notes]";
    protected static bool s_Debug = false;

    static void ReadDebugFlag()
    {
        s_Debug = FileExist("$profile:NimbyNotes\\debug");
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

    // Shared rules 4.2: a line that names a player carries both ids, BI hash and Steam64
    // (PlayerIdentity.GetId and GetPlainId, gameplay.c:368,370).
    static string PlayerTag(PlayerIdentity identity)
    {
        if (!identity)
        {
            return "player=none";
        }
        return "player=" + identity.GetId() + "/" + identity.GetPlainId();
    }

    // The BI id alone, as a map key and as the writer record (N17); "none" for no identity.
    static string IdentityKey(PlayerIdentity identity)
    {
        if (!identity)
        {
            return "none";
        }
        return identity.GetId();
    }

    static void Info(string line)
    {
        Print(PREFIX + " " + line);
    }

    // Shared rules 4.3: an Info line a client can trigger per packet is written at most once
    // per NimbyNotesRules.LOG_GATE_MS per key (the sender's id, or a sheet type for lines with
    // no sender). Suppressed lines are counted and the count rides on the next line written
    // for that key, so a flood is visible as one line with suppressed=N instead of N lines.
    protected static ref map<string, int> s_GateLast;
    protected static ref map<string, int> s_GateSuppressed;

    static void InfoGated(string key, string line)
    {
        if (!s_GateLast)
        {
            s_GateLast = new map<string, int>();
            s_GateSuppressed = new map<string, int>();
        }
        int now = GetGame().GetTime();
        int last = 0;
        if (s_GateLast.Contains(key))
        {
            last = s_GateLast.Get(key);
        }
        if (!NimbyNotesRules.GateAllows(last, now, NimbyNotesRules.LOG_GATE_MS))
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
            line = line + " suppressed=" + held.ToString();
        }
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
}
