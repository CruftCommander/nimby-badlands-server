// -----------------------------------------------------------------------------
// NimbyNotes - action registration and the write rule.
// Purpose : (N5) Registers ActionWritePaper and ActionReadPaper with the action
//           manager so AddAction on Paper and Pen_ColorBase resolves them; (N2)
//           restricts writing to a single sheet; (N11) ends either action when
//           the note menu closes; (N17) records the writer on the sheet at the
//           server start and refuses a second player's start while a write is
//           open; (N18) requires a world target within reach.
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
    // N17: how long one "someone is writing" box stands for on the client before another
    // approach shows it again.
    static const float NOTICE_GAP_SECONDS = 4.0;

    protected ref NimbyNotesMenuWatch m_NimbyNotesWatch;
    protected float m_NimbyNotesNoticeAt;

    void ActionWritePaper()
    {
        m_NimbyNotesWatch = new NimbyNotesMenuWatch("write");
        m_NimbyNotesNoticeAt = -100.0;
    }

    // N17: the server records the writer the moment the action is accepted, after vanilla has
    // bound the pen and the sheet (ActionWritePaper.c:78-90).
    override void OnStartServer(ActionData action_data)
    {
        super.OnStartServer(action_data);
        Paper sheet = NimbyNotesSheetOf(action_data);
        if (!sheet || !action_data.m_Player)
        {
            return;
        }
        sheet.NimbyNotesOpenWrite(action_data.m_Player.GetIdentity());
    }

    // N17: the action ended on the server, by the menu closing (OK or Cancel, through the N11
    // watcher), an interrupt, or the player leaving. The record outlives this by WRITE_LATE_MS
    // for the OK's RPC.
    override void OnEndServer(ActionData action_data)
    {
        super.OnEndServer(action_data);
        Paper sheet = NimbyNotesSheetOf(action_data);
        if (!sheet)
        {
            return;
        }
        sheet.NimbyNotesEndWrite();
    }

    // The sheet of a write, whichever side it is on (ActionWritePaper.c:22-30).
    protected static Paper NimbyNotesSheetOf(ActionData action_data)
    {
        if (!action_data || !action_data.m_MainItem || !action_data.m_Target)
        {
            return null;
        }
        if (action_data.m_MainItem.ConfigIsExisting("writingColor"))
        {
            return Paper.Cast(action_data.m_Target.GetObject());
        }
        return Paper.Cast(action_data.m_MainItem);
    }

    // N18: the target (the sheet in the pen-in-hands order, the pen in the sheet-in-hands order)
    // is in the writer's own inventory or within WRITE_REACH_METERS of the writer, measured on
    // whichever side runs this from object positions, never from the client's cursor. Vanilla
    // commented its own distance check out (ActionWritePaper.c:54), which let a client name
    // any sheet in its bubble as the target (review SCRAWL-R2-01).
    protected bool NimbyNotesTargetInReach(PlayerBase player, ActionTarget target)
    {
        Object obj = target.GetObject();
        if (!obj)
        {
            return false;
        }
        bool held = obj.GetHierarchyRootPlayer() == player;
        float fromFeet = vector.Distance(obj.GetPosition(), player.GetPosition());
        float fromHead = vector.Distance(obj.GetPosition(), MiscGameplayFunctions.GetPlayerHeadPosition(player));
        return NimbyNotesRules.WriteTargetInReach(held, fromFeet, fromHead);
    }

    // N17: a box on the client, once per approach, when the prompt is withheld because
    // another player's write is open on the sheet.
    protected void NimbyNotesLockedNotice(PlayerBase player)
    {
        if (GetGame().IsServer() && GetGame().IsMultiplayer())
        {
            return;
        }
        float now = GetGame().GetTickTime();
        if (now - m_NimbyNotesNoticeAt < NOTICE_GAP_SECONDS)
        {
            return;
        }
        m_NimbyNotesNoticeAt = now;
        NotificationSystem.SendNotificationToPlayerExtended(player, 3.0, "Notes", "Someone is writing on this sheet.", "");
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

    // The sheet of a condition: whichever side is not the pen (ActionWritePaper.c:57).
    protected static Paper NimbyNotesSheetOfCondition(ActionTarget target, ItemBase item)
    {
        if (item && item.ConfigIsExisting("writingColor"))
        {
            if (!target)
            {
                return null;
            }
            return Paper.Cast(target.GetObject());
        }
        return Paper.Cast(item);
    }

    // Vanilla's pair test (ActionWritePaper.c:59), restated so the continue condition can run
    // it without going through ActionCondition: a pen in hands with a Paper target, or a Paper
    // in hands with a pen target.
    protected static bool NimbyNotesPairOk(ActionTarget target, ItemBase item)
    {
        if (!item || !target)
        {
            return false;
        }
        Object obj = target.GetObject();
        if (!obj)
        {
            return false;
        }
        if (item.ConfigIsExisting("writingColor") && Paper.Cast(obj))
        {
            return true;
        }
        return obj.ConfigIsExisting("writingColor") && Paper.Cast(item);
    }

    // N2 (one sheet) and N18 (reach), after the pair test. Shared by the start condition and
    // the continue condition.
    protected bool NimbyNotesBaseCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (!NimbyNotesPairOk(target, item))
        {
            return false;
        }
        Paper sheet = NimbyNotesSheetOfCondition(target, item);
        if (!sheet || !player)
        {
            return false;
        }
        if (!NimbyNotesRules.CanWrite(sheet.GetQuantity()))
        {
            return false;
        }
        // N18: reach, on both sides. The server's answer is the one that counts
        // (ActionBase.Can, ActionManagerServer.c:168); the client's hides the prompt.
        if (!NimbyNotesTargetInReach(player, target))
        {
            if (GetGame().IsServer() && GetGame().IsMultiplayer())
            {
                NimbyNotesLog.InfoGated(NimbyNotesLog.IdentityKey(player.GetIdentity()), "write refused: target out of reach sheet=" + sheet.GetType() + " " + NimbyNotesLog.PlayerTag(player.GetIdentity()));
            }
            return false;
        }
        return true;
    }

    // The start condition: base plus N17, one open write per sheet. The client sees the
    // net-synced lock and withholds the prompt with a box; the server knows the writer and
    // lets the same player restart.
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (!super.ActionCondition(player, target, item))
        {
            return false;
        }
        if (!NimbyNotesBaseCondition(player, target, item))
        {
            return false;
        }
        Paper sheet = NimbyNotesSheetOfCondition(target, item);
        if (GetGame().IsServer() && GetGame().IsMultiplayer())
        {
            if (!sheet.NimbyNotesCanStartWrite(player.GetIdentity()))
            {
                NimbyNotesLog.InfoGated(NimbyNotesLog.IdentityKey(player.GetIdentity()), "write refused: another write is open sheet=" + sheet.GetType() + " " + NimbyNotesLog.PlayerTag(player.GetIdentity()));
                return false;
            }
            return true;
        }
        if (sheet.NimbyNotesIsWriteLocked())
        {
            NimbyNotesLockedNotice(player);
            return false;
        }
        return true;
    }

    // The continue condition runs every tick of the running action (AnimatedActionBase.c:239).
    // It must not read the lock: the writer's own lock syncs to their client before the menu
    // opens, and vanilla's default would end the action on it.
    override bool ActionConditionContinue(ActionData action_data)
    {
        if (!action_data)
        {
            return false;
        }
        return NimbyNotesBaseCondition(action_data.m_Player, action_data.m_Target, action_data.m_MainItem);
    }
}
