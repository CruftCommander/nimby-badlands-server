// -----------------------------------------------------------------------------
// NimbyTapes - the tape (Plan 13, Architecture, Tape; T-5).
// Purpose : Script class of NimbyTapes_Tape_Base and of every tape derived from it in config.
//           It reads the two config lists a deck plays from and holds the remembered track.
// Memory  : m_NimbyTapesTrack is server-side and, in this step, in memory only. The store
//           override that saves it (T-5, T-12; shared rules 6.3: one int, save version 1,
//           behind a version guard) is the next step of Phase 0 step 4 and is not in this file
//           yet. Until then a tape forgets its track at a restart.
// API     : Object.ConfigGetTextArray and ConfigGetIntArray
//           (3_Game/DayZ/Entities/Object.c:901, :923).
//           Lines are relative to vendor/dayz-1.30-scripts/scripts/ (build 1.30.164289).
// Style   : one statement per line; no variable name is declared twice in one function.
// -----------------------------------------------------------------------------
class NimbyTapes_Tape_Base : Inventory_Base
{
    protected int m_NimbyTapesTrack;

    // The remembered track as stored. The deck brings it into range with
    // NimbyTapesRules.ResolveRememberedTrack before it plays.
    int NimbyTapesGetRememberedTrack()
    {
        return m_NimbyTapesTrack;
    }

    void NimbyTapesSetRememberedTrack(int track)
    {
        m_NimbyTapesTrack = track;
    }

    // Sound set names in play order, from tracks[].
    array<string> NimbyTapesGetTrackSets()
    {
        array<string> trackSets = new array<string>;
        ConfigGetTextArray("tracks", trackSets);
        return trackSets;
    }

    // Length of each track in whole seconds, from trackSeconds[].
    array<int> NimbyTapesGetTrackSeconds()
    {
        array<int> trackSeconds = new array<int>;
        ConfigGetIntArray("trackSeconds", trackSeconds);
        return trackSeconds;
    }
}
