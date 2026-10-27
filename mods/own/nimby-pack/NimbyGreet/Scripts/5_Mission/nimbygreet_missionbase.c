// -----------------------------------------------------------------------------
// NimbyGreet - self-test hook (shared rules 8).
// Purpose : Runs NimbyGreetSelfTest at mission start when $profile:NimbyGreet\selftest
//           exists, on both the server (so the Build-NimbyPack boot check sees PASS|FAIL
//           lines) and the client.
// Note    : Prints only when the flag file exists. The completion line's cases=N is
//           what the boot check's self-test gate counts.
// -----------------------------------------------------------------------------

modded class MissionBase extends MissionBaseWorld
{
    override void OnInit()
    {
        super.OnInit();
        NimbyGreetRunSelfTestIfFlagged();
    }

    protected void NimbyGreetRunSelfTestIfFlagged()
    {
        if (!FileExist(NimbyGreetRules.FILE_SELFTEST))
        {
            return;
        }
        array<string> results;
        int failures = NimbyGreetSelfTest.Run(results);
        foreach (string result : results)
        {
            Print("[NIMBY][Greet][SelfTest] " + result);
        }
        Print("[NIMBY][Greet] selftest complete; cases=" + results.Count().ToString() + " failures=" + failures.ToString());
    }
}
