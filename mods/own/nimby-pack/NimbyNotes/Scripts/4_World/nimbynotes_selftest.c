// -----------------------------------------------------------------------------
// NimbyNotes - self-test hook (shared rules 8).
// Purpose : Runs NimbyNotesRules.SelfTest at mission start when
//           $profile:NimbyNotes\selftest exists, on server and client, so the
//           Build-NimbyPack boot check sees the PASS|FAIL lines.
// -----------------------------------------------------------------------------
modded class MissionBaseWorld
{
    override void OnInit()
    {
        super.OnInit();
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
