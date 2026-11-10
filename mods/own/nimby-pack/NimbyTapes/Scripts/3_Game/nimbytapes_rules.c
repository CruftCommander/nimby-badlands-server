// -----------------------------------------------------------------------------
// NimbyTapes - rules: the constants every other NimbyTapes file reads, the engine-free
// arithmetic of the deck, and the self-test (shared rules 8). No engine call in this file.
// Plan  : docs/plans/2026-11-09-music-plan.md v0.3: T-5 (tape memory), T-6 (skip rules),
//         T-10 (packed state), T-11 (test-kit flag), Architecture (Rules, Rules cases).
//         A constant change is a plan change first.
// Cases : The self-test prints the plan's 18 Rules cases by name; test row T0-02 checks the
//         list and expects cases=18 failures=0.
// Style : one statement per line; no variable name is declared twice in one function.
// -----------------------------------------------------------------------------

class NimbyTapesRules
{
    // T-10: play state travels as one packed int: generation, track, elapsed seconds.
    static const int GENERATION_BITS = 8;
    static const int TRACK_BITS = 6;
    static const int ELAPSED_BITS = 16;

    // The field sizes above as counts and place values. Arithmetic, not bit operators, so
    // the layout reads the same in a log line: packed = generation * 4194304 + track * 65536
    // + elapsed, and the highest packed value is 2^30 - 1.
    static const int GENERATION_COUNT = 256;
    static const int TRACK_COUNT = 64;
    static const int ELAPSED_COUNT = 65536;
    static const int TRACK_PLACE = 65536;
    static const int GENERATION_PLACE = 4194304;

    // T-10: the highest track value means stopped, so a tape holds at most 63 tracks (0 to 62).
    static const int TRACK_STOPPED = 63;
    static const int MAX_TRACKS = 63;

    // T-6: Previous restarts the current track when more than this many seconds in.
    static const int PREVIOUS_RESTART_SECONDS = 3;

    // Shared rules 4.3 and 8, and T-11: the flag files, read once at mission start.
    static const string DIR = "$profile:NimbyTapes";
    static const string FILE_DEBUG = "$profile:NimbyTapes\\debug";
    static const string FILE_SELFTEST = "$profile:NimbyTapes\\selftest";
    static const string FILE_TESTKIT = "$profile:NimbyTapes\\spawn-testkit";

    // ---- Packed state (T-10) ------------------------------------------------------------------

    // Out-of-range inputs are brought into range: generation wraps, track falls back to
    // stopped, elapsed is held at its ends. A caller never publishes a value Unpack cannot read.
    static int Pack(int generation, int track, int elapsed)
    {
        int safeGeneration = generation % GENERATION_COUNT;
        if (safeGeneration < 0)
        {
            safeGeneration = safeGeneration + GENERATION_COUNT;
        }
        int safeTrack = track;
        if (safeTrack < 0 || safeTrack >= TRACK_COUNT)
        {
            safeTrack = TRACK_STOPPED;
        }
        int safeElapsed = elapsed;
        if (safeElapsed < 0)
        {
            safeElapsed = 0;
        }
        if (safeElapsed >= ELAPSED_COUNT)
        {
            safeElapsed = ELAPSED_COUNT - 1;
        }
        return safeGeneration * GENERATION_PLACE + safeTrack * TRACK_PLACE + safeElapsed;
    }

    static int UnpackGeneration(int packed)
    {
        if (packed < 0)
        {
            return 0;
        }
        int generation = packed / GENERATION_PLACE;
        return generation % GENERATION_COUNT;
    }

    static int UnpackTrack(int packed)
    {
        if (packed < 0)
        {
            return TRACK_STOPPED;
        }
        int upper = packed / TRACK_PLACE;
        return upper % TRACK_COUNT;
    }

    static int UnpackElapsed(int packed)
    {
        if (packed < 0)
        {
            return 0;
        }
        return packed % ELAPSED_COUNT;
    }

    static bool IsStopped(int track)
    {
        return track == TRACK_STOPPED;
    }

    // The generation after this one. It increments on every start, track change, and restart of
    // a track, and wraps from 255 to 0.
    static int NextGeneration(int generation)
    {
        int following = generation + 1;
        return following % GENERATION_COUNT;
    }

    // ---- Start offset -------------------------------------------------------------------------

    // The share of the track already played, for AbstractWave.SetStartOffset. Returns false,
    // meaning do not start, for a length of zero or less, a negative elapsed time, or an
    // elapsed time at or past the end.
    static bool StartFraction(float elapsedSeconds, float lengthSeconds, out float fraction)
    {
        fraction = 0;
        if (lengthSeconds <= 0)
        {
            return false;
        }
        if (elapsedSeconds < 0)
        {
            return false;
        }
        if (elapsedSeconds >= lengthSeconds)
        {
            return false;
        }
        fraction = elapsedSeconds / lengthSeconds;
        return true;
    }

    // ---- Skip rules (T-6) ---------------------------------------------------------------------

    // The track Next moves to. TRACK_STOPPED means the tape ends and the deck switches off:
    // Next on the last track, a track outside the tape, or a deck that is already stopped.
    static int NextTrack(int track, int trackCount)
    {
        if (track == TRACK_STOPPED)
        {
            return TRACK_STOPPED;
        }
        if (track < 0 || track >= trackCount)
        {
            return TRACK_STOPPED;
        }
        int following = track + 1;
        if (following >= trackCount)
        {
            return TRACK_STOPPED;
        }
        return following;
    }

    // The track Previous moves to; it always starts from its beginning. More than
    // PREVIOUS_RESTART_SECONDS in: the same track. Otherwise one back, and track 0 stays track 0.
    // A stopped deck stays stopped. The elapsed time is taken unrounded: 3.5 seconds in is more
    // than 3 (T-6), which a whole-second value would miss.
    static int PreviousTrack(int track, float elapsedSeconds)
    {
        if (track == TRACK_STOPPED)
        {
            return TRACK_STOPPED;
        }
        if (track < 0)
        {
            return 0;
        }
        if (elapsedSeconds > PREVIOUS_RESTART_SECONDS)
        {
            return track;
        }
        if (track > 0)
        {
            return track - 1;
        }
        return 0;
    }

    // ---- Tape config --------------------------------------------------------------------------

    // True when the tape's two config lists describe at least one playable track. reason names
    // the first fault found, for the server's refusal line.
    static bool ValidateTape(array<string> trackSets, array<int> trackSeconds, out string reason)
    {
        reason = "";
        if (!trackSets || trackSets.Count() == 0)
        {
            reason = "config-empty";
            return false;
        }
        if (trackSets.Count() > MAX_TRACKS)
        {
            reason = "config-too-many";
            return false;
        }
        if (!trackSeconds || trackSeconds.Count() != trackSets.Count())
        {
            reason = "config-mismatch";
            return false;
        }
        for (int index = 0; index < trackSets.Count(); index++)
        {
            if (trackSets.Get(index) == "")
            {
                reason = "config-missing-name";
                return false;
            }
            if (trackSeconds.Get(index) <= 0 || trackSeconds.Get(index) >= ELAPSED_COUNT)
            {
                reason = "config-bad-length";
                return false;
            }
        }
        return true;
    }

    // ---- Tape memory (T-5) --------------------------------------------------------------------

    // The track a tape resumes at: the remembered one when the tape has it, otherwise the first.
    static int ResolveRememberedTrack(int remembered, int trackCount)
    {
        if (remembered < 0 || remembered >= trackCount)
        {
            return 0;
        }
        return remembered;
    }
}

class NimbyTapesSelfTest
{
    // Returns the number of failures; the caller prints one PASS|FAIL line per result.
    static int Run(out array<string> results)
    {
        results = new array<string>;
        int failures = 0;
        failures += PackCases(results);
        failures += FractionCases(results);
        failures += SkipCases(results);
        failures += ConfigCases(results);
        failures += MemoryCases(results);
        return failures;
    }

    // ---- pack-roundtrip, pack-generation-wrap -------------------------------------------------

    protected static bool RoundTrips(int generation, int track, int elapsed)
    {
        int packed = NimbyTapesRules.Pack(generation, track, elapsed);
        if (packed < 0)
        {
            return false;
        }
        if (NimbyTapesRules.UnpackGeneration(packed) != generation)
        {
            return false;
        }
        if (NimbyTapesRules.UnpackTrack(packed) != track)
        {
            return false;
        }
        if (NimbyTapesRules.UnpackElapsed(packed) != elapsed)
        {
            return false;
        }
        return true;
    }

    protected static int PackCases(array<string> results)
    {
        int failures = 0;

        // Generation 0, 1, 255; track 0, 62, 63; elapsed 0, 1, 65535: all 27 combinations.
        array<int> generations = new array<int>;
        generations.Insert(0);
        generations.Insert(1);
        generations.Insert(255);
        array<int> tracks = new array<int>;
        tracks.Insert(0);
        tracks.Insert(62);
        tracks.Insert(63);
        array<int> elapsedValues = new array<int>;
        elapsedValues.Insert(0);
        elapsedValues.Insert(1);
        elapsedValues.Insert(65535);
        bool roundTrip = true;
        for (int g = 0; g < generations.Count(); g++)
        {
            for (int t = 0; t < tracks.Count(); t++)
            {
                for (int e = 0; e < elapsedValues.Count(); e++)
                {
                    if (!RoundTrips(generations.Get(g), tracks.Get(t), elapsedValues.Get(e)))
                    {
                        roundTrip = false;
                    }
                }
            }
        }
        // The highest value uses 30 bits and no more.
        if (NimbyTapesRules.Pack(255, 63, 65535) != 1073741823)
        {
            roundTrip = false;
        }
        failures += Case(results, "pack-roundtrip", roundTrip);

        bool wrap = true;
        if (NimbyTapesRules.NextGeneration(255) != 0)
        {
            wrap = false;
        }
        if (NimbyTapesRules.NextGeneration(0) != 1)
        {
            wrap = false;
        }
        if (NimbyTapesRules.UnpackGeneration(NimbyTapesRules.Pack(256, 0, 0)) != 0)
        {
            wrap = false;
        }
        failures += Case(results, "pack-generation-wrap", wrap);

        return failures;
    }

    // ---- fraction-start, fraction-mid, fraction-past-end, fraction-bad-length -----------------

    protected static int FractionCases(array<string> results)
    {
        int failures = 0;
        float fraction;
        bool accepted;

        accepted = NimbyTapesRules.StartFraction(0, 240, fraction);
        failures += Case(results, "fraction-start", accepted && fraction == 0);

        accepted = NimbyTapesRules.StartFraction(120, 240, fraction);
        failures += Case(results, "fraction-mid", accepted && Math.AbsFloat(fraction - 0.5) < 0.0001);

        accepted = NimbyTapesRules.StartFraction(300, 240, fraction);
        bool pastEndRefused = !accepted;
        accepted = NimbyTapesRules.StartFraction(240, 240, fraction);
        if (accepted)
        {
            pastEndRefused = false;
        }
        failures += Case(results, "fraction-past-end", pastEndRefused);

        accepted = NimbyTapesRules.StartFraction(0, 0, fraction);
        bool badLengthRefused = !accepted;
        accepted = NimbyTapesRules.StartFraction(10, -5, fraction);
        if (accepted)
        {
            badLengthRefused = false;
        }
        failures += Case(results, "fraction-bad-length", badLengthRefused);

        return failures;
    }

    // ---- next-middle, next-last, previous-late, previous-early, previous-first, skip-while-off

    protected static int SkipCases(array<string> results)
    {
        int failures = 0;

        failures += Case(results, "next-middle", NimbyTapesRules.NextTrack(0, 2) == 1);
        failures += Case(results, "next-last", NimbyTapesRules.NextTrack(1, 2) == NimbyTapesRules.TRACK_STOPPED);
        bool lateRestarts = true;
        if (NimbyTapesRules.PreviousTrack(1, 10) != 1)
        {
            lateRestarts = false;
        }
        // Just past the limit, whole and fractional: still the same track.
        if (NimbyTapesRules.PreviousTrack(1, 4) != 1)
        {
            lateRestarts = false;
        }
        if (NimbyTapesRules.PreviousTrack(1, 3.5) != 1)
        {
            lateRestarts = false;
        }
        failures += Case(results, "previous-late", lateRestarts);

        bool earlyGoesBack = true;
        if (NimbyTapesRules.PreviousTrack(1, 2) != 0)
        {
            earlyGoesBack = false;
        }
        // Exactly at the limit is not more than it: one track back.
        if (NimbyTapesRules.PreviousTrack(1, 3) != 0)
        {
            earlyGoesBack = false;
        }
        failures += Case(results, "previous-early", earlyGoesBack);
        failures += Case(results, "previous-first", NimbyTapesRules.PreviousTrack(0, 2) == 0);

        bool offUnchanged = true;
        if (NimbyTapesRules.NextTrack(NimbyTapesRules.TRACK_STOPPED, 2) != NimbyTapesRules.TRACK_STOPPED)
        {
            offUnchanged = false;
        }
        if (NimbyTapesRules.PreviousTrack(NimbyTapesRules.TRACK_STOPPED, 0) != NimbyTapesRules.TRACK_STOPPED)
        {
            offUnchanged = false;
        }
        failures += Case(results, "skip-while-off", offUnchanged);

        return failures;
    }

    // ---- config-empty, config-mismatch, config-missing-name, config-too-many ------------------

    protected static int ConfigCases(array<string> results)
    {
        int failures = 0;
        string reason;
        bool valid;

        array<string> noSets = new array<string>;
        array<int> noSeconds = new array<int>;
        valid = NimbyTapesRules.ValidateTape(noSets, noSeconds, reason);
        failures += Case(results, "config-empty", !valid && reason == "config-empty");

        array<string> twoSets = new array<string>;
        twoSets.Insert("NimbyTapes_TimingA_SoundSet");
        twoSets.Insert("NimbyTapes_TimingB_SoundSet");
        array<int> oneLength = new array<int>;
        oneLength.Insert(240);
        valid = NimbyTapesRules.ValidateTape(twoSets, oneLength, reason);
        failures += Case(results, "config-mismatch", !valid && reason == "config-mismatch");

        array<string> blankSet = new array<string>;
        blankSet.Insert("NimbyTapes_TimingA_SoundSet");
        blankSet.Insert("");
        array<int> twoLengths = new array<int>;
        twoLengths.Insert(240);
        twoLengths.Insert(60);
        valid = NimbyTapesRules.ValidateTape(blankSet, twoLengths, reason);
        bool missingNameRefused = !valid && reason == "config-missing-name";
        // The same lengths with both names present are accepted, so the refusal is the name's.
        valid = NimbyTapesRules.ValidateTape(twoSets, twoLengths, reason);
        if (!valid)
        {
            missingNameRefused = false;
        }
        failures += Case(results, "config-missing-name", missingNameRefused);

        array<string> manySets = new array<string>;
        array<int> manyLengths = new array<int>;
        for (int filled = 0; filled < 64; filled++)
        {
            manySets.Insert("NimbyTapes_TimingA_SoundSet");
            manyLengths.Insert(60);
        }
        valid = NimbyTapesRules.ValidateTape(manySets, manyLengths, reason);
        bool tooManyRefused = !valid && reason == "config-too-many";
        // 63 tracks, indexes 0 to 62, is the most a tape holds.
        manySets.Remove(63);
        manyLengths.Remove(63);
        valid = NimbyTapesRules.ValidateTape(manySets, manyLengths, reason);
        if (!valid)
        {
            tooManyRefused = false;
        }
        failures += Case(results, "config-too-many", tooManyRefused);

        return failures;
    }

    // ---- memory-in-range, memory-out-of-range -------------------------------------------------

    protected static int MemoryCases(array<string> results)
    {
        int failures = 0;

        failures += Case(results, "memory-in-range", NimbyTapesRules.ResolveRememberedTrack(1, 2) == 1);

        bool outOfRange = true;
        if (NimbyTapesRules.ResolveRememberedTrack(5, 2) != 0)
        {
            outOfRange = false;
        }
        if (NimbyTapesRules.ResolveRememberedTrack(-1, 2) != 0)
        {
            outOfRange = false;
        }
        failures += Case(results, "memory-out-of-range", outOfRange);

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
