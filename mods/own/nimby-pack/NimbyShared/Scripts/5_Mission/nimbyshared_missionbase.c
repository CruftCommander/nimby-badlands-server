// -----------------------------------------------------------------------------
// NimbyShared - self-test hook (shared rules 8, Plan 11 v0.2 C8).
// Purpose : Runs NimbySharedSelfTest (3_Game) and the pause-registry cases (5_Mission, below) at
//           mission start when $profile:NimbyShared\selftest exists, on both the server (so the
//           Build-NimbyPack boot check sees PASS|FAIL lines) and the client. Same shape as the
//           Greet hook.
// Note    : Prints only when the flag file exists. The completion line's cases=N is what the
//           boot check's self-test gate counts, and it counts both layers.
// -----------------------------------------------------------------------------

// A registry entry that exists only for the self-test; it never stays registered.
class NimbySharedSelfTestEntry extends NimbySharedPauseEntry
{
    override string Id()
    {
        return "nimbyshared-selftest";
    }

    override string LabelFallback()
    {
        return "self-test";
    }
}

class NimbySharedShellSelfTest
{
    static int Run(array<string> results)
    {
        int failures = 0;
        int before = NimbySharedPauseButtons.Count();
        NimbySharedPauseButtons.Register(new NimbySharedSelfTestEntry());
        int at = NimbySharedPauseButtons.IndexOf("nimbyshared-selftest");
        failures += NimbySharedSelfTest.Case(results, "pause: a registered entry is listed", NimbySharedPauseButtons.Count() == before + 1 && at >= 0);
        NimbySharedPauseButtons.Register(new NimbySharedSelfTestEntry());
        failures += NimbySharedSelfTest.Case(results, "pause: the same id registered twice is listed once", NimbySharedPauseButtons.Count() == before + 1);
        NimbySharedPauseButtons.Register(null);
        failures += NimbySharedSelfTest.Case(results, "pause: a null entry is refused", NimbySharedPauseButtons.Count() == before + 1);
        NimbySharedPauseButtons.Unregister("nimbyshared-selftest");
        failures += NimbySharedSelfTest.Case(results, "pause: an unregistered entry is gone", NimbySharedPauseButtons.Count() == before && NimbySharedPauseButtons.IndexOf("nimbyshared-selftest") < 0);
        return failures;
    }
}

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
        failures += NimbySharedShellSelfTest.Run(results);
        foreach (string result : results)
        {
            Print("[NIMBY][Shared][SelfTest] " + result);
        }
        Print("[NIMBY][Shared] selftest complete; cases=" + results.Count().ToString() + " failures=" + failures.ToString());
    }
}
