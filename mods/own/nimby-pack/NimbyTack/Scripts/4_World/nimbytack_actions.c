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

    // Runs on the client every frame while the tool is in hands and on the server when the
    // action is requested. Cheap checks first; the raycast last.
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (!player || !target || !item)
        {
            return false;
        }
        if (!ToolMatches(item))
        {
            return false;
        }
        NimbyTackConfig config = NimbyTackConfigLoader.Get();
        if (!ToolHasPortion(item, config))
        {
            return false;
        }
        if (GetFastener() == NimbyTackFastener.NAIL && !NimbyTackWorld.FindNail(player))
        {
            return false;
        }
        if (!NimbyTackWorld.FindSingleSheet(player))
        {
            return false;
        }
        // Only the knife needs the surface name here (wood test); the debug flag asks for it
        // so the test record sees every surface. Everyone else skips the fire-geometry ray.
        bool wantSurface = GetFastener() == NimbyTackFastener.KNIFE || NimbyTackLog.IsDebug();
        NimbyTackHit hit;
        if (!NimbyTackWorld.ProbeWall(player, target, config, wantSurface, hit))
        {
            return false;
        }
        if (GetFastener() == NimbyTackFastener.KNIFE && !NimbyTackRules.IsWoodSurface(hit.surface))
        {
            return false;
        }
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
        if (!NimbyTackWorld.ProbeWall(player, action_data.m_Target, config, true, hit))
        {
            NimbyTackLog.Info("refuse pin fastener=" + name + " reason=no-wall detail=" + NimbyTackWorld.LastProbeReason() + " " + who);
            player.MessageStatus("There is no wall there.");
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
        NimbyTack_PinnedNote pinned = NimbyTackWorld.CreatePinned(fastener, hit, sheet);
        if (!pinned)
        {
            NimbyTackLog.Info("failed pin fastener=" + name + " reason=create " + who + " " + NimbyTackWorld.PositionTag(hit.position));
            return;
        }
        ConsumeFastener(player, tool, nail, pinned, config);
        GetGame().ObjectDelete(sheet);
        // Built in two statements: one concatenation this long is "Formula too complex" to Enforce.
        string line = "pin fastener=" + name + " class=" + pinned.GetType() + " wall=" + hit.wall.GetType() + " surface=" + hit.surface + " normal=" + hit.normal.ToString() + " offset=" + config.wallOffsetMeters.ToString();
        line = line + " health=" + pinned.GetHealth().ToString() + " nearby=" + nearby.ToString() + " " + who + " " + NimbyTackWorld.PositionTag(hit.position);
        NimbyTackLog.Info(line);
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
    }

    override void OnUpdate(ActionData action_data)
    {
        super.OnUpdate(action_data);
        m_NimbyMenuWatch.Update(action_data);
    }

    override void OnEndRequest(ActionData action_data)
    {
        if (action_data.m_Callback)
        {
            action_data.m_Callback.InternalCommand(DayZPlayerConstants.CMD_ACTIONINT_INTERRUPT);
        }
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
