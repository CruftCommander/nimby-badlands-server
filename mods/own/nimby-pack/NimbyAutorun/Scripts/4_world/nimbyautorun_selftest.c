// -----------------------------------------------------------------------------
// NimbyAutorun - Legs. Mission-start hook (shared rules 4.3 and 8).
// Purpose : Reads the debug flag, loads the server config, and runs
//           NimbyAutorunRules.SelfTest when $profile:NimbyAutorun\selftest exists,
//           on server and client, so the Build-NimbyPack boot check sees the
//           PASS|FAIL lines.
// -----------------------------------------------------------------------------
modded class MissionBaseWorld
{
    override void OnInit()
    {
        super.OnInit();
        NimbyAutorunLog.ReadDebugFlag();
        if (GetGame().IsServer())
        {
            NimbyAutorunConfigLoader.Load();
        }
        NimbyAutorunLog.Info("loaded; server=" + NimbyAutorunLog.Bool(GetGame().IsServer()) + " debug=" + NimbyAutorunLog.Bool(NimbyAutorunLog.IsDebug()));
        if (!FileExist("$profile:NimbyAutorun\\selftest"))
        {
            return;
        }
        array<string> results = new array<string>;
        int failures = NimbyAutorunRules.SelfTest(results);
        foreach (string result : results)
        {
            Print("[NIMBY][Autorun][SelfTest] " + result);
        }
        Print("[NIMBY][Autorun] selftest complete; cases=" + results.Count().ToString() + " failures=" + failures.ToString());
    }
}
