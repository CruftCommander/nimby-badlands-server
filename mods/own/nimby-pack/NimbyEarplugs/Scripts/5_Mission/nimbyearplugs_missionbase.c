// -----------------------------------------------------------------------------
// NimbyEarplugs - self-test hook (shared rules 8).
// Purpose : Runs the engine-free validators in NimbyEarplugsLevel at mission start
//           when $profile:NimbyEarplugs\selftest exists, on both the server (so the
//           Build-NimbyPack boot check sees PASS|FAIL lines) and the client.
// Note    : This is the only Ears code that executes on a server, and it only prints
//           when the flag file exists. Ears has no server feature (plan, "What this is not").
// -----------------------------------------------------------------------------

modded class MissionBase extends MissionBaseWorld
{
    override void OnInit()
    {
        super.OnInit();
        NimbyEarplugsRunSelfTestIfFlagged();
    }

    protected void NimbyEarplugsRunSelfTestIfFlagged()
    {
        if (!FileExist("$profile:NimbyEarplugs\\selftest"))
        {
            return;
        }
        array<string> results = new array<string>;
        int failures = NimbyEarplugsLevel.SelfTest(results);
        foreach (string result : results)
        {
            Print("[NIMBY][Earplugs][SelfTest] " + result);
        }
        Print("[NIMBY][Earplugs] selftest complete; cases=" + results.Count().ToString() + " failures=" + failures.ToString());
    }
}
