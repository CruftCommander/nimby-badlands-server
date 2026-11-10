// -----------------------------------------------------------------------------
// NimbyTapes - rules: the constants every other NimbyTapes file reads, and the engine-free
// self-test (shared rules 8). No engine call in this file.
// Plan  : docs/plans/2026-11-09-music-plan.md v0.3: T-10 (packed state), T-11 (test-kit
//         flag), Architecture (Rules). A constant change is a plan change first.
// Stub  : On main the file holds the packed-state widths, the stopped marker, the flag
//         paths, and three stub cases that prove the addon loads and the boot check's
//         self-test gate sees it. Phase 0 step 4 replaces the stub cases with the plan's
//         18 named Rules cases; T0-02 expects exactly those 18 (cases=18 failures=0).
// Style : one statement per line.
// -----------------------------------------------------------------------------

class NimbyTapesRules
{
    // T-10: play state travels as one packed int: generation, track, elapsed seconds.
    static const int GENERATION_BITS = 8;
    static const int TRACK_BITS = 6;
    static const int ELAPSED_BITS = 16;

    // T-10: the highest track value means stopped, so a tape holds at most 63 tracks (0 to 62).
    static const int TRACK_STOPPED = 63;

    // Shared rules 4.3 and 8, and T-11: the flag files, read once at mission start.
    static const string DIR = "$profile:NimbyTapes";
    static const string FILE_DEBUG = "$profile:NimbyTapes\\debug";
    static const string FILE_SELFTEST = "$profile:NimbyTapes\\selftest";
    static const string FILE_TESTKIT = "$profile:NimbyTapes\\spawn-testkit";
}

class NimbyTapesSelfTest
{
    // Returns the number of failures; the caller prints one PASS|FAIL line per result.
    static int Run(out array<string> results)
    {
        results = new array<string>;
        int failures = 0;
        failures += StubCases(results);
        return failures;
    }

    // ---- Stub (November 10, 2026): the constants the Phase 0 body will depend on ---------------

    protected static int StubCases(array<string> results)
    {
        int failures = 0;

        // The three fields must fit one signed 32-bit int with the sign bit left alone.
        int totalBits = NimbyTapesRules.GENERATION_BITS + NimbyTapesRules.TRACK_BITS + NimbyTapesRules.ELAPSED_BITS;
        failures += Case(results, "stub: packed state is 30 bits and fits one int (T-10)", totalBits == 30);

        // The stopped marker must be the highest value the track field can hold.
        int trackValues = 1;
        for (int bit = 0; bit < NimbyTapesRules.TRACK_BITS; bit++)
        {
            trackValues = trackValues * 2;
        }
        failures += Case(results, "stub: the stopped marker is the highest track value (T-10)", trackValues - 1 == NimbyTapesRules.TRACK_STOPPED);

        bool flagsUnderDir = true;
        if (NimbyTapesRules.FILE_DEBUG.IndexOf(NimbyTapesRules.DIR) != 0)
        {
            flagsUnderDir = false;
        }
        if (NimbyTapesRules.FILE_SELFTEST.IndexOf(NimbyTapesRules.DIR) != 0)
        {
            flagsUnderDir = false;
        }
        if (NimbyTapesRules.FILE_TESTKIT.IndexOf(NimbyTapesRules.DIR) != 0)
        {
            flagsUnderDir = false;
        }
        failures += Case(results, "stub: the debug, selftest, and spawn-testkit flags live under $profile:NimbyTapes (T-11)", flagsUnderDir);

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
