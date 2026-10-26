// -----------------------------------------------------------------------------
// NimbyGreet - Phase 0 probe window (plan Phase 0 items 2, 4, 5, 7, 9, 10, 11).
// Purpose : One UIScriptedMenu (3_Game/DayZ/tools/UIScriptedMenu.c) built from
//           NimbyGreet/GUI/nimbygreet_probe.layout: a RichTextWidget body, a
//           VideoWidget and an ImageWidget for item 9, a status line, and one button per
//           probe. Every button logs its result under [NIMBY][Greet][Probe]; the test
//           record quotes those lines plus Jeff's screenshots.
// Vanilla : CreateWidgets (1_Core/DayZ/proto/EnWidgets.c:181); RichTextWidget
//           GetContentHeight/GetContentOffset/SetContentOffset/GetNumLines
//           (EnWidgets.c:224-231); VideoWidget Load/Play/Stop/Unload/SetCallback
//           (:543-590); ImageWidget LoadImageFile/SetImage (:257,267); OpenURL
//           (3_Game/DayZ/Global/Game.c:1401); CopyToClipboard/CopyFromClipboard
//           (Game.c:663-664); Widget.TranslateString (EnWidgets.c:112);
//           DayZGame.GetCurrentDisplayLanguageIdx (DayZGame.c:3958);
//           ListOptionsAccess.GetItemText (gameplay.c:1437); UAUIBack close is the
//           NoteMenu pattern (5_Mission/DayZ/GUI/NoteMenu.c:152-158).
// Client  : UIScriptedMenu exists only on the client; the server never builds one.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyGreetProbeMenu extends UIScriptedMenu
{
    static const string URL = "https://discord.gg/kH3eJ5mdEv";
    // Run 2: the PBO path resolves (ON_ERROR after 500 ms, against 8 ms for an unresolved path) but the
    // run 1 clip (H.264 High, no audio) does not decode, so each press of Media (PBO) tries the next
    // encoding; Video alt path tries the vanilla-style game-root video\ folder (Jeff copies the files).
    static ref array<string> VIDEO_PBO_LIST = {"NimbyGreet/Media/probe.mp4", "NimbyGreet/Media/probe-base.mp4", "NimbyGreet/Media/probe-base-aac.mp4", "NimbyGreet/Media/probe-high-aac.mp4", "NimbyGreet/Media/probe-mpeg4.mp4"};
    static ref array<string> VIDEO_ALT_LIST = {"video\\probe.mp4", "video\\probe-base-aac.mp4", "video\\probe-mpeg4.mp4", "video/probe-base-aac.mp4"};
    static const string VIDEO_LOOSE = "$profile:NimbyGreet\\probe.mp4";
    static const string IMAGE_EDDS = "NimbyGreet/Media/probe.edds";
    static const string IMAGE_PAA = "NimbyGreet/Media/probe.paa";

    protected TextWidget m_Title;
    protected RichTextWidget m_Body;
    protected TextWidget m_Status;
    protected TextWidget m_MediaNote;
    protected VideoWidget m_Video;
    protected ImageWidget m_Image;
    protected UAInput m_OpenInput;
    protected string m_Text;
    protected string m_LastVideoPath;
    protected int m_UpdatePressCount;
    protected int m_VideoPboIndex;
    protected int m_VideoAltIndex;

    void NimbyGreetProbeMenu()
    {
        SetID(NimbyGreetRules.MENU_ID);
        m_Text = "(no text yet)";
    }

    void SetText(string text)
    {
        m_Text = text;
        if (m_Body)
        {
            m_Body.SetText(text);
            m_Body.SetContentOffset(0, false);
            NimbyGreetLog.Probe("body set bytes=" + text.Length().ToString() + " lines=" + m_Body.GetNumLines().ToString() + " contentHeight=" + m_Body.GetContentHeight().ToString());
        }
    }

    void SetStatus(string text)
    {
        if (m_Status)
        {
            m_Status.SetText(text);
        }
    }

    override Widget Init()
    {
        layoutRoot = g_Game.GetWorkspace().CreateWidgets("NimbyGreet/GUI/nimbygreet_probe.layout");
        if (!layoutRoot)
        {
            NimbyGreetLog.Probe("layout failed to load: NimbyGreet/GUI/nimbygreet_probe.layout");
            return null;
        }
        m_Title = TextWidget.Cast(layoutRoot.FindAnyWidget("Title"));
        m_Body = RichTextWidget.Cast(layoutRoot.FindAnyWidget("Body"));
        m_Status = TextWidget.Cast(layoutRoot.FindAnyWidget("Status"));
        m_MediaNote = TextWidget.Cast(layoutRoot.FindAnyWidget("MediaNote"));
        m_Video = VideoWidget.Cast(layoutRoot.FindAnyWidget("Video"));
        m_Image = ImageWidget.Cast(layoutRoot.FindAnyWidget("Image"));
        if (m_Video)
        {
            m_Video.SetCallback(VideoCallback.ON_ERROR, OnVideoError);
            m_Video.SetCallback(VideoCallback.ON_PLAY, OnVideoPlay);
            m_Video.SetCallback(VideoCallback.ON_END, OnVideoEnd);
            m_Video.SetCallback(VideoCallback.ON_LOAD, OnVideoLoad);
        }
        if (m_Title)
        {
            m_Title.SetText(Widget.TranslateString("#STR_NimbyGreet_ProbeTitle"));
        }
        if (GetUApi())
        {
            m_OpenInput = GetUApi().GetInputByName("UANimbyGreetOpen");
        }
        SetText(m_Text);
        NimbyGreetLog.Probe("menu Init: layout loaded; body=" + NimbyGreetLog.Bool(m_Body != null) + " video=" + NimbyGreetLog.Bool(m_Video != null) + " image=" + NimbyGreetLog.Bool(m_Image != null));
        return layoutRoot;
    }

    override void OnShow()
    {
        super.OnShow();
        UIScriptedMenu top = GetGame().GetUIManager().GetMenu();
        int topId = -1;
        if (top)
        {
            topId = top.GetID();
        }
        NimbyGreetLog.Probe("menu shown id=" + GetID().ToString() + " topMenuId=" + topId.ToString() + " cursor=" + NimbyGreetLog.Bool(GetGame().GetUIManager().IsCursorVisible()));
    }

    override void OnHide()
    {
        super.OnHide();
        if (m_Video)
        {
            m_Video.Stop();
            m_Video.Unload();
        }
        NimbyGreetLog.Probe("menu hidden id=" + GetID().ToString() + " updatePresses=" + m_UpdatePressCount.ToString());
        MissionGameplay mission = MissionGameplay.Cast(g_Game.GetMission());
        if (mission)
        {
            mission.NimbyGreetOnMenuClosed();
        }
    }

    override void Update(float timeslice)
    {
        super.Update(timeslice);
        if (!g_Game)
        {
            return;
        }
        if (GetUApi().GetInputByID(UAUIBack).LocalPress())
        {
            NimbyGreetLog.Probe("menu Update: UAUIBack (Escape) pressed; closing");
            Close();
            return;
        }
        // Item 11: does the Greet input still fire while this menu holds focus (LockControls)?
        if (m_OpenInput)
        {
            if (m_OpenInput.LocalPress())
            {
                m_UpdatePressCount = m_UpdatePressCount + 1;
                NimbyGreetLog.Probe("menu Update: UANimbyGreetOpen LocalPress seen while the menu held focus (poll path works) count=" + m_UpdatePressCount.ToString());
            }
        }
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        super.OnClick(w, x, y, button);
        if (!w)
        {
            return false;
        }
        int id = w.GetUserID();
        if (id == 1)
        {
            NimbyGreetLog.Probe("click Close");
            Close();
            return true;
        }
        if (id == 2)
        {
            NimbyGreetLog.Probe("click Open URL: calling OpenURL " + URL);
            GetGame().OpenURL(URL);
            SetStatus("OpenURL called; did the Steam overlay open?");
            return true;
        }
        if (id == 3)
        {
            ProbeCopy();
            return true;
        }
        if (id == 4)
        {
            SetText(MarkupSheet());
            SetStatus("markup sheet shown; screenshot it");
            return true;
        }
        if (id == 14)
        {
            SetText(LongText());
            SetStatus("long text shown; use Scroll down / Scroll up");
            return true;
        }
        if (id == 10)
        {
            ProbeScroll(120.0);
            return true;
        }
        if (id == 11)
        {
            ProbeScroll(-120.0);
            return true;
        }
        if (id == 9)
        {
            ProbeLanguage();
            return true;
        }
        if (id == 8)
        {
            ProbeMedia();
            return true;
        }
        if (id == 13)
        {
            ProbeVideo(VIDEO_ALT_LIST.Get(m_VideoAltIndex));
            m_VideoAltIndex = (m_VideoAltIndex + 1) % VIDEO_ALT_LIST.Count();
            return true;
        }
        if (id == 12)
        {
            ProbeVideo(VIDEO_LOOSE);
            return true;
        }
        if (id == 15)
        {
            ProbeMediaStop();
            return true;
        }
        MissionGameplay mission = MissionGameplay.Cast(g_Game.GetMission());
        if (!mission)
        {
            return false;
        }
        if (id == 5)
        {
            SetStatus("RPC ceiling test running; watch the log");
            mission.NimbyGreetStartCeiling();
            return true;
        }
        if (id == 6)
        {
            SetStatus("big payload requested; watch the log");
            mission.NimbyGreetSendProbe("probe:big");
            return true;
        }
        if (id == 7)
        {
            SetStatus("race test: two big requests 100 ms apart");
            mission.NimbyGreetStartRace();
            return true;
        }
        return false;
    }

    // ---- probes --------------------------------------------------------------------------------

    protected void ProbeCopy()
    {
        GetGame().CopyToClipboard(URL);
        string back;
        GetGame().CopyFromClipboard(back);
        bool same = back == URL;
        NimbyGreetLog.Probe("click Copy: CopyToClipboard then CopyFromClipboard same=" + NimbyGreetLog.Bool(same) + " back=" + back);
        SetStatus("copied; clipboard read back same=" + NimbyGreetLog.Bool(same));
    }

    protected void ProbeScroll(float delta)
    {
        if (!m_Body)
        {
            return;
        }
        float before = m_Body.GetContentOffset();
        m_Body.SetContentOffset(before + delta, false);
        float after = m_Body.GetContentOffset();
        float height = m_Body.GetContentHeight();
        float w;
        float h;
        m_Body.GetScreenSize(w, h);
        NimbyGreetLog.Probe("scroll delta=" + delta.ToString() + " offsetBefore=" + before.ToString() + " offsetAfter=" + after.ToString() + " contentHeight=" + height.ToString() + " paneHeight=" + h.ToString() + " lines=" + m_Body.GetNumLines().ToString());
        SetStatus("offset " + after.ToString() + " of content " + height.ToString() + " (pane " + h.ToString() + ")");
    }

    protected void ProbeLanguage()
    {
        string key = Widget.TranslateString("#STR_NimbyGreet_Language");
        string unknown = Widget.TranslateString("#STR_NimbyGreet_DoesNotExist");
        int idx = g_Game.GetCurrentDisplayLanguageIdx();
        string option = "";
        GameOptions options = new GameOptions();
        ListOptionsAccess access = ListOptionsAccess.Cast(options.GetOptionByType(OptionAccessType.AT_OPTIONS_LANGUAGE));
        if (access)
        {
            access.GetItemText(idx, option);
        }
        NimbyGreetLog.Probe("language: TranslateString(#STR_NimbyGreet_Language)=" + key + " idx=" + idx.ToString() + " optionText=" + option + " unknownKey=" + unknown);
        SetStatus("language key=" + key + " idx=" + idx.ToString() + " option=" + option);
    }

    protected void ProbeMedia()
    {
        string path = VIDEO_PBO_LIST.Get(m_VideoPboIndex);
        m_VideoPboIndex = (m_VideoPboIndex + 1) % VIDEO_PBO_LIST.Count();
        ProbeVideo(path);
        if (!m_Image)
        {
            return;
        }
        m_Image.Show(true);
        bool edds = m_Image.LoadImageFile(0, IMAGE_EDDS);
        bool paa = m_Image.LoadImageFile(1, IMAGE_PAA);
        int sx;
        int sy;
        m_Image.GetImageSize(0, sx, sy);
        bool shown = m_Image.SetImage(0);
        if (!edds)
        {
            shown = m_Image.SetImage(1);
        }
        NimbyGreetLog.Probe("image: LoadImageFile edds=" + NimbyGreetLog.Bool(edds) + " paa=" + NimbyGreetLog.Bool(paa) + " size0=" + sx.ToString() + "x" + sy.ToString() + " setImage=" + NimbyGreetLog.Bool(shown));
        if (m_MediaNote)
        {
            m_MediaNote.SetText("video " + path + " | image edds=" + NimbyGreetLog.Bool(edds) + " paa=" + NimbyGreetLog.Bool(paa));
        }
    }

    protected void ProbeVideo(string path)
    {
        if (!m_Video)
        {
            return;
        }
        m_Video.Stop();
        m_Video.Unload();
        m_Video.Show(true);
        m_LastVideoPath = path;
        bool loaded = m_Video.Load(path, true, 0);
        bool playing = m_Video.Play();
        int state = m_Video.GetState();
        NimbyGreetLog.Probe("video: Load(" + path + ", looping) returned=" + NimbyGreetLog.Bool(loaded) + " Play returned=" + NimbyGreetLog.Bool(playing) + " state=" + state.ToString() + " totalTime=" + m_Video.GetTotalTime().ToString() + " (callbacks follow; return values are deprecated per EnWidgets.c:547)");
        SetStatus("video " + path + " load=" + NimbyGreetLog.Bool(loaded) + " play=" + NimbyGreetLog.Bool(playing));
        // Report the state again after 2 s and 4 s: a 3 s looping clip should still be PLAYING at 4 s.
        g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(ReportVideoState, 2000, false);
        g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(ReportVideoState, 4000, false);
    }

    protected void ReportVideoState()
    {
        if (!m_Video)
        {
            return;
        }
        int state = m_Video.GetState();
        NimbyGreetLog.Probe("video state: path=" + m_LastVideoPath + " isPlaying=" + NimbyGreetLog.Bool(m_Video.IsPlaying()) + " state=" + state.ToString() + " time=" + m_Video.GetTime().ToString() + " total=" + m_Video.GetTotalTime().ToString() + " looping=" + NimbyGreetLog.Bool(m_Video.IsLooping()));
    }

    protected void ProbeMediaStop()
    {
        if (m_Video)
        {
            m_Video.Stop();
            m_Video.Unload();
            m_Video.Show(false);
        }
        if (m_Image)
        {
            m_Image.Show(false);
        }
        NimbyGreetLog.Probe("media stopped and unloaded");
        SetStatus("media stopped");
    }

    protected void OnVideoError()
    {
        NimbyGreetLog.Probe("video callback ON_ERROR path=" + m_LastVideoPath);
        SetStatus("video ON_ERROR for " + m_LastVideoPath);
    }

    protected void OnVideoPlay()
    {
        NimbyGreetLog.Probe("video callback ON_PLAY path=" + m_LastVideoPath);
    }

    protected void OnVideoEnd()
    {
        NimbyGreetLog.Probe("video callback ON_END path=" + m_LastVideoPath + " (a looping clip should not end)");
    }

    protected void OnVideoLoad()
    {
        NimbyGreetLog.Probe("video callback ON_LOAD path=" + m_LastVideoPath);
    }

    // Item 7: every tag the G13 converter may emit, and every candidate for a literal <.
    // Each line is numbered so the screenshot maps to the record: honored, ignored, or printed literally.
    static string MarkupSheet()
    {
        string s = "01 plain: PLAIN end<br/>";
        s = s + "02 b: <b>BOLD</b> end<br/>";
        s = s + "03 i: <i>ITALIC</i> end<br/>";
        s = s + "04 u: <u>UNDERLINE</u> end<br/>";
        s = s + "05 color hex: <color hex=\"0xFFFF8800\">ORANGE</color> end<br/>";
        s = s + "06 color rgba: <color rgba=\"255, 64, 64, 255\">RED</color> end<br/>";
        s = s + "07 font size: <font size=\"30\">BIG</font> end<br/>";
        s = s + "08 p: <p>PARA</p> end<br/>";
        s = s + "09 br: before<br/>after<br/>";
        s = s + "10 image: <image set=\"dayz_gui\" name=\"externallink\" scale=\"1\" /> end<br/>";
        s = s + "11 lt entity: &lt;b&gt;NOT BOLD&lt;/b&gt; end<br/>";
        s = s + "12 lt numeric: &#60;b&#62;NOT BOLD&#60;/b&#62; end<br/>";
        s = s + "13 doubled: <<b>>NOT BOLD<</b>> end<br/>";
        s = s + "14 fullwidth: ＜b＞NOT BOLD＜/b＞ end<br/>";
        s = s + "15 raw lt: a < b and c > d end<br/>";
        s = s + "16 newline: before\nafter<br/>";
        s = s + "17 nested: <b>bold <i>bold italic</i> bold</b> end<br/>";
        s = s + "18 unterminated: <b>bold to the end of this line end<br/>";
        s = s + "19 cyrillic: дайз Бэдлендс end<br/>";
        s = s + "20 cjk: 字符测试 end<br/>";
        s = s + "21 lt no gt after: a < b end<br/>";
        s = s + "22 single angle quotes: ‹b› NOT BOLD end<br/>";
        s = s + "23 modifier arrowheads: ˂b˃ NOT BOLD end<br/>";
        s = s + "24 math angle brackets: ⟨b⟩ NOT BOLD end<br/>";
        s = s + "25 amp entity: &amp; and a bare & end<br/>";
        s = s + "26 lt space: < b> maybe a tag end";
        return s;
    }

    static string LongText()
    {
        string s = "";
        int i = 1;
        while (i <= 120)
        {
            s = s + "Line " + i.ToString() + ": the quick brown fox jumps over the lazy dog, again and again, to make the body taller than the pane.<br/>";
            i = i + 1;
        }
        return s;
    }
}
