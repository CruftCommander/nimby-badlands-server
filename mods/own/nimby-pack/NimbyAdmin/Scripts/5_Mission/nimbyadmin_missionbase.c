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
        // Phase 1 A4: every value is a local before the call (the 164200 argument-slot rule).
        int before = NimbyAdminPages.Count();
        NimbyAdminPage fake = new NimbyAdminSelfTestPage();
        NimbyAdminPages.Register(fake);
        int afterOne = NimbyAdminPages.Count();
        int fakeAt = NimbyAdminPages.IndexOf("nimbyadmin-selftest");
        bool registered = afterOne == before + 1 && fakeAt >= 0;
        failures += NimbyAdminSelfTest.Case(results, "pages: a fake page registers (N9 seam)", registered);
        NimbyAdminPage again = new NimbyAdminSelfTestPage();
        NimbyAdminPages.Register(again);
        int afterTwice = NimbyAdminPages.Count();
        bool once = afterTwice == before + 1;
        failures += NimbyAdminSelfTest.Case(results, "pages: the same id registered twice is listed once", once);
        NimbyAdminPages.Register(null);
        int afterNull = NimbyAdminPages.Count();
        bool nullRefused = afterNull == before + 1;
        failures += NimbyAdminSelfTest.Case(results, "pages: a null page is refused", nullRefused);
        NimbyAdminPages.Unregister("nimbyadmin-selftest");
        int afterRemove = NimbyAdminPages.Count();
        int goneAt = NimbyAdminPages.IndexOf("nimbyadmin-selftest");
        bool removed = afterRemove == before && goneAt < 0;
        failures += NimbyAdminSelfTest.Case(results, "pages: the fake page unregisters", removed);
        NimbyAdminPage players = new NimbyAdminPlayersPage();
        string playersId = players.Id();
        string playersLabel = players.Label();
        bool labelled = playersId == "players" && playersLabel != "";
        failures += NimbyAdminSelfTest.Case(results, "pages: the Players page has an id and a label", labelled);
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
        int cases = results.Count();
        string casesText = cases.ToString();
        string failuresText = failures.ToString();
        Print("[NIMBY][Admin] selftest complete; cases=" + casesText + " failures=" + failuresText);
    }
}
