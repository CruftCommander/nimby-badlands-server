// -----------------------------------------------------------------------------
// NimbyGreet - the window (plan v0.4, G4, G5, G6, G7, G10, G13, G15, G16, G17).
// Purpose : One UIScriptedMenu built from NimbyGreet/GUI/nimbygreet_window.layout in the
//           vanilla look (DayZDefaultPanel, Metron fonts, the pause menu's button shape):
//           title and tagline, a vertical section list, the selected section's blocks in a
//           ScrollWidget, the section's link rows, and a footer with the dismissal toggle,
//           text size, animations, language and Close. Blocks come from the G13 converter
//           and are created from the small block layouts under GUI/.
// Phase 0 : the FAIL and the three findings this file answers:
//           - NIMBY-T-039: OnShow adds the vanilla "menu" input exclude (PlayerBase.c:5812,
//             missionGameplay.c:894) and OnHide removes exactly that (missionGameplay.c:847),
//             so the player stops moving while reading; UANimbyGreetOpen and the UAUI inputs
//             are outside the set.
//           - NIMBY-T-046: this menu never polls UANimbyGreetOpen; MissionGameplay.OnUpdate is
//             the one reader and closes the window through NimbyGreetClose.
//           - NIMBY-T-041: the body scrolls in a ScrollWidget (EnWidgets.c:481), which clamps
//             itself; RichTextWidget.SetContentOffset is not used.
//           - NIMBY-T-043, G15: media is an ImageWidget with "no wrap" 1; a clip is up to 8
//             power-of-two .edds frames flipped on a timer.
// Inputs  : UAUIBack closes (NoteMenu.c:152-158 pattern). UAUITabLeft and UAUITabRight
//           ([ ] and the shoulder buttons, preset_X1MouseKey.xml:631-637) move between
//           sections; UAUINextUp and UAUINextDown (the triggers, :696-701) page the body; the
//           wheel scrolls it natively; Tab and the stick move the engine's own button focus;
//           UAUISelect activates. Buttons show the vanilla hover and focus state (black
//           panel, red label, InGameMenu.c:365-380).
// Client  : UIScriptedMenu exists only on the client; the server never builds one.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyGreetClip
{
    ImageWidget image;
    int frames;
    int frame;
    int frameMs;
}

class NimbyGreetMenu extends UIScriptedMenu
{
    static const int ID_CLOSE = 1;
    static const int ID_NEVER = 2;
    static const int ID_TEXTSIZE = 3;
    static const int ID_ANIMATIONS = 4;
    static const int ID_LANGUAGE = 5;
    static const int ID_SECTION_BASE = 100;
    static const int ID_LINK_OPEN_BASE = 200;
    static const int ID_LINK_COPY_BASE = 300;

    static const int BODY_TEXT_SIZE = 18;
    static const int HEADING1_SIZE = 24;
    static const int HEADING2_SIZE = 20;
    static const int TABLE_TEXT_SIZE = 17;
    static const int SECTION_TEXT_SIZE = 17;
    static const int LINK_TEXT_SIZE = 15;
    static const int TABLE_ROW_HEIGHT = 28;
    static const int TABLE_RELAYOUT_FRAMES = 3;
    static const int BLOCK_GAP = 10;   // extra air around tables and rules (Phase 1 run 2, Jeff)
    static const int MEDIA_ALT_HEIGHT = 20;

    // Phase 1 run 1 (Jeff): buttons should look like buttons. Footer and link buttons sit on a visible
    // dark panel in their normal state; the section list stays flat with a lit selected row; hover
    // and focus are the vanilla pause-menu state (black panel, red label).
    static const int COLOR_PANEL_HIGHLIGHT = 0xFF000000;
    static const int COLOR_PANEL_NORMAL = 0x00000000;
    static const int COLOR_PANEL_BUTTON = 0xD2333333;
    static const int COLOR_PANEL_SELECTED = 0xFF2A2A2A;
    static const int COLOR_LABEL_HIGHLIGHT = 0xFFFF0000;
    static const int COLOR_LABEL_NORMAL = 0xFFFFFFFF;
    static const int COLOR_LABEL_DIM = 0xFFCCCCCC;

    protected ref NimbyGreetLanguage m_Language;
    protected ref NimbyGreetDone m_Done;
    protected ref NimbyGreetPrefs m_Prefs;
    protected ref NimbyGreetSeen m_Seen;
    protected ref NimbyGreetManifest m_Manifest;
    protected ref array<string> m_MediaIds;
    protected string m_Source = "spawn";
    protected string m_CloseReason = "";
    protected bool m_NeverChecked;
    protected bool m_AddedExclude;
    protected int m_Selected = -1;

    protected TextWidget m_Title;
    protected TextWidget m_Tagline;
    protected TextWidget m_Hint;
    protected ScrollWidget m_SectionScroll;
    protected Widget m_SectionList;
    protected ScrollWidget m_BodyScroll;
    protected Widget m_BodyContent;
    protected ScrollWidget m_LinkScroll;
    protected Widget m_LinkRows;
    protected Widget m_BtnClose;
    protected Widget m_BtnNever;
    protected Widget m_BtnTextSize;
    protected Widget m_BtnAnimations;
    protected Widget m_BtnLanguage;

    protected ref array<Widget> m_SectionButtons;
    protected ref array<Widget> m_BlockWidgets;
    // Table rows and their cells, re-measured for a few frames after each build (Phase 1 run 5): a
    // RichTextWidget measured right after SetText has not been laid out at its width yet, so a cell
    // that wraps reported one line and the row kept the base height (cut off at 1920x1080).
    protected ref array<Widget> m_TableRows;
    protected ref array<RichTextWidget> m_TableLeft;
    protected ref array<RichTextWidget> m_TableRight;
    protected int m_TableRelayoutFrames;
    protected ref array<Widget> m_LinkWidgets;
    protected ref array<ref NimbyGreetLink> m_Links;
    protected ref array<ref NimbyGreetClip> m_Clips;
    protected ref array<ref NimbyGreetConverted> m_ConvertedCache;
    protected ref array<Widget> m_ScaledBody;      // text widgets whose size follows the text scale
    protected ref array<int> m_ScaledBodyBase;

    void NimbyGreetMenu()
    {
        SetID(NimbyGreetRules.MENU_ID);
        m_SectionButtons = new array<Widget>;
        m_BlockWidgets = new array<Widget>;
        m_TableRows = new array<Widget>;
        m_TableLeft = new array<RichTextWidget>;
        m_TableRight = new array<RichTextWidget>;
        m_LinkWidgets = new array<Widget>;
        m_Links = new array<ref NimbyGreetLink>;
        m_Clips = new array<ref NimbyGreetClip>;
        m_ConvertedCache = new array<ref NimbyGreetConverted>;
        m_ScaledBody = new array<Widget>;
        m_ScaledBodyBase = new array<int>;
        m_MediaIds = new array<string>;
    }

    // Called before ShowScriptedMenu. prefs and seen are the mission's own objects (shared).
    void Setup(NimbyGreetLanguage language, NimbyGreetDone done, NimbyGreetPrefs prefs, NimbyGreetSeen seen, NimbyGreetManifest manifest, string source)
    {
        m_Language = language;
        m_Done = done;
        m_Prefs = prefs;
        m_Seen = seen;
        m_Manifest = manifest;
        m_Source = source;
        m_NeverChecked = seen.neverShow;
        m_MediaIds.Clear();
        if (manifest && manifest.media)
        {
            foreach (NimbyGreetMediaEntry entry : manifest.media)
            {
                if (entry)
                {
                    m_MediaIds.Insert(entry.id);
                }
            }
        }
    }

    // A new payload for an open window (language switch, G14): rebuild the content in place.
    void SetContent(NimbyGreetLanguage language, NimbyGreetDone done)
    {
        m_Language = language;
        m_Done = done;
        m_ConvertedCache.Clear();
        if (!layoutRoot)
        {
            return;
        }
        BuildChrome();
        BuildSections();
        m_Selected = -1;
        BuildBody(-1);
        BuildLinks();
        SelectSection(0);
    }

    void SetCloseReason(string reason)
    {
        m_CloseReason = reason;
    }

    string GetCloseReason()
    {
        return m_CloseReason;
    }

    bool IsNeverChecked()
    {
        return m_NeverChecked;
    }

    string GetSelectedSectionId()
    {
        if (!m_Language || m_Selected < 0 || m_Selected >= m_Language.sections.Count())
        {
            return "";
        }
        return m_Language.sections.Get(m_Selected).id;
    }

    // ---- lifecycle ---------------------------------------------------------------------------

    override Widget Init()
    {
        layoutRoot = g_Game.GetWorkspace().CreateWidgets("NimbyGreet/GUI/nimbygreet_window.layout");
        if (!layoutRoot)
        {
            NimbyGreetLog.Info("layout failed to load: NimbyGreet/GUI/nimbygreet_window.layout");
            return null;
        }
        m_Title = TextWidget.Cast(layoutRoot.FindAnyWidget("Title"));
        m_Tagline = TextWidget.Cast(layoutRoot.FindAnyWidget("Tagline"));
        m_Hint = TextWidget.Cast(layoutRoot.FindAnyWidget("Hint"));
        m_SectionScroll = ScrollWidget.Cast(layoutRoot.FindAnyWidget("SectionScroll"));
        m_SectionList = layoutRoot.FindAnyWidget("SectionList");
        m_BodyScroll = ScrollWidget.Cast(layoutRoot.FindAnyWidget("BodyScroll"));
        m_BodyContent = layoutRoot.FindAnyWidget("BodyContent");
        m_LinkScroll = ScrollWidget.Cast(layoutRoot.FindAnyWidget("LinkScroll"));
        m_LinkRows = layoutRoot.FindAnyWidget("LinkRows");
        m_BtnClose = layoutRoot.FindAnyWidget("BtnClose");
        m_BtnNever = layoutRoot.FindAnyWidget("BtnNever");
        m_BtnTextSize = layoutRoot.FindAnyWidget("BtnTextSize");
        m_BtnAnimations = layoutRoot.FindAnyWidget("BtnAnimations");
        m_BtnLanguage = layoutRoot.FindAnyWidget("BtnLanguage");
        BuildChrome();
        BuildSections();
        SelectSection(0);
        if (m_BtnClose)
        {
            SetFocus(m_BtnClose);
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
            NimbyGreetLog.Debug("input exclude added: menu");
        }
    }

    // NIMBY-T-052: the clip timer is queued on this menu; a menu destroyed without OnHide must not leave it queued.
    void ~NimbyGreetMenu()
    {
        StopClips();
    }

    override void OnHide()
    {
        super.OnHide();
        StopClips();
        if (m_AddedExclude)
        {
            Mission mission = g_Game.GetMission();
            if (mission)
            {
                mission.RemoveActiveInputExcludes({"menu"}, false);
                NimbyGreetLog.Debug("input exclude removed: menu");
            }
            m_AddedExclude = false;
        }
        MissionGameplay gameplay = MissionGameplay.Cast(g_Game.GetMission());
        if (gameplay)
        {
            gameplay.NimbyGreetOnMenuClosed(this);
        }
    }

    override void Update(float timeslice)
    {
        super.Update(timeslice);
        if (!g_Game)
        {
            return;
        }
        if (m_TableRelayoutFrames > 0)
        {
            m_TableRelayoutFrames = m_TableRelayoutFrames - 1;
            RelayoutTables(m_TableRelayoutFrames == 0);
        }
        UAInputAPI api = GetUApi();
        if (!api)
        {
            return;
        }
        if (api.GetInputByID(UAUIBack).LocalPress())
        {
            if (m_CloseReason == "")
            {
                m_CloseReason = "player";
            }
            Close();
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
        if (api.GetInputByID(UAUINextUp).LocalPress())
        {
            PageBody(-1);
            return;
        }
        if (api.GetInputByID(UAUINextDown).LocalPress())
        {
            PageBody(1);
            return;
        }
    }

    // ---- chrome ------------------------------------------------------------------------------

    protected void BuildChrome()
    {
        if (m_Title)
        {
            m_Title.SetText(m_Language.title);
        }
        if (m_Tagline)
        {
            m_Tagline.SetText(m_Language.tagline);
            m_Tagline.Show(m_Language.tagline.Length() > 0);
        }
        if (m_Hint)
        {
            m_Hint.SetText(Tr("#STR_NimbyGreet_Hint", "The Greet key (default Insert) opens this window again. Esc closes it."));
        }
        SetButtonLabel(m_BtnClose, Tr("#STR_NimbyGreet_Close", "Close"));
        PaintButtonNormal(m_BtnClose);
        PaintButtonNormal(m_BtnNever);
        PaintButtonNormal(m_BtnTextSize);
        PaintButtonNormal(m_BtnAnimations);
        PaintButtonNormal(m_BtnLanguage);
        RefreshNeverLabel();
        RefreshTextSizeLabel();
        RefreshAnimationsLabel();
        RefreshLanguageLabel();
    }

    protected string NeverLabel()
    {
        string box = "[  ] ";
        if (m_NeverChecked)
        {
            box = "[X] ";
        }
        return box + Tr("#STR_NimbyGreet_NeverShow", "Don't show this again");
    }

    protected void RefreshNeverLabel()
    {
        SetButtonLabel(m_BtnNever, NeverLabel());
    }

    protected void RefreshTextSizeLabel()
    {
        string key = "#STR_NimbyGreet_TextNormal";
        string fallback = "Text: normal";
        if (m_Prefs.textScale >= NimbyGreetRules.TEXT_SCALE_LARGER)
        {
            key = "#STR_NimbyGreet_TextLarger";
            fallback = "Text: larger";
        }
        else
        {
            if (m_Prefs.textScale >= NimbyGreetRules.TEXT_SCALE_LARGE)
            {
                key = "#STR_NimbyGreet_TextLarge";
                fallback = "Text: large";
            }
        }
        SetButtonLabel(m_BtnTextSize, Tr(key, fallback));
    }

    protected void RefreshAnimationsLabel()
    {
        string key = "#STR_NimbyGreet_AnimationsOff";
        string fallback = "Animations: off";
        if (m_Prefs.playAnimations)
        {
            key = "#STR_NimbyGreet_AnimationsOn";
            fallback = "Animations: on";
        }
        SetButtonLabel(m_BtnAnimations, Tr(key, fallback));
        bool anyMedia = m_MediaIds.Count() > 0;
        if (m_BtnAnimations)
        {
            m_BtnAnimations.Show(anyMedia);
        }
    }

    protected void RefreshLanguageLabel()
    {
        if (!m_BtnLanguage)
        {
            return;
        }
        bool several = m_Done && m_Done.languages && m_Done.languages.Count() > 1;
        m_BtnLanguage.Show(several);
        if (!several)
        {
            return;
        }
        SetButtonLabel(m_BtnLanguage, Tr("#STR_NimbyGreet_LanguageLabel", "Language") + ": " + NimbyGreetLanguageName(m_Language.language));
    }

    static string NimbyGreetLanguageName(string code)
    {
        if (code == "english") { return "English"; }
        if (code == "czech") { return "Czech"; }
        if (code == "german") { return "German"; }
        if (code == "russian") { return "Russian"; }
        if (code == "polish") { return "Polish"; }
        if (code == "hungarian") { return "Hungarian"; }
        if (code == "italian") { return "Italian"; }
        if (code == "spanish") { return "Spanish"; }
        if (code == "french") { return "French"; }
        if (code == "chinese") { return "Chinese (Traditional)"; }
        if (code == "japanese") { return "Japanese"; }
        if (code == "portuguese") { return "Portuguese"; }
        if (code == "chinesesimp") { return "Chinese (Simplified)"; }
        return code;
    }

    // Widget.TranslateString returns the key without its # when the table has no entry for the
    // client's language (NIMBY-T-044); Phase 1 run 1 showed raw keys after an in-session language
    // change, before the client restart that reloads mod string tables. English is the fallback.
    static string Tr(string key, string fallback)
    {
        string text = Widget.TranslateString(key);
        if (text.Length() == 0)
        {
            return fallback;
        }
        if (NimbyGreetRules.StartsWith(text, "STR_NimbyGreet_"))
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
        int i = 0;
        while (i < m_Language.sections.Count())
        {
            NimbyGreetSection section = m_Language.sections.Get(i);
            Widget button = g_Game.GetWorkspace().CreateWidgets("NimbyGreet/GUI/nimbygreet_section_button.layout", m_SectionList);
            if (!button)
            {
                NimbyGreetLog.Info("section button layout failed to load");
                return;
            }
            button.SetUserID(ID_SECTION_BASE + i);
            TextWidget label = TextWidget.Cast(button.FindAnyWidget("SectionBtn_label"));
            if (label)
            {
                label.SetText(section.heading);
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
        int count = m_Language.sections.Count();
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
        BuildBody(index);
        BuildLinks();
        if (m_BodyScroll)
        {
            m_BodyScroll.VScrollToPos(0);
        }
        NimbyGreetLog.Debug("section selected index=" + index.ToString() + " id=" + m_Language.sections.Get(index).id + " blocks=" + m_BlockWidgets.Count().ToString() + " links=" + m_Links.Count().ToString());
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

    protected NimbyGreetConverted ConvertedFor(int index)
    {
        if (index < 0 || index >= m_Language.sections.Count())
        {
            return new NimbyGreetConverted();
        }
        while (m_ConvertedCache.Count() <= index)
        {
            m_ConvertedCache.Insert(null);
        }
        NimbyGreetConverted converted = m_ConvertedCache.Get(index);
        if (!converted)
        {
            converted = NimbyGreetMarkdown.Convert(m_Language.sections.Get(index).body, m_MediaIds);
            m_ConvertedCache.Set(index, converted);
            foreach (string note : converted.notes)
            {
                NimbyGreetLog.Info("section " + m_Language.sections.Get(index).id + ": " + note);
            }
        }
        return converted;
    }

    // ---- body blocks -------------------------------------------------------------------------

    protected void BuildBody(int index)
    {
        StopClips();
        foreach (Widget old : m_BlockWidgets)
        {
            if (old)
            {
                old.Unlink();
            }
        }
        m_BlockWidgets.Clear();
        m_TableRows.Clear();
        m_TableLeft.Clear();
        m_TableRight.Clear();
        m_TableRelayoutFrames = 0;
        m_ScaledBody.Clear();
        m_ScaledBodyBase.Clear();
        m_Links.Clear();
        if (!m_BodyContent)
        {
            return;
        }
        if (index < 0 || index >= m_Language.sections.Count())
        {
            // A language with no section (the server never serves one, but a stale cache could): an empty body.
            return;
        }
        NimbyGreetConverted converted = ConvertedFor(index);
        foreach (NimbyGreetLink link : converted.links)
        {
            m_Links.Insert(link);
        }
        foreach (NimbyGreetBlock block : converted.blocks)
        {
            if (block.kind == NimbyGreetBlockKind.HEADING1)
            {
                AddTextBlock("NimbyGreet/GUI/nimbygreet_block_heading.layout", "BlockHeading", "<b>" + block.markup + "</b>", HEADING1_SIZE);
                continue;
            }
            if (block.kind == NimbyGreetBlockKind.HEADING2)
            {
                AddTextBlock("NimbyGreet/GUI/nimbygreet_block_heading.layout", "BlockHeading", "<b>" + block.markup + "</b>", HEADING2_SIZE);
                continue;
            }
            if (block.kind == NimbyGreetBlockKind.RULE)
            {
                Widget rule = g_Game.GetWorkspace().CreateWidgets("NimbyGreet/GUI/nimbygreet_block_rule.layout", m_BodyContent);
                if (rule)
                {
                    m_BlockWidgets.Insert(rule);
                }
                continue;
            }
            if (block.kind == NimbyGreetBlockKind.TABLE)
            {
                AddGap(BLOCK_GAP);
                AddTableBlock(block);
                AddGap(BLOCK_GAP);
                continue;
            }
            if (block.kind == NimbyGreetBlockKind.MEDIA)
            {
                AddMediaBlock(block);
                continue;
            }
            AddTextBlock("NimbyGreet/GUI/nimbygreet_block_text.layout", "BlockText", block.markup, BODY_TEXT_SIZE);
        }
        if (m_BodyScroll)
        {
            m_BodyScroll.Update();
        }
        if (m_TableRows.Count() > 0)
        {
            m_TableRelayoutFrames = TABLE_RELAYOUT_FRAMES;
        }
    }

    protected void AddTextBlock(string layout, string name, string markup, int baseSize)
    {
        Widget root = g_Game.GetWorkspace().CreateWidgets(layout, m_BodyContent);
        if (!root)
        {
            NimbyGreetLog.Info("block layout failed to load: " + layout);
            return;
        }
        RichTextWidget text = RichTextWidget.Cast(root);
        if (!text)
        {
            text = RichTextWidget.Cast(root.FindAnyWidget(name));
        }
        if (text)
        {
            text.SetTextExactSize(Scaled(baseSize));
            text.SetText(markup);
            m_ScaledBody.Insert(text);
            m_ScaledBodyBase.Insert(baseSize);
        }
        m_BlockWidgets.Insert(root);
    }

    protected void AddTableBlock(NimbyGreetBlock block)
    {
        int r = 0;
        while (r < block.rows.Count())
        {
            array<string> row = block.rows.Get(r);
            Widget root = g_Game.GetWorkspace().CreateWidgets("NimbyGreet/GUI/nimbygreet_block_table_row.layout", m_BodyContent);
            if (!root)
            {
                NimbyGreetLog.Info("table row layout failed to load");
                return;
            }
            RichTextWidget left = RichTextWidget.Cast(root.FindAnyWidget("TableRow_left"));
            RichTextWidget right = RichTextWidget.Cast(root.FindAnyWidget("TableRow_right"));
            string leftMarkup = "<b>" + row.Get(0) + "</b>";
            string rightMarkup = row.Get(1);
            if (r == 0 && !block.headerless)
            {
                rightMarkup = "<b>" + row.Get(1) + "</b>";
            }
            float rowHeight = Scaled(TABLE_ROW_HEIGHT);
            if (left)
            {
                left.SetTextExactSize(Scaled(TABLE_TEXT_SIZE));
                left.SetText(leftMarkup);
                m_ScaledBody.Insert(left);
                m_ScaledBodyBase.Insert(TABLE_TEXT_SIZE);
                rowHeight = GrowRow(rowHeight, left.GetContentHeight());
            }
            if (right)
            {
                right.SetTextExactSize(Scaled(TABLE_TEXT_SIZE));
                right.SetText(rightMarkup);
                m_ScaledBody.Insert(right);
                m_ScaledBodyBase.Insert(TABLE_TEXT_SIZE);
                rowHeight = GrowRow(rowHeight, right.GetContentHeight());
            }
            // Phase 1 run 1: long cells were cut off. Cells wrap and the row grows to the taller cell;
            // the measure here is a first guess, RelayoutTables corrects it once the cells are laid out.
            root.SetSize(1.0, rowHeight);
            m_TableRows.Insert(root);
            m_TableLeft.Insert(left);
            m_TableRight.Insert(right);
            Widget line = root.FindAnyWidget("TableRow_line");
            if (line)
            {
                line.Show(r > 0);
            }
            m_BlockWidgets.Insert(root);
            r = r + 1;
        }
    }

    // An invisible spacer block: the rule layout with its line hidden, so a table or a rule gets air.
    protected void AddGap(int pixels)
    {
        Widget gap = g_Game.GetWorkspace().CreateWidgets("NimbyGreet/GUI/nimbygreet_block_rule.layout", m_BodyContent);
        if (!gap)
        {
            return;
        }
        Widget line = gap.FindAnyWidget("BlockRule_line");
        if (line)
        {
            line.Show(false);
        }
        gap.SetSize(1.0, Scaled(pixels));
        m_BlockWidgets.Insert(gap);
    }

    // Phase 1 run 5: re-measure every table row after the body has been laid out, from Update (no
    // CallLater: a queued 5_Mission function outlives its module, NIMBY-T-052). Runs for
    // TABLE_RELAYOUT_FRAMES frames after each build, the text-size rebuild included.
    protected void RelayoutTables(bool last)
    {
        int tall = 0;
        string sample = "";
        int i = 0;
        while (i < m_TableRows.Count())
        {
            Widget row = m_TableRows.Get(i);
            if (!row)
            {
                i = i + 1;
                continue;
            }
            // Run 5 at 2560x1440 with larger text: GetContentHeight and SetSize disagree on units above
            // 1080p and rows came up a third of a line short. The cells size themselves to their text
            // (size to text v, as body paragraphs do); their on-screen height converts to the row's
            // units through the row's own size and screen size, so the units never have to be known.
            float rowW;
            float rowUnits;
            row.GetSize(rowW, rowUnits);
            float rowScreenW;
            float rowScreenH;
            row.GetScreenSize(rowScreenW, rowScreenH);
            float cellScreenH = TableCellScreenHeight(m_TableLeft.Get(i));
            float rightScreenH = TableCellScreenHeight(m_TableRight.Get(i));
            if (rightScreenH > cellScreenH)
            {
                cellScreenH = rightScreenH;
            }
            float rowHeight = Scaled(TABLE_ROW_HEIGHT);
            if (rowScreenH > 0 && rowUnits > 0)
            {
                float unitsPerPixel = rowUnits / rowScreenH;
                rowHeight = GrowRow(rowHeight, cellScreenH * unitsPerPixel);
            }
            row.SetSize(1.0, rowHeight);
            if (rowHeight > Scaled(TABLE_ROW_HEIGHT))
            {
                tall = tall + 1;
            }
            if (last && i == m_TableRows.Count() - 1)
            {
                sample = " last row: cell px=" + cellScreenH.ToString() + " row px=" + rowScreenH.ToString();
                sample = sample + " row units " + rowUnits.ToString() + " to " + rowHeight.ToString();
            }
            i = i + 1;
        }
        if (m_BodyContent)
        {
            m_BodyContent.Update();
        }
        if (m_BodyScroll)
        {
            m_BodyScroll.Update();
        }
        if (last)
        {
            NimbyGreetLog.Debug("table relayout rows=" + m_TableRows.Count().ToString() + " taller than one line=" + tall.ToString() + sample);
        }
    }

    protected float TableCellScreenHeight(RichTextWidget cell)
    {
        if (!cell)
        {
            return 0;
        }
        float w;
        float h;
        cell.GetScreenSize(w, h);
        return h;
    }

    // A table row is at least its base height and at most eight lines; the cell's measured content height wins between.
    protected float GrowRow(float current, float contentHeight)
    {
        float wanted = contentHeight + 8;
        float ceiling = Scaled(TABLE_ROW_HEIGHT) * 8;
        if (wanted > ceiling)
        {
            wanted = ceiling;
        }
        if (wanted > current)
        {
            return wanted;
        }
        return current;
    }

    protected void AddMediaBlock(NimbyGreetBlock block)
    {
        NimbyGreetMediaEntry entry = FindMedia(block.mediaId);
        if (!entry)
        {
            NimbyGreetLog.Info("media unknown id=" + block.mediaId);
            return;
        }
        Widget root = g_Game.GetWorkspace().CreateWidgets("NimbyGreet/GUI/nimbygreet_block_media.layout", m_BodyContent);
        if (!root)
        {
            NimbyGreetLog.Info("media layout failed to load");
            return;
        }
        ImageWidget image = ImageWidget.Cast(root.FindAnyWidget("BlockMedia_image"));
        TextWidget alt = TextWidget.Cast(root.FindAnyWidget("BlockMedia_alt"));
        float paneWidth = 0;
        float paneHeight = 0;
        if (m_BodyScroll)
        {
            m_BodyScroll.GetScreenSize(paneWidth, paneHeight);
            paneWidth = paneWidth - m_BodyScroll.GetScrollbarWidth() - 8;
        }
        if (paneWidth < 64)
        {
            paneWidth = 64;
        }
        float imageHeight = paneWidth * 0.5;
        if (entry.width > 0 && entry.height > 0)
        {
            imageHeight = paneWidth * entry.height / entry.width;
        }
        if (paneHeight > 0 && imageHeight > paneHeight * 0.8)
        {
            imageHeight = paneHeight * 0.8;
        }
        int frames = entry.frames;
        if (frames < 1)
        {
            frames = 1;
        }
        if (frames > NimbyGreetRules.MAX_FRAMES)
        {
            frames = NimbyGreetRules.MAX_FRAMES;
        }
        bool loadedAll = true;
        if (image)
        {
            if (entry.kind == "clip")
            {
                int f = 0;
                while (f < frames)
                {
                    if (!image.LoadImageFile(f, NimbyGreetRules.MEDIA_PREFIX + entry.id + "-" + f.ToString() + ".edds"))
                    {
                        loadedAll = false;
                    }
                    f = f + 1;
                }
            }
            else
            {
                frames = 1;
                loadedAll = image.LoadImageFile(0, NimbyGreetRules.MEDIA_PREFIX + entry.id + ".edds");
            }
            image.SetImage(0);
            image.SetSize(1.0, imageHeight);
        }
        if (!loadedAll)
        {
            NimbyGreetLog.Info("media error id=" + entry.id + " kind=" + entry.kind + " frames=" + frames.ToString() + ": a frame failed to load");
        }
        if (alt)
        {
            alt.SetText(block.alt);
            alt.SetPos(0, imageHeight);
            alt.SetSize(1.0, MEDIA_ALT_HEIGHT);
        }
        root.SetSize(1.0, imageHeight + MEDIA_ALT_HEIGHT);
        m_BlockWidgets.Insert(root);
        if (entry.kind == "clip" && frames > 1 && loadedAll && m_Prefs.playAnimations && image)
        {
            NimbyGreetClip clip = new NimbyGreetClip();
            clip.image = image;
            clip.frames = frames;
            clip.frame = 0;
            clip.frameMs = entry.frameMs;
            if (clip.frameMs < NimbyGreetRules.MIN_FRAME_MS)
            {
                clip.frameMs = NimbyGreetRules.MIN_FRAME_MS;
            }
            m_Clips.Insert(clip);
            g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(FlipClip, clip.frameMs, true, clip);
            NimbyGreetLog.Debug("clip started id=" + entry.id + " frames=" + frames.ToString() + " frameMs=" + clip.frameMs.ToString());
        }
    }

    protected NimbyGreetMediaEntry FindMedia(string id)
    {
        if (!m_Manifest || !m_Manifest.media)
        {
            return null;
        }
        foreach (NimbyGreetMediaEntry entry : m_Manifest.media)
        {
            if (entry && entry.id == id)
            {
                return entry;
            }
        }
        return null;
    }

    protected void FlipClip(NimbyGreetClip clip)
    {
        if (!clip || !clip.image)
        {
            return;
        }
        clip.frame = (clip.frame + 1) % clip.frames;
        clip.image.SetImage(clip.frame);
    }

    protected void StopClips()
    {
        if (g_Game)
        {
            g_Game.GetCallQueue(CALL_CATEGORY_GUI).Remove(FlipClip);
        }
        if (m_Clips)
        {
            m_Clips.Clear();
        }
    }

    // ---- link rows (G10) ---------------------------------------------------------------------

    protected void BuildLinks()
    {
        foreach (Widget old : m_LinkWidgets)
        {
            if (old)
            {
                old.Unlink();
            }
        }
        m_LinkWidgets.Clear();
        if (!m_LinkRows)
        {
            return;
        }
        int i = 0;
        while (i < m_Links.Count())
        {
            NimbyGreetLink link = m_Links.Get(i);
            Widget root = g_Game.GetWorkspace().CreateWidgets("NimbyGreet/GUI/nimbygreet_link_row.layout", m_LinkRows);
            if (!root)
            {
                NimbyGreetLog.Info("link row layout failed to load");
                return;
            }
            // The label is itself a button that opens the link (Phase 1 run 1: the first instinct is to click it).
            Widget labelButton = root.FindAnyWidget("LinkRow_text");
            if (labelButton)
            {
                labelButton.SetUserID(ID_LINK_OPEN_BASE + i);
                TextWidget label = TextWidget.Cast(root.FindAnyWidget("LinkRow_text_label"));
                if (label)
                {
                    label.SetText("[" + link.index.ToString() + "] " + link.label + "   " + link.url);
                    label.SetTextExactSize(Scaled(LINK_TEXT_SIZE));
                }
            }
            Widget open = root.FindAnyWidget("LinkRow_open");
            Widget copy = root.FindAnyWidget("LinkRow_copy");
            if (open)
            {
                open.SetUserID(ID_LINK_OPEN_BASE + i);
                SetButtonLabel(open, Tr("#STR_NimbyGreet_Open", "Open"));
                PaintButtonNormal(open);
            }
            if (copy)
            {
                copy.SetUserID(ID_LINK_COPY_BASE + i);
                SetButtonLabel(copy, Tr("#STR_NimbyGreet_Copy", "Copy"));
                PaintButtonNormal(copy);
            }
            m_LinkWidgets.Insert(root);
            i = i + 1;
        }
        if (m_LinkScroll)
        {
            m_LinkScroll.Show(m_Links.Count() > 0);
            m_LinkScroll.Update();
        }
    }

    // ---- scale (G16) -------------------------------------------------------------------------

    protected int Scaled(int base)
    {
        return (int)(base * m_Prefs.textScale);
    }

    protected void ApplyScale()
    {
        int i = 0;
        while (i < m_ScaledBody.Count())
        {
            TextWidget text = TextWidget.Cast(m_ScaledBody.Get(i));
            if (text)
            {
                text.SetTextExactSize(Scaled(m_ScaledBodyBase.Get(i)));
            }
            i = i + 1;
        }
        foreach (Widget button : m_SectionButtons)
        {
            TextWidget label = TextWidget.Cast(button.FindAnyWidget("SectionBtn_label"));
            if (label)
            {
                label.SetTextExactSize(Scaled(SECTION_TEXT_SIZE));
            }
        }
        foreach (Widget row : m_LinkWidgets)
        {
            TextWidget rowLabel = TextWidget.Cast(row.FindAnyWidget("LinkRow_text_label"));
            if (rowLabel)
            {
                rowLabel.SetTextExactSize(Scaled(LINK_TEXT_SIZE));
            }
        }
        // Table rows and the body re-measure on the next build; the cheap way to a correct layout is a rebuild.
        BuildBody(m_Selected);
        BuildLinks();
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
            if (m_CloseReason == "")
            {
                m_CloseReason = "player";
            }
            Close();
            return true;
        }
        if (id == ID_NEVER)
        {
            m_NeverChecked = !m_NeverChecked;
            RefreshNeverLabel();
            return true;
        }
        if (id == ID_TEXTSIZE)
        {
            CycleTextSize();
            return true;
        }
        if (id == ID_ANIMATIONS)
        {
            m_Prefs.playAnimations = !m_Prefs.playAnimations;
            NimbyGreetClientFiles.SavePrefs(m_Prefs);
            RefreshAnimationsLabel();
            BuildBody(m_Selected);
            BuildLinks();
            NimbyGreetLog.Info("pref animations=" + NimbyGreetLog.Bool(m_Prefs.playAnimations));
            return true;
        }
        if (id == ID_LANGUAGE)
        {
            CycleLanguage();
            return true;
        }
        if (id >= ID_SECTION_BASE && id < ID_LINK_OPEN_BASE)
        {
            SelectSection(id - ID_SECTION_BASE);
            return true;
        }
        if (id >= ID_LINK_OPEN_BASE && id < ID_LINK_COPY_BASE)
        {
            int openIndex = id - ID_LINK_OPEN_BASE;
            if (openIndex < m_Links.Count())
            {
                string url = m_Links.Get(openIndex).url;
                NimbyGreetLog.Info("open url=" + url + " section=" + GetSelectedSectionId());
                GetGame().OpenURL(url);
            }
            return true;
        }
        if (id >= ID_LINK_COPY_BASE && id < ID_LINK_COPY_BASE + 100)
        {
            int copyIndex = id - ID_LINK_COPY_BASE;
            if (copyIndex < m_Links.Count())
            {
                GetGame().CopyToClipboard(m_Links.Get(copyIndex).url);
                SetButtonLabel(w, Tr("#STR_NimbyGreet_Copied", "Copied"));
                NimbyGreetLog.Debug("copied url=" + m_Links.Get(copyIndex).url);
            }
            return true;
        }
        return false;
    }

    protected void CycleTextSize()
    {
        float next = NimbyGreetRules.TEXT_SCALE_NORMAL;
        if (m_Prefs.textScale < NimbyGreetRules.TEXT_SCALE_LARGE)
        {
            next = NimbyGreetRules.TEXT_SCALE_LARGE;
        }
        else
        {
            if (m_Prefs.textScale < NimbyGreetRules.TEXT_SCALE_LARGER)
            {
                next = NimbyGreetRules.TEXT_SCALE_LARGER;
            }
        }
        m_Prefs.textScale = next;
        NimbyGreetClientFiles.SavePrefs(m_Prefs);
        RefreshTextSizeLabel();
        ApplyScale();
        NimbyGreetLog.Info("pref textScale=" + m_Prefs.textScale.ToString());
    }

    protected void CycleLanguage()
    {
        if (!m_Done || !m_Done.languages || m_Done.languages.Count() < 2)
        {
            return;
        }
        int at = m_Done.languages.Find(m_Language.language);
        int next = (at + 1) % m_Done.languages.Count();
        string code = m_Done.languages.Get(next);
        m_Prefs.language = code;
        NimbyGreetClientFiles.SavePrefs(m_Prefs);
        NimbyGreetLog.Info("pref language=" + code);
        MissionGameplay gameplay = MissionGameplay.Cast(g_Game.GetMission());
        if (gameplay)
        {
            gameplay.NimbyGreetSendRequest(true, "selector");
        }
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
        if (id >= ID_SECTION_BASE && id < ID_LINK_OPEN_BASE)
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
        int panelColor = COLOR_PANEL_BUTTON;
        int labelColor = COLOR_LABEL_NORMAL;
        if (w.GetName() == "LinkRow_text")
        {
            panelColor = COLOR_PANEL_NORMAL;
        }
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
