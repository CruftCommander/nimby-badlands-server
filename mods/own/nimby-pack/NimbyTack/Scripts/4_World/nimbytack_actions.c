// -----------------------------------------------------------------------------
// NimbyTack - the pin actions (T2), the read action (T3), and the take-down action (T4).
// Purpose : Four continuous actions, one per fastener, each with the fastener in
//           hands as the main item and the wall under the cursor as the target;
//           one continuous action on a pinned note that opens its text in place;
//           one continuous action on a pinned note that returns a sheet. All on the
//           vanilla action framework (shared rules 4.1: no RPC of our own; the read
//           reuses vanilla ERPCs.RPC_READ_NOTE, which ItemBase routes to
//           WrittenNoteData.OnRPC and the NoteMenu).
// Vanilla : ActionContinuousBase hooks ActionContinuousBase.c:289 (OnFinishProgressServer);
//           CCTCursor and CCINonRuined as in ActionCreateIndoorFireplace.c;
//           CMD_ACTIONFB_ASSEMBLE is the hammering loop (ActionBuildPart.c:38),
//           CMD_ACTIONFB_INTERACT the generic one (ActionBuildPart.c:255).
// Server  : The client evaluates ActionCondition with the default config so the
//           prompt appears; the server re-evaluates everything with its own config
//           in OnFinishProgressServer and refuses with a status message, so a
//           client cannot pin where the server says no (T8).
// Labels  : Plain English action text (T9); the Menu addon localizes later.
// Part B  : With the debug flag the pin actions write one pin-check line per
//           change of (reason, target) on the client, and one per request on the
//           server, naming the first check that refused (T-014 study, October 20,
//           2026). Without the flag Can is vanilla's and the client builds no line
//           and no detail string per frame; the server builds the probe's refusal
//           detail once per request for its production refuse pin line.
// Guard   : NIMBY-T-015 (T17, October 21, 2026): a probe refused with reason
//           physics-ahead gets its own status message, because the wall is there
//           but the note would be buried inside it.
// Hold    : NIMBY-T-022 and T-024 (T18, Jeff, October 24, 2026). Vanilla freezes the
//           action target at setup (ActionBase.c:174) but re-runs the target condition
//           and ActionConditionContinue on every tick (ActionBase.c:975-990). Tack's
//           condition re-ran the wall probe from the moving head and the full-body
//           animation cancelled the hold at the edge of reach. Now the full check
//           runs when the action starts; during the hold CCTNimbyTackHold confirms
//           the same target within NimbyTackRules.HOLD_REACH_METERS of the frozen
//           cursor point, the pin re-checks only its tool, and the server pins from
//           the hit it probed at the start (NimbyTackPinActionData). The burn is a
//           Tack action (ActionNimbyTackBurn) under the same rule; vanilla Ignite no
//           longer offers on a pinned note. With the debug flag, a stopped hold
//           writes one hold-cancel line naming why.
// Place   : October 25, 2026 placement round (plan v0.15): the finish measures where
//           the note goes before creating it (NimbyTackPlace.Compute: post centring
//           T20, fire-relative offset T19) and attaches a door note (T21b). T24
//           (late evening): a door leaf pins open or closed; the door-not-closed
//           refusal is gone.
// Refusals: T26, NIMBY-T-047 (Jeff, October 25, 2026): a refusal the player could read
//           as a buried note shows as the top-right notification box
//           (NotificationSystem.SendNotificationToPlayerExtended, NotificationSystem.c:125),
//           not the small status line; every production refuse pin line names the
//           wall class and door index. The area cap is counted on the client in the
//           condition (CountPinnedNear over the networked notes, at most every
//           CAP_CHECK_SECONDS) so the prompt withdraws before the animation and a
//           local box says why, once per approach (ClientNotice), as does a knife on
//           a non-wood wall; the server count at the finish stays the authority. A stack with no single sheet stays silent
//           (Jeff: vanilla style, option A). T27: config refusalNotices 0 turns every
//           box off; the client reads it from the net-synced flag on PlayerBase.
// -----------------------------------------------------------------------------

// T18: a hold-cancel line, debug only, written once (the action ends on the first false).
class NimbyTackHold
{
    static bool Refuse(string reason, string detail, PlayerBase player)
    {
        if (NimbyTackLog.IsDebug())
        {
            NimbyTackLog.Debug("hold-cancel reason=" + reason + " detail=" + detail + " server=" + NimbyTackLog.Bool(GetGame().IsServer()) + " " + NimbyTackWorld.PlayerTag(player));
        }
        return false;
    }
}

// T18: vanilla's cursor condition when the action starts (CCTCursor.c:15-36, 2.0 m from the
// head or the feet); during the hold, the same target, still there, with its frozen cursor point
// within HOLD_REACH_METERS of the head or the feet. The cursor point does not move during an
// action, so the live aim (a chain-link mesh's gaps, TK-F6) cannot cancel it.
class CCTNimbyTackHold : CCTCursor
{
    override bool CanContinue(PlayerBase player, ActionTarget target)
    {
        if (!player || !target)
        {
            return NimbyTackHold.Refuse("no-target", "-", player);
        }
        Object targetObject = target.GetObject();
        if (!targetObject)
        {
            targetObject = target.GetParent();
        }
        if (!targetObject)
        {
            return NimbyTackHold.Refuse("target-gone", "-", player);
        }
        if (targetObject.IsDamageDestroyed())
        {
            return NimbyTackHold.Refuse("target-destroyed", NimbyTackWorld.ClassOf(targetObject), player);
        }
        EntityAI entity = EntityAI.Cast(targetObject);
        if (entity && entity.IsSetForDeletion())
        {
            return NimbyTackHold.Refuse("target-deleted", NimbyTackWorld.ClassOf(targetObject), player);
        }
        vector cursor = target.GetCursorHitPos();
        float head = vector.Distance(MiscGameplayFunctions.GetPlayerHeadPosition(player), cursor);
        float feet = vector.Distance(player.GetPosition(), cursor);
        if (NimbyTackRules.WithinHoldReach(head, feet, NimbyTackRules.HOLD_REACH_METERS))
        {
            return true;
        }
        string detail = "-";
        if (NimbyTackLog.IsDebug())
        {
            detail = "head=" + head.ToString() + " feet=" + feet.ToString() + " limit=" + NimbyTackRules.HOLD_REACH_METERS.ToString();
        }
        return NimbyTackHold.Refuse("reach", detail, player);
    }
}

// T18: the wall hit the server probed when the pin started, used again at the finish.
class NimbyTackPinActionData : ActionData
{
    ref NimbyTackHit m_NimbyHit;
    // m_NimbyStartProbed is set by OnStartServer; until then the hold check does not judge the hit.
    bool m_NimbyStartProbed;
    bool m_NimbyHitValid;
    string m_NimbyProbeReason;
    string m_NimbyProbeDetail;
}
class ActionNimbyTackPinCB : ActionContinuousBaseCB
{
    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime(ActionNimbyTackPinBase.PIN_SECONDS);
    }
}

class ActionNimbyTackPinBase : ActionContinuousBase
{
    static const float PIN_SECONDS = 2.0;
    // Part B (B1): at most one client pin-check line per this many seconds.
    static const float CHECK_FLOOR_SECONDS = 0.25;
    // Part B (B1): set by ActionCondition when Tack's own checks all passed, so a refusal after
    // it is the stance check that follows (ActionBase.c:957-963).
    static const string REASON_PASSED = "condition-passed";
    // Part B (B1): the reason and detail of the evaluation in progress. Actions are one instance
    // per class (ActionManagerBase.c:50), shared by every player on the server; the fields are
    // written and read inside one synchronous Can call, so that is safe. The gate fields are used
    // only on the client, which has one player.
    protected string m_NimbyReason = "";
    protected string m_NimbyDetail = "-";
    protected string m_NimbyLastKey = "";
    protected float m_NimbyLastSeconds = -1000.0;
    // T26: the client's area-cap count is refreshed at most this often while the prompt is
    // evaluated (one GetObjectsAtPosition3D per refresh, not per frame), and its box shows at most
    // this often.
    static const float CAP_CHECK_SECONDS = 0.5;
    // T26: a client-side box shows once per approach: when a refusal reason first appears, and
    // again only after the aim has been off that refusal for NOTICE_GAP_SECONDS; never two boxes
    // within NOTICE_MIN_SECONDS (test 117, October 25, 2026: a box every 10 s while aiming read
    // as spam).
    static const float NOTICE_GAP_SECONDS = 1.0;
    static const float NOTICE_MIN_SECONDS = 5.0;
    static const float NOTIFY_SECONDS = 4.0;
    static const string NOTIFY_ICON = "set:dayz_gui image:icon_x";
    protected float m_NimbyCapCheckedAt = -1000.0;
    protected int m_NimbyCapCount = 0;
    protected string m_NimbyNoticeReason = "";
    protected float m_NimbyNoticeSeenAt = -1000.0;
    protected float m_NimbyNoticeShownAt = -1000.0;

    // T26: the notification box, server side to one player (NotificationSystem.c:125). T27: off
    // when the server config says so.
    static void NotifyPlayer(PlayerBase player, string title, string detail)
    {
        if (!player || NimbyTackConfigLoader.Get().refusalNotices == 0)
        {
            return;
        }
        NotificationSystem.SendNotificationToPlayerExtended(player, NOTIFY_SECONDS, title, detail, NOTIFY_ICON);
    }

    // T26: a client-side refusal box, once per approach (see NOTICE_GAP_SECONDS). Called on the
    // client only, from the condition, while the refusal holds.
    protected void ClientNotice(PlayerBase player, string reason, string title, string detail)
    {
        // T27: the server's switch, carried on the player (nimbytack_playerbase.c).
        if (!player || !player.NimbyTackNoticesOn())
        {
            return;
        }
        float now = GetGame().GetTickTime();
        bool fresh = reason != m_NimbyNoticeReason || now - m_NimbyNoticeSeenAt > NOTICE_GAP_SECONDS;
        m_NimbyNoticeReason = reason;
        m_NimbyNoticeSeenAt = now;
        if (!fresh || now - m_NimbyNoticeShownAt < NOTICE_MIN_SECONDS)
        {
            return;
        }
        m_NimbyNoticeShownAt = now;
        NotificationSystem.AddNotificationExtended(NOTIFY_SECONDS, title, detail, NOTIFY_ICON);
    }

    // T26: the client's own count of pinned notes around the hit, refreshed at most every
    // CAP_CHECK_SECONDS; true when the cap is reached. T27: the cap and radius are the server's,
    // carried on the player (nimbytack_playerbase.c), not the client's default config.
    protected bool ClientCapReached(PlayerBase player, vector position)
    {
        if (!player)
        {
            return false;
        }
        float now = GetGame().GetTickTime();
        if (now - m_NimbyCapCheckedAt >= CAP_CHECK_SECONDS)
        {
            m_NimbyCapCount = NimbyTackWorld.CountPinnedNear(position, player.NimbyTackCapRadius());
            m_NimbyCapCheckedAt = now;
        }
        if (NimbyTackRules.WithinAreaCap(m_NimbyCapCount, player.NimbyTackCapCount()))
        {
            return false;
        }
        ClientNotice(player, "area-cap", "Too many notes nearby", "Take one down first.");
        return true;
    }

    void ActionNimbyTackPinBase()
    {
        m_CallbackClass = ActionNimbyTackPinCB;
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
        m_FullBody = true;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
        m_Text = "Pin note";
        // The target is a wall, not an inventory entity. With the default lock the server tries
        // to reserve the target's inventory and logs "Target does not have inventory"
        // (ActionBase.c:1183; seen on Land_Shed_W1, October 17, 2026).
        m_LockTargetOnUse = false;
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINonRuined;
        m_ConditionTarget = new CCTNimbyTackHold;
    }

    override ActionData CreateActionData()
    {
        NimbyTackPinActionData action_data = new NimbyTackPinActionData;
        return action_data;
    }

    // Each fastener class answers these three.
    int GetFastener()
    {
        return -1;
    }

    bool ToolMatches(ItemBase item)
    {
        return false;
    }

    bool ToolHasPortion(ItemBase item, NimbyTackConfig config)
    {
        return true;
    }

    // Part B (B1): the public entry the action inputs call (ActionInput.c:333,343,608; the
    // server's request check, ActionManagerServer.c:168, through the three-argument form,
    // ActionBase.c:968-973). With the debug flag it names the first check that refused; a
    // refusal with no reason set came from the vanilla stage before ActionCondition, which
    // VanillaRefusal re-runs to name. Same precedent: ActionDigInStash.c:29.
    override bool Can(PlayerBase player, ActionTarget target, ItemBase item, int condition_mask)
    {
        if (!NimbyTackLog.IsDebug())
        {
            return super.Can(player, target, item, condition_mask);
        }
        bool client = !GetGame().IsServer();
        m_NimbyReason = "";
        m_NimbyDetail = "-";
        if (client)
        {
            NimbyTackWorld.HoldTrace();
        }
        bool allowed = super.Can(player, target, item, condition_mask);
        string reason = m_NimbyReason;
        string detail = m_NimbyDetail;
        if (allowed)
        {
            reason = "ok";
        }
        else if (reason == "")
        {
            reason = VanillaRefusal(player, target, item, condition_mask);
            detail = m_NimbyDetail;
        }
        else if (reason == REASON_PASSED)
        {
            reason = "stance-change";
        }
        bool write = true;
        if (client)
        {
            string key = reason + "|" + NimbyTackWorld.TargetKey(target);
            float now = GetGame().GetTickTime();
            write = NimbyTackRules.ShouldLogCheck(m_NimbyLastKey, m_NimbyLastSeconds, key, now, CHECK_FLOOR_SECONDS);
            if (write)
            {
                m_NimbyLastKey = key;
                m_NimbyLastSeconds = now;
            }
        }
        if (write)
        {
            LogPinCheck(player, target, allowed, reason, detail);
            bool targeting = reason == "no-target-object" || reason == "cursor-too-far" || reason == "target-not-building";
            if (client && targeting)
            {
                NimbyTackWorld.LogCursorRay(player);
            }
        }
        if (client)
        {
            NimbyTackWorld.ReleaseTrace(write);
        }
        return allowed;
    }

    // Part B (B1): the vanilla checks of ActionBase.Can (ActionBase.c:929-952) in their order,
    // run only after a refusal that Tack's own checks did not explain. Sets m_NimbyDetail.
    protected string VanillaRefusal(PlayerBase player, ActionTarget target, ItemBase item, int condition_mask)
    {
        m_NimbyDetail = "-";
        if (!player)
        {
            return "no-player";
        }
        int refused = NimbyTackRules.RefusedMaskBits(condition_mask, m_ConditionMask);
        if (refused != 0)
        {
            m_NimbyDetail = "mask=" + refused.ToString();
            return "player-state";
        }
        if (player.IsRolling())
        {
            m_NimbyDetail = "rolling";
            return "player-state";
        }
        if (!IsFullBodyEx(player, target, item) && !player.IsPlayerInStance(GetStanceMaskEx(player, target, item)))
        {
            m_NimbyDetail = "stance";
            return "player-state";
        }
        if (!target)
        {
            return "no-target-object";
        }
        EntityAI entity = EntityAI.Cast(target.GetObject());
        if (entity && !entity.IsMan())
        {
            Man holder = entity.GetHierarchyRootPlayer();
            if (holder && holder != player)
            {
                return "target-held";
            }
        }
        // CCTCursor (CCTCursor.c:15-36): object else parent, not destroyed, cursor within 2.0 m of
        // the feet or the head.
        Object targetObject = target.GetObject();
        if (!targetObject)
        {
            targetObject = target.GetParent();
        }
        if (!targetObject)
        {
            return "no-target-object";
        }
        if (targetObject.IsDamageDestroyed())
        {
            return "target-destroyed";
        }
        vector cursor = target.GetCursorHitPos();
        float head = vector.Distance(MiscGameplayFunctions.GetPlayerHeadPosition(player), cursor);
        float feet = vector.Distance(player.GetPosition(), cursor);
        float limit = UAMaxDistances.DEFAULT;
        if (head > limit && feet > limit)
        {
            m_NimbyDetail = "head=" + head.ToString() + " feet=" + feet.ToString() + " limit=" + limit.ToString();
            return "cursor-too-far";
        }
        // CCINonRuined (CCINonRuined.c:3-6).
        if (!item || item.IsDamageDestroyed())
        {
            return "tool-ruined";
        }
        return "vanilla-other";
    }

    protected void LogPinCheck(PlayerBase player, ActionTarget target, bool allowed, string reason, string detail)
    {
        string result = "refuse";
        if (allowed)
        {
            result = "ok";
        }
        string line = "pin-check fastener=" + NimbyTackFastener.Name(GetFastener()) + " result=" + result + " reason=" + reason + " detail=" + detail;
        line = line + " " + NimbyTackWorld.TargetTag(target);
        line = line + " " + NimbyTackWorld.ReachTag(player, target) + " " + NimbyTackWorld.PlayerTag(player);
        NimbyTackLog.Debug(line);
    }

    // Records why ActionCondition refused, for the pin-check line. Free without the debug flag:
    // two string assignments, no detail is built.
    protected bool Refuse(string reason)
    {
        m_NimbyReason = reason;
        return false;
    }

    // Runs on the client every frame while the tool is in hands and on the server when the
    // action is requested. Cheap checks first; the raycast last.
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        m_NimbyReason = "";
        m_NimbyDetail = "-";
        bool debugOn = NimbyTackLog.IsDebug();
        if (!player || !target || !item)
        {
            return Refuse("no-target");
        }
        if (!ToolMatches(item))
        {
            return Refuse("tool-mismatch");
        }
        NimbyTackConfig config = NimbyTackConfigLoader.Get();
        if (!ToolHasPortion(item, config))
        {
            if (debugOn)
            {
                m_NimbyDetail = "quantity=" + item.GetQuantity().ToString();
            }
            return Refuse("no-portion");
        }
        if (GetFastener() == NimbyTackFastener.NAIL && !NimbyTackWorld.FindNail(player))
        {
            return Refuse("no-nail");
        }
        if (!NimbyTackWorld.FindSingleSheet(player))
        {
            if (debugOn)
            {
                m_NimbyDetail = NimbyTackWorld.SheetSummary(player);
            }
            return Refuse("no-single-sheet");
        }
        // Only the knife needs the surface name here (wood test); the debug flag asks for it
        // so the test record sees every surface. Everyone else skips the fire-geometry ray.
        bool wantSurface = GetFastener() == NimbyTackFastener.KNIFE || debugOn;
        NimbyTackHit hit;
        if (!NimbyTackWorld.ProbeWall(player, target, config, wantSurface, hit))
        {
            m_NimbyDetail = NimbyTackWorld.LastProbeDetail();
            string probeReason = NimbyTackWorld.LastProbeReason();
            // Player builds (fences, watchtowers, tents) are ItemBase and refused by the probe
            // until the player-build round lands (review TACK-R2-04; Jeff, October 27, 2026);
            // until then the player at their own base is told why instead of seeing nothing.
            if (probeReason == "target-not-building" && !GetGame().IsServer())
            {
                ClientNotice(player, "target-not-building", "Notes pin on map buildings", "Player-built walls and tents are not supported yet.");
            }
            return Refuse(probeReason);
        }
        if (GetFastener() == NimbyTackFastener.KNIFE && !NimbyTackRules.IsWoodSurface(hit.surface))
        {
            if (debugOn)
            {
                m_NimbyDetail = "surface=" + hit.surface;
            }
            if (!GetGame().IsServer())
            {
                ClientNotice(player, "not-wood", "The knife will not go into that", "Only wood takes a knife.");
            }
            return Refuse("not-wood");
        }
        // T26, NIMBY-T-047: the area cap before the animation, on the client only (the server
        // counts at the finish with its own config).
        if (!GetGame().IsServer() && ClientCapReached(player, hit.position))
        {
            if (debugOn)
            {
                m_NimbyDetail = "nearby=" + m_NimbyCapCount.ToString() + " cap=" + player.NimbyTackCapCount().ToString() + " radius=" + player.NimbyTackCapRadius().ToString();
            }
            return Refuse("area-cap");
        }
        // Part B (B1): what the pin would use. On the client this is the default config, which
        // the server's matches unless its config.json changes the offsets.
        if (debugOn)
        {
            m_NimbyDetail = "surface=" + hit.surface + " fallback-offset=" + NimbyTackRules.OffsetFor(hit.surface, config).ToString() + " door=" + hit.doorIndex.ToString();
        }
        m_NimbyReason = REASON_PASSED;
        return true;
    }

    // T18: during the hold, no wall probe (ActionCondition ran it when the action was offered and
    // requested). Only the tool is re-checked, and on the server a start probe that failed ends
    // the hold early instead of after the full animation.
    override bool ActionConditionContinue(ActionData action_data)
    {
        ItemBase item = action_data.m_MainItem;
        NimbyTackConfig config = NimbyTackConfigLoader.Get();
        if (!item || !ToolMatches(item) || !ToolHasPortion(item, config))
        {
            return NimbyTackHold.Refuse("tool", "fastener=" + NimbyTackFastener.Name(GetFastener()), action_data.m_Player);
        }
        if (GetGame().IsServer())
        {
            NimbyTackPinActionData pinData = NimbyTackPinActionData.Cast(action_data);
            if (pinData && pinData.m_NimbyStartProbed && !pinData.m_NimbyHitValid)
            {
                return NimbyTackHold.Refuse("start-probe", pinData.m_NimbyProbeReason, action_data.m_Player);
            }
        }
        return true;
    }

    // T18: the server's full wall check, once, when the action starts. The request check ran the
    // same probe a moment earlier (ActionManagerServer.c:168 through Can), so this normally
    // passes; it is stored so the finish pins where the check passed, not where the animation has
    // moved the head by then.
    override void OnStartServer(ActionData action_data)
    {
        super.OnStartServer(action_data);
        NimbyTackPinActionData pinData = NimbyTackPinActionData.Cast(action_data);
        if (!pinData)
        {
            return;
        }
        NimbyTackConfig config = NimbyTackConfigLoader.Get();
        NimbyTackHit hit;
        pinData.m_NimbyHitValid = NimbyTackWorld.ProbeWall(action_data.m_Player, action_data.m_Target, config, true, hit);
        pinData.m_NimbyStartProbed = true;
        pinData.m_NimbyHit = hit;
        pinData.m_NimbyProbeReason = NimbyTackWorld.LastProbeReason();
        pinData.m_NimbyProbeDetail = NimbyTackWorld.LastProbeDetail();
        if (!pinData.m_NimbyHitValid)
        {
            RefuseNoWall(action_data.m_Player, action_data.m_Target, pinData.m_NimbyProbeReason, pinData.m_NimbyProbeDetail, "start");
            return;
        }
        if (NimbyTackLog.IsDebug())
        {
            NimbyTackLog.Debug("pin start fastener=" + NimbyTackFastener.Name(GetFastener()) + " surface=" + hit.surface + " " + NimbyTackWorld.PositionTag(hit.position) + " " + NimbyTackWorld.PlayerTag(action_data.m_Player));
        }
    }

    // The production refuse pin line for a failed probe, with the target and the cursor, and the
    // status message. at= says whether the start probe or the finish fallback refused.
    protected void RefuseNoWall(PlayerBase player, ActionTarget target, string reason, string detail, string at)
    {
        string name = NimbyTackFastener.Name(GetFastener());
        string refuseLine = "refuse pin fastener=" + name + " reason=no-wall detail=" + reason + " probe-detail=" + detail + " at=" + at;
        refuseLine = refuseLine + " " + NimbyTackWorld.TargetTag(target) + " " + NimbyTackWorld.ReachTag(player, target) + " " + NimbyTackWorld.PlayerTag(player);
        NimbyTackLog.Info(refuseLine);
        if (!player)
        {
            return;
        }
        if (reason == NimbyTackRules.REASON_PHYSICS_AHEAD)
        {
            NotifyPlayer(player, "The wall is not where it looks", "The note would be buried.");
        }
        else
        {
            NotifyPlayer(player, "There is no wall there", "");
        }
    }

    override void OnFinishProgressServer(ActionData action_data)
    {
        PlayerBase player = action_data.m_Player;
        ItemBase tool = action_data.m_MainItem;
        int fastener = GetFastener();
        string who = NimbyTackWorld.PlayerTag(player);
        string name = NimbyTackFastener.Name(fastener);
        NimbyTackConfig config = NimbyTackConfigLoader.Get();
        if (!player || !tool || !ToolMatches(tool) || !ToolHasPortion(tool, config))
        {
            NimbyTackLog.Info("refuse pin fastener=" + name + " reason=tool " + who);
            return;
        }
        Paper sheet = NimbyTackWorld.FindSingleSheet(player);
        if (!sheet)
        {
            NimbyTackLog.Info("refuse pin fastener=" + name + " reason=no-sheet " + who);
            return;
        }
        ItemBase nail = null;
        if (fastener == NimbyTackFastener.NAIL)
        {
            nail = NimbyTackWorld.FindNail(player);
            if (!nail)
            {
                NimbyTackLog.Info("refuse pin fastener=" + name + " reason=no-nail " + who);
                return;
            }
        }
        // T18: the hit the server probed when the action started (OnStartServer). The probe ran
        // with wantSurface true, so hit.surface already holds the view lookup or, when that is
        // empty, the fire-geometry lookup; OffsetFor below reads it. Without a start record (not
        // expected) the finish probes once, as before T18.
        NimbyTackHit hit = null;
        NimbyTackPinActionData pinData = NimbyTackPinActionData.Cast(action_data);
        if (pinData && pinData.m_NimbyStartProbed)
        {
            if (!pinData.m_NimbyHitValid || !pinData.m_NimbyHit)
            {
                // The hold should have ended at the first tick; refused again here, without a
                // second status message.
                NimbyTackLog.Info("refuse pin fastener=" + name + " reason=no-wall detail=" + pinData.m_NimbyProbeReason + " at=finish-stored " + who);
                return;
            }
            hit = pinData.m_NimbyHit;
        }
        else if (!NimbyTackWorld.ProbeWall(player, action_data.m_Target, config, true, hit))
        {
            RefuseNoWall(player, action_data.m_Target, NimbyTackWorld.LastProbeReason(), NimbyTackWorld.LastProbeDetail(), "finish");
            return;
        }
        // T18: a player-built wall can be dismantled or destroyed during the hold.
        if (!hit.wall || hit.wall.IsDamageDestroyed())
        {
            NimbyTackLog.Info("refuse pin fastener=" + name + " reason=wall-gone " + NimbyTackWorld.WallTag(hit) + " " + who + " " + NimbyTackWorld.PositionTag(hit.position));
            NotifyPlayer(player, "There is no wall there", "");
            return;
        }
        if (fastener == NimbyTackFastener.KNIFE && !NimbyTackRules.IsWoodSurface(hit.surface))
        {
            NimbyTackLog.Info("refuse pin fastener=" + name + " reason=not-wood surface=" + hit.surface + " " + NimbyTackWorld.WallTag(hit) + " " + who + " " + NimbyTackWorld.PositionTag(hit.position));
            NotifyPlayer(player, "The knife will not go into that", "Only wood takes a knife.");
            return;
        }
        int nearby = NimbyTackWorld.CountPinnedNear(hit.position, config.areaCapRadiusMeters);
        if (!NimbyTackRules.WithinAreaCap(nearby, config.areaCapCount))
        {
            NimbyTackLog.Info("refuse pin fastener=" + name + " reason=area-cap nearby=" + nearby.ToString() + " cap=" + config.areaCapCount.ToString() + " " + NimbyTackWorld.WallTag(hit) + " " + who + " " + NimbyTackWorld.PositionTag(hit.position));
            NotifyPlayer(player, "Too many notes nearby", "Take one down first.");
            return;
        }
        // T24: a door leaf pins open or closed (the T21 closed-door finish re-check is retired).
        // T19, T20: where the note goes, measured before it exists (NIMBY-T-028, T-025).
        NimbyTackPlacement placement = NimbyTackPlace.Compute(player, hit, config);
        // Read before the sheet is deleted (NIMBY-T-009: the log tells which sheet went up).
        string textTag = NimbyTackRules.TextTag(NimbyTackWorld.IsWritten(sheet));
        NimbyTack_PinnedNote pinned = NimbyTackWorld.CreatePinned(fastener, placement, sheet);
        if (!pinned)
        {
            NimbyTackLog.Info("failed pin fastener=" + name + " reason=create " + who + " " + NimbyTackWorld.PositionTag(hit.position));
            return;
        }
        ConsumeFastener(player, tool, nail, pinned, config);
        GetGame().ObjectDelete(sheet);
        // T21: a note on a door leaf syncs its door and pivot so each client swings it.
        string doorTag = "-";
        if (hit.doorIndex >= 0)
        {
            doorTag = pinned.NimbyTackAttachDoorServer(hit.wall, hit.doorIndex, hit.component, "pin");
        }
        // Built in statements: one concatenation this long is "Formula too complex" to Enforce.
        string line = "pin fastener=" + name + " class=" + pinned.GetType() + " wall=" + NimbyTackWorld.ClassOf(hit.wall) + " surface=" + hit.surface + " normal=" + placement.normal.ToString() + " offset=" + placement.offset.ToString();
        line = line + " " + placement.Tag() + " door=" + doorTag;
        line = line + " text=" + textTag + " health=" + pinned.GetHealth().ToString() + " nearby=" + nearby.ToString() + " " + who + " " + NimbyTackWorld.PositionTag(hit.position);
        NimbyTackLog.Info(line);
        // Part B (B5): debug only; measures the footprint, moves nothing.
        NimbyTackWorld.LogFootprint(player, hit, pinned, placement.offset, config.wallOffsetMeters);
    }

    // T2: what the pin costs. Nail, tape, and epoxy are spent and never come back; the knife
    // moves into the pinned note's Knife slot and returns on take-down.
    protected void ConsumeFastener(PlayerBase player, ItemBase tool, ItemBase nail, NimbyTack_PinnedNote pinned, NimbyTackConfig config)
    {
        int fastener = GetFastener();
        if (fastener == NimbyTackFastener.NAIL && nail)
        {
            nail.AddQuantity(-1.0);
            return;
        }
        if (fastener == NimbyTackFastener.TAPE)
        {
            tool.AddQuantity(-config.tapeQuantityPerPin);
            return;
        }
        if (fastener == NimbyTackFastener.EPOXY)
        {
            tool.AddQuantity(-config.epoxyQuantityPerPin);
            return;
        }
        if (fastener == NimbyTackFastener.KNIFE)
        {
            if (!player.ServerTakeEntityToTargetAttachment(pinned, tool))
            {
                NimbyTackLog.Info("failed pin fastener=knife reason=attach " + NimbyTackWorld.PlayerTag(player));
            }
        }
    }
}

class ActionNimbyTackPinNail : ActionNimbyTackPinBase
{
    void ActionNimbyTackPinNail()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_ASSEMBLE;
        m_Text = "Nail note to wall";
    }

    override int GetFastener()
    {
        return NimbyTackFastener.NAIL;
    }

    override bool ToolMatches(ItemBase item)
    {
        return NimbyTackRules.IsHammerTool(item.GetType());
    }
}

class ActionNimbyTackPinTape : ActionNimbyTackPinBase
{
    void ActionNimbyTackPinTape()
    {
        m_Text = "Tape note to wall";
    }

    override int GetFastener()
    {
        return NimbyTackFastener.TAPE;
    }

    override bool ToolMatches(ItemBase item)
    {
        return item.GetType() == "DuctTape";
    }

    override bool ToolHasPortion(ItemBase item, NimbyTackConfig config)
    {
        return NimbyTackRules.HasPortion(item.GetQuantity(), config.tapeQuantityPerPin);
    }
}

class ActionNimbyTackPinEpoxy : ActionNimbyTackPinBase
{
    void ActionNimbyTackPinEpoxy()
    {
        m_Text = "Glue note to wall";
    }

    override int GetFastener()
    {
        return NimbyTackFastener.EPOXY;
    }

    override bool ToolMatches(ItemBase item)
    {
        return item.GetType() == "EpoxyPutty";
    }

    override bool ToolHasPortion(ItemBase item, NimbyTackConfig config)
    {
        return NimbyTackRules.HasPortion(item.GetQuantity(), config.epoxyQuantityPerPin);
    }
}

class ActionNimbyTackPinKnife : ActionNimbyTackPinBase
{
    void ActionNimbyTackPinKnife()
    {
        m_Text = "Knife note to wall";
    }

    override int GetFastener()
    {
        return NimbyTackFastener.KNIFE;
    }

    override bool ToolMatches(ItemBase item)
    {
        return NimbyTackRules.IsKnife(item.GetType());
    }
}

// T3: read a pinned note where it hangs. Vanilla ActionReadPaper reads the item in hands
// (CCTNone, HasTarget false, ActionReadPaper.c), so a Paper on a wall never offers it; this is
// the same action shaped as a target action: hold F on the note, the server sends the text
// with RPC_READ_NOTE to the reader only (ActionReadPaper.c:24), the client's WrittenNoteData
// opens the NoteMenu read view (WrittenNoteData.c:46), and the NimbyNotes watcher ends the
// endless component when the menu closes (N11). No target lock: reading changes nothing and
// two players may read one note at once.
class ActionNimbyTackReadCB : ActionContinuousBaseCB
{
    override void CreateActionComponent()
    {
        EnableStateChangeCallback();
        m_ActionData.m_ActionComponent = new CAContinuousTime(-1.0);
    }

    override void EndActionComponent()
    {
    }

    override void OnStateChange(int pOldState, int pCurrentState)
    {
        if (pCurrentState == STATE_NONE && !g_Game.IsHeadlessOrDedicatedServer())
        {
            if (g_Game.GetUIManager() && g_Game.GetUIManager().IsMenuOpen(MENU_NOTE))
            {
                g_Game.GetUIManager().FindMenu(MENU_NOTE).Close();
            }
            return;
        }
        if (pCurrentState != STATE_LOOP_LOOP || pOldState == STATE_LOOP_LOOP)
        {
            return;
        }
        if (g_Game.IsMultiplayer() && !g_Game.IsServer())
        {
            return;
        }
        NimbyTack_PinnedNote pinned = NimbyTack_PinnedNote.Cast(m_ActionData.m_Target.GetObject());
        if (!pinned)
        {
            NimbyTackLog.Info("refuse read reason=no-target " + NimbyTackWorld.PlayerTag(m_ActionData.m_Player));
            return;
        }
        string text = "";
        WrittenNoteData data = pinned.GetWrittenNoteData();
        if (data)
        {
            text = data.GetNoteText();
        }
        Param1<string> param = new Param1<string>(text);
        pinned.RPCSingleParam(ERPCs.RPC_READ_NOTE, param, true, m_ActionData.m_Player.GetIdentity());
        NimbyTackLog.Info("read class=" + pinned.GetType() + " length=" + text.Length().ToString() + " " + NimbyTackWorld.PlayerTag(m_ActionData.m_Player) + " " + NimbyTackWorld.PositionTag(pinned.GetPosition()));
    }
}

class ActionNimbyTackRead : ActionContinuousBase
{
    protected ref NimbyNotesMenuWatch m_NimbyMenuWatch;
    // Debug trace of the client action state (October 18, 2026: the pose and the progress bar
    // outlived the note menu). Written only with the debug flag.
    protected int m_NimbyLastState = -99;

    void ActionNimbyTackRead()
    {
        m_CallbackClass = ActionNimbyTackReadCB;
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_VIEWNOTE;
        m_FullBody = true;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
        m_Text = "Read note";
        m_LockTargetOnUse = false;
        m_NimbyMenuWatch = new NimbyNotesMenuWatch("tack-read");
    }

    // Target actions are only read by the interact input (hold F), as on take-down (T14).
    override typename GetInputType()
    {
        return ContinuousInteractActionInput;
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINone;
        m_ConditionTarget = new CCTCursor(UAMaxDistances.DEFAULT);
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (!player || !target)
        {
            return false;
        }
        NimbyTack_PinnedNote pinned = NimbyTack_PinnedNote.Cast(target.GetObject());
        if (!pinned)
        {
            return false;
        }
        if (pinned.IsSetForDeletion())
        {
            return false;
        }
        return pinned.GetNimbyTackFastener() >= 0;
    }

    override void OnStartClient(ActionData action_data)
    {
        super.OnStartClient(action_data);
        m_NimbyMenuWatch.Reset();
        m_NimbyLastState = -99;
        NimbyTackLog.Debug("read start-client");
    }

    override void OnUpdate(ActionData action_data)
    {
        super.OnUpdate(action_data);
        if (action_data && action_data.m_State != m_NimbyLastState)
        {
            m_NimbyLastState = action_data.m_State;
            NimbyTackLog.Debug("read state=" + action_data.m_State.ToString() + " menu=" + NimbyTackLog.Bool(NimbyNotesMenuWatch.MenuOpen()) + " server=" + NimbyTackLog.Bool(GetGame().IsServer()));
        }
        m_NimbyMenuWatch.Update(action_data);
    }

    override void OnEndInput(ActionData action_data)
    {
        NimbyTackLog.Debug("read end-input state=" + action_data.m_State.ToString());
        super.OnEndInput(action_data);
    }

    override void OnEndRequest(ActionData action_data)
    {
        NimbyTackLog.Debug("read end-request state=" + action_data.m_State.ToString());
        if (action_data.m_Callback)
        {
            action_data.m_Callback.InternalCommand(DayZPlayerConstants.CMD_ACTIONINT_INTERRUPT);
        }
    }

    override void OnEnd(ActionData action_data)
    {
        NimbyTackLog.Debug("read end state=" + action_data.m_State.ToString() + " server=" + NimbyTackLog.Bool(GetGame().IsServer()));
        super.OnEnd(action_data);
    }
}

// T4: pull a pinned note down by hand. No tool; the sheet comes back to the player's inventory
// (or the ground when it is full) with its text and the fastener's damage applied; the knife
// comes back with it.
class ActionNimbyTackTakeDownCB : ActionContinuousBaseCB
{
    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime(ActionNimbyTackTakeDown.TAKE_DOWN_SECONDS);
    }
}

class ActionNimbyTackTakeDown : ActionContinuousBase
{
    static const float TAKE_DOWN_SECONDS = 1.5;

    void ActionNimbyTackTakeDown()
    {
        m_CallbackClass = ActionNimbyTackTakeDownCB;
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
        m_FullBody = true;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
        m_Text = "Take down note";
        // The target lock stays at its default (true, ActionBase.c:89). The pin actions drop it
        // because their target is a wall with no inventory; this target is a Paper with one,
        // so the lock reserves it and a second player's take-down on the same note waits.
        // Without it two take-downs in one frame each created a sheet from one note
        // (self-review S1, October 17, 2026). The IsSetForDeletion guards below cover the
        // frame between the first finish and the engine's delete.
    }

    // The action lives on the pinned note (the target), not on an item in hands. Only the
    // interact input (hold F) looks at the target's actions: ContinuousInteractActionInput sets
    // m_DetectFromTarget = true, the default continuous input (ActionContinuousBase.c:179)
    // sets it false. With the default the prompt never appeared (October 17, 2026).
    override typename GetInputType()
    {
        return ContinuousInteractActionInput;
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINone;
        m_ConditionTarget = new CCTCursor(UAMaxDistances.DEFAULT);
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (!player || !target)
        {
            return false;
        }
        NimbyTack_PinnedNote pinned = NimbyTack_PinnedNote.Cast(target.GetObject());
        if (!pinned)
        {
            return false;
        }
        if (pinned.IsSetForDeletion())
        {
            return false;
        }
        return pinned.GetNimbyTackFastener() >= 0;
    }

    override void OnFinishProgressServer(ActionData action_data)
    {
        PlayerBase player = action_data.m_Player;
        string who = NimbyTackWorld.PlayerTag(player);
        NimbyTack_PinnedNote pinned = NimbyTack_PinnedNote.Cast(action_data.m_Target.GetObject());
        if (!player || !pinned)
        {
            NimbyTackLog.Info("refuse takedown reason=no-target " + who);
            return;
        }
        if (pinned.IsSetForDeletion())
        {
            // Another take-down or a burn finished on this note first; a second sheet would
            // duplicate the text.
            NimbyTackLog.Info("refuse takedown reason=deleted class=" + pinned.GetType() + " " + who + " " + NimbyTackWorld.PositionTag(pinned.GetPosition()));
            return;
        }
        NimbyTackConfig config = NimbyTackConfigLoader.Get();
        int fastener = pinned.GetNimbyTackFastener();
        string name = NimbyTackFastener.Name(fastener);
        vector position = pinned.GetPosition();
        float damage = NimbyTackRules.TakeDownDamage(fastener, config);
        float health = NimbyTackRules.HealthAfterPull(pinned.GetHealth(), damage);
        string text = "";
        WrittenNoteData source = pinned.GetWrittenNoteData();
        if (source)
        {
            text = source.GetNoteText();
        }
        string knifeNote = "";
        if (fastener == NimbyTackFastener.KNIFE)
        {
            EntityAI knife = pinned.FindAttachmentBySlotName("Knife");
            if (knife)
            {
                if (player.ServerTakeEntityToInventory(FindInventoryLocationType.ANY, knife))
                {
                    knifeNote = " knife=inventory";
                }
                else
                {
                    pinned.GetInventory().DropEntity(InventoryMode.SERVER, pinned, knife);
                    knifeNote = " knife=ground";
                }
            }
            else
            {
                knifeNote = " knife=missing";
            }
        }
        Paper sheet = Paper.Cast(player.GetInventory().CreateInInventory("Paper"));
        string where = "inventory";
        if (!sheet)
        {
            sheet = Paper.Cast(GetGame().CreateObjectEx("Paper", player.GetPosition(), ECE_PLACE_ON_SURFACE));
            where = "ground";
        }
        if (!sheet)
        {
            NimbyTackLog.Info("failed takedown fastener=" + name + " reason=create-sheet " + who + " " + NimbyTackWorld.PositionTag(position));
            return;
        }
        WrittenNoteData destination = sheet.GetWrittenNoteData();
        if (destination && text != "")
        {
            destination.SetNoteText(text);
        }
        // DecreaseHealth with auto_delete would delete a ruined sheet (Object.c:1115); the text
        // survives only if the sheet does, so the health is set directly and the sheet stays.
        sheet.SetHealth(health);
        GetGame().ObjectDelete(pinned);
        NimbyTackLog.Info("takedown fastener=" + name + " sheet=" + where + " health=" + health.ToString() + " damage=" + damage.ToString() + knifeNote + " " + who + " " + NimbyTackWorld.PositionTag(position));
    }
}

// T18 (NIMBY-T-022, Jeff, October 24, 2026): burn a pinned note with an igniter in hands. Replaces
// vanilla ActionLightItemOnFire on the note, whose CCTNonRuined measured the feet to the note on
// every tick and cancelled at the edge of reach; the pinned note now refuses vanilla Ignite
// (CanBeIgnitedBy). Same animation, stance, and time as vanilla ignite (ActionLightItemOnFire.c:5,
// 14-16), the hold rule of the pin, and vanilla's success path at the finish: the igniter pays
// (Matchbox.OnIgnitedTarget takes one match) and the note burns (NimbyTack_PinnedNote.OnIgnitedThis
// drops a knife, logs the burn line, and deletes the note). Offered by the igniters that carry
// vanilla Ignite (nimbytack_tools.c). The target lock stays at its default, as on take-down, so a
// take-down and a burn cannot both finish on one note.
class ActionNimbyTackBurnCB : ActionContinuousBaseCB
{
    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime(UATimeSpent.FIREPLACE_IGNITE);
    }
}

class ActionNimbyTackBurn : ActionContinuousBase
{
    void ActionNimbyTackBurn()
    {
        m_CallbackClass = ActionNimbyTackBurnCB;
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_STARTFIRE;
        m_FullBody = true;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH;
        m_Text = "Burn note";
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINonRuined;
        m_ConditionTarget = new CCTNimbyTackHold;
    }

    // The igniter must be able to light (vanilla CanIgniteItem: a match left and dry, a lighter
    // with fuel, a torch, flare, or stove already burning).
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (!player || !target || !item)
        {
            return false;
        }
        NimbyTack_PinnedNote pinned = NimbyTack_PinnedNote.Cast(target.GetObject());
        if (!pinned || pinned.IsSetForDeletion() || pinned.GetNimbyTackFastener() < 0)
        {
            return false;
        }
        return item.CanIgniteItem(pinned);
    }

    // During the hold: the same checks, which measure no distance (the target condition does).
    override bool ActionConditionContinue(ActionData action_data)
    {
        if (ActionCondition(action_data.m_Player, action_data.m_Target, action_data.m_MainItem))
        {
            return true;
        }
        string source = "none";
        if (action_data.m_MainItem)
        {
            source = action_data.m_MainItem.GetType();
        }
        return NimbyTackHold.Refuse("burn-condition", "source=" + source, action_data.m_Player);
    }

    override void OnFinishProgressServer(ActionData action_data)
    {
        PlayerBase player = action_data.m_Player;
        ItemBase igniter = action_data.m_MainItem;
        string who = NimbyTackWorld.PlayerTag(player);
        NimbyTack_PinnedNote pinned = NimbyTack_PinnedNote.Cast(action_data.m_Target.GetObject());
        if (!player || !igniter || !pinned)
        {
            NimbyTackLog.Info("refuse burn reason=no-target " + who);
            return;
        }
        if (pinned.IsSetForDeletion())
        {
            NimbyTackLog.Info("refuse burn reason=deleted class=" + pinned.GetType() + " " + who + " " + NimbyTackWorld.PositionTag(pinned.GetPosition()));
            return;
        }
        if (!igniter.CanIgniteItem(pinned))
        {
            NimbyTackLog.Info("refuse burn reason=igniter source=" + igniter.GetType() + " " + who);
            return;
        }
        igniter.OnIgnitedTarget(pinned);
        pinned.OnIgnitedThis(igniter);
    }
}

modded class ActionConstructor
{
    override void RegisterActions(TTypenameArray actions)
    {
        super.RegisterActions(actions);
        actions.Insert(ActionNimbyTackPinNail);
        actions.Insert(ActionNimbyTackPinTape);
        actions.Insert(ActionNimbyTackPinEpoxy);
        actions.Insert(ActionNimbyTackPinKnife);
        actions.Insert(ActionNimbyTackRead);
        actions.Insert(ActionNimbyTackTakeDown);
        actions.Insert(ActionNimbyTackBurn);
    }
}
