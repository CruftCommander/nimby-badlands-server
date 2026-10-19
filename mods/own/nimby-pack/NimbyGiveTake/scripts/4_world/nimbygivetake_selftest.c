// -----------------------------------------------------------------------------
// NimbyGiveTake - mission-start hook (shared rules 4.3 and 8).
// Purpose : Reads the debug flag and runs NimbyGiveTakeRules.SelfTest when
//           $profile:NimbyGiveTake\selftest exists, on server and client, so the
//           Build-NimbyPack boot check sees the PASS|FAIL lines.
// -----------------------------------------------------------------------------
modded class MissionBaseWorld
{
    override void OnInit()
    {
        super.OnInit();
        NimbyGiveTakeLog.ReadDebugFlag();
        NimbyGiveTakeLog.Info("loaded; server=" + NimbyGiveTakeLog.Bool(GetGame().IsServer()) + " debug=" + NimbyGiveTakeLog.Bool(NimbyGiveTakeLog.IsDebug()));
        if (!FileExist("$profile:NimbyGiveTake\\selftest"))
        {
            return;
        }
        array<string> results = new array<string>;
        int failures = NimbyGiveTakeRules.SelfTest(results);
        foreach (string result : results)
        {
            Print("[NIMBY][GiveTake][SelfTest] " + result);
        }
        Print("[NIMBY][GiveTake] selftest complete; failures=" + failures.ToString());
    }
}
