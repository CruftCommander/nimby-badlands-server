// -----------------------------------------------------------------------------
// NimbyTapes - mission-start hook (shared rules 8).
// Purpose : Runs NimbyTapesSelfTest when $profile:NimbyTapes\selftest exists, so the
//           Build-NimbyPack boot check sees the PASS|FAIL lines. Same shape as the Tack
//           hook (NimbyTack/Scripts/4_World/nimbytack_selftest.c).
// Note    : Prints only when the flag file exists. The completion line's cases=N is what
//           the boot check's self-test gate counts.
// Cases   : The 18 named Rules cases of Plan 13 (test row T0-02).
// Next    : The debug and spawn-testkit flag reads land here with the deck (Plan 13,
//           Architecture, Mission hook; T-11).
// -----------------------------------------------------------------------------
modded class MissionBaseWorld
{
    override void OnInit()
    {
        super.OnInit();
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
