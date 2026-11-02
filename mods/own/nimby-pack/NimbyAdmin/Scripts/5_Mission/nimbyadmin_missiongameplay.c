// -----------------------------------------------------------------------------
// NimbyAdmin - client orchestration (Plan 3 v0.8, recon 8.3 to 8.5, rulings of October 31, 2026).
// Purpose : Owns the role the server reported, the one pending request, and the Players
//           receiver. After OnPlayerLoaded on the controlled player it sends session.hello
//           (ruling 8); the server answers with a role push (kind 1). The window opens on Home
//           (UANimbyAdminOpen, read here and nowhere else, NIMBY-T-046) or from the pause-menu
//           entry, only while the role is owner or admin; with no role a press does nothing and
//           sends nothing (gate row 1). The client decides nothing: the server authorizes every
//           request again (shared rules 8.1).
// Timers  : No CallLater (shared rules 7.1). The request timeout and the open-after-pause wait are
//           tick-time checks in OnUpdate.
// Vanilla : UAInput.LocalPress and GetInputByName (3_Game/DayZ/InputAPI/UAInput.c:60,218),
//           GetPersistentWrapper (UAInput.c:99, the Greet precedent); UIManager.GetMenu and
//           ShowScriptedMenu (3_Game/DayZ/tools/UIManager.c:59); CGame.GetTickTime
//           (3_Game/DayZ/Global/Game.c:914); JsonSerializer.ReadFromString (3_Game/DayZ/gameplay.c:100).
// Style   : One statement per line.
// -----------------------------------------------------------------------------

modded class MissionGameplay extends MissionBase
{
    protected UAIDWrapper m_NimbyAdminInput;
    protected string m_NimbyAdminRole = "none";
    protected int m_NimbyAdminRoleSequence;
    protected ref NimbyAdminMenu m_NimbyAdminMenu;
    protected int m_NimbyAdminPendingId;
    protected string m_NimbyAdminPendingCommand = "";
    protected float m_NimbyAdminPendingSince;
    protected ref NimbySharedChunkReceiver m_NimbyAdminList;
    protected bool m_NimbyAdminOpenWhenClear;

    override void OnInit()
    {
        super.OnInit();
        m_NimbyAdminList = new NimbySharedChunkReceiver(NimbyAdminRules.MAX_LIST_BYTES);
        if (GetUApi())
        {
            UAInput open = GetUApi().GetInputByName("UANimbyAdminOpen");
            if (open)
            {
                m_NimbyAdminInput = open.GetPersistentWrapper();
            }
        }
        NimbyAdminClient.Spawned().Insert(NimbyAdminOnSpawned);
        NimbyAdminClient.Result().Insert(NimbyAdminOnResult);
        NimbyAdminClient.Push().Insert(NimbyAdminOnPush);
        NimbyAdminClient.Chunk().Insert(NimbyAdminOnChunk);
        NimbyAdminClient.Done().Insert(NimbyAdminOnDone);
        // Phase 1 A4: every value is a local before the call (the 164200 argument-slot rule).
        NimbyAdminPauseEntry pauseEntry = new NimbyAdminPauseEntry();
        NimbySharedPauseButtons.Register(pauseEntry);
        NimbyAdminPage playersPage = new NimbyAdminPlayersPage();
        NimbyAdminPages.Register(playersPage);
        bool bound = m_NimbyAdminInput != null;
        string boundText = NimbyAdminLog.Bool(bound);
        NimbyAdminLog.Debug("client ready; input bound=" + boundText);
    }

    override void OnMissionFinish()
    {
        NimbyAdminClient.Spawned().Remove(NimbyAdminOnSpawned);
        NimbyAdminClient.Result().Remove(NimbyAdminOnResult);
        NimbyAdminClient.Push().Remove(NimbyAdminOnPush);
        NimbyAdminClient.Chunk().Remove(NimbyAdminOnChunk);
        NimbyAdminClient.Done().Remove(NimbyAdminOnDone);
        super.OnMissionFinish();
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        NimbyAdminHandleHotkey();
        float now = GetGame().GetTickTime();
        if (m_NimbyAdminPendingId != 0 && now - m_NimbyAdminPendingSince > NimbyAdminRules.REQUEST_TIMEOUT_SECONDS)
        {
            string pendingText = m_NimbyAdminPendingId.ToString();
            NimbyAdminLog.Debug("request id=" + pendingText + " command=" + m_NimbyAdminPendingCommand + " no answer");
            m_NimbyAdminPendingId = 0;
            NimbyAdminStatus("No answer from the server.");
        }
        if (m_NimbyAdminOpenWhenClear && NimbyAdminCurrentMenuId() == -1)
        {
            m_NimbyAdminOpenWhenClear = false;
            NimbyAdminOpenMenu("menu");
        }
    }

    bool NimbyAdminHasRole()
    {
        return NimbyAdminRules.IsKnownRole(m_NimbyAdminRole);
    }

    void NimbyAdminOpenWhenClear(string source)
    {
        m_NimbyAdminOpenWhenClear = true;
    }

    // ---- events from PlayerBase ---------------------------------------------------------------

    protected void NimbyAdminOnSpawned(PlayerBase player)
    {
        NimbyAdminSend(NimbyAdminRules.CMD_HELLO, "");
    }

    protected void NimbyAdminOnPush(int kind, int sequence, string payload)
    {
        if (!NimbyAdminPushKinds.IsKnown(kind))
        {
            string kindText = kind.ToString();
            NimbyAdminLog.Debug("push dropped: unknown kind=" + kindText);
            return;
        }
        if (sequence <= m_NimbyAdminRoleSequence)
        {
            string oldText = sequence.ToString();
            NimbyAdminLog.Debug("push dropped: old sequence=" + oldText);
            return;
        }
        m_NimbyAdminRoleSequence = sequence;
        string role = "none";
        if (NimbyAdminRules.IsKnownRole(payload))
        {
            role = payload;
        }
        m_NimbyAdminRole = role;
        string sequenceText = sequence.ToString();
        NimbyAdminLog.Debug("role received role=" + role + " sequence=" + sequenceText);
        if (m_NimbyAdminMenu)
        {
            if (!NimbyAdminHasRole())
            {
                m_NimbyAdminMenu.RequestClose("role");
                return;
            }
            m_NimbyAdminMenu.SetRole(role);
        }
    }

    protected void NimbyAdminOnResult(int requestId, int code, string detail)
    {
        if (requestId == m_NimbyAdminPendingId)
        {
            m_NimbyAdminPendingId = 0;
        }
        string idText = requestId.ToString();
        string codeName = NimbyAdminRules.CodeName(code);
        NimbyAdminLog.Debug("result id=" + idText + " code=" + codeName + " detail=" + detail);
        if (requestId == 0)
        {
            return;
        }
        string statusText = codeName + ": " + detail;
        NimbyAdminStatus(statusText);
    }

    protected void NimbyAdminOnChunk(int requestId, int index, string text)
    {
        string why;
        float now = GetGame().GetTickTime();
        if (!m_NimbyAdminList.AcceptChunk(requestId, index, text, now, why))
        {
            string chunkIdText = requestId.ToString();
            string indexText = index.ToString();
            NimbyAdminLog.Debug("chunk dropped id=" + chunkIdText + " index=" + indexText + " why=" + why);
            return;
        }
        NimbyAdminFinishList();
    }

    protected void NimbyAdminOnDone(int requestId, int chunkCount, string doneJson)
    {
        NimbyAdminDone done = new NimbyAdminDone();
        JsonSerializer js = new JsonSerializer();
        string error;
        bool readable = js.ReadFromString(done, doneJson, error);
        string why;
        float now = GetGame().GetTickTime();
        if (!m_NimbyAdminList.AcceptDone(requestId, chunkCount, readable, done.bytes, done.hash, now, why))
        {
            string doneIdText = requestId.ToString();
            NimbyAdminLog.Debug("done dropped id=" + doneIdText + " why=" + why);
            return;
        }
        NimbyAdminFinishList();
    }

    protected void NimbyAdminFinishList()
    {
        if (!m_NimbyAdminList.IsComplete())
        {
            return;
        }
        int requestId = m_NimbyAdminList.m_RequestId;
        string payload = m_NimbyAdminList.Assemble();
        string verdict = m_NimbyAdminList.Verify(payload);
        m_NimbyAdminList.Begin(0, 0);
        if (requestId == m_NimbyAdminPendingId)
        {
            m_NimbyAdminPendingId = 0;
        }
        string listIdText = requestId.ToString();
        if (verdict != "intact")
        {
            NimbyAdminLog.Info("client: players list rejected id=" + listIdText + " " + verdict);
            NimbyAdminStatus("The player list arrived damaged; refresh.");
            return;
        }
        NimbyAdminPlayerList list = new NimbyAdminPlayerList();
        JsonSerializer js = new JsonSerializer();
        string error;
        if (!js.ReadFromString(list, payload, error))
        {
            NimbyAdminLog.Info("client: players list unreadable id=" + listIdText);
            return;
        }
        int listCount = list.players.Count();
        string listCountText = listCount.ToString();
        int payloadBytes = payload.Length();
        string payloadText = payloadBytes.ToString();
        NimbyAdminLog.Debug("players list id=" + listIdText + " players=" + listCountText + " bytes=" + payloadText);
        if (m_NimbyAdminMenu)
        {
            string notice = m_NimbyAdminMenu.SetPlayers(list);
            string status = list.players.Count().ToString() + " players.";
            if (notice != "")
            {
                status = status + " " + notice + ".";
            }
            m_NimbyAdminMenu.SetStatus(status);
        }
    }

    // ---- requests -----------------------------------------------------------------------------

    // Sends one request; returns its id, or 0 when there is no controlled player.
    protected int NimbyAdminSend(string command, string args)
    {
        Man man = GetGame().GetPlayer();
        PlayerBase player = PlayerBase.Cast(man);
        if (!player)
        {
            return 0;
        }
        int requestId = NimbyAdminRequestIds.Next();
        player.NimbyAdminSendRequest(requestId, command, args);
        if (command != NimbyAdminRules.CMD_HELLO)
        {
            m_NimbyAdminPendingId = requestId;
            m_NimbyAdminPendingCommand = command;
            m_NimbyAdminPendingSince = GetGame().GetTickTime();
        }
        return requestId;
    }

    void NimbyAdminRequestPlayers()
    {
        int requestId = NimbyAdminSend(NimbyAdminRules.CMD_PLAYERS, "");
        if (requestId != 0)
        {
            float now = GetGame().GetTickTime();
            m_NimbyAdminList.Begin(requestId, now);
            NimbyAdminStatus("Refreshing...");
        }
    }

    void NimbyAdminRequestTeleportTo(NimbyAdminPlayerRow target)
    {
        NimbyAdminArgs args = new NimbyAdminArgs();
        args.Set("target", target.biId);
        // Phase 1 A4: the session number and the encoded args are locals before the calls.
        string sessionText = target.session.ToString();
        args.Set("session", sessionText);
        string encoded = args.Encode();
        int sent = NimbyAdminSend(NimbyAdminRules.CMD_TELEPORT_TO, encoded);
        if (sent != 0)
        {
            NimbyAdminStatus("Teleport to " + target.name + " requested.");
        }
    }

    protected void NimbyAdminStatus(string text)
    {
        if (m_NimbyAdminMenu)
        {
            m_NimbyAdminMenu.SetStatus(text);
        }
    }

    // ---- window -------------------------------------------------------------------------------

    protected void NimbyAdminOpenMenu(string source)
    {
        if (!NimbyAdminHasRole())
        {
            NimbyAdminLog.Debug("open ignored: no role source=" + source);
            return;
        }
        if (m_NimbyAdminMenu)
        {
            return;
        }
        m_NimbyAdminMenu = new NimbyAdminMenu();
        m_NimbyAdminMenu.SetRole(m_NimbyAdminRole);
        GetGame().GetUIManager().ShowScriptedMenu(m_NimbyAdminMenu, null);
        NimbyAdminLog.Debug("open source=" + source + " role=" + m_NimbyAdminRole);
        NimbyAdminRequestPlayers();
    }

    // The window closed (Close from any path, through NimbySharedWindow.OnHide and the menu's
    // OnWindowClosed, the Greet NimbyGreetOnMenuClosed shape); the mission drops its reference.
    void NimbyAdminOnMenuClosed(NimbyAdminMenu menu)
    {
        if (m_NimbyAdminMenu == menu)
        {
            m_NimbyAdminMenu = null;
        }
    }

    protected void NimbyAdminHandleHotkey()
    {
        if (!m_NimbyAdminInput)
        {
            return;
        }
        UAInput open = m_NimbyAdminInput.InputP();
        if (!open || !open.LocalPress())
        {
            return;
        }
        if (!NimbyAdminHasRole())
        {
            NimbyAdminLog.Debug("hotkey ignored: no role");
            return;
        }
        int menuId = NimbyAdminCurrentMenuId();
        if (menuId == NimbyAdminRules.MENU_ID && m_NimbyAdminMenu)
        {
            m_NimbyAdminMenu.RequestClose("hotkey");
            return;
        }
        if (menuId != -1)
        {
            string menuText = menuId.ToString();
            NimbyAdminLog.Debug("hotkey ignored: another menu is open id=" + menuText);
            return;
        }
        NimbyAdminOpenMenu("hotkey");
    }

    protected int NimbyAdminCurrentMenuId()
    {
        UIScriptedMenu menu = GetGame().GetUIManager().GetMenu();
        if (!menu)
        {
            return -1;
        }
        return menu.GetID();
    }
}
