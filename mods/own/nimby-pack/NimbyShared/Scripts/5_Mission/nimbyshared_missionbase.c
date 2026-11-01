// -----------------------------------------------------------------------------
// NimbyShared - self-test hook (shared rules 8).
// Purpose : Runs NimbySharedSelfTest at mission start when $profile:NimbyShared\selftest
//           exists, on both the server (so the Build-NimbyPack boot check sees PASS|FAIL
//           lines) and the client. Same shape as the Greet hook.
// Note    : Prints only when the flag file exists. The completion line's cases=N is what
//           the boot check's self-test gate counts.
// -----------------------------------------------------------------------------

modded class MissionBase extends MissionBaseWorld
{
    override void OnInit()
    {
        super.OnInit();
        NimbySharedRunSelfTestIfFlagged();
    }

    protected void NimbySharedRunSelfTestIfFlagged()
    {
        if (!FileExist(NimbySharedRules.FILE_SELFTEST))
        {
            return;
        }
        array<string> results;
        int failures = NimbySharedSelfTest.Run(results);
        foreach (string result : results)
        {
            Print("[NIMBY][Shared][SelfTest] " + result);
        }
        Print("[NIMBY][Shared] selftest complete; cases=" + results.Count().ToString() + " failures=" + failures.ToString());
    }
}
