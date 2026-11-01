// -----------------------------------------------------------------------------
// NimbyAdmin - self-test hook (shared rules 8).
// Purpose : Runs NimbyAdminSelfTest at mission start when $profile:NimbyAdmin\selftest
//           exists, on both the server (so the Build-NimbyPack boot check sees PASS|FAIL
//           lines) and the client. Same shape as the Greet and NimbyShared hooks.
// Note    : Prints only when the flag file exists. The completion line's cases=N is what
//           the boot check's self-test gate counts.
// -----------------------------------------------------------------------------

modded class MissionBase extends MissionBaseWorld
{
    override void OnInit()
    {
        super.OnInit();
        NimbyAdminRunSelfTestIfFlagged();
    }

    protected void NimbyAdminRunSelfTestIfFlagged()
    {
        if (!FileExist(NimbyAdminRules.FILE_SELFTEST))
        {
            return;
        }
        array<string> results;
        int failures = NimbyAdminSelfTest.Run(results);
        foreach (string result : results)
        {
            Print("[NIMBY][Admin][SelfTest] " + result);
        }
        Print("[NIMBY][Admin] selftest complete; cases=" + results.Count().ToString() + " failures=" + failures.ToString());
    }
}
