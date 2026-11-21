// -----------------------------------------------------------------------------
// NimbyTapes - log lines and the debug flag (shared rules 4.3).
// Purpose : One place that prints "[NIMBY][Tapes] ..." (production: refusals and failures) and
//           "[NIMBY][Tapes][Debug] ..." (only while $profile:NimbyTapes\debug exists).
// Flag    : Read at mission start (the MissionBaseWorld hook calls ReadFlags) and, because the
//           storage load creates decks before the mission's OnInit runs, on the first question
//           before that. Test row T0-20 needs the load-time lines.
// Probe   : A line "offset <value>" in the debug file makes every client start pass <value>
//           to SetStartOffset instead of the computed share. It exists for the unit probe of
//           the recon (Question 3: "offset 0.5", then "offset 120") and for nothing else.
// API     : FileExist (1_Core/DayZ/proto/EnSystem.c:397), OpenFile (:417), CloseFile (:443),
//           FGets (:501); string.ToFloat, Substring, Length, IndexOf, TrimInPlace
//           (1_Core/DayZ/proto/EnString.c:46, :113, :199, :240, :319).
//           Lines are relative to vendor/dayz-1.30-scripts/scripts/ (build 1.30.164289).
// Style   : one statement per line; no variable name is declared twice in one function.
// -----------------------------------------------------------------------------
class NimbyTapesLog
{
    protected static bool s_Read;
    protected static bool s_Debug;
    protected static float s_ProbeOffset;

    static void ReadFlags()
    {
        s_Read = true;
        s_ProbeOffset = -1;
        s_Debug = FileExist(NimbyTapesRules.FILE_DEBUG);
        if (!s_Debug)
        {
            return;
        }
        FileHandle handle = OpenFile(NimbyTapesRules.FILE_DEBUG, FileMode.READ);
        if (handle == 0)
        {
            return;
        }
        string line;
        string value;
        while (FGets(handle, line) >= 0)
        {
            line.TrimInPlace();
            if (line.IndexOf("offset ") == 0)
            {
                value = line.Substring(7, line.Length() - 7);
                value.TrimInPlace();
                s_ProbeOffset = value.ToFloat();
            }
        }
        CloseFile(handle);
    }

    static bool IsDebug()
    {
        if (!s_Read)
        {
            ReadFlags();
        }
        return s_Debug;
    }

    // The start offset forced by the debug file, or a negative value when there is none.
    static float ProbeOffset()
    {
        if (!s_Read)
        {
            ReadFlags();
        }
        return s_ProbeOffset;
    }

    static void Info(string message)
    {
        Print("[NIMBY][Tapes] " + message);
    }

    static void Trace(string message)
    {
        if (!IsDebug())
        {
            return;
        }
        Print("[NIMBY][Tapes][Debug] " + message);
    }

    static string Flag(bool value)
    {
        if (value)
        {
            return "1";
        }
        return "0";
    }
}
