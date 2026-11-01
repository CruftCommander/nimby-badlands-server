// -----------------------------------------------------------------------------
// NimbyAdmin - rules: the constants every other NimbyAdmin file reads, and the engine-free
// self-test (shared rules 8). No engine call in this file.
// Plan  : docs/plans/2026-10-15-nimby-admin-plan.md v0.7: N1 (identifiers), N9 (reserved
//         observation ids), Phase 0 security contract. A constant change is a plan change first.
// Stub  : On main the file holds the RPC block, the flag paths, and a self-test that proves
//         the addon loads and the boot check's self-test gate sees it. The request envelope
//         validators arrive in Phase 0 with their cases.
// Style : one statement per line.
// -----------------------------------------------------------------------------

class NimbyAdminRules
{
    // N1 and shared rules 4.1: the 65503xx block. Command ids are assigned in Phase 0;
    // 6550350-6550399 stay reserved for observation feeds (N9d) and are never assigned here.
    static const int RPC_BLOCK_FIRST = 6550300;
    static const int RPC_BLOCK_LAST = 6550399;
    static const int RPC_OBSERVE_FIRST = 6550350;
    static const int RPC_OBSERVE_LAST = 6550399;
    static const int MENU_ID = 65503;

    // Shared rules 4.3 and 8: the flag files, read once at mission start.
    static const string DIR = "$profile:NimbyAdmin";
    static const string FILE_DEBUG = "$profile:NimbyAdmin\\debug";
    static const string FILE_SELFTEST = "$profile:NimbyAdmin\\selftest";

    // True for an RPC id a command MAY use: inside the block and outside the reserved range.
    static bool IsCommandRpc(int id)
    {
        if (id < RPC_BLOCK_FIRST)
        {
            return false;
        }
        if (id > RPC_BLOCK_LAST)
        {
            return false;
        }
        if (id >= RPC_OBSERVE_FIRST && id <= RPC_OBSERVE_LAST)
        {
            return false;
        }
        return true;
    }
}

class NimbyAdminSelfTest
{
    // Returns the number of failures; the caller prints one PASS|FAIL line per result.
    static int Run(out array<string> results)
    {
        results = new array<string>;
        int failures = 0;
        failures += StubCases(results);
        return failures;
    }

    // ---- Stub (October 31, 2026): the identifiers the plan fixes -----------------------------

    protected static int StubCases(array<string> results)
    {
        int failures = 0;
        failures += Case(results, "rules: 6550301 is a command id", NimbyAdminRules.IsCommandRpc(6550301));
        failures += Case(results, "rules: 6550350 is reserved for observation (N9)", NimbyAdminRules.IsCommandRpc(6550350) == false);
        failures += Case(results, "rules: 6550400 is outside the block", NimbyAdminRules.IsCommandRpc(6550400) == false);
        failures += Case(results, "rules: selftest flag lives under $profile:NimbyAdmin", NimbyAdminRules.FILE_SELFTEST.IndexOf(NimbyAdminRules.DIR) == 0);
        return failures;
    }

    protected static int Case(array<string> results, string name, bool passed)
    {
        if (passed)
        {
            results.Insert("PASS " + name);
            return 0;
        }
        results.Insert("FAIL " + name);
        return 1;
    }
}
