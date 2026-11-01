// -----------------------------------------------------------------------------
// NimbyShared - rules: the constants every other NimbyShared file reads, and the engine-free
// decisions the self-test covers (shared rules 8). No engine call in this file.
// Plan  : docs/plans/2026-10-31-nimby-shared-plan.md v0.2: C1 (identifiers), C5 (the text-size
//         steps of the window shell), C7 (codec), C8 (self-test). A constant change is a plan
//         change first.
// Origin: CHUNK_BYTES and the text-size steps moved here from NimbyGreetRules (Greet plan
//         v0.4, G5 and G16) in Plan 11 Phase 1; their values are unchanged.
// Style : one statement per line.
// -----------------------------------------------------------------------------

class NimbySharedRules
{
    // C7: an RPC string arrives intact at 1,000 bytes and throws "String CORRUPTED" on Read
    // at 1,100 and above (Greet NIMBY-T-037). The value is the codec's, not a consumer's.
    static const int CHUNK_BYTES = 1000;

    // C5 (Greet G16): the three text sizes a shell window cycles through.
    static const float TEXT_SCALE_NORMAL = 1.0;
    static const float TEXT_SCALE_LARGE = 1.25;
    static const float TEXT_SCALE_LARGER = 1.5;

    // Shared rules 4.3 and 8: the flag files, read once at first use.
    static const string DIR = "$profile:NimbyShared";
    static const string FILE_DEBUG = "$profile:NimbyShared\\debug";
    static const string FILE_SELFTEST = "$profile:NimbyShared\\selftest";

    // The next step of the text-size cycle: normal, large, larger, then normal again.
    static float NextTextScale(float current)
    {
        if (current < TEXT_SCALE_LARGE)
        {
            return TEXT_SCALE_LARGE;
        }
        if (current < TEXT_SCALE_LARGER)
        {
            return TEXT_SCALE_LARGER;
        }
        return TEXT_SCALE_NORMAL;
    }
}
