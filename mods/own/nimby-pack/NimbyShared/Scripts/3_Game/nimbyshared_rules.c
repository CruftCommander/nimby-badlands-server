// -----------------------------------------------------------------------------
// NimbyShared - rules: the constants every other NimbyShared file reads, and the engine-free
// self-test (shared rules 8). No engine call in this file.
// Plan  : docs/plans/2026-10-31-nimby-shared-plan.md v0.1: C1 (identifiers), C7 (codec),
//         C8 (self-test). A constant change is a plan change first.
// Stub  : On main the file holds the flag paths, CHUNK_BYTES, and a self-test that proves
//         the addon loads and the boot check's self-test gate sees it. The codec moves here
//         from NimbyGreetRules in Plan 11 Phase 1, with its Phase 0 cases.
// Style : one statement per line.
// -----------------------------------------------------------------------------

class NimbySharedRules
{
    // C7: an RPC string arrives intact at 1,000 bytes and throws "String CORRUPTED" on Read
    // at 1,100 and above (Greet NIMBY-T-037). The value is the codec's, not a consumer's.
    static const int CHUNK_BYTES = 1000;

    // Shared rules 4.3 and 8: the flag files, read once at mission start.
    static const string DIR = "$profile:NimbyShared";
    static const string FILE_DEBUG = "$profile:NimbyShared\\debug";
    static const string FILE_SELFTEST = "$profile:NimbyShared\\selftest";
}

class NimbySharedSelfTest
{
    // Returns the number of failures; the caller prints one PASS|FAIL line per result.
    static int Run(out array<string> results)
    {
        results = new array<string>;
        int failures = 0;
        failures += StubCases(results);
        return failures;
    }

    // ---- Stub (October 31, 2026): the constants the consumers will depend on -------------------

    protected static int StubCases(array<string> results)
    {
        int failures = 0;
        failures += Case(results, "rules: CHUNK_BYTES is 1000 (NIMBY-T-037)", NimbySharedRules.CHUNK_BYTES == 1000);
        failures += Case(results, "rules: debug flag lives under $profile:NimbyShared", NimbySharedRules.FILE_DEBUG.IndexOf(NimbySharedRules.DIR) == 0);
        failures += Case(results, "rules: selftest flag lives under $profile:NimbyShared", NimbySharedRules.FILE_SELFTEST.IndexOf(NimbySharedRules.DIR) == 0);
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
