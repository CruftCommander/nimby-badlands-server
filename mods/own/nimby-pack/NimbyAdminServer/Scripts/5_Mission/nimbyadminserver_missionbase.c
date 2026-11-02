// -----------------------------------------------------------------------------
// NimbyAdminServer - self-test hook (shared rules 8).
// Purpose : Runs NimbyAdminServerSelfTest at mission start when
//           $profile:NimbyAdminServer\selftest exists. This PBO loads on the server only
//           (-servermod=), which is where the Build-NimbyPack boot check reads the lines.
// Note    : Prints only when the flag file exists. The component is AdminServer so the gate
//           counts it apart from the client half's [NIMBY][Admin][SelfTest] lines.
// -----------------------------------------------------------------------------

modded class MissionBase extends MissionBaseWorld
{
    override void OnInit()
    {
        super.OnInit();
        NimbyAdminServerRunSelfTestIfFlagged();
    }

    protected void NimbyAdminServerRunSelfTestIfFlagged()
    {
        if (!FileExist(NimbyAdminServerRules.FILE_SELFTEST))
        {
            return;
        }
        array<string> results;
        int failures = NimbyAdminServerSelfTest.Run(results);
        foreach (string result : results)
        {
            Print("[NIMBY][AdminServer][SelfTest] " + result);
        }
        // Phase 1 A4: the counts are locals, not calls inside Print's argument.
        int cases = results.Count();
        string casesText = cases.ToString();
        string failuresText = failures.ToString();
        Print("[NIMBY][AdminServer] selftest complete; cases=" + casesText + " failures=" + failuresText);
    }
}
