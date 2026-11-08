// -----------------------------------------------------------------------------
// NimbyDpv - self-test hook (shared rules 8). Same shape as the Ears hook: runs at
// mission start when $profile:NimbyDpv\selftest exists and prints the lines the
// Build-NimbyPack boot gate counts under the log prefix [NIMBY][Dpv].
// -----------------------------------------------------------------------------
modded class MissionBase extends MissionBaseWorld
{
    override void OnInit()
    {
        super.OnInit();
        NimbyDpvRunSelfTestIfFlagged();
    }

    protected void NimbyDpvRunSelfTestIfFlagged()
    {
        if (!FileExist("$profile:NimbyDpv\\selftest"))
        {
            return;
        }
        array<string> results = new array<string>;
        int failures = NimbyDpvSelfTest.Run(results);
        foreach (string result : results)
        {
            Print("[NIMBY][Dpv][SelfTest] " + result);
        }
        Print("[NIMBY][Dpv] selftest complete; cases=" + results.Count().ToString() + " failures=" + failures.ToString());
    }
}
