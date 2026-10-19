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
