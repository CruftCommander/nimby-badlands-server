// -----------------------------------------------------------------------------
// NimbyShared - the window shell (Plan 11 v0.2, C5 as built).
// Purpose : NimbySharedWindow is the UIScriptedMenu every NIMBY window subclasses. It owns
//           NimbyShared/GUI/nimbyshared_window.layout (dim, panel, title, tagline, the section
//           column, the hint) in the vanilla look, and loads the subclass's content layout into
//           the panel-sized "Content" frame (R1), so the subclass keeps every coordinate of its
//           own body and footer. The base owns open and close with the input exclude and the
//           zeroing guard, the section column, Esc, the tab and page inputs, focus and hover
//           painting, and the text-size cycle. The subclass supplies the content through the
//           virtuals below. The base names no addon (shared rules 8.1).
// Origin  : NimbyGreetMenu (Greet plan v0.4, G4, G7, G16), moved in Plan 11 Phase 1; behavior
//           and colors unchanged. Findings this file carries:
//           - NIMBY-T-039: OnShow adds the vanilla "menu" input exclude (PlayerBase.c:5812,
//             missionGameplay.c:894; Mission.AddActiveInputExcludes, gameplay.c:800) and OnHide
//             removes exactly that (gameplay.c:798), so the player stops moving while reading.
//           - NIMBY-T-058: the "menu" exclude does not cover UAZeroingUp and UAZeroingDown (Page
//             Up, Page Down, = and -, the D-pad; preset_X1MouseKey.xml:309-318), so OnShow
//             force-disables both (UAInput.ForceDisable, UAInput.c:94); OnHide and the destructor
//             release them, once.
//           - NIMBY-T-046: a shell window never polls an addon's open key; the addon's mission
//             reads it in one place.
//           - The body scrolls in a ScrollWidget (EnWidgets.c:481), which clamps itself.
// Inputs  : UAUIBack closes (NoteMenu.c:152-158 pattern). UAUITabLeft and UAUITabRight ([ ] and the
//           shoulder buttons, preset_X1MouseKey.xml:631-637) move between sections; UAUINextUp and
//           UAUINextDown (the triggers, :696-701) and NimbyShared's UANimbySharedPageUp and
//           UANimbySharedPageDown (Page Up and Page Down, Data/Inputs.xml; Plan 11 question 1)
//           page the body; the wheel scrolls it natively; the arrow keys, the D-pad and the stick
//           move the engine's own button focus; UAUISelect activates. Buttons show the vanilla
//           hover and focus state (black panel, red label, InGameMenu.c:365-380).
// Rules   : Shared rules 7.1: this class queues no CallLater, Call or CallByName; a subclass that
//           needs a timer queues it through its own 4_World hook.
// Logs    : [NIMBY][Shared] lines end in menu=<GetID()> so two windows can be told apart.
// Vanilla : UIScriptedMenu (3_Game/DayZ/tools/UIScriptedMenu.c:66: Init :163, OnShow :173, OnHide
//           :184, Update :195, OnClick :238); Widget.TranslateString (1_Core/DayZ/proto/
//           EnWidgets.c:112); WorkspaceWidget.CreateWidgets (EnWidgets.c:181); UAInput.LocalPress
//           and GetInputByName (UAInput.c:60, :218).
// Client  : UIScriptedMenu exists only on the client; the server never builds one.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbySharedWindow extends UIScriptedMenu
{
    static const string LAYOUT = "NimbyShared/GUI/nimbyshared_window.layout";
    static const string SECTION_BUTTON_LAYOUT = "NimbyShared/GUI/nimbyshared_section_button.layout";
    static const string INPUT_PAGE_UP = "UANimbySharedPageUp";
    static const string INPUT_PAGE_DOWN = "UANimbySharedPageDown";

    // userID ranges the base handles; a subclass uses any other id (Greet: 2 to 5 and 200 to 399).
    static const int ID_CLOSE = 1;
    static const int ID_SECTION_BASE = 100;
    static const int ID_SECTION_END = 200;

    static const int SECTION_TEXT_SIZE = 17;

    // Greet Phase 1 run 1 (Jeff): footer and link buttons sit on a visible dark panel in their normal
    // state; the section list stays flat with a lit selected row; hover and focus are the vanilla
    // pause-menu state (black panel, red label).
    static const int COLOR_PANEL_HIGHLIGHT = 0xFF000000;
    static const int COLOR_PANEL_NORMAL = 0x00000000;
    static const int COLOR_PANEL_BUTTON = 0xD2333333;
    static const int COLOR_PANEL_SELECTED = 0xFF2A2A2A;
    static const int COLOR_LABEL_HIGHLIGHT = 0xFFFF0000;
    static const int COLOR_LABEL_NORMAL = 0xFFFFFFFF;
    static const int COLOR_LABEL_DIM = 0xFFCCCCCC;

    protected float m_TextScale = 1.0;
    protected int m_Selected = -1;
    protected bool m_AddedExclude;
    protected bool m_ZeroingDisabled;   // NIMBY-T-058

    protected TextWidget m_Title;
    protected TextWidget m_Tagline;
    protected TextWidget m_Hint;
    protected ScrollWidget m_SectionScroll;
    protected Widget m_SectionList;
    protected Widget m_Content;          // the panel-sized slot the content layout is created in (R1)
    protected ScrollWidget m_BodyScroll; // "BodyScroll" in the content layout, or null: the page target
    protected ref array<Widget> m_SectionButtons;

    void NimbySharedWindow()
    {
        m_SectionButtons = new array<Widget>;
    }

    // NIMBY-T-058: a menu destroyed without OnHide must not leave the weapon's zeroing disabled.
    // The NimbyShared self-test proves the base destructor runs after a subclass's own (risk 1).
    void ~NimbySharedWindow()
    {
        SetZeroingDisabled(false);
    }

    // ---- supplied by the subclass ------------------------------------------------------------

    // The content layout, created inside the panel-sized "Content" frame; "" for none.
    protected string ContentLayout()
    {
        return "";
    }

    // Look up the content widgets. Runs once in Init, after the content layout is created.
    protected void OnWindowInit()
    {
    }

    // Title, tagline, hint, and the subclass's own labels; calls SetTitle and SetHint.
    protected void BuildChrome()
    {
    }

    protected int SectionCount()
    {
        return 0;
    }

    protected string SectionLabel(int index)
    {
        return "";
    }

    // Build the body of the section just selected.
    protected void OnSectionSelected(int index)
    {
    }

    protected Widget InitialFocus()
    {
        return null;
    }

    // Per frame, before the base reads its inputs.
    protected void OnWindowUpdate(float timeslice)
    {
    }

    // After CycleTextSize has set m_TextScale and called ApplyScale: persist and relabel.
    protected void OnTextScaleChanged()
    {
    }

    // The normal-state panel color of a button the base paints; Greet's link labels are flat.
    protected int NormalPanelColor(Widget w)
    {
        return COLOR_PANEL_BUTTON;
    }

    // From OnHide, after the zeroing guard and the input exclude are released.
    protected void OnWindowClosed()
    {
    }

    // Esc and the ID_CLOSE button come here with reason "player".
    void RequestClose(string reason)
    {
        Close();
    }

    // ---- lifecycle ---------------------------------------------------------------------------

    override Widget Init()
    {
        layoutRoot = g_Game.GetWorkspace().CreateWidgets(LAYOUT);
        if (!layoutRoot)
        {
            NimbySharedLog.Info("layout failed to load: " + LAYOUT + " menu=" + GetID().ToString());
            return null;
        }
        m_Title = TextWidget.Cast(layoutRoot.FindAnyWidget("Title"));
        m_Tagline = TextWidget.Cast(layoutRoot.FindAnyWidget("Tagline"));
        m_Hint = TextWidget.Cast(layoutRoot.FindAnyWidget("Hint"));
        m_SectionScroll = ScrollWidget.Cast(layoutRoot.FindAnyWidget("SectionScroll"));
        m_SectionList = layoutRoot.FindAnyWidget("SectionList");
        m_Content = layoutRoot.FindAnyWidget("Content");
        string content = ContentLayout();
        if (content != "")
        {
            Widget contentRoot = null;
            if (m_Content)
            {
                contentRoot = g_Game.GetWorkspace().CreateWidgets(content, m_Content);
            }
            if (!contentRoot)
            {
                NimbySharedLog.Info("layout failed to load: " + content + " menu=" + GetID().ToString());
                return null;
            }
        }
        m_BodyScroll = ScrollWidget.Cast(layoutRoot.FindAnyWidget("BodyScroll"));
        OnWindowInit();
        BuildChrome();
        BuildSections();
        SelectSection(0);
        Widget focus = InitialFocus();
        if (focus)
        {
            SetFocus(focus);
        }
        return layoutRoot;
    }

    override void OnShow()
    {
        super.OnShow();
        Mission mission = g_Game.GetMission();
        if (mission)
        {
            mission.AddActiveInputExcludes({"menu"});
            m_AddedExclude = true;
            NimbySharedLog.Debug("input exclude added: menu menu=" + GetID().ToString());
        }
        SetZeroingDisabled(true);
    }

    override void OnHide()
    {
        super.OnHide();
        SetZeroingDisabled(false);
        if (m_AddedExclude)
        {
            Mission mission = g_Game.GetMission();
            if (mission)
            {
                mission.RemoveActiveInputExcludes({"menu"}, false);
                NimbySharedLog.Debug("input exclude removed: menu menu=" + GetID().ToString());
            }
            m_AddedExclude = false;
        }
        OnWindowClosed();
    }

    // NIMBY-T-058: zeroing off while the window is open, back on when it closes. Only this window's
    // own disable is ever released; the flag keeps OnHide and the destructor from releasing twice.
    protected void SetZeroingDisabled(bool disabled)
    {
        if (disabled == m_ZeroingDisabled)
        {
            return;
        }
        UAInputAPI api = GetUApi();
        if (!api)
        {
            return;
        }
        api.GetInputByID(UAZeroingUp).ForceDisable(disabled);
        api.GetInputByID(UAZeroingDown).ForceDisable(disabled);
        m_ZeroingDisabled = disabled;
        NimbySharedLog.Debug("zeroing disabled=" + NimbySharedLog.Bool(disabled) + " menu=" + GetID().ToString());
    }

    override void Update(float timeslice)
    {
        super.Update(timeslice);
        if (!g_Game)
        {
            return;
        }
        OnWindowUpdate(timeslice);
        UAInputAPI api = GetUApi();
        if (!api)
        {
            return;
        }
        if (api.GetInputByID(UAUIBack).LocalPress())
        {
            RequestClose("player");
            return;
        }
        if (api.GetInputByID(UAUITabLeft).LocalPress())
        {
            SelectSection(m_Selected - 1);
            return;
        }
        if (api.GetInputByID(UAUITabRight).LocalPress())
        {
            SelectSection(m_Selected + 1);
            return;
        }
        if (api.GetInputByID(UAUINextUp).LocalPress() || Pressed(api, INPUT_PAGE_UP))
        {
            PageBody(-1);
            return;
        }
        if (api.GetInputByID(UAUINextDown).LocalPress() || Pressed(api, INPUT_PAGE_DOWN))
        {
            PageBody(1);
            return;
        }
    }

    // ---- chrome ------------------------------------------------------------------------------

    protected void SetTitle(string title, string tagline)
    {
        if (m_Title)
        {
            m_Title.SetText(title);
        }
        if (m_Tagline)
        {
            m_Tagline.SetText(tagline);
            m_Tagline.Show(tagline.Length() > 0);
        }
    }

    protected void SetHint(string text)
    {
        if (m_Hint)
        {
            m_Hint.SetText(text);
        }
    }

    // Widget.TranslateString returns the key without its # when the table has no entry for the
    // client's language (NIMBY-T-044); Greet Phase 1 run 1 showed raw keys after an in-session
    // language change, before the client restart that reloads mod string tables. English is the
    // fallback.
    static string Tr(string key, string fallback)
    {
        string text = Widget.TranslateString(key);
        if (text.Length() == 0)
        {
            return fallback;
        }
        string bare = key;
        if (key.IndexOf("#") == 0)
        {
            bare = key.Substring(1, key.Length() - 1);
        }
        if (text == bare)
        {
            return fallback;
        }
        return text;
    }

    protected void SetButtonLabel(Widget button, string text)
    {
        if (!button)
        {
            return;
        }
        TextWidget label = TextWidget.Cast(button.FindAnyWidget(button.GetName() + "_label"));
        if (label)
        {
            label.SetText(text);
        }
    }

    // ---- sections ----------------------------------------------------------------------------

    protected void BuildSections()
    {
        foreach (Widget old : m_SectionButtons)
        {
            if (old)
            {
                old.Unlink();
            }
        }
        m_SectionButtons.Clear();
        if (!m_SectionList)
        {
            return;
        }
        int count = SectionCount();
        int limit = ID_SECTION_END - ID_SECTION_BASE;
        if (count > limit)
        {
            NimbySharedLog.Info("sections=" + count.ToString() + " above " + limit.ToString() + "; the rest are not built menu=" + GetID().ToString());
            count = limit;
        }
        int i = 0;
        while (i < count)
        {
            Widget button = g_Game.GetWorkspace().CreateWidgets(SECTION_BUTTON_LAYOUT, m_SectionList);
            if (!button)
            {
                NimbySharedLog.Info("section button layout failed to load menu=" + GetID().ToString());
                return;
            }
            button.SetUserID(ID_SECTION_BASE + i);
            TextWidget label = TextWidget.Cast(button.FindAnyWidget("SectionBtn_label"));
            if (label)
            {
                label.SetText(SectionLabel(i));
                label.SetTextExactSize(Scaled(SECTION_TEXT_SIZE));
            }
            m_SectionButtons.Insert(button);
            i = i + 1;
        }
        if (m_SectionScroll)
        {
            m_SectionScroll.Update();
        }
    }

    void SelectSection(int index)
    {
        int count = SectionCount();
        if (count == 0)
        {
            return;
        }
        if (index < 0)
        {
            index = count - 1;
        }
        if (index >= count)
        {
            index = 0;
        }
        m_Selected = index;
        int b = 0;
        while (b < m_SectionButtons.Count())
        {
            PaintSectionButton(b, false);
            b = b + 1;
        }
        OnSectionSelected(index);
        if (m_BodyScroll)
        {
            m_BodyScroll.VScrollToPos(0);
        }
    }

    protected void PaintSectionButton(int index, bool hover)
    {
        if (index < 0 || index >= m_SectionButtons.Count())
        {
            return;
        }
        Widget button = m_SectionButtons.Get(index);
        Widget panel = button.FindAnyWidget("SectionBtn_panel");
        TextWidget label = TextWidget.Cast(button.FindAnyWidget("SectionBtn_label"));
        int panelColor = COLOR_PANEL_NORMAL;
        int labelColor = COLOR_LABEL_DIM;
        if (index == m_Selected)
        {
            panelColor = COLOR_PANEL_SELECTED;
            labelColor = COLOR_LABEL_NORMAL;
        }
        if (hover)
        {
            panelColor = COLOR_PANEL_HIGHLIGHT;
            labelColor = COLOR_LABEL_HIGHLIGHT;
        }
        if (panel)
        {
            panel.SetColor(panelColor);
        }
        if (label)
        {
            label.SetColor(labelColor);
        }
    }

    // ---- scale (Greet G16) -------------------------------------------------------------------

    protected int Scaled(int base)
    {
        return (int)(base * m_TextScale);
    }

    // The base rescales the section labels; a subclass adds its own text and calls super.
    protected void ApplyScale()
    {
        foreach (Widget button : m_SectionButtons)
        {
            TextWidget label = TextWidget.Cast(button.FindAnyWidget("SectionBtn_label"));
            if (label)
            {
                label.SetTextExactSize(Scaled(SECTION_TEXT_SIZE));
            }
        }
    }

    protected void CycleTextSize()
    {
        m_TextScale = NimbySharedRules.NextTextScale(m_TextScale);
        ApplyScale();
        OnTextScaleChanged();
    }

    // ---- inputs ------------------------------------------------------------------------------

    // An input by name, the Ears and Legs precedent (GetInputByName, UAInput.c:218); false when unbound.
    protected bool Pressed(UAInputAPI api, string name)
    {
        if (name == "")
        {
            return false;
        }
        UAInput input = api.GetInputByName(name);
        if (!input)
        {
            return false;
        }
        return input.LocalPress();
    }

    protected void PageBody(int direction)
    {
        if (!m_BodyScroll)
        {
            return;
        }
        float w = 0;
        float h = 0;
        m_BodyScroll.GetScreenSize(w, h);
        float pos = m_BodyScroll.GetVScrollPos() + direction * h * 0.8;
        if (pos < 0)
        {
            pos = 0;
        }
        m_BodyScroll.VScrollToPos(pos);
    }

    // ---- clicks and focus --------------------------------------------------------------------

    // True when the base handled the click; a subclass calls super first and handles the rest.
    override bool OnClick(Widget w, int x, int y, int button)
    {
        super.OnClick(w, x, y, button);
        if (!w)
        {
            return false;
        }
        int id = w.GetUserID();
        if (id == ID_CLOSE)
        {
            RequestClose("player");
            return true;
        }
        if (id >= ID_SECTION_BASE && id < ID_SECTION_END)
        {
            SelectSection(id - ID_SECTION_BASE);
            return true;
        }
        return false;
    }

    override bool OnMouseEnter(Widget w, int x, int y)
    {
        Highlight(w, true);
        return super.OnMouseEnter(w, x, y);
    }

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
        Highlight(w, false);
        return super.OnMouseLeave(w, enterW, x, y);
    }

    override bool OnFocus(Widget w, int x, int y)
    {
        Highlight(w, true);
        return super.OnFocus(w, x, y);
    }

    override bool OnFocusLost(Widget w, int x, int y)
    {
        Highlight(w, false);
        return super.OnFocusLost(w, x, y);
    }

    // The vanilla pause-menu state: black panel and red label on hover or focus (InGameMenu.c:365-380).
    protected void Highlight(Widget w, bool on)
    {
        if (!w)
        {
            return;
        }
        int id = w.GetUserID();
        if (id >= ID_SECTION_BASE && id < ID_SECTION_END)
        {
            PaintSectionButton(id - ID_SECTION_BASE, on);
            return;
        }
        Widget panel = w.FindAnyWidget(w.GetName() + "_panel");
        TextWidget label = TextWidget.Cast(w.FindAnyWidget(w.GetName() + "_label"));
        if (!panel && !label)
        {
            return;
        }
        int panelColor = NormalPanelColor(w);
        int labelColor = COLOR_LABEL_NORMAL;
        if (on)
        {
            panelColor = COLOR_PANEL_HIGHLIGHT;
            labelColor = COLOR_LABEL_HIGHLIGHT;
        }
        if (panel)
        {
            panel.SetColor(panelColor);
        }
        if (label)
        {
            label.SetColor(labelColor);
        }
    }

    protected void PaintButtonNormal(Widget button)
    {
        Highlight(button, false);
    }
}
