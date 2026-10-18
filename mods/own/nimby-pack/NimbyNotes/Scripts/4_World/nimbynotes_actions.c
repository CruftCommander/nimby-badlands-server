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
//           The action objects are per-client singletons, so the flag is safe
//           on the client and never read on the server.
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

// N11: shared by the read and write actions. Returns true when the action was ended.
class NimbyNotesMenuWatch
{
    // Each action keeps its own seen flag (Enforce out parameters on members are
    // unreliable, shared rules) and calls these two helpers from OnUpdate.
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
        if (action_data.m_State != UA_PROCESSING)
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
        Print("[NIMBY][Notes] " + which + " ended: note menu closed");
        return true;
    }
}

modded class ActionReadPaper
{
    protected bool m_NimbyNotesMenuSeen;

    override void OnStartClient(ActionData action_data)
    {
        super.OnStartClient(action_data);
        m_NimbyNotesMenuSeen = false;
    }

    override void OnUpdate(ActionData action_data)
    {
        super.OnUpdate(action_data);
        if (g_Game.IsHeadlessOrDedicatedServer())
        {
            return;
        }
        if (NimbyNotesMenuWatch.MenuOpen())
        {
            m_NimbyNotesMenuSeen = true;
            return;
        }
        if (!m_NimbyNotesMenuSeen)
        {
            return;
        }
        if (NimbyNotesMenuWatch.EndAction(action_data, "read"))
        {
            m_NimbyNotesMenuSeen = false;
        }
    }
}

modded class ActionWritePaper
{
    protected bool m_NimbyNotesMenuSeen;

    override void OnStartClient(ActionData action_data)
    {
        super.OnStartClient(action_data);
        m_NimbyNotesMenuSeen = false;
    }

    override void OnUpdate(ActionData action_data)
    {
        super.OnUpdate(action_data);
        if (g_Game.IsHeadlessOrDedicatedServer())
        {
            return;
        }
        if (NimbyNotesMenuWatch.MenuOpen())
        {
            m_NimbyNotesMenuSeen = true;
            return;
        }
        if (!m_NimbyNotesMenuSeen)
        {
            return;
        }
        if (NimbyNotesMenuWatch.EndAction(action_data, "write"))
        {
            m_NimbyNotesMenuSeen = false;
        }
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
