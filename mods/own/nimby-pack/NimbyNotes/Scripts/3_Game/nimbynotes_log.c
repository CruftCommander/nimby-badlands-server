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
}
