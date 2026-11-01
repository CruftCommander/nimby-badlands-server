// -----------------------------------------------------------------------------
// NimbyShared - logging (shared rules 4.3).
// Purpose : [NIMBY][Shared] lines. Info is unconditional; Debug prints only when
//           $profile:NimbyShared\debug exists (read once, at first use). The shape of the Greet
//           log helper, without the player-identity helpers: no NimbyShared line names a player.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbySharedLog
{
    protected static bool s_DebugRead;
    protected static bool s_Debug;

    static void Info(string message)
    {
        Print("[NIMBY][Shared] " + message);
    }

    static void Debug(string message)
    {
        if (!IsDebug())
        {
            return;
        }
        Print("[NIMBY][Shared][Debug] " + message);
    }

    static bool IsDebug()
    {
        if (!s_DebugRead)
        {
            s_DebugRead = true;
            s_Debug = FileExist(NimbySharedRules.FILE_DEBUG);
        }
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
}
