// -----------------------------------------------------------------------------
// NimbyAdminServer - rules: the constants every other NimbyAdminServer file reads, and the
// engine-free self-test (shared rules 8). No engine call in this file.
// Plan  : docs/plans/2026-10-15-nimby-admin-plan.md v0.7: N4, N5, N8 (profile files), the
//         config.json table. A constant change is a plan change first.
// Stub  : On main the file holds the profile paths and a self-test that proves the server-only
//         PBO loads from -servermod= and the boot check's self-test gate sees it. The permission,
//         ban, config, and audit validators arrive in Phase 0 with their cases.
// Style : one statement per line.
// -----------------------------------------------------------------------------

class NimbyAdminServerRules
{
    // Plan, Data formats: everything under $profile:NimbyAdmin; the webhook URL only in
    // config.json and never in a log line or a client sync (N5).
    static const string DIR = "$profile:NimbyAdmin";
    static const string FILE_CONFIG = "$profile:NimbyAdmin\\config.json";
    static const string FILE_PERMISSIONS = "$profile:NimbyAdmin\\permissions.json";
    static const string FILE_PERMISSIONS_LKG = "$profile:NimbyAdmin\\permissions.json.lkg";
    static const string FILE_BANS = "$profile:NimbyAdmin\\bans.json";
    static const string FILE_BANS_LKG = "$profile:NimbyAdmin\\bans.json.lkg";
    static const string FILE_DEBUG = "$profile:NimbyAdmin\\debug";
    // The self-test flag is per pack.json prefix (Build-NimbyPack creates <profile>\<Prefix>\selftest).
    static const string FILE_SELFTEST = "$profile:NimbyAdminServer\\selftest";

    // A Steam64 id is 17 digits and starts with 7656 (permissions.json and bans.json rows;
    // an invalid row is skipped and logged, never granted).
    static bool IsSteam64(string value)
    {
        if (value.Length() != 17)
        {
            return false;
        }
        if (value.Substring(0, 4) != "7656")
        {
            return false;
        }
        for (int i = 0; i < 17; i++)
        {
            int c = value.Get(i).ToAscii();
            if (c < 48 || c > 57)
            {
                return false;
            }
        }
        return true;
    }
}

class NimbyAdminServerSelfTest
{
    // Returns the number of failures; the caller prints one PASS|FAIL line per result.
    static int Run(out array<string> results)
    {
        results = new array<string>;
        int failures = 0;
        failures += StubCases(results);
        return failures;
    }

    // ---- Stub (October 31, 2026): the identity rule every store depends on ---------------------

    protected static int StubCases(array<string> results)
    {
        int failures = 0;
        failures += Case(results, "steam64: 17 digits starting 7656 accepted", NimbyAdminServerRules.IsSteam64("76561198354705597"));
        failures += Case(results, "steam64: 16 digits refused", NimbyAdminServerRules.IsSteam64("7656119835470559") == false);
        failures += Case(results, "steam64: letters refused", NimbyAdminServerRules.IsSteam64("7656119835470559x") == false);
        failures += Case(results, "steam64: BI hash refused", NimbyAdminServerRules.IsSteam64("h8o5rWLPVptWLIPrMHl9JnGQMT0bM8CfAyoaOMMnH5Q=") == false);
        failures += Case(results, "rules: selftest flag lives under $profile:NimbyAdminServer", NimbyAdminServerRules.FILE_SELFTEST.IndexOf("$profile:NimbyAdminServer") == 0);
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
