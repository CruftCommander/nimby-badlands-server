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

    // "player=<name> (<bi-id>, <steam64>)" (shared rules 4.2: both ids on every line that names a player).
    static string Who(PlayerIdentity identity)
    {
        if (!identity)
        {
            return "player=<no identity>";
        }
        return "player=" + identity.GetName() + " (" + identity.GetId() + ", " + identity.GetPlainId() + ")";
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
