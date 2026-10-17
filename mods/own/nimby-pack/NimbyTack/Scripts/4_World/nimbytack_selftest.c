// -----------------------------------------------------------------------------
// NimbyTack - mission-start hook (shared rules 4.3 and 8).
// Purpose : Reads the debug flag, loads the server config (T6), and runs
//           NimbyTackRules.SelfTest when $profile:NimbyTack\selftest exists, on
//           server and client, so the Build-NimbyPack boot check sees the
//           PASS|FAIL lines.
// -----------------------------------------------------------------------------
modded class MissionBaseWorld
{
    override void OnInit()
    {
        super.OnInit();
        NimbyTackLog.ReadDebugFlag();
        if (GetGame().IsServer())
        {
            NimbyTackConfigLoader.Load();
        }
        NimbyTackLog.Info("loaded; server=" + NimbyTackLog.Bool(GetGame().IsServer()) + " debug=" + NimbyTackLog.Bool(NimbyTackLog.IsDebug()));
        if (!FileExist("$profile:NimbyTack\\selftest"))
        {
            return;
        }
        array<string> results = new array<string>;
        int failures = NimbyTackRules.SelfTest(results);
        foreach (string result : results)
        {
            Print("[NIMBY][Tack][SelfTest] " + result);
        }
        Print("[NIMBY][Tack] selftest complete; failures=" + failures.ToString());
    }
}
