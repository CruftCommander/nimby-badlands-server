// -----------------------------------------------------------------------------
// NimbyNotes - mission-start hook (shared rules 4.3 and 8).
// Purpose : Reads the debug flag, writes the load line, and runs
//           NimbyNotesRules.SelfTest at mission start when
//           $profile:NimbyNotes\selftest exists, on server and client, so the
//           Build-NimbyPack boot check sees the PASS|FAIL lines.
// -----------------------------------------------------------------------------
modded class MissionBaseWorld
{
    override void OnInit()
    {
        super.OnInit();
        NimbyNotesLog.ReadDebugFlag();
        NimbyNotesLog.Info("loaded; server=" + NimbyNotesLog.Bool(GetGame().IsServer()) + " debug=" + NimbyNotesLog.Bool(NimbyNotesLog.IsDebug()));
        if (!FileExist("$profile:NimbyNotes\\selftest"))
        {
            return;
        }
        array<string> results = new array<string>;
        int failures = NimbyNotesRules.SelfTest(results);
        foreach (string result : results)
        {
            Print("[NIMBY][Notes][SelfTest] " + result);
        }
        Print("[NIMBY][Notes] selftest complete; failures=" + failures.ToString());
    }
}
