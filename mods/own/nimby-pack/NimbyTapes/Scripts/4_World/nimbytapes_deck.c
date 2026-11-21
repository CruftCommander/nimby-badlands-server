// -----------------------------------------------------------------------------
// NimbyTapes - the deck ("Cassette Player", T-13). Plan 13 v0.3, Architecture: Deck (server),
// After load, Synced state, Deck (client), Playback, Following the carrier, Effect lifetime.
// Server  : While the deck works (switched on and powered), the server holds the playing track
//           and the tick time it started at, and publishes one packed int (T-10) once a second.
//           Elapsed time comes from the clock (GetTickTime), never from a tick count.
// Client  : OnVariablesSynchronized unpacks the int and reconciles it with the one EffectSound
//           this deck may hold. Progress updates never restart a wave.
// Load    : OnWorkStart can fire while the deck is still loading, before its tape exists
//           (recon, Question 8). Before IsInitialized() it only records that work started.
//           AfterStoreLoad starts the remembered track (T-9). DeferredInit covers a deck that
//           was spawned already working; it starts only when power and the tape are both
//           there and never switches off, because it can run before a loading deck has its
//           battery or its tape. Open until test row T0-20: when the deck is already
//           initialized at its battery's attach event during a load and the tape is not
//           attached yet, OnWorkStart takes the live path and refuses (self-review of
//           November 21, 2026). The deck has no store override (shared rules 6.3).
// Actions : The four vanilla switch actions, Next and Previous in an in-hands and an on-ground
//           form (T-6), and the two test-kit actions, Ruin and Delete (T-11). The action
//           classes are in nimbytapes_actions.c; they call NimbyTapesServerNext,
//           NimbyTapesServerPrevious, and NimbyTapesIsPlaying here.
// Debug   : [NIMBY][Tapes][Debug] lines only while $profile:NimbyTapes\debug exists. Server:
//           start, advance, publish, stop, defer, wait. Client: create, delete, play, seek,
//           sync, reparent, stop, replace (a new generation takes over), ended, hold. No line
//           names a player.
// Credit  : The sync method (the server publishes elapsed seconds, each client starts its own
//           sound at that share of the track) follows Zens Music by Zenarchist
//           (ZenMusic:ZenMusicBase/scripts/4_World/ZenMusicBase/Entities/ZenMusic/Devices/
//           Zen_MusicDeviceBase.c:226-227, 329-344, commit 3de1452). Written from the plan;
//           no line is copied. See NOTICE.
// API     : Lines are relative to vendor/dayz-1.30-scripts/scripts/ (build 1.30.164289). A
//           declaration is evidence that an API exists, not of how it behaves; the T0 rows of
//           Plan 13 test the behavior.
//           3_Game/DayZ/Entities/EntityAI.c: DeferredInit :277, IsInitialized :282,
//             GetHierarchyRoot :879, EEDelete :946, EEItemLocationChanged :976,
//             EEHealthLevelChanged :1043, EEItemAttached :1159, EEItemDetached :1199,
//             AfterStoreLoad :1419, FindAttachmentBySlotName :1897,
//             RegisterNetSyncVariableInt :2850, SetSynchDirty :3064,
//             OnVariablesSynchronized :3069, GetCompEM :3409, HasEnergyManager :3420,
//             OnWorkStart :3428, OnWorkStop :3434.
//           4_World/DayZ/Entities/ItemBase.c: SetActions :246, AddAction :255,
//             OnInventoryEnter :3906, OnInventoryExit :3918.
//           3_Game/DayZ/tools/Component/ComponentEnergyManager.c: SwitchOff :402, CanWork :768,
//             IsSwitchedOn :884.
//           3_Game/DayZ/Entities/Object.c: GetNetworkIDString :821, IsRuined :1219.
//           3_Game/DayZ/Global/Game.c: GetTickTime :914, IsServer :1120, IsDedicatedServer :1125.
//           3_Game/DayZ/tools/tools.c: Timer :576, Run :595, Stop :617; IsRunning :264 (on
//             TimerBase, which Timer inherits).
//           3_Game/DayZ/EffectManager.c: CreateSound :153, PlaySoundOnObject :256-263 (the
//             create, parent, local position, play order used here), DestroyEffect :280.
//           3_Game/DayZ/Effects/EffectSound.c: SoundPlay :199, SoundStop :217,
//             SetSoundAutodestroy :626, SetParent :658, SetCurrentLocalPosition :746.
//           3_Game/DayZ/Effect.c: Managed base :16, SetParent(Object) :411, SetLocalPosition :503.
//           3_Game/DayZ/Sound.c: SetStartOffset :183, GetCurrPosition :187.
//           3_Game/DayZ/constants.c: STATE_RUINED :909. 1_Core/DayZ/proto/EnMath.c: Floor :427.
//           4_World/DayZ/Entities/ItemBase/ClockBase.c:32-38: vanilla destroys effects that
//             were started with autodestroy on, as the teardown here does.
//           The four switch actions: 4_World/DayZ/Classes/UserActionsComponent/Actions/
//             SingleUse/ActionTurnOnWhileInHands.c, ActionTurnOffWhileInHands.c and
//             Interact/ActionTurnOnWhileOnGround.c, ActionTurnOffWhileOnGround.c.
// Style   : one statement per line; no ternary; no variable name is declared twice in one
//           function.
// -----------------------------------------------------------------------------
class NimbyTapes_Deck : Inventory_Base
{
    static const string SLOT_TAPE = "NimbyTape";
    static const string SLOT_BATTERY = "BatteryD";

    // The server publishes once a second; the client checks its parent and a held start twice
    // a second, and holds a start for a missing tape for 10 checks (5 seconds).
    static const float SERVER_TICK_SECONDS = 1.0;
    static const float CLIENT_CHECK_SECONDS = 0.5;
    static const int CLIENT_HOLD_CHECKS = 10;

    // T-10: generation, track, and elapsed seconds in one net-synced int.
    protected int m_NimbyTapesState;

    // ---- server ----
    protected ref Timer m_NimbyTapesServerTimer;
    protected ref array<string> m_NimbyTapesSets;
    protected ref array<int> m_NimbyTapesSeconds;
    protected bool m_NimbyTapesPlaying;
    protected bool m_NimbyTapesWorkPending;
    protected bool m_NimbyTapesInAttach;
    protected int m_NimbyTapesGeneration;
    protected int m_NimbyTapesTrack;
    protected float m_NimbyTapesTrackStart;

    // ---- client ----
    // Not a strong reference: the effect manager owns the effect, and an effect that ends on its
    // own destroys itself (autodestroy), which clears this variable.
    protected EffectSound m_NimbyTapesEffect;
    protected Object m_NimbyTapesParent;
    protected ref Timer m_NimbyTapesClientTimer;
    protected bool m_NimbyTapesEffectCounted;
    protected int m_NimbyTapesLocalGeneration;
    protected int m_NimbyTapesLocalTrack;
    protected int m_NimbyTapesHoldChecks;
    protected int m_NimbyTapesQuietGeneration;
    protected int m_NimbyTapesFailGeneration;
    protected string m_NimbyTapesNetId;

    // Effects this client holds across all decks: created minus destroyed (plan risk K4).
    protected static int s_NimbyTapesEffects;

    void NimbyTapes_Deck()
    {
        m_NimbyTapesTrack = NimbyTapesRules.TRACK_STOPPED;
        m_NimbyTapesState = NimbyTapesRules.Pack(0, NimbyTapesRules.TRACK_STOPPED, 0);
        m_NimbyTapesLocalGeneration = -1;
        m_NimbyTapesLocalTrack = NimbyTapesRules.TRACK_STOPPED;
        m_NimbyTapesHoldChecks = -1;
        m_NimbyTapesQuietGeneration = -1;
        m_NimbyTapesFailGeneration = -1;
        // Name only, no range: a range quantizes the value (EntityAI.c:2843-2850).
        RegisterNetSyncVariableInt("m_NimbyTapesState");
    }

    void ~NimbyTapes_Deck()
    {
        if (m_NimbyTapesServerTimer)
        {
            m_NimbyTapesServerTimer.Stop();
        }
        if (m_NimbyTapesClientTimer)
        {
            m_NimbyTapesClientTimer.Stop();
        }
        // A deck that leaves this client's bubble is destroyed here without a stop state.
        // No entity call from a destructor: the id is the copy kept by NimbyTapesId().
        NimbyTapesClientRelease();
        if (NimbyTapesOnClient())
        {
            NimbyTapesLog.Trace("client delete deck=" + m_NimbyTapesNetId + " effects=" + s_NimbyTapesEffects.ToString());
        }
    }

    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionTurnOnWhileInHands);
        AddAction(ActionTurnOffWhileInHands);
        AddAction(ActionTurnOnWhileOnGround);
        AddAction(ActionTurnOffWhileOnGround);
        AddAction(ActionNimbyTapesNextInHands);
        AddAction(ActionNimbyTapesPreviousInHands);
        AddAction(ActionNimbyTapesNextOnGround);
        AddAction(ActionNimbyTapesPreviousOnGround);
        AddAction(ActionNimbyTapesDebugRuin);
        AddAction(ActionNimbyTapesDebugDelete);
    }

    // True while a track is published. Reads the synced state, so it answers on both sides;
    // the skip actions show only while it is true (T-6: both do nothing while the deck is off).
    bool NimbyTapesIsPlaying()
    {
        return !NimbyTapesRules.IsStopped(NimbyTapesRules.UnpackTrack(m_NimbyTapesState));
    }

    // The deck's id for a log line written outside this class.
    string NimbyTapesLogId()
    {
        return NimbyTapesId();
    }

    // ---- shared helpers -----------------------------------------------------------------------

    protected string NimbyTapesId()
    {
        m_NimbyTapesNetId = GetNetworkIDString();
        return m_NimbyTapesNetId;
    }

    protected bool NimbyTapesOnServer()
    {
        if (!GetGame())
        {
            return false;
        }
        return GetGame().IsServer();
    }

    protected bool NimbyTapesOnClient()
    {
        if (!GetGame())
        {
            return false;
        }
        return !GetGame().IsDedicatedServer();
    }

    protected NimbyTapes_Tape_Base NimbyTapesGetTape()
    {
        return NimbyTapes_Tape_Base.Cast(FindAttachmentBySlotName(SLOT_TAPE));
    }

    protected bool NimbyTapesSlotIs(string slotName, string wanted)
    {
        string left = slotName;
        string right = wanted;
        left.ToLower();
        right.ToLower();
        return left == right;
    }

    // "deck=<net id> gen=<g> track=<i> elapsed=<s>" for a debug line.
    protected string NimbyTapesFields(int generation, int track, int elapsed)
    {
        return "deck=" + NimbyTapesId() + " gen=" + generation.ToString() + " track=" + track.ToString() + " elapsed=" + elapsed.ToString();
    }

    // ---- server: energy manager events --------------------------------------------------------

    // Where an OnWorkStart came from, for test row T0-20: "attach" inside one of the deck's own
    // attach events, "live" once the deck is initialized, otherwise "storeload" (before
    // initialization and outside an attach event, which is the storage load or a scripted
    // switch-on in the first frames of a spawn).
    protected string NimbyTapesPhase()
    {
        if (m_NimbyTapesInAttach)
        {
            return "attach";
        }
        if (IsInitialized())
        {
            return "live";
        }
        return "storeload";
    }

    protected string NimbyTapesWorkFields(string phase)
    {
        bool hasTape = NimbyTapesGetTape() != null;
        bool hasBattery = FindAttachmentBySlotName(SLOT_BATTERY) != null;
        return "init=" + NimbyTapesLog.Flag(IsInitialized()) + " tape=" + NimbyTapesLog.Flag(hasTape) + " battery=" + NimbyTapesLog.Flag(hasBattery) + " phase=" + phase;
    }

    override void EEItemAttached(EntityAI item, string slot_name)
    {
        // The energy manager plugs the deck into a battery inside super, which can raise
        // OnWorkStart from here (recon, Question 8, step 3).
        m_NimbyTapesInAttach = true;
        super.EEItemAttached(item, slot_name);
        m_NimbyTapesInAttach = false;
    }

    override void EEItemDetached(EntityAI item, string slot_name)
    {
        bool onServer = NimbyTapesOnServer();
        bool isTape = NimbyTapes_Tape_Base.Cast(item) != null;
        if (onServer && m_NimbyTapesPlaying)
        {
            // Stop here, before super unplugs the battery, so the reason is the real one. The
            // OnWorkStop that follows finds nothing playing.
            if (isTape)
            {
                NimbyTapesServerStop("no-tape");
            }
            else if (NimbyTapesSlotIs(slot_name, SLOT_BATTERY))
            {
                NimbyTapesServerStop("power");
            }
        }
        super.EEItemDetached(item, slot_name);
        if (onServer && isTape && !IsPendingDeletion() && HasEnergyManager())
        {
            m_NimbyTapesWorkPending = false;
            if (GetCompEM().IsSwitchedOn())
            {
                GetCompEM().SwitchOff();
            }
        }
    }

    override void OnWorkStart()
    {
        super.OnWorkStart();
        if (!NimbyTapesOnServer())
        {
            return;
        }
        if (m_NimbyTapesPlaying)
        {
            return;
        }
        string phase = NimbyTapesPhase();
        if (!IsInitialized())
        {
            m_NimbyTapesWorkPending = true;
            NimbyTapesLog.Trace("defer deck=" + NimbyTapesId() + " " + NimbyTapesWorkFields(phase));
            return;
        }
        NimbyTapesServerStart("switch-on", phase);
    }

    override void OnWorkStop()
    {
        super.OnWorkStop();
        if (!NimbyTapesOnServer())
        {
            return;
        }
        m_NimbyTapesWorkPending = false;
        // A refused start switches the deck off from inside OnWorkStart, and this event
        // follows it with nothing playing (recon, Question 1).
        if (!m_NimbyTapesPlaying)
        {
            return;
        }
        string reason = "switch-off";
        if (IsRuined())
        {
            reason = "ruined";
        }
        else if (!GetCompEM().CanWork())
        {
            reason = "power";
        }
        NimbyTapesServerStop(reason);
    }

    override void AfterStoreLoad()
    {
        super.AfterStoreLoad();
        if (!NimbyTapesOnServer())
        {
            return;
        }
        NimbyTapesServerResume("after-load", true);
    }

    override void DeferredInit()
    {
        super.DeferredInit();
        if (NimbyTapesOnServer())
        {
            NimbyTapesServerResume("deferred-init", false);
        }
        if (NimbyTapesOnClient())
        {
            NimbyTapesLog.Trace("client create deck=" + NimbyTapesId() + " effects=" + s_NimbyTapesEffects.ToString());
        }
    }

    override void EEHealthLevelChanged(int oldLevel, int newLevel, string zone)
    {
        super.EEHealthLevelChanged(oldLevel, newLevel, zone);
        if (!NimbyTapesOnServer())
        {
            return;
        }
        if (newLevel != GameConstants.STATE_RUINED)
        {
            return;
        }
        if (m_NimbyTapesPlaying)
        {
            NimbyTapesServerStop("ruined");
        }
        m_NimbyTapesWorkPending = false;
        if (HasEnergyManager() && GetCompEM().IsSwitchedOn())
        {
            GetCompEM().SwitchOff();
        }
    }

    override void EEDelete(EntityAI parent)
    {
        if (NimbyTapesOnServer())
        {
            if (m_NimbyTapesServerTimer)
            {
                m_NimbyTapesServerTimer.Stop();
            }
            if (m_NimbyTapesPlaying)
            {
                NimbyTapesLog.Trace("stop " + NimbyTapesFields(m_NimbyTapesGeneration, m_NimbyTapesTrack, 0) + " reason=deleted");
            }
            m_NimbyTapesPlaying = false;
            m_NimbyTapesWorkPending = false;
        }
        if (NimbyTapesOnClient())
        {
            // The "client delete" line comes from the destructor, which also runs when the
            // deck only leaves this client's bubble.
            NimbyTapesClientTeardown("stop");
        }
        super.EEDelete(parent);
    }

    // ---- server: playback state ---------------------------------------------------------------

    // The work that OnWorkStart put off. hard is true from AfterStoreLoad, where every child
    // has loaded and missing power or a missing tape is final; false from DeferredInit, which
    // can run before a loading deck has its battery or its tape, and so only starts or waits.
    protected void NimbyTapesServerResume(string reason, bool hard)
    {
        if (m_NimbyTapesPlaying)
        {
            return;
        }
        if (!HasEnergyManager())
        {
            return;
        }
        if (!GetCompEM().IsSwitchedOn())
        {
            // T-9: a deck that was off stays off.
            m_NimbyTapesWorkPending = false;
            return;
        }
        if (!GetCompEM().CanWork())
        {
            if (!hard)
            {
                // CanWork() is false while no battery is plugged (ComponentEnergyManager.c:
                // 768-851). The battery's attach raises OnWorkStart, or AfterStoreLoad decides.
                NimbyTapesLog.Trace("wait deck=" + NimbyTapesId() + " reason=no-power-yet " + NimbyTapesWorkFields(reason));
                return;
            }
            // T-9: switched on with no power. Off, so it does not start by itself later.
            m_NimbyTapesWorkPending = false;
            NimbyTapesLog.Trace("stop " + NimbyTapesFields(m_NimbyTapesGeneration, NimbyTapesRules.TRACK_STOPPED, 0) + " reason=power " + NimbyTapesWorkFields(reason));
            GetCompEM().SwitchOff();
            return;
        }
        if (!m_NimbyTapesWorkPending)
        {
            // Switched on and powered, but OnWorkStart has not come yet. When it comes the
            // deck is initialized and starts on the live path.
            NimbyTapesLog.Trace("wait deck=" + NimbyTapesId() + " reason=no-work-start " + NimbyTapesWorkFields(reason));
            return;
        }
        if (!hard && !NimbyTapesGetTape())
        {
            NimbyTapesLog.Trace("wait deck=" + NimbyTapesId() + " reason=no-tape-yet " + NimbyTapesWorkFields(reason));
            return;
        }
        NimbyTapesServerStart(reason, reason);
    }

    // Validates the tape and starts its remembered track from the beginning, or switches off.
    protected void NimbyTapesServerStart(string reason, string phase)
    {
        m_NimbyTapesWorkPending = false;
        NimbyTapes_Tape_Base tape = NimbyTapesGetTape();
        if (!tape)
        {
            NimbyTapesServerRefuse("no-tape", phase);
            return;
        }
        m_NimbyTapesSets = tape.NimbyTapesGetTrackSets();
        m_NimbyTapesSeconds = tape.NimbyTapesGetTrackSeconds();
        string fault;
        if (!NimbyTapesRules.ValidateTape(m_NimbyTapesSets, m_NimbyTapesSeconds, fault))
        {
            // A tape whose config cannot play is a fault worth a production line. The Phase 0
            // probe tape, which has no track on purpose, is the exception.
            if (fault != "config-empty")
            {
                NimbyTapesLog.Info("tape refused; tape=" + tape.GetType() + " reason=" + fault);
            }
            NimbyTapesServerRefuse(fault, phase);
            return;
        }
        int track = NimbyTapesRules.ResolveRememberedTrack(tape.NimbyTapesGetRememberedTrack(), m_NimbyTapesSets.Count());
        NimbyTapesServerPlayTrack(tape, track, "start", reason + " " + NimbyTapesWorkFields(phase));
        if (!m_NimbyTapesServerTimer)
        {
            m_NimbyTapesServerTimer = new Timer();
        }
        m_NimbyTapesServerTimer.Run(SERVER_TICK_SECONDS, this, "NimbyTapesServerTick", null, true);
    }

    protected void NimbyTapesServerRefuse(string reason, string phase)
    {
        NimbyTapesLog.Trace("stop " + NimbyTapesFields(m_NimbyTapesGeneration, NimbyTapesRules.TRACK_STOPPED, 0) + " reason=" + reason + " " + NimbyTapesWorkFields(phase));
        if (HasEnergyManager() && GetCompEM().IsSwitchedOn())
        {
            GetCompEM().SwitchOff();
        }
    }

    // Starts one track from its beginning: a new generation, the clock mark, the tape's memory
    // (T-5), and one publish. verb is start or advance; an advance carries the reason
    // track-end, next, or previous.
    protected void NimbyTapesServerPlayTrack(NimbyTapes_Tape_Base tape, int track, string verb, string reason)
    {
        m_NimbyTapesTrack = track;
        m_NimbyTapesTrackStart = GetGame().GetTickTime();
        m_NimbyTapesGeneration = NimbyTapesRules.NextGeneration(m_NimbyTapesGeneration);
        m_NimbyTapesPlaying = true;
        if (tape)
        {
            tape.NimbyTapesSetRememberedTrack(track);
        }
        NimbyTapesServerPublish(0);
        NimbyTapesLog.Trace(verb + " " + NimbyTapesFields(m_NimbyTapesGeneration, track, 0) + " reason=" + reason);
    }

    // One assignment and one SetSynchDirty per change, so a client never sees half an update.
    protected void NimbyTapesServerPublish(int elapsed)
    {
        int packed = NimbyTapesRules.Pack(m_NimbyTapesGeneration, m_NimbyTapesTrack, elapsed);
        if (packed == m_NimbyTapesState)
        {
            return;
        }
        m_NimbyTapesState = packed;
        SetSynchDirty();
    }

    // The 1-second timer. Public because the timer calls it by name.
    void NimbyTapesServerTick()
    {
        if (!m_NimbyTapesPlaying)
        {
            if (m_NimbyTapesServerTimer)
            {
                m_NimbyTapesServerTimer.Stop();
            }
            return;
        }
        NimbyTapes_Tape_Base tape = NimbyTapesGetTape();
        if (!tape || !m_NimbyTapesSeconds || m_NimbyTapesTrack < 0 || m_NimbyTapesTrack >= m_NimbyTapesSeconds.Count())
        {
            NimbyTapesServerStop("no-tape");
            if (HasEnergyManager() && GetCompEM().IsSwitchedOn())
            {
                GetCompEM().SwitchOff();
            }
            return;
        }
        float elapsed = GetGame().GetTickTime() - m_NimbyTapesTrackStart;
        int length = m_NimbyTapesSeconds.Get(m_NimbyTapesTrack);
        if (elapsed >= length)
        {
            int following = NimbyTapesRules.NextTrack(m_NimbyTapesTrack, m_NimbyTapesSeconds.Count());
            if (NimbyTapesRules.IsStopped(following))
            {
                NimbyTapesServerEndTape(tape);
                return;
            }
            NimbyTapesServerPlayTrack(tape, following, "advance", "track-end");
            return;
        }
        int whole = Math.Floor(elapsed);
        NimbyTapesServerPublish(whole);
        if (NimbyTapesLog.IsDebug())
        {
            NimbyTapesLog.Trace("publish " + NimbyTapesFields(m_NimbyTapesGeneration, m_NimbyTapesTrack, whole));
        }
    }

    // T-6 and T-5: after the last track the deck switches off and the tape's memory returns to
    // the first track.
    protected void NimbyTapesServerEndTape(NimbyTapes_Tape_Base tape)
    {
        if (tape)
        {
            tape.NimbyTapesSetRememberedTrack(0);
        }
        NimbyTapesServerStop("end-of-tape");
        if (HasEnergyManager() && GetCompEM().IsSwitchedOn())
        {
            GetCompEM().SwitchOff();
        }
    }

    // ---- server: skip (T-6) -------------------------------------------------------------------

    // The server-side check every skip repeats at execution: the deck is whole, switched on,
    // playing, and still holds the tape it started from.
    protected bool NimbyTapesServerCanSkip()
    {
        if (!NimbyTapesOnServer())
        {
            return false;
        }
        if (IsRuined())
        {
            return false;
        }
        if (!m_NimbyTapesPlaying)
        {
            return false;
        }
        if (!HasEnergyManager() || !GetCompEM().IsSwitchedOn())
        {
            return false;
        }
        if (!NimbyTapesGetTape())
        {
            return false;
        }
        if (!m_NimbyTapesSeconds || m_NimbyTapesTrack < 0 || m_NimbyTapesTrack >= m_NimbyTapesSeconds.Count())
        {
            return false;
        }
        return true;
    }

    // Next: the following track from its beginning, or the end of the tape on the last track.
    void NimbyTapesServerNext()
    {
        if (!NimbyTapesServerCanSkip())
        {
            return;
        }
        NimbyTapes_Tape_Base tape = NimbyTapesGetTape();
        int following = NimbyTapesRules.NextTrack(m_NimbyTapesTrack, m_NimbyTapesSeconds.Count());
        if (NimbyTapesRules.IsStopped(following))
        {
            NimbyTapesServerEndTape(tape);
            return;
        }
        NimbyTapesServerPlayTrack(tape, following, "advance", "next");
    }

    // Previous: the same track from its beginning when more than 3 seconds in, otherwise the
    // one before it. The elapsed time is taken unrounded (rules, PreviousTrack).
    void NimbyTapesServerPrevious()
    {
        if (!NimbyTapesServerCanSkip())
        {
            return;
        }
        NimbyTapes_Tape_Base tape = NimbyTapesGetTape();
        float elapsed = GetGame().GetTickTime() - m_NimbyTapesTrackStart;
        int target = NimbyTapesRules.PreviousTrack(m_NimbyTapesTrack, elapsed);
        if (NimbyTapesRules.IsStopped(target))
        {
            return;
        }
        NimbyTapesServerPlayTrack(tape, target, "advance", "previous");
    }

    // Stops the timer and publishes stopped. Does nothing when nothing plays.
    protected void NimbyTapesServerStop(string reason)
    {
        if (m_NimbyTapesServerTimer)
        {
            m_NimbyTapesServerTimer.Stop();
        }
        m_NimbyTapesWorkPending = false;
        if (!m_NimbyTapesPlaying)
        {
            return;
        }
        float elapsed = GetGame().GetTickTime() - m_NimbyTapesTrackStart;
        int whole = Math.Floor(elapsed);
        NimbyTapesLog.Trace("stop " + NimbyTapesFields(m_NimbyTapesGeneration, m_NimbyTapesTrack, whole) + " reason=" + reason);
        m_NimbyTapesPlaying = false;
        m_NimbyTapesTrack = NimbyTapesRules.TRACK_STOPPED;
        NimbyTapesServerPublish(0);
    }

    // ---- client: reconciliation ---------------------------------------------------------------

    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();
        if (!NimbyTapesOnClient())
        {
            return;
        }
        NimbyTapesClientReconcile();
    }

    // Plan 13, Architecture, the client reconciliation table.
    protected void NimbyTapesClientReconcile()
    {
        int generation = NimbyTapesRules.UnpackGeneration(m_NimbyTapesState);
        int track = NimbyTapesRules.UnpackTrack(m_NimbyTapesState);
        int elapsed = NimbyTapesRules.UnpackElapsed(m_NimbyTapesState);
        NimbyTapesClientNoteEnded();
        if (NimbyTapesRules.IsStopped(track))
        {
            m_NimbyTapesHoldChecks = -1;
            m_NimbyTapesQuietGeneration = -1;
            NimbyTapesClientTeardown("stop");
            return;
        }
        if (m_NimbyTapesEffect && m_NimbyTapesLocalGeneration == generation && m_NimbyTapesLocalTrack == track)
        {
            // Same generation, already playing: a progress update never restarts a wave.
            NimbyTapesClientTraceSync(generation, track, elapsed);
            return;
        }
        NimbyTapes_Tape_Base tape = NimbyTapesGetTape();
        if (!tape)
        {
            // The tape attachment can reach a client after the deck's variables do. Hold the
            // state and look again on the 0.5-second check, for 5 seconds, then stay silent
            // for this generation.
            // Whatever still plays belongs to an older generation.
            NimbyTapesClientTeardown("replace");
            if (m_NimbyTapesQuietGeneration == generation)
            {
                return;
            }
            if (m_NimbyTapesHoldChecks < 0)
            {
                m_NimbyTapesHoldChecks = 0;
                NimbyTapesLog.Trace("client hold " + NimbyTapesFields(generation, track, elapsed) + " reason=no-tape");
            }
            NimbyTapesClientRunTimer();
            return;
        }
        m_NimbyTapesHoldChecks = -1;
        array<string> trackSets = tape.NimbyTapesGetTrackSets();
        array<int> trackSeconds = tape.NimbyTapesGetTrackSeconds();
        string fault;
        bool playable = NimbyTapesRules.ValidateTape(trackSets, trackSeconds, fault);
        if (playable && track >= trackSets.Count())
        {
            playable = false;
            fault = "track-out-of-range";
        }
        if (!playable)
        {
            NimbyTapesClientFail(generation, track, fault);
            NimbyTapesClientTeardown("stop");
            return;
        }
        // A new generation, or the same one with nothing playing (the wave ended early or a
        // start failed): replace whatever plays and start at the published elapsed time.
        NimbyTapesClientTeardown("replace");
        NimbyTapesClientStart(trackSets.Get(track), generation, track, elapsed, trackSeconds.Get(track));
    }

    // Logs a client start failure once per generation; the next sync tries again.
    protected void NimbyTapesClientFail(int generation, int track, string reason)
    {
        if (m_NimbyTapesFailGeneration == generation)
        {
            return;
        }
        m_NimbyTapesFailGeneration = generation;
        NimbyTapesLog.Info("client cannot play; deck=" + NimbyTapesId() + " gen=" + generation.ToString() + " track=" + track.ToString() + " reason=" + reason);
    }

    protected void NimbyTapesClientTraceSync(int generation, int track, int elapsed)
    {
        if (!NimbyTapesLog.IsDebug())
        {
            return;
        }
        string position = "none";
        AbstractWave wave = m_NimbyTapesEffect.NimbyTapesGetWave();
        if (wave)
        {
            // The wave's position as a share of its length (recon, Question 3). Times the
            // track's length, against elapsed, it is this client's skew.
            position = wave.GetCurrPosition().ToString();
        }
        NimbyTapesLog.Trace("client sync " + NimbyTapesFields(generation, track, elapsed) + " pos=" + position + " effects=" + s_NimbyTapesEffects.ToString());
    }

    // ---- client: playback ---------------------------------------------------------------------

    protected Object NimbyTapesRoot()
    {
        EntityAI root = GetHierarchyRoot();
        if (root)
        {
            return root;
        }
        return this;
    }

    protected void NimbyTapesClientStart(string soundSet, int generation, int track, int elapsed, int length)
    {
        float fraction;
        if (!NimbyTapesRules.StartFraction(elapsed, length, fraction))
        {
            // At or past the end of the track: the server is about to advance or stop.
            return;
        }
        Object root = NimbyTapesRoot();
        // The order of the vanilla PlaySoundOnObject (EffectManager.c:256-263), with autodestroy
        // set before the play.
        EffectSound effect = SEffectManager.CreateSound(soundSet, root.GetPosition(), 0, 0, false);
        if (!effect)
        {
            NimbyTapesClientFail(generation, track, "no-effect");
            return;
        }
        effect.SetParent(root);
        effect.SetLocalPosition(vector.Zero);
        effect.SetSoundAutodestroy(true);
        if (!effect.SoundPlay())
        {
            SEffectManager.DestroyEffect(effect);
            NimbyTapesClientFail(generation, track, "play-refused set=" + soundSet);
            return;
        }
        AbstractWave wave = effect.NimbyTapesGetWave();
        if (!wave)
        {
            SEffectManager.DestroyEffect(effect);
            NimbyTapesClientFail(generation, track, "no-wave set=" + soundSet);
            return;
        }
        m_NimbyTapesEffect = effect;
        m_NimbyTapesEffectCounted = true;
        s_NimbyTapesEffects = s_NimbyTapesEffects + 1;
        m_NimbyTapesParent = root;
        m_NimbyTapesLocalGeneration = generation;
        m_NimbyTapesLocalTrack = track;
        float probe = NimbyTapesLog.ProbeOffset();
        if (probe >= 0)
        {
            // Unit probe only (recon, Question 3): the debug file forces the value.
            wave.SetStartOffset(probe);
            NimbyTapesLog.Trace("client seek " + NimbyTapesFields(generation, track, elapsed) + " probe=" + probe.ToString());
        }
        else if (elapsed > 0)
        {
            wave.SetStartOffset(fraction);
            NimbyTapesLog.Trace("client seek " + NimbyTapesFields(generation, track, elapsed) + " offset=" + elapsed.ToString() + " fraction=" + fraction.ToString());
        }
        NimbyTapesLog.Trace("client play " + NimbyTapesFields(generation, track, elapsed) + " offset=" + elapsed.ToString() + " set=" + soundSet + " effects=" + s_NimbyTapesEffects.ToString());
        NimbyTapesClientRunTimer();
    }

    // Lets go of the effect: counter, stop, destroy, clear. Safe to call twice, and safe from
    // the destructor (no entity call, no log line).
    protected void NimbyTapesClientRelease()
    {
        EffectSound effect = m_NimbyTapesEffect;
        m_NimbyTapesEffect = null;
        m_NimbyTapesParent = null;
        if (m_NimbyTapesEffectCounted)
        {
            m_NimbyTapesEffectCounted = false;
            s_NimbyTapesEffects = s_NimbyTapesEffects - 1;
        }
        if (effect)
        {
            effect.SoundStop();
            SEffectManager.DestroyEffect(effect);
        }
    }

    // The one teardown path (plan risk K4). verb is stop or replace.
    protected void NimbyTapesClientTeardown(string verb)
    {
        bool counted = m_NimbyTapesEffectCounted;
        int generation = m_NimbyTapesLocalGeneration;
        int track = m_NimbyTapesLocalTrack;
        NimbyTapesClientRelease();
        m_NimbyTapesLocalGeneration = -1;
        m_NimbyTapesLocalTrack = NimbyTapesRules.TRACK_STOPPED;
        if (counted)
        {
            NimbyTapesLog.Trace("client " + verb + " deck=" + NimbyTapesId() + " gen=" + generation.ToString() + " track=" + track.ToString() + " effects=" + s_NimbyTapesEffects.ToString());
        }
    }

    // An effect that played to its end destroyed itself, which cleared the reference. Bring the
    // counter back in step.
    protected void NimbyTapesClientNoteEnded()
    {
        if (!m_NimbyTapesEffectCounted)
        {
            return;
        }
        if (m_NimbyTapesEffect)
        {
            return;
        }
        m_NimbyTapesEffectCounted = false;
        s_NimbyTapesEffects = s_NimbyTapesEffects - 1;
        m_NimbyTapesParent = null;
        NimbyTapesLog.Trace("client ended deck=" + NimbyTapesId() + " gen=" + m_NimbyTapesLocalGeneration.ToString() + " track=" + m_NimbyTapesLocalTrack.ToString() + " effects=" + s_NimbyTapesEffects.ToString());
    }

    // ---- client: following the carrier --------------------------------------------------------

    // Moves the sound to the deck's hierarchy root when that changed (T-2).
    protected void NimbyTapesClientFollow(string trigger)
    {
        if (!m_NimbyTapesEffect)
        {
            return;
        }
        Object root = NimbyTapesRoot();
        if (root == m_NimbyTapesParent)
        {
            return;
        }
        m_NimbyTapesParent = root;
        m_NimbyTapesEffect.SetParent(root);
        m_NimbyTapesEffect.SetCurrentLocalPosition(vector.Zero);
        NimbyTapesLog.Trace("client reparent deck=" + NimbyTapesId() + " gen=" + m_NimbyTapesLocalGeneration.ToString() + " track=" + m_NimbyTapesLocalTrack.ToString() + " trigger=" + trigger + " parent=" + root.GetType() + " effects=" + s_NimbyTapesEffects.ToString());
    }

    override void EEItemLocationChanged(notnull InventoryLocation oldLoc, notnull InventoryLocation newLoc)
    {
        super.EEItemLocationChanged(oldLoc, newLoc);
        if (NimbyTapesOnClient())
        {
            NimbyTapesClientFollow("location");
        }
    }

    override void OnInventoryEnter(Man player)
    {
        super.OnInventoryEnter(player);
        if (NimbyTapesOnClient())
        {
            NimbyTapesClientFollow("enter");
        }
    }

    override void OnInventoryExit(Man player)
    {
        super.OnInventoryExit(player);
        if (NimbyTapesOnClient())
        {
            NimbyTapesClientFollow("exit");
        }
    }

    protected void NimbyTapesClientRunTimer()
    {
        if (!m_NimbyTapesClientTimer)
        {
            m_NimbyTapesClientTimer = new Timer();
        }
        if (!m_NimbyTapesClientTimer.IsRunning())
        {
            m_NimbyTapesClientTimer.Run(CLIENT_CHECK_SECONDS, this, "NimbyTapesClientCheck", null, true);
        }
    }

    // The 0.5-second client check: a held start, an ended effect, and the root of a playing
    // one (a parent container can move without the deck's own location changing). Public
    // because the timer calls it by name.
    void NimbyTapesClientCheck()
    {
        NimbyTapesClientNoteEnded();
        if (m_NimbyTapesHoldChecks >= 0)
        {
            m_NimbyTapesHoldChecks = m_NimbyTapesHoldChecks + 1;
            if (m_NimbyTapesHoldChecks > CLIENT_HOLD_CHECKS)
            {
                m_NimbyTapesHoldChecks = -1;
                m_NimbyTapesQuietGeneration = NimbyTapesRules.UnpackGeneration(m_NimbyTapesState);
                NimbyTapesClientFail(m_NimbyTapesQuietGeneration, NimbyTapesRules.UnpackTrack(m_NimbyTapesState), "tape-not-present");
            }
            else
            {
                NimbyTapesClientReconcile();
            }
        }
        if (m_NimbyTapesEffect)
        {
            NimbyTapesClientFollow("check");
            return;
        }
        if (m_NimbyTapesHoldChecks < 0 && m_NimbyTapesClientTimer)
        {
            m_NimbyTapesClientTimer.Stop();
        }
    }
}
