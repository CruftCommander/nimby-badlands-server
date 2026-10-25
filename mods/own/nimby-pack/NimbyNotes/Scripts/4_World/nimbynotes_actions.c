// -----------------------------------------------------------------------------
// NimbyNotes - action registration and the write rule.
// Purpose : (N5) Registers ActionWritePaper and ActionReadPaper with the action
//           manager so AddAction on Paper and Pen_ColorBase resolves them; (N2)
//           restricts writing to a single sheet; (N11) ends either action when
//           the note menu closes.
// Vanilla : ActionConstructor.RegisterActions ActionConstructor.c:27;
//           ActionWritePaper.ActionCondition ActionWritePaper.c:57-59.
// N11     : Both vanilla actions loop CMD_ACTIONFB_VIEWNOTE on a CAContinuousTime(-1)
//           and only end from OnUpdate when m_State == UA_FINISHED
//           (ActionReadPaper.c:57), a state an endless component never reaches.
//           After the menu closes the player stays in the full-body note pose:
//           no mouse look, no jump, until the sheet leaves the hands (seen
//           October 17, 2026 on every read). NimbyNotesMenuWatch ends the
//           action the first frame the menu is closed after having been open.
//           The action objects are per-client singletons, so one watcher per
//           action is safe on the client and never read on the server. Read and
//           write share the watcher class so the logic lives once (S10).
// N9      : Writing from the inventory is ActionWritePaper itself, started the
//           vanilla 1.30 way. Dropping an item on the Combine zone of the item in
//           hands runs Icon.Combine (Icon.c:700-725), which sets an action from the
//           inventory (ItemManager.c:738-742, ActionManagerClient.c:1199-1332):
//           the inventory closes, the dropped item becomes the forced target
//           (InventoryActionHandler.c:42-61, ActionManagerClient.c:482-501), and
//           the player holds the action key. ActionWritePaper qualifies because
//           HasTarget and CanBeSetFromInventory default to true
//           (ActionBase.c:259-262,351-354) and ActionBase.Can does not exclude a
//           target inside the player's own inventory (ActionBase.c:929-966). The
//           client hook the note menu needs is vanilla's OnStartClient, which binds
//           pen and sheet before the server's RPC_WRITE_NOTE arrives
//           (ActionWritePaper.c:20-33,66-76). The WriteLetter recipe the plan named
//           is not registered in 1.30 (PluginRecipesManagerBase.c:14 onward, 217
//           recipes, none of them WriteLetter), so Scrawl adds no recipe (Jeff,
//           October 19, 2026). The lines below trace the three steps (target set,
//           action started, menu requested) under the debug flag.
// -----------------------------------------------------------------------------
modded class ActionConstructor
{
    override void RegisterActions(TTypenameArray actions)
    {
        super.RegisterActions(actions);
        actions.Insert(ActionWritePaper);
        actions.Insert(ActionReadPaper);
    }
}

// N11: one instance per action. Reset at action start; Update every client frame.
class NimbyNotesMenuWatch
{
    protected string m_Which;
    protected bool m_Seen;
    protected bool m_Reported;

    void NimbyNotesMenuWatch(string which)
    {
        m_Which = which;
        m_Seen = false;
        m_Reported = false;
    }

    void Reset()
    {
        m_Seen = false;
        m_Reported = false;
    }

    // Ends the action the first frame the note menu is closed after having been open.
    void Update(ActionData action_data)
    {
        if (g_Game.IsHeadlessOrDedicatedServer())
        {
            return;
        }
        if (MenuOpen())
        {
            if (!m_Seen)
            {
                NimbyNotesLog.Debug(m_Which + " watcher: note menu seen");
            }
            m_Seen = true;
            return;
        }
        if (!m_Seen)
        {
            return;
        }
        if (EndAction(action_data, m_Which))
        {
            m_Seen = false;
            return;
        }
        if (action_data && !m_Reported)
        {
            m_Reported = true;
            NimbyNotesLog.Debug(m_Which + " watcher: menu closed but not ending; state=" + action_data.m_State.ToString());
        }
    }

    static bool MenuOpen()
    {
        if (g_Game.IsHeadlessOrDedicatedServer())
        {
            return false;
        }
        if (!g_Game.GetUIManager())
        {
            return false;
        }
        return g_Game.GetUIManager().IsMenuOpen(MENU_NOTE);
    }

    static bool EndAction(ActionData action_data, string which)
    {
        if (!action_data)
        {
            return false;
        }
        // UA_PROCESSING: the loop is still running (the menu was closed before the key was
        // released). UA_FINISHED: the key was released while the menu was open, the component
        // finished, and the action waits in the note pose because EndActionComponent is empty
        // by design (ActionReadPaper.c:10). Vanilla ends the second case from OnUpdate once the
        // menu is gone (ActionReadPaper.c:57); a target action on a wall needs the same (the
        // October 19, 2026 trace: state 2, end-input, state 4, menu closed, pose held).
        if (action_data.m_State != UA_PROCESSING && action_data.m_State != UA_FINISHED)
        {
            return false;
        }
        if (!action_data.m_Player)
        {
            return false;
        }
        ActionManagerClient manager = ActionManagerClient.Cast(action_data.m_Player.GetActionManager());
        if (!manager)
        {
            return false;
        }
        manager.RequestEndAction();
        NimbyNotesLog.Debug(which + " ended: note menu closed");
        return true;
    }
}

modded class ActionReadPaper
{
    protected ref NimbyNotesMenuWatch m_NimbyNotesWatch;

    void ActionReadPaper()
    {
        m_NimbyNotesWatch = new NimbyNotesMenuWatch("read");
    }

    override void OnStartClient(ActionData action_data)
    {
        super.OnStartClient(action_data);
        m_NimbyNotesWatch.Reset();
    }

    override void OnUpdate(ActionData action_data)
    {
        super.OnUpdate(action_data);
        m_NimbyNotesWatch.Update(action_data);
    }
}

// N9: the pen and the sheet of a write, whichever side each is on, and where the target
// lies. "inventory" when the target is in the writer's own inventory (the Combine drop),
// "world" when it lies on the ground.
class NimbyNotesWritePair
{
    ItemBase m_Pen;
    ItemBase m_Sheet;
    string m_From;

    void NimbyNotesWritePair(ActionData action_data)
    {
        m_From = "world";
        if (!action_data || !action_data.m_Target)
        {
            return;
        }
        ItemBase main = action_data.m_MainItem;
        ItemBase target = ItemBase.Cast(action_data.m_Target.GetObject());
        if (!main || !target)
        {
            return;
        }
        if (main.ConfigIsExisting("writingColor"))
        {
            m_Pen = main;
            m_Sheet = target;
        }
        else
        {
            m_Pen = target;
            m_Sheet = main;
        }
        if (action_data.m_Player && target.GetHierarchyRootPlayer() == action_data.m_Player)
        {
            m_From = "inventory";
        }
    }

    bool IsComplete()
    {
        return m_Pen && m_Sheet;
    }

    string Describe()
    {
        return "pen=" + m_Pen.GetType() + " sheet=" + m_Sheet.GetType();
    }
}

// N9: the server line for the moment the write menu is requested, the same state change
// on which vanilla sends RPC_WRITE_NOTE (ActionWritePaper.c:20-33).
modded class ActionWritePaperCB
{
    override void OnStateChange(int pOldState, int pCurrentState)
    {
        super.OnStateChange(pOldState, pCurrentState);
        if (!NimbyNotesLog.IsDebug())
        {
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
        NimbyNotesWritePair pair = new NimbyNotesWritePair(m_ActionData);
        if (!pair.IsComplete())
        {
            return;
        }
        PlayerIdentity identity = null;
        if (m_ActionData.m_Player)
        {
            identity = m_ActionData.m_Player.GetIdentity();
        }
        NimbyNotesLog.Debug(pair.m_From + " write: menu requested " + pair.Describe() + " " + NimbyNotesLog.PlayerTag(identity));
    }
}

// N9: the client line for the drop on the Combine zone. Any forced target goes through
// here (the inventory drop, InventoryActionHandler.c:58, and the quickbar,
// PlayerBase.c:5069); only a pen-and-sheet pair is logged.
modded class ActionManagerClient
{
    override void ForceTarget(Object targetObject)
    {
        super.ForceTarget(targetObject);
        if (!NimbyNotesLog.IsDebug())
        {
            return;
        }
        ItemBase target = ItemBase.Cast(targetObject);
        if (!target || !m_Player)
        {
            return;
        }
        ItemBase hands = m_Player.GetItemInHands();
        if (!hands)
        {
            return;
        }
        bool penOnSheet = hands.ConfigIsExisting("writingColor") && Paper.Cast(target);
        bool sheetOnPen = target.ConfigIsExisting("writingColor") && Paper.Cast(hands);
        if (!penOnSheet && !sheetOnPen)
        {
            return;
        }
        NimbyNotesLog.Debug("inventory write: target set hands=" + hands.GetType() + " target=" + target.GetType() + "; the write starts on the action key");
    }
}

modded class ActionWritePaper
{
    protected ref NimbyNotesMenuWatch m_NimbyNotesWatch;

    void ActionWritePaper()
    {
        m_NimbyNotesWatch = new NimbyNotesMenuWatch("write");
    }

    override void OnStartClient(ActionData action_data)
    {
        super.OnStartClient(action_data);
        m_NimbyNotesWatch.Reset();
        if (!NimbyNotesLog.IsDebug())
        {
            return;
        }
        NimbyNotesWritePair pair = new NimbyNotesWritePair(action_data);
        if (pair.IsComplete())
        {
            NimbyNotesLog.Debug(pair.m_From + " write: started " + pair.Describe());
        }
    }

    override void OnUpdate(ActionData action_data)
    {
        super.OnUpdate(action_data);
        m_NimbyNotesWatch.Update(action_data);
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (!super.ActionCondition(player, target, item))
        {
            return false;
        }
        // The sheet is whichever side is not the pen (ActionWritePaper.c:57).
        ItemBase sheet = item;
        if (item && item.ConfigIsExisting("writingColor"))
        {
            sheet = ItemBase.Cast(target.GetObject());
        }
        if (!sheet)
        {
            return false;
        }
        return NimbyNotesRules.CanWrite(sheet.GetQuantity());
    }
}
