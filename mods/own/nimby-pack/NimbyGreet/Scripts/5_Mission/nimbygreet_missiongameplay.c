// -----------------------------------------------------------------------------
// NimbyGreet - client orchestration (plan v0.4, G4, G5, G7, G14).
// Purpose : Owns the fetch and the once-per-connection state on the client. After
//           OnPlayerLoaded on the controlled player (PlayerBase, through
//           NimbyGreetClient.Spawned), waits FETCH_DELAY_MS, sends the request with the
//           client's language, dismissedVersion and neverShow, assembles the chunks, and
//           schedules the window openDelaySeconds after the payload is complete, only when
//           no scripted menu is open and the player is alive and conscious; otherwise it
//           re-checks every second for openRetrySeconds and gives up with one log line.
//           The window opens by itself at most once per connection (G4); MissionGameplay
//           is created on connect and destroyed on disconnect, so the flag lives here.
//           Reads UANimbyGreetOpen in OnUpdate and nowhere else (NIMBY-T-046): with the
//           Greet window open the press closes it, with another menu open, dead or
//           unconscious it is ignored, in play or in a vehicle it sends a manual request.
// Language: Widget.TranslateString("#STR_NimbyGreet_Language") returns the client's column
//           name (NIMBY-T-044); a value outside the 14 columns means the key did not resolve
//           and the default language is requested. prefs.json's language overrides it.
// Phase 1 step (a)/(b): the window itself lands in step (c); NimbyGreetOpenWindow logs the
//           converted payload so the first build proves fetch, caps and conversion in game.
// Vanilla : UAInput.LocalPress (3_Game/DayZ/InputAPI/UAInput.c:60), GetInputByName (:218);
//           UIManager.GetMenu (3_Game/DayZ/tools/UIManager.c:59); CallLater (ScriptCallQueue);
//           Man.IsUnconscious, Object.IsAlive, PlayerBase.IsInVehicle.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

modded class MissionGameplay extends MissionBase
{
    protected ref NimbyGreetFetch m_NimbyGreetFetch;
    protected ref NimbyGreetSeen m_NimbyGreetSeen;
    protected ref NimbyGreetPrefs m_NimbyGreetPrefs;
    protected UAInput m_NimbyGreetInput;
    protected string m_NimbyGreetDetectedLanguage = "english";
    protected bool m_NimbyGreetAutoOpened;         // G4: at most one automatic open per connection
    protected bool m_NimbyGreetOpenPending;
    protected int m_NimbyGreetOpenRetries;
    protected int m_NimbyGreetOpenRetryMax = 30;
    protected string m_NimbyGreetPendingSource = "spawn";
    protected bool m_NimbyGreetPendingForced;
    protected ref NimbyGreetLanguage m_NimbyGreetPendingLanguage;
    protected ref NimbyGreetDone m_NimbyGreetPendingDone;
    protected int m_NimbyGreetSpawnCount;

    override void OnInit()
    {
        super.OnInit();
        m_NimbyGreetFetch = new NimbyGreetFetch();
        m_NimbyGreetSeen = NimbyGreetClientFiles.LoadSeen();
        m_NimbyGreetPrefs = NimbyGreetClientFiles.LoadPrefs();
        if (GetUApi())
        {
            m_NimbyGreetInput = GetUApi().GetInputByName("UANimbyGreetOpen");
        }
        NimbyGreetClient.Spawned().Insert(NimbyGreetOnSpawned);
        NimbyGreetClient.Chunk().Insert(NimbyGreetOnChunk);
        NimbyGreetClient.Done().Insert(NimbyGreetOnDone);
        NimbyGreetDetectLanguage();
        NimbyGreetLog.Debug("client ready; input bound=" + NimbyGreetLog.Bool(m_NimbyGreetInput != null) + " language detected=" + m_NimbyGreetDetectedLanguage + " prefLanguage=" + m_NimbyGreetPrefs.language + " neverShow=" + NimbyGreetLog.Bool(m_NimbyGreetSeen.neverShow) + " dismissedVersion=" + m_NimbyGreetSeen.dismissedVersion.ToString() + " textScale=" + m_NimbyGreetPrefs.textScale.ToString() + " animations=" + NimbyGreetLog.Bool(m_NimbyGreetPrefs.playAnimations));
    }

    override void OnMissionFinish()
    {
        NimbyGreetClient.Spawned().Remove(NimbyGreetOnSpawned);
        NimbyGreetClient.Chunk().Remove(NimbyGreetOnChunk);
        NimbyGreetClient.Done().Remove(NimbyGreetOnDone);
        m_NimbyGreetOpenPending = false;
        NimbyGreetLog.Debug("mission finished; spawns this connection=" + m_NimbyGreetSpawnCount.ToString() + " autoOpened=" + NimbyGreetLog.Bool(m_NimbyGreetAutoOpened));
        super.OnMissionFinish();
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        NimbyGreetHandleHotkey();
    }

    // ---- language (G14) ----------------------------------------------------------------------

    protected void NimbyGreetDetectLanguage()
    {
        string key = Widget.TranslateString("#STR_NimbyGreet_Language");
        if (NimbyGreetRules.IsKnownLanguage(key))
        {
            m_NimbyGreetDetectedLanguage = key;
        }
        else
        {
            m_NimbyGreetDetectedLanguage = "";
            NimbyGreetLog.Info("language key did not resolve (got '" + key + "'); the server default will be requested");
        }
        NimbyGreetLog.Debug("language: TranslateString=" + key + " idx=" + g_Game.GetCurrentDisplayLanguageIdx().ToString());
    }

    // The language to request: the prefs override, else the detected one, else empty (server default).
    string NimbyGreetRequestLanguage()
    {
        if (m_NimbyGreetPrefs.language.Length() > 0)
        {
            return m_NimbyGreetPrefs.language;
        }
        return m_NimbyGreetDetectedLanguage;
    }

    // ---- spawn and fetch (G5) ----------------------------------------------------------------

    void NimbyGreetOnSpawned(PlayerBase player)
    {
        m_NimbyGreetSpawnCount = m_NimbyGreetSpawnCount + 1;
        if (m_NimbyGreetAutoOpened)
        {
            NimbyGreetLog.Debug("spawn #" + m_NimbyGreetSpawnCount.ToString() + ": automatic open already used this connection; no request");
            return;
        }
        NimbyGreetLog.Debug("spawn #" + m_NimbyGreetSpawnCount.ToString() + ": request in " + NimbyGreetRules.FETCH_DELAY_MS.ToString() + " ms");
        g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(NimbyGreetAutoRequest, NimbyGreetRules.FETCH_DELAY_MS, false);
    }

    protected void NimbyGreetAutoRequest()
    {
        NimbyGreetSendRequest(false, "spawn");
    }

    // Sends one request; a new request replaces the one in flight (its late chunks are dropped).
    int NimbyGreetSendRequest(bool manual, string source)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
        {
            NimbyGreetLog.Debug("request not sent: no controlled player");
            return -1;
        }
        int requestId = NimbyGreetClient.NextRequestId();
        if (m_NimbyGreetFetch.IsActive() && !m_NimbyGreetFetch.IsComplete())
        {
            NimbyGreetLog.Debug("request id=" + requestId.ToString() + " replaces incomplete id=" + m_NimbyGreetFetch.m_RequestId.ToString() + " (received=" + m_NimbyGreetFetch.m_Received.ToString() + ")");
        }
        m_NimbyGreetOpenPending = false;
        m_NimbyGreetPendingSource = source;
        string language = NimbyGreetRequestLanguage();
        m_NimbyGreetFetch.Begin(requestId, language, manual, GetGame().GetTickTime());
        player.NimbyGreetSendRequest(requestId, language, m_NimbyGreetSeen.dismissedVersion, m_NimbyGreetSeen.neverShow, manual);
        return requestId;
    }

    void NimbyGreetOnChunk(int requestId, int index, string text)
    {
        string why;
        if (!m_NimbyGreetFetch.AcceptChunk(requestId, index, text, GetGame().GetTickTime(), why))
        {
            NimbyGreetLog.Debug("chunk dropped id=" + requestId.ToString() + " index=" + index.ToString() + " bytes=" + text.Length().ToString() + " why=" + why);
            return;
        }
        NimbyGreetLog.Debug("chunk id=" + requestId.ToString() + " index=" + index.ToString() + " bytes=" + text.Length().ToString() + " received=" + m_NimbyGreetFetch.m_Received.ToString());
        if (m_NimbyGreetFetch.IsComplete())
        {
            NimbyGreetOnPayloadComplete();
        }
    }

    void NimbyGreetOnDone(int requestId, int chunkCount, string doneJson)
    {
        string error;
        NimbyGreetDone done = NimbyGreetDone.FromJson(doneJson, error);
        if (!done)
        {
            NimbyGreetLog.Info("done record unreadable id=" + requestId.ToString() + ": " + error);
        }
        string why;
        if (!m_NimbyGreetFetch.AcceptDone(requestId, chunkCount, done, GetGame().GetTickTime(), why))
        {
            NimbyGreetLog.Debug("done dropped id=" + requestId.ToString() + " why=" + why);
            return;
        }
        NimbyGreetLog.Debug("done id=" + requestId.ToString() + " chunkCount=" + chunkCount.ToString() + " version=" + done.version.ToString() + " served=" + done.servedLanguage + " reason=" + done.reason + " received=" + m_NimbyGreetFetch.m_Received.ToString());
        if (m_NimbyGreetFetch.IsComplete())
        {
            NimbyGreetOnPayloadComplete();
        }
    }

    protected void NimbyGreetOnPayloadComplete()
    {
        NimbyGreetDone done = m_NimbyGreetFetch.m_Done;
        bool manual = m_NimbyGreetFetch.m_Manual;
        if (m_NimbyGreetFetch.m_ChunkCount == 0)
        {
            NimbyGreetLog.Info("nothing to show id=" + m_NimbyGreetFetch.m_RequestId.ToString() + " reason=" + done.reason + " version=" + done.version.ToString() + " manual=" + NimbyGreetLog.Bool(manual));
            if (!manual)
            {
                m_NimbyGreetAutoOpened = true;
            }
            return;
        }
        string assembled = m_NimbyGreetFetch.Assemble();
        string verdict = m_NimbyGreetFetch.Verify(assembled);
        int elapsed = m_NimbyGreetFetch.ElapsedMs();
        if (verdict != "intact")
        {
            NimbyGreetLog.Info("payload rejected id=" + m_NimbyGreetFetch.m_RequestId.ToString() + " " + verdict);
            return;
        }
        NimbyGreetLanguage language = new NimbyGreetLanguage();
        string error;
        JsonSerializer serializer = new JsonSerializer();
        if (!serializer.ReadFromString(language, assembled, error))
        {
            NimbyGreetLog.Info("payload unreadable id=" + m_NimbyGreetFetch.m_RequestId.ToString() + ": " + error);
            return;
        }
        array<string> capNotes;
        int cuts = NimbyGreetText.ApplyCaps(language, capNotes);
        NimbyGreetLog.Debug("payload complete id=" + m_NimbyGreetFetch.m_RequestId.ToString() + " language=" + language.language + " version=" + done.version.ToString() + " chunks=" + m_NimbyGreetFetch.m_ChunkCount.ToString() + " bytes=" + assembled.Length().ToString() + " sections=" + language.sections.Count().ToString() + " clientCuts=" + cuts.ToString() + " elapsedMs=" + elapsed.ToString());
        m_NimbyGreetPendingLanguage = language;
        m_NimbyGreetPendingDone = done;
        m_NimbyGreetPendingForced = false;
        if (!manual)
        {
            m_NimbyGreetAutoOpened = true;
            m_NimbyGreetPendingForced = done.forceShow && m_NimbyGreetSeen.neverShow && done.version > m_NimbyGreetSeen.dismissedVersion;
        }
        float delaySeconds = done.openDelaySeconds;
        if (manual)
        {
            delaySeconds = 0.0;
        }
        m_NimbyGreetOpenRetryMax = (int)(done.openRetrySeconds * 1000.0 / NimbyGreetRules.OPEN_RETRY_MS);
        if (m_NimbyGreetOpenRetryMax < 1)
        {
            m_NimbyGreetOpenRetryMax = 1;
        }
        m_NimbyGreetOpenRetries = 0;
        m_NimbyGreetOpenPending = true;
        int delayMs = (int)(delaySeconds * 1000.0);
        NimbyGreetLog.Debug("open scheduled in " + delayMs.ToString() + " ms source=" + m_NimbyGreetPendingSource);
        g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(NimbyGreetTryOpen, delayMs, false);
    }

    // ---- open (G5) ---------------------------------------------------------------------------

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
            NimbyGreetLog.Debug("open deferred #" + m_NimbyGreetOpenRetries.ToString() + ": menu=" + menuId.ToString() + " alive=" + NimbyGreetLog.Bool(alive) + " unconscious=" + NimbyGreetLog.Bool(unconscious));
            if (m_NimbyGreetOpenRetries >= m_NimbyGreetOpenRetryMax)
            {
                m_NimbyGreetOpenPending = false;
                NimbyGreetLog.Info("open gave up after " + m_NimbyGreetOpenRetryMax.ToString() + " retries source=" + m_NimbyGreetPendingSource + " menu=" + menuId.ToString() + " alive=" + NimbyGreetLog.Bool(alive) + " unconscious=" + NimbyGreetLog.Bool(unconscious));
                return;
            }
            g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(NimbyGreetTryOpen, NimbyGreetRules.OPEN_RETRY_MS, false);
            return;
        }
        m_NimbyGreetOpenPending = false;
        NimbyGreetOpenWindow(m_NimbyGreetPendingSource);
    }

    // Step (c) replaces the body with the window. Until then the converted payload is logged.
    protected void NimbyGreetOpenWindow(string source)
    {
        if (!m_NimbyGreetPendingLanguage)
        {
            return;
        }
        if (m_NimbyGreetPendingForced)
        {
            // G4: a forced version is recorded at open, not at close, so it is seen exactly once.
            m_NimbyGreetPendingForced = false;
            m_NimbyGreetSeen.dismissedVersion = m_NimbyGreetPendingDone.version;
            NimbyGreetClientFiles.SaveSeen(m_NimbyGreetSeen);
            NimbyGreetLog.Info("forced version " + m_NimbyGreetPendingDone.version.ToString() + " recorded as seen");
        }
        NimbyGreetLog.Info("open version=" + m_NimbyGreetPendingDone.version.ToString() + " source=" + source + " language=" + m_NimbyGreetPendingLanguage.language + " title=" + m_NimbyGreetPendingLanguage.title + " (window not built yet: Phase 1 step c)");
        foreach (NimbyGreetSection section : m_NimbyGreetPendingLanguage.sections)
        {
            NimbyGreetConverted converted = NimbyGreetMarkdown.Convert(section.body, null);
            NimbyGreetLog.Debug("section id=" + section.id + " heading=" + section.heading + " blocks=" + converted.blocks.Count().ToString() + " links=" + converted.links.Count().ToString() + " notes=" + converted.notes.Count().ToString());
            foreach (string note : converted.notes)
            {
                NimbyGreetLog.Info("section " + section.id + ": " + note);
            }
        }
    }

    // ---- hotkey (G7, NIMBY-T-046: read here and nowhere else) --------------------------------

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
        if (menuId == NimbyGreetRules.MENU_ID)
        {
            NimbyGreetCloseWindow("hotkey");
            return;
        }
        if (menuId != -1)
        {
            NimbyGreetLog.Debug("hotkey ignored: another menu is open id=" + menuId.ToString());
            return;
        }
        if (!alive)
        {
            NimbyGreetLog.Debug("hotkey ignored: player dead");
            return;
        }
        if (unconscious)
        {
            NimbyGreetLog.Debug("hotkey ignored: player unconscious");
            return;
        }
        NimbyGreetLog.Debug("hotkey: manual request vehicle=" + NimbyGreetLog.Bool(vehicle));
        NimbyGreetSendRequest(true, "hotkey");
    }

    // Step (c) closes the window here.
    protected void NimbyGreetCloseWindow(string reason)
    {
        NimbyGreetLog.Debug("close requested reason=" + reason + " (window not built yet: Phase 1 step c)");
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
}
