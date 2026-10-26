// -----------------------------------------------------------------------------
// NimbyGreet - client orchestration (plan G5, G7; Phase 0 items 2, 3, 8, 10, 11).
// Purpose : Owns the window and the fetch on the client. After OnPlayerLoaded on the
//           controlled player (reported by PlayerBase through NimbyGreetClient.Spawned),
//           waits 0.5 s, sends the request, assembles the chunks, and opens the probe
//           window 2 s after the payload is complete, only when no scripted menu is open;
//           otherwise it re-checks every second for up to 30 s and logs each deferral.
//           Reads UANimbyGreetOpen (Insert) every frame: with the Greet window open the
//           press closes it (and proves the UAInput fires under LockControls); with
//           another menu open, dead, or unconscious the press is logged and ignored; in a
//           vehicle or in play it sends a manual request and opens on done.
//           Item 8: the ceiling test sends one single-chunk request per size and kind,
//           waits for done or a 4 s timeout, and moves on; the race test sends two big
//           requests 100 ms apart and shows that only the second requestId assembles.
// Probe   : the window opens on EVERY spawn, including a respawn in the same session, so
//           item 3 can record all three spawn paths; the G4 once-per-connection rule is
//           Phase 1 behavior. MissionGameplay is created on connect and destroyed on
//           disconnect, so the invoker subscriptions are removed in OnMissionFinish.
// Vanilla : UAInput.LocalPress (3_Game/DayZ/InputAPI/UAInput.c:60), GetInputByName
//           (:218); UIManager.GetMenu (3_Game/DayZ/tools/UIManager.c:59);
//           ShowScriptedMenu (:9); CallLater (ScriptCallQueue); Man.IsUnconscious,
//           Object.IsAlive, PlayerBase.IsInVehicle; MENU_CONNECTION_DIALOGUE = 46
//           (3_Game/DayZ/constants.c:221).
// Style   : One statement per line.
// -----------------------------------------------------------------------------

modded class MissionGameplay extends MissionBase
{
    static const int FETCH_DELAY_MS = 500;
    static const int OPEN_DELAY_MS = 2000;
    static const int OPEN_RETRY_MS = 1000;
    static const int OPEN_RETRY_MAX = 30;
    static const int CEILING_TIMEOUT_MS = 4000;

    protected ref NimbyGreetFetch m_NimbyGreetFetch;
    protected UAInput m_NimbyGreetInput;
    protected ref NimbyGreetProbeMenu m_NimbyGreetMenu;
    protected int m_NimbyGreetLastMenuId = -2;
    protected int m_NimbyGreetSpawnCount;
    protected int m_NimbyGreetOpenRetries;
    protected string m_NimbyGreetPendingText;
    protected bool m_NimbyGreetOpenPending;
    protected ref array<string> m_NimbyGreetCeilingPlan;
    protected int m_NimbyGreetCeilingStep = -1;
    protected int m_NimbyGreetCeilingRequestId;
    protected int m_NimbyGreetRaceFirstId;

    override void OnInit()
    {
        super.OnInit();
        m_NimbyGreetFetch = new NimbyGreetFetch();
        if (GetUApi())
        {
            m_NimbyGreetInput = GetUApi().GetInputByName("UANimbyGreetOpen");
        }
        NimbyGreetClient.Spawned().Insert(NimbyGreetOnSpawned);
        NimbyGreetClient.Chunk().Insert(NimbyGreetOnChunk);
        NimbyGreetClient.Done().Insert(NimbyGreetOnDone);
        string key = Widget.TranslateString("#STR_NimbyGreet_Language");
        int idx = g_Game.GetCurrentDisplayLanguageIdx();
        NimbyGreetLog.Probe("loaded; input=UANimbyGreetOpen bound=" + NimbyGreetLog.Bool(m_NimbyGreetInput != null) + " languageKey=" + key + " languageIdx=" + idx.ToString() + " debug=" + NimbyGreetLog.Bool(NimbyGreetLog.IsDebug()));
    }

    override void OnMissionFinish()
    {
        NimbyGreetClient.Spawned().Remove(NimbyGreetOnSpawned);
        NimbyGreetClient.Chunk().Remove(NimbyGreetOnChunk);
        NimbyGreetClient.Done().Remove(NimbyGreetOnDone);
        NimbyGreetLog.Probe("mission finished; invokers released; spawns this connection=" + m_NimbyGreetSpawnCount.ToString());
        super.OnMissionFinish();
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        NimbyGreetTrackMenu();
        NimbyGreetHandleHotkey();
    }

    // ---- spawn and fetch -------------------------------------------------------------------

    void NimbyGreetOnSpawned(PlayerBase player)
    {
        m_NimbyGreetSpawnCount = m_NimbyGreetSpawnCount + 1;
        NimbyGreetLog.Probe("spawn #" + m_NimbyGreetSpawnCount.ToString() + " this connection; menu=" + NimbyGreetMenuName(NimbyGreetCurrentMenuId()) + "; request in " + FETCH_DELAY_MS.ToString() + " ms");
        g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(NimbyGreetAutoRequest, FETCH_DELAY_MS, false);
    }

    protected void NimbyGreetAutoRequest()
    {
        NimbyGreetSendRequest("english", false);
    }

    // Sends one request; a new request replaces the one in flight (its late chunks are dropped).
    int NimbyGreetSendRequest(string language, bool manual)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
        {
            NimbyGreetLog.Probe("request not sent: no controlled player");
            return -1;
        }
        int requestId = NimbyGreetClient.NextRequestId();
        if (m_NimbyGreetFetch.m_RequestId != 0)
        {
            if (!m_NimbyGreetFetch.IsComplete())
            {
                NimbyGreetLog.Probe("request id=" + requestId.ToString() + " replaces incomplete id=" + m_NimbyGreetFetch.m_RequestId.ToString() + " (received=" + m_NimbyGreetFetch.m_Received.ToString() + ")");
            }
        }
        m_NimbyGreetFetch.Begin(requestId, language, manual, GetGame().GetTickTime());
        player.NimbyGreetSendRequest(requestId, language, 0, false, manual);
        return requestId;
    }

    void NimbyGreetSendProbe(string language)
    {
        NimbyGreetSendRequest(language, true);
    }

    void NimbyGreetOnChunk(int requestId, int index, string text)
    {
        bool accepted = m_NimbyGreetFetch.AcceptChunk(requestId, index, text, GetGame().GetTickTime());
        if (!accepted)
        {
            NimbyGreetLog.Probe("chunk dropped id=" + requestId.ToString() + " index=" + index.ToString() + " bytes=" + text.Length().ToString() + " inFlight=" + m_NimbyGreetFetch.m_RequestId.ToString());
            return;
        }
        NimbyGreetLog.Probe("chunk id=" + requestId.ToString() + " index=" + index.ToString() + " bytes=" + text.Length().ToString() + " chars=" + text.LengthUtf8().ToString() + " received=" + m_NimbyGreetFetch.m_Received.ToString());
        if (m_NimbyGreetFetch.IsComplete())
        {
            NimbyGreetOnPayloadComplete();
        }
    }

    void NimbyGreetOnDone(int requestId, int chunkCount, int totalBytes, int hash, string servedLanguage, string info)
    {
        bool accepted = m_NimbyGreetFetch.AcceptDone(requestId, chunkCount, totalBytes, hash, servedLanguage, info, GetGame().GetTickTime());
        if (!accepted)
        {
            NimbyGreetLog.Probe("done dropped id=" + requestId.ToString() + " inFlight=" + m_NimbyGreetFetch.m_RequestId.ToString());
            return;
        }
        NimbyGreetLog.Probe("done id=" + requestId.ToString() + " chunkCount=" + chunkCount.ToString() + " totalBytes=" + totalBytes.ToString() + " info=" + info + " received=" + m_NimbyGreetFetch.m_Received.ToString());
        if (m_NimbyGreetFetch.IsComplete())
        {
            NimbyGreetOnPayloadComplete();
        }
    }

    protected void NimbyGreetOnPayloadComplete()
    {
        string assembled = m_NimbyGreetFetch.Assemble();
        string verdict = m_NimbyGreetFetch.Verify(assembled);
        NimbyGreetLog.Probe("payload complete id=" + m_NimbyGreetFetch.m_RequestId.ToString() + " info=" + m_NimbyGreetFetch.m_Info + " chunks=" + m_NimbyGreetFetch.m_ChunkCount.ToString() + " bytes=" + assembled.Length().ToString() + " chars=" + assembled.LengthUtf8().ToString() + " elapsedMs=" + m_NimbyGreetFetch.ElapsedMs().ToString() + " verdict=" + verdict);
        // Mark the fetch consumed so a later request does not report it as replaced.
        if (m_NimbyGreetFetch.m_Language.IndexOf("probe:") == 0)
        {
            if (m_NimbyGreetMenu)
            {
                m_NimbyGreetMenu.SetStatus(m_NimbyGreetFetch.m_Info + ": " + verdict + " in " + m_NimbyGreetFetch.ElapsedMs().ToString() + " ms, " + m_NimbyGreetFetch.m_ChunkCount.ToString() + " chunks");
            }
            if (m_NimbyGreetCeilingStep >= 0)
            {
                if (m_NimbyGreetFetch.m_RequestId == m_NimbyGreetCeilingRequestId)
                {
                    NimbyGreetLog.Probe("ceiling: " + m_NimbyGreetFetch.m_Info + " -> " + verdict);
                    NimbyGreetCeilingNext();
                }
            }
            return;
        }
        m_NimbyGreetPendingText = assembled;
        if (m_NimbyGreetMenu)
        {
            m_NimbyGreetMenu.SetText(assembled);
            return;
        }
        m_NimbyGreetOpenRetries = 0;
        m_NimbyGreetOpenPending = true;
        NimbyGreetLog.Probe("open scheduled in " + OPEN_DELAY_MS.ToString() + " ms; menu now=" + NimbyGreetMenuName(NimbyGreetCurrentMenuId()));
        g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(NimbyGreetTryOpen, OPEN_DELAY_MS, false);
    }

    // ---- window -----------------------------------------------------------------------------

    protected void NimbyGreetTryOpen()
    {
        if (!m_NimbyGreetOpenPending)
        {
            return;
        }
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        bool alive = false;
        bool unconscious = true;
        if (player)
        {
            alive = player.IsAlive();
            unconscious = player.IsUnconscious();
        }
        int menuId = NimbyGreetCurrentMenuId();
        if (menuId != -1 || !alive || unconscious)
        {
            m_NimbyGreetOpenRetries = m_NimbyGreetOpenRetries + 1;
            NimbyGreetLog.Probe("open deferred #" + m_NimbyGreetOpenRetries.ToString() + ": menu=" + NimbyGreetMenuName(menuId) + " alive=" + NimbyGreetLog.Bool(alive) + " unconscious=" + NimbyGreetLog.Bool(unconscious));
            if (m_NimbyGreetOpenRetries >= OPEN_RETRY_MAX)
            {
                m_NimbyGreetOpenPending = false;
                NimbyGreetLog.Probe("open abandoned after " + OPEN_RETRY_MAX.ToString() + " retries");
                return;
            }
            g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(NimbyGreetTryOpen, OPEN_RETRY_MS, false);
            return;
        }
        m_NimbyGreetOpenPending = false;
        NimbyGreetOpenWindow(m_NimbyGreetPendingText);
    }

    protected void NimbyGreetOpenWindow(string text)
    {
        if (m_NimbyGreetMenu)
        {
            m_NimbyGreetMenu.SetText(text);
            return;
        }
        m_NimbyGreetMenu = new NimbyGreetProbeMenu();
        m_NimbyGreetMenu.SetText(text);
        GetGame().GetUIManager().ShowScriptedMenu(m_NimbyGreetMenu, null);
        int menuId = NimbyGreetCurrentMenuId();
        NimbyGreetLog.Probe("open: ShowScriptedMenu called; top menu now=" + NimbyGreetMenuName(menuId) + " (" + menuId.ToString() + ")");
    }

    void NimbyGreetOnMenuClosed()
    {
        m_NimbyGreetMenu = null;
        NimbyGreetLog.Probe("window closed; top menu now=" + NimbyGreetMenuName(NimbyGreetCurrentMenuId()));
    }

    // ---- hotkey (item 11) -----------------------------------------------------------------

    protected void NimbyGreetHandleHotkey()
    {
        if (!m_NimbyGreetInput)
        {
            return;
        }
        if (!m_NimbyGreetInput.LocalPress())
        {
            return;
        }
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        bool alive = false;
        bool unconscious = false;
        bool vehicle = false;
        if (player)
        {
            alive = player.IsAlive();
            unconscious = player.IsUnconscious();
            vehicle = player.IsInVehicle();
        }
        int menuId = NimbyGreetCurrentMenuId();
        string state = "menu=" + NimbyGreetMenuName(menuId) + " alive=" + NimbyGreetLog.Bool(alive) + " unconscious=" + NimbyGreetLog.Bool(unconscious) + " vehicle=" + NimbyGreetLog.Bool(vehicle);
        if (m_NimbyGreetMenu)
        {
            if (menuId == NimbyGreetRules.MENU_ID)
            {
                NimbyGreetLog.Probe("hotkey: press seen in MissionGameplay.OnUpdate while the Greet window held focus (UAInput fires under LockControls); closing; " + state);
                m_NimbyGreetMenu.Close();
                return;
            }
        }
        if (menuId != -1)
        {
            NimbyGreetLog.Probe("hotkey: press ignored, another menu is open; " + state);
            return;
        }
        if (!alive)
        {
            NimbyGreetLog.Probe("hotkey: press ignored, player dead; " + state);
            return;
        }
        if (unconscious)
        {
            NimbyGreetLog.Probe("hotkey: press ignored, player unconscious; " + state);
            return;
        }
        NimbyGreetLog.Probe("hotkey: press accepted, manual request; " + state);
        NimbyGreetSendRequest("english", true);
    }

    // ---- item 8: ceiling and race -------------------------------------------------------

    void NimbyGreetStartCeiling()
    {
        m_NimbyGreetCeilingPlan = new array<string>;
        array<string> kinds = {"ascii", "cyr", "cjk"};
        array<int> sizes = {1000, 1500, 2000, 4000, 8000};
        foreach (string kind : kinds)
        {
            foreach (int size : sizes)
            {
                m_NimbyGreetCeilingPlan.Insert("probe:" + kind + ":" + size.ToString());
            }
        }
        m_NimbyGreetCeilingStep = -1;
        NimbyGreetLog.Probe("ceiling: start; " + m_NimbyGreetCeilingPlan.Count().ToString() + " single-chunk requests, " + CEILING_TIMEOUT_MS.ToString() + " ms timeout each");
        NimbyGreetCeilingNext();
    }

    protected void NimbyGreetCeilingNext()
    {
        m_NimbyGreetCeilingStep = m_NimbyGreetCeilingStep + 1;
        if (m_NimbyGreetCeilingStep >= m_NimbyGreetCeilingPlan.Count())
        {
            m_NimbyGreetCeilingStep = -1;
            NimbyGreetLog.Probe("ceiling: finished");
            if (m_NimbyGreetMenu)
            {
                m_NimbyGreetMenu.SetStatus("RPC ceiling test finished; read the log");
            }
            return;
        }
        string language = m_NimbyGreetCeilingPlan.Get(m_NimbyGreetCeilingStep);
        m_NimbyGreetCeilingRequestId = NimbyGreetSendRequest(language, true);
        g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(NimbyGreetCeilingTimeout, CEILING_TIMEOUT_MS, false, m_NimbyGreetCeilingRequestId);
    }

    protected void NimbyGreetCeilingTimeout(int requestId)
    {
        if (m_NimbyGreetCeilingStep < 0)
        {
            return;
        }
        if (requestId != m_NimbyGreetCeilingRequestId)
        {
            return;
        }
        if (m_NimbyGreetFetch.m_RequestId == requestId)
        {
            if (m_NimbyGreetFetch.IsComplete())
            {
                return;
            }
        }
        string language = m_NimbyGreetCeilingPlan.Get(m_NimbyGreetCeilingStep);
        NimbyGreetLog.Probe("ceiling: " + language + " -> LOST (no complete payload within " + CEILING_TIMEOUT_MS.ToString() + " ms; received=" + m_NimbyGreetFetch.m_Received.ToString() + " doneSeen=" + NimbyGreetLog.Bool(m_NimbyGreetFetch.m_DoneSeen) + ")");
        NimbyGreetCeilingNext();
    }

    void NimbyGreetStartRace()
    {
        m_NimbyGreetRaceFirstId = NimbyGreetSendRequest("probe:big", true);
        NimbyGreetLog.Probe("race: first request id=" + m_NimbyGreetRaceFirstId.ToString() + "; second in 100 ms");
        g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(NimbyGreetRaceSecond, 100, false);
    }

    protected void NimbyGreetRaceSecond()
    {
        int second = NimbyGreetSendRequest("probe:big", true);
        NimbyGreetLog.Probe("race: second request id=" + second.ToString() + "; chunks of id=" + m_NimbyGreetRaceFirstId.ToString() + " must now be dropped");
    }

    // ---- menu tracking ----------------------------------------------------------------------

    protected void NimbyGreetTrackMenu()
    {
        int menuId = NimbyGreetCurrentMenuId();
        if (menuId == m_NimbyGreetLastMenuId)
        {
            return;
        }
        if (m_NimbyGreetLastMenuId != -2)
        {
            NimbyGreetLog.Probe("menu " + NimbyGreetMenuName(m_NimbyGreetLastMenuId) + " -> " + NimbyGreetMenuName(menuId));
        }
        m_NimbyGreetLastMenuId = menuId;
    }

    protected int NimbyGreetCurrentMenuId()
    {
        UIScriptedMenu menu = GetGame().GetUIManager().GetMenu();
        if (!menu)
        {
            return -1;
        }
        return menu.GetID();
    }

    protected string NimbyGreetMenuName(int menuId)
    {
        if (menuId < 0)
        {
            return "none";
        }
        if (menuId == NimbyGreetRules.MENU_ID)
        {
            return "greet";
        }
        if (menuId == MENU_INGAME)
        {
            return "pause";
        }
        if (menuId == MENU_INVENTORY)
        {
            return "inventory";
        }
        if (menuId == MENU_CHAT_INPUT)
        {
            return "chat";
        }
        if (menuId == MENU_NOTE)
        {
            return "note";
        }
        if (menuId == MENU_CONNECTION_DIALOGUE)
        {
            return "connecting";
        }
        if (menuId == MENU_RESPAWN_DIALOGUE)
        {
            return "respawn";
        }
        return "id" + menuId.ToString();
    }
}
