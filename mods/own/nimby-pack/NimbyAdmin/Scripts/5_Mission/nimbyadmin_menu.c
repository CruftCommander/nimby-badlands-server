// -----------------------------------------------------------------------------
// NimbyAdmin - the Central Scrutinizer window (Plan 3 v0.8 N16, recon 8.2).
// Purpose : NimbyAdminMenu is a NimbySharedWindow subclass (Plan 11 C5). The shell's section
//           column lists the registered pages (N9c); the content layout holds the player list
//           (left), the detail pane (BodyScroll, so the shared page keys page it), the Teleport
//           to and Refresh buttons, the status line, and Close. The window renders and sends;
//           it decides nothing (shared rules 8.1). Every request goes through MissionGameplay
//           (nimbyadmin_missiongameplay.c), which owns the role and the pending request.
// Pages   : NimbyAdminPage subclasses register in MissionGameplay.OnInit through
//           NimbyAdminPages, the shape of NimbySharedPauseButtons (replace by id, 5_Mission
//           only, NIMBY-T-052). Phase 0 registers Players; the self-test registers and removes a
//           fake page.
// Rules   : Shared rules 7.1: no CallLater, Call or CallByName here.
// Vanilla : WorkspaceWidget.CreateWidgets (1_Core/DayZ/proto/EnWidgets.c:181); Widget.SetUserID,
//           GetUserID; RichTextWidget.SetText.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyAdminPage
{
    string Id()
    {
        return "";
    }

    string Label()
    {
        return "";
    }

    // Called when the page's section is selected.
    void Show(NimbyAdminMenu menu)
    {
    }
}

class NimbyAdminPlayersPage extends NimbyAdminPage
{
    override string Id()
    {
        return "players";
    }

    override string Label()
    {
        return "Players";
    }

    override void Show(NimbyAdminMenu menu)
    {
        menu.ShowPlayers();
    }
}

class NimbyAdminPages
{
    protected static ref array<ref NimbyAdminPage> s_Pages;

    protected static void Ensure()
    {
        if (!s_Pages)
        {
            s_Pages = new array<ref NimbyAdminPage>;
        }
    }

    // A page with the same id replaces the old one in place; an empty id is refused.
    static void Register(NimbyAdminPage page)
    {
        Ensure();
        if (!page || page.Id() == "")
        {
            return;
        }
        int at = IndexOf(page.Id());
        if (at >= 0)
        {
            s_Pages.Set(at, page);
            return;
        }
        s_Pages.Insert(page);
    }

    static void Unregister(string id)
    {
        Ensure();
        int at = IndexOf(id);
        if (at >= 0)
        {
            s_Pages.Remove(at);
        }
    }

    static int Count()
    {
        Ensure();
        return s_Pages.Count();
    }

    static NimbyAdminPage Get(int index)
    {
        Ensure();
        if (index < 0 || index >= s_Pages.Count())
        {
            return null;
        }
        return s_Pages.Get(index);
    }

    static int IndexOf(string id)
    {
        Ensure();
        int i = 0;
        while (i < s_Pages.Count())
        {
            NimbyAdminPage page = s_Pages.Get(i);
            if (page && page.Id() == id)
            {
                return i;
            }
            i = i + 1;
        }
        return -1;
    }
}

class NimbyAdminMenu extends NimbySharedWindow
{
    static const string CONTENT_LAYOUT = "NimbyAdmin/GUI/nimbyadmin_content.layout";
    static const string ROW_LAYOUT = "NimbyAdmin/GUI/nimbyadmin_player_row.layout";
    static const string DETAIL_LAYOUT = "NimbyAdmin/GUI/nimbyadmin_detail_text.layout";

    // userIDs this window uses (the base keeps 1 and 100 to 199).
    static const int ID_TELEPORT = 10;
    static const int ID_REFRESH = 11;
    static const int ID_ROW_BASE = 200;
    static const int ID_ROW_END = 400;

    protected Widget m_PlayerRows;
    protected Widget m_BodyContent;
    protected TextWidget m_Status;
    protected Widget m_BtnTeleport;
    protected Widget m_BtnRefresh;
    protected Widget m_BtnClose;
    protected ref array<Widget> m_RowWidgets;
    protected ref NimbyAdminPlayerList m_Players;
    protected int m_SelectedRow = -1;
    protected string m_StatusText = "";
    protected string m_Role = "none";

    void NimbyAdminMenu()
    {
        SetID(NimbyAdminRules.MENU_ID);
        m_RowWidgets = new array<Widget>;
        m_Players = new NimbyAdminPlayerList();
    }

    void SetRole(string role)
    {
        m_Role = role;
        if (!layoutRoot)
        {
            return;
        }
        BuildChrome();
    }

    override protected string ContentLayout()
    {
        return CONTENT_LAYOUT;
    }

    override protected void OnWindowInit()
    {
        m_PlayerRows = layoutRoot.FindAnyWidget("PlayerRows");
        m_BodyContent = layoutRoot.FindAnyWidget("BodyContent");
        m_Status = TextWidget.Cast(layoutRoot.FindAnyWidget("Status"));
        m_BtnTeleport = layoutRoot.FindAnyWidget("BtnTeleport");
        m_BtnRefresh = layoutRoot.FindAnyWidget("BtnRefresh");
        m_BtnClose = layoutRoot.FindAnyWidget("BtnClose");
        SetButtonLabel(m_BtnTeleport, "Teleport to");
        SetButtonLabel(m_BtnRefresh, "Refresh");
        SetButtonLabel(m_BtnClose, "Close");
        PaintButtonNormal(m_BtnTeleport);
        PaintButtonNormal(m_BtnRefresh);
        PaintButtonNormal(m_BtnClose);
    }

    override protected void BuildChrome()
    {
        SetTitle("Central Scrutinizer", "Role: " + m_Role);
        SetHint("Select a player, then an action. Every action is audited.");
    }

    override protected int SectionCount()
    {
        return NimbyAdminPages.Count();
    }

    override protected string SectionLabel(int index)
    {
        NimbyAdminPage page = NimbyAdminPages.Get(index);
        if (!page)
        {
            return "";
        }
        return page.Label();
    }

    override protected void OnSectionSelected(int index)
    {
        NimbyAdminPage page = NimbyAdminPages.Get(index);
        if (page)
        {
            page.Show(this);
        }
    }

    override protected Widget InitialFocus()
    {
        return m_BtnRefresh;
    }

    override protected int NormalPanelColor(Widget w)
    {
        int id = w.GetUserID();
        if (id >= ID_ROW_BASE && id < ID_ROW_END)
        {
            if (id - ID_ROW_BASE == m_SelectedRow)
            {
                return COLOR_PANEL_SELECTED;
            }
            return COLOR_PANEL_NORMAL;
        }
        return COLOR_PANEL_BUTTON;
    }

    override protected void OnWindowClosed()
    {
        MissionGameplay gameplay = MissionGameplay.Cast(g_Game.GetMission());
        if (gameplay)
        {
            gameplay.NimbyAdminOnMenuClosed(this);
        }
    }

    override void RequestClose(string reason)
    {
        NimbyAdminLog.Debug("menu close reason=" + reason);
        Close();
    }

    // ---- data from MissionGameplay ------------------------------------------------------------

    // Review finding 2: the selection is kept only for the same BI id and session number
    // (NimbyAdminSelection.Rebind). Returns "" or the notice for the status line.
    string SetPlayers(NimbyAdminPlayerList list)
    {
        string selectedBi = "";
        int selectedSession = 0;
        NimbyAdminPlayerRow before = SelectedPlayer();
        if (before)
        {
            selectedBi = before.biId;
            selectedSession = before.session;
        }
        m_Players = list;
        string notice;
        m_SelectedRow = NimbyAdminSelection.Rebind(m_Players, selectedBi, selectedSession, notice);
        ShowPlayers();
        return notice;
    }

    void SetStatus(string text)
    {
        m_StatusText = text;
        if (m_Status)
        {
            m_Status.SetText(text);
        }
    }

    NimbyAdminPlayerRow SelectedPlayer()
    {
        if (!m_Players)
        {
            return null;
        }
        if (m_SelectedRow < 0 || m_SelectedRow >= m_Players.players.Count())
        {
            return null;
        }
        return m_Players.players.Get(m_SelectedRow);
    }

    // ---- the Players page ---------------------------------------------------------------------

    void ShowPlayers()
    {
        if (!m_PlayerRows)
        {
            return;
        }
        foreach (Widget old : m_RowWidgets)
        {
            if (old)
            {
                old.Unlink();
            }
        }
        m_RowWidgets.Clear();
        int i = 0;
        while (i < m_Players.players.Count() && ID_ROW_BASE + i < ID_ROW_END)
        {
            NimbyAdminPlayerRow row = m_Players.players.Get(i);
            Widget button = g_Game.GetWorkspace().CreateWidgets(ROW_LAYOUT, m_PlayerRows);
            if (!button)
            {
                NimbyAdminLog.Info("client: nimbyadmin_player_row.layout failed to load");
                break;
            }
            button.SetUserID(ID_ROW_BASE + i);
            string label = row.name;
            if (!row.alive)
            {
                label = label + " (dead)";
            }
            SetButtonLabel(button, label);
            m_RowWidgets.Insert(button);
            PaintButtonNormal(button);
            i = i + 1;
        }
        if (m_PlayerRows)
        {
            m_PlayerRows.Update();
        }
        ShowDetail();
        SetStatus(m_StatusText);
    }

    protected void ShowDetail()
    {
        if (!m_BodyContent)
        {
            return;
        }
        Widget child = m_BodyContent.GetChildren();
        while (child)
        {
            Widget next = child.GetSibling();
            child.Unlink();
            child = next;
        }
        RichTextWidget text = RichTextWidget.Cast(g_Game.GetWorkspace().CreateWidgets(DETAIL_LAYOUT, m_BodyContent));
        if (!text)
        {
            return;
        }
        NimbyAdminPlayerRow row = SelectedPlayer();
        if (!row)
        {
            text.SetText(m_Players.players.Count().ToString() + " players online. Select one.");
            m_BodyContent.Update();
            return;
        }
        string body = row.name + "\nSteam64: " + row.steam64 + "\nBI id: " + row.biId;
        body = body + "\nSession: " + row.session.ToString() + "\nAlive: " + NimbyAdminLog.Bool(row.alive);
        body = body + "\nDistance: " + Math.Round(row.distance).ToString() + " m";
        text.SetText(body);
        m_BodyContent.Update();
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (super.OnClick(w, x, y, button))
        {
            return true;
        }
        if (!w)
        {
            return false;
        }
        int id = w.GetUserID();
        if (id >= ID_ROW_BASE && id < ID_ROW_END)
        {
            m_SelectedRow = id - ID_ROW_BASE;
            foreach (Widget row : m_RowWidgets)
            {
                PaintButtonNormal(row);
            }
            ShowDetail();
            return true;
        }
        MissionGameplay gameplay = MissionGameplay.Cast(g_Game.GetMission());
        if (!gameplay)
        {
            return false;
        }
        if (id == ID_REFRESH)
        {
            gameplay.NimbyAdminRequestPlayers();
            return true;
        }
        if (id == ID_TELEPORT)
        {
            NimbyAdminPlayerRow target = SelectedPlayer();
            if (!target)
            {
                SetStatus("Select a player first.");
                return true;
            }
            gameplay.NimbyAdminRequestTeleportTo(target);
            return true;
        }
        return false;
    }
}
