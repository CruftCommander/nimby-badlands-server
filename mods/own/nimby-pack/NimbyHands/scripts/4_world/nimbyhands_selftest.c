// -----------------------------------------------------------------------------
// NimbyHands - mission-start hook (shared rules 4.3 and 8).
// Purpose : Reads the debug flag and runs NimbyHandsRules.SelfTest when
//           $profile:NimbyHands\selftest exists, on server and client, so the
//           Build-NimbyPack boot check sees the PASS|FAIL lines.
// -----------------------------------------------------------------------------
modded class MissionBaseWorld
{
    override void OnInit()
    {
        super.OnInit();
        NimbyHandsLog.ReadDebugFlag();
        NimbyHandsLog.Info("loaded; server=" + NimbyHandsLog.Bool(GetGame().IsServer()) + " debug=" + NimbyHandsLog.Bool(NimbyHandsLog.IsDebug()));
        if (!FileExist("$profile:NimbyHands\\selftest"))
        {
            return;
        }
        array<string> results = new array<string>;
        int failures = NimbyHandsRules.SelfTest(results);
        foreach (string result : results)
        {
            Print("[NIMBY][Hands][SelfTest] " + result);
        }
        Print("[NIMBY][Hands] selftest complete; failures=" + failures.ToString());
    }
}
