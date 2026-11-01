// -----------------------------------------------------------------------------
// NimbyAdmin - self-test hook (shared rules 8).
// Purpose : Runs NimbyAdminSelfTest (3_Game) and the page-registry cases below (5_Mission) at
//           mission start when $profile:NimbyAdmin\selftest exists, on both the server (so the
//           Build-NimbyPack boot check sees PASS|FAIL lines) and the client. Same shape as the
//           Greet and NimbyShared hooks.
// Note    : Prints only when the flag file exists. The completion line's cases=N is what the boot
//           check's self-test gate counts, and it counts both layers.
// -----------------------------------------------------------------------------

// A page that exists only for the self-test (N9, "one fake page"); it never stays registered.
class NimbyAdminSelfTestPage extends NimbyAdminPage
{
    override string Id()
    {
        return "nimbyadmin-selftest";
    }

    override string Label()
    {
        return "self-test";
    }
}

class NimbyAdminShellSelfTest
{
    static int Run(array<string> results)
    {
        int failures = 0;
        int before = NimbyAdminPages.Count();
        NimbyAdminPages.Register(new NimbyAdminSelfTestPage());
        failures += NimbyAdminSelfTest.Case(results, "pages: a fake page registers (N9 seam)", NimbyAdminPages.Count() == before + 1 && NimbyAdminPages.IndexOf("nimbyadmin-selftest") >= 0);
        NimbyAdminPages.Register(new NimbyAdminSelfTestPage());
        failures += NimbyAdminSelfTest.Case(results, "pages: the same id registered twice is listed once", NimbyAdminPages.Count() == before + 1);
        NimbyAdminPages.Register(null);
        failures += NimbyAdminSelfTest.Case(results, "pages: a null page is refused", NimbyAdminPages.Count() == before + 1);
        NimbyAdminPages.Unregister("nimbyadmin-selftest");
        failures += NimbyAdminSelfTest.Case(results, "pages: the fake page unregisters", NimbyAdminPages.Count() == before && NimbyAdminPages.IndexOf("nimbyadmin-selftest") < 0);
        NimbyAdminPage players = new NimbyAdminPlayersPage();
        failures += NimbyAdminSelfTest.Case(results, "pages: the Players page has an id and a label", players.Id() == "players" && players.Label() != "");
        return failures;
    }
}

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
        failures += NimbyAdminShellSelfTest.Run(results);
        foreach (string result : results)
        {
            Print("[NIMBY][Admin][SelfTest] " + result);
        }
        Print("[NIMBY][Admin] selftest complete; cases=" + results.Count().ToString() + " failures=" + failures.ToString());
    }
}
