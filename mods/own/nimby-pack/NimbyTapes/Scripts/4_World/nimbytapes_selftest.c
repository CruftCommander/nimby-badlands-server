// -----------------------------------------------------------------------------
// NimbyTapes - mission-start hook (shared rules 4.3 and 8).
// Purpose : Reads the debug flag, then runs NimbyTapesSelfTest when
//           $profile:NimbyTapes\selftest exists, so the Build-NimbyPack boot check sees the
//           PASS|FAIL lines. Same shape as the Tack hook
//           (NimbyTack/Scripts/4_World/nimbytack_selftest.c). Runs on the client and on the
//           server.
// Note    : Prints only when a flag file exists. The completion line's cases=N is what the
//           boot check's self-test gate counts.
// Cases   : The 18 named Rules cases of Plan 13 (test row T0-02).
// Next    : The spawn-testkit flag read lands with the test kit (Plan 13, T-11).
// -----------------------------------------------------------------------------
modded class MissionBaseWorld
{
    override void OnInit()
    {
        super.OnInit();
        NimbyTapesLog.ReadFlags();
        if (NimbyTapesLog.IsDebug())
        {
            NimbyTapesLog.Info("debug lines on; probe offset=" + NimbyTapesLog.ProbeOffset().ToString());
        }
        if (!FileExist(NimbyTapesRules.FILE_SELFTEST))
        {
            return;
        }
        array<string> results;
        int failures = NimbyTapesSelfTest.Run(results);
        foreach (string result : results)
        {
            Print("[NIMBY][Tapes][SelfTest] " + result);
        }
        Print("[NIMBY][Tapes] selftest complete; cases=" + results.Count().ToString() + " failures=" + failures.ToString());
    }
}
