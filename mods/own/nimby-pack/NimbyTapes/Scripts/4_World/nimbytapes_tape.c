// -----------------------------------------------------------------------------
// NimbyTapes - the tape (Plan 13, Architecture, Tape; T-5).
// Purpose : Script class of NimbyTapes_Tape_Base and of every tape derived from it in config.
//           It reads the two config lists a deck plays from and holds the remembered track.
// Memory  : m_NimbyTapesTrack is server-side and saved (T-5, T-12). This is the addon's one
//           store override and its whole field list, as shared rules 6.3 records it: after
//           super, the int STORE_VERSION (1), then the int m_NimbyTapesTrack. The load reads
//           behind the version guard, in the form the Tack note uses
//           (NimbyTack/Scripts/4_World/nimbytack_pinnednote.c:85-127): a tape saved before the
//           field existed has nothing to read and loads with track 0. A value outside 0 to 62
//           loads as 0. A later field needs a higher STORE_VERSION and its own guard (plan
//           risk K13). No vanilla class gains saved data.
// API     : Object.ConfigGetTextArray and ConfigGetIntArray
//           (3_Game/DayZ/Entities/Object.c:901, :923); ItemBase.OnStoreLoad and OnStoreSave
//           (4_World/DayZ/Entities/ItemBase.c:3093, :3194); Serializer.Write and Read
//           (1_Core/DayZ/proto/Serializer.c:57, :58).
//           Lines are relative to vendor/dayz-1.30-scripts/scripts/ (build 1.30.164289).
// Style   : one statement per line; no variable name is declared twice in one function.
// -----------------------------------------------------------------------------
class NimbyTapes_Tape_Base : Inventory_Base
{
    static const int STORE_VERSION = 1;

    protected int m_NimbyTapesTrack;

    override void OnStoreSave(ParamsWriteContext ctx)
    {
        super.OnStoreSave(ctx);
        ctx.Write(STORE_VERSION);
        ctx.Write(m_NimbyTapesTrack);
    }

    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
        {
            return false;
        }
        int storeVersion;
        if (!ctx.Read(storeVersion))
        {
            return true;
        }
        if (storeVersion < 1)
        {
            return true;
        }
        int track;
        if (!ctx.Read(track))
        {
            return false;
        }
        if (track < 0 || track >= NimbyTapesRules.MAX_TRACKS)
        {
            track = 0;
        }
        m_NimbyTapesTrack = track;
        return true;
    }

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
