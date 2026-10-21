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
// -----------------------------------------------------------------------------
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
        m_ConditionTarget = new CCTCursor(UAMaxDistances.DEFAULT);
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
            return Refuse(NimbyTackWorld.LastProbeReason());
        }
        if (GetFastener() == NimbyTackFastener.KNIFE && !NimbyTackRules.IsWoodSurface(hit.surface))
        {
            if (debugOn)
            {
                m_NimbyDetail = "surface=" + hit.surface;
            }
            return Refuse("not-wood");
        }
        // Part B (B1): what the pin would use. On the client this is the default config, which
        // the server's matches unless its config.json changes the offsets.
        if (debugOn)
        {
            m_NimbyDetail = "surface=" + hit.surface + " offset=" + NimbyTackRules.OffsetFor(hit.surface, config).ToString();
        }
        m_NimbyReason = REASON_PASSED;
        return true;
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
        NimbyTackHit hit;
        // wantSurface is true here, so hit.surface already holds the view lookup or, when that is
        // empty, the fire-geometry lookup. OffsetFor below reads it; no further raycast is needed.
        if (!NimbyTackWorld.ProbeWall(player, action_data.m_Target, config, true, hit))
        {
            // Part B: the target and the cursor ride on this line, so a refusal at pin time names
            // the object the player aimed at without the debug flag.
            string refuseLine = "refuse pin fastener=" + name + " reason=no-wall detail=" + NimbyTackWorld.LastProbeReason() + " probe-detail=" + NimbyTackWorld.LastProbeDetail();
            refuseLine = refuseLine + " " + NimbyTackWorld.TargetTag(action_data.m_Target) + " " + NimbyTackWorld.ReachTag(player, action_data.m_Target) + " " + who;
            NimbyTackLog.Info(refuseLine);
            if (NimbyTackWorld.LastProbeReason() == NimbyTackRules.REASON_PHYSICS_AHEAD)
            {
                player.MessageStatus("The wall is not where it looks; the note would be buried.");
            }
            else
            {
                player.MessageStatus("There is no wall there.");
            }
            return;
        }
        if (fastener == NimbyTackFastener.KNIFE && !NimbyTackRules.IsWoodSurface(hit.surface))
        {
            NimbyTackLog.Info("refuse pin fastener=" + name + " reason=not-wood surface=" + hit.surface + " " + who);
            player.MessageStatus("The knife will not go into that.");
            return;
        }
        int nearby = NimbyTackWorld.CountPinnedNear(hit.position, config.areaCapRadiusMeters);
        if (!NimbyTackRules.WithinAreaCap(nearby, config.areaCapCount))
        {
            NimbyTackLog.Info("refuse pin fastener=" + name + " reason=area-cap nearby=" + nearby.ToString() + " cap=" + config.areaCapCount.ToString() + " " + who + " " + NimbyTackWorld.PositionTag(hit.position));
            player.MessageStatus("Too many notes on these walls already.");
            return;
        }
        // NIMBY-T-008: a surface with no name (the tent's cloth) takes the unknown-surface offset;
        // NIMBY-T-013: a rough material (mud brick) takes the rough-surface offset.
        float pinOffset = NimbyTackRules.OffsetFor(hit.surface, config);
        // Read before the sheet is deleted (NIMBY-T-009: the log tells which sheet went up).
        string textTag = NimbyTackRules.TextTag(NimbyTackWorld.IsWritten(sheet));
        NimbyTack_PinnedNote pinned = NimbyTackWorld.CreatePinned(fastener, hit, sheet, pinOffset);
        if (!pinned)
        {
            NimbyTackLog.Info("failed pin fastener=" + name + " reason=create " + who + " " + NimbyTackWorld.PositionTag(hit.position));
            return;
        }
        ConsumeFastener(player, tool, nail, pinned, config);
        GetGame().ObjectDelete(sheet);
        // Built in two statements: one concatenation this long is "Formula too complex" to Enforce.
        string line = "pin fastener=" + name + " class=" + pinned.GetType() + " wall=" + NimbyTackWorld.ClassOf(hit.wall) + " surface=" + hit.surface + " normal=" + hit.normal.ToString() + " offset=" + pinOffset.ToString();
        line = line + " text=" + textTag + " health=" + pinned.GetHealth().ToString() + " nearby=" + nearby.ToString() + " " + who + " " + NimbyTackWorld.PositionTag(hit.position);
        NimbyTackLog.Info(line);
        // Part B (B5): debug only; measures the footprint, moves nothing.
        NimbyTackWorld.LogFootprint(player, hit, pinned, pinOffset, config.wallOffsetMeters);
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
    }
}
