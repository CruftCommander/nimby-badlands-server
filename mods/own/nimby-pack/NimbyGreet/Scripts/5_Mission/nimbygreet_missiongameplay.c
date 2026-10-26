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
// Window  : NimbyGreetMenu (nimbygreet_menu.c) is created here, shown with ShowScriptedMenu,
//           and closed here with a reason: player, escape, hotkey, hit (PlayerBase.OnReceivedHit
//           or EEHitByRemote through NimbyGreetClient.Hit), unconscious (polled while open),
//           death (vanilla's own close, UIScriptedMenu.c:603-611). The dismissal is written
//           when the window closes (G4, Q6); a forced version is recorded at open.
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
    protected ref NimbyGreetMenu m_NimbyGreetMenu;
    protected ref NimbyGreetManifest m_NimbyGreetManifest;
    protected bool m_NimbyGreetRetryPending;
    protected bool m_NimbyGreetDropArmed;   // G-21: $profile:NimbyGreet\dropchunk drops chunk index 1 of the next response once

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
        NimbyGreetClient.Hit().Insert(NimbyGreetOnHit);
        NimbyGreetDetectLanguage();
        NimbyGreetLoadManifest();
        m_NimbyGreetDropArmed = FileExist(NimbyGreetRules.FILE_DROPCHUNK);
        if (m_NimbyGreetDropArmed)
        {
            NimbyGreetLog.Info("debug: dropchunk flag present; chunk index 1 of the next response will be dropped once (G-21)");
        }
        string ready = "client ready; input bound=" + NimbyGreetLog.Bool(m_NimbyGreetInput != null) + " language detected=" + m_NimbyGreetDetectedLanguage + " prefLanguage=" + m_NimbyGreetPrefs.language;
        ready = ready + " neverShow=" + NimbyGreetLog.Bool(m_NimbyGreetSeen.neverShow) + " dismissedVersion=" + m_NimbyGreetSeen.dismissedVersion.ToString();
        ready = ready + " textScale=" + m_NimbyGreetPrefs.textScale.ToString() + " animations=" + NimbyGreetLog.Bool(m_NimbyGreetPrefs.playAnimations);
        NimbyGreetLog.Debug(ready);
    }

    override void OnMissionFinish()
    {
        NimbyGreetClient.Spawned().Remove(NimbyGreetOnSpawned);
        NimbyGreetClient.Chunk().Remove(NimbyGreetOnChunk);
        NimbyGreetClient.Done().Remove(NimbyGreetOnDone);
        NimbyGreetClient.Hit().Remove(NimbyGreetOnHit);
        m_NimbyGreetOpenPending = false;
        NimbyGreetLog.Debug("mission finished; spawns this connection=" + m_NimbyGreetSpawnCount.ToString() + " autoOpened=" + NimbyGreetLog.Bool(m_NimbyGreetAutoOpened));
        super.OnMissionFinish();
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        NimbyGreetHandleHotkey();
        NimbyGreetWatchWhileOpen();
    }

    // G15: the media list shipped in the PBO. No manifest or an unreadable one means no media.
    protected void NimbyGreetLoadManifest()
    {
        NimbyGreetManifest manifest = new NimbyGreetManifest();
        string error;
        if (!JsonFileLoader<NimbyGreetManifest>.LoadFile(NimbyGreetRules.MANIFEST_PATH, manifest, error))
        {
            NimbyGreetLog.Debug("manifest not loaded (" + error + "); no media this build");
            m_NimbyGreetManifest = new NimbyGreetManifest();
            return;
        }
        if (!manifest.media)
        {
            manifest.media = new array<ref NimbyGreetMediaEntry>;
        }
        m_NimbyGreetManifest = manifest;
        NimbyGreetLog.Debug("manifest loaded entries=" + manifest.media.Count().ToString());
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
        m_NimbyGreetRetryPending = false;
        if (m_NimbyGreetFetch.IsActive() && !m_NimbyGreetFetch.IsComplete())
        {
            NimbyGreetLog.Debug("request id=" + requestId.ToString() + " replaces incomplete id=" + m_NimbyGreetFetch.m_RequestId.ToString() + " (received=" + m_NimbyGreetFetch.m_Received.ToString() + ")");
        }
        m_NimbyGreetOpenPending = false;
        m_NimbyGreetPendingSource = source;
        string language = NimbyGreetRequestLanguage();
        m_NimbyGreetFetch.Begin(requestId, language, manual, GetGame().GetTickTime());
        player.NimbyGreetSendRequest(requestId, language, m_NimbyGreetSeen.dismissedVersion, m_NimbyGreetSeen.neverShow, manual);
        g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(NimbyGreetFetchTimeout, NimbyGreetRules.FETCH_TIMEOUT_MS, false, requestId);
        return requestId;
    }

    // G-21: a payload that never completes is given up once, with the counts; nothing partial ever opens.
    protected void NimbyGreetFetchTimeout(int requestId)
    {
        if (m_NimbyGreetFetch.m_RequestId != requestId)
        {
            return;
        }
        if (m_NimbyGreetFetch.IsComplete() || m_NimbyGreetFetch.IsRefused())
        {
            return;
        }
        string gaveUp = "fetch gave up id=" + requestId.ToString() + " received=" + m_NimbyGreetFetch.m_Received.ToString() + " of " + m_NimbyGreetFetch.m_ChunkCount.ToString();
        gaveUp = gaveUp + " doneSeen=" + NimbyGreetLog.Bool(m_NimbyGreetFetch.m_DoneSeen) + " source=" + m_NimbyGreetPendingSource + " (Insert sends a new request)";
        NimbyGreetLog.Info(gaveUp);
        m_NimbyGreetFetch.Begin(0, "", false, 0.0);
    }

    void NimbyGreetOnChunk(int requestId, int index, string text)
    {
        if (m_NimbyGreetDropArmed && index == 1 && requestId == m_NimbyGreetFetch.m_RequestId)
        {
            m_NimbyGreetDropArmed = false;
            NimbyGreetLog.Info("debug: dropchunk dropped index 1 of id=" + requestId.ToString() + " (G-21)");
            return;
        }
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
        string doneLine = "done id=" + requestId.ToString() + " chunkCount=" + chunkCount.ToString() + " version=" + done.version.ToString();
        doneLine = doneLine + " served=" + done.servedLanguage + " reason=" + done.reason + " received=" + m_NimbyGreetFetch.m_Received.ToString();
        NimbyGreetLog.Debug(doneLine);
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
            if (done.reason == "cooldown")
            {
                NimbyGreetScheduleRetry(done.retryAfterSeconds, manual);
                return;
            }
            NimbyGreetLog.Info("nothing to show id=" + m_NimbyGreetFetch.m_RequestId.ToString() + " reason=" + done.reason + " version=" + done.version.ToString() + " manual=" + NimbyGreetLog.Bool(manual));
            if (!manual)
            {
                m_NimbyGreetAutoOpened = true;
            }
            return;
        }
        m_NimbyGreetRetryPending = false;
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
        string completeLine = "payload complete id=" + m_NimbyGreetFetch.m_RequestId.ToString() + " language=" + language.language + " version=" + done.version.ToString();
        completeLine = completeLine + " chunks=" + m_NimbyGreetFetch.m_ChunkCount.ToString() + " bytes=" + assembled.Length().ToString();
        completeLine = completeLine + " sections=" + language.sections.Count().ToString() + " clientCuts=" + cuts.ToString() + " elapsedMs=" + elapsed.ToString();
        NimbyGreetLog.Debug(completeLine);
        m_NimbyGreetPendingLanguage = language;
        m_NimbyGreetPendingDone = done;
        if (m_NimbyGreetMenu)
        {
            m_NimbyGreetMenu.SetContent(language, done);
            NimbyGreetLog.Info("content replaced in the open window language=" + language.language + " version=" + done.version.ToString() + " source=" + m_NimbyGreetPendingSource);
            return;
        }
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

    // A cooldown refusal (G5 rate limit) retries the same request once when the window is waiting on it.
    protected void NimbyGreetScheduleRetry(float afterSeconds, bool manual)
    {
        if (m_NimbyGreetRetryPending)
        {
            NimbyGreetLog.Debug("cooldown refusal while a retry is already pending; dropped");
            return;
        }
        int delayMs = (int)(afterSeconds * 1000.0) + 150;
        if (delayMs < 150)
        {
            delayMs = 150;
        }
        m_NimbyGreetRetryPending = true;
        NimbyGreetLog.Debug("request refused by cooldown; retry in " + delayMs.ToString() + " ms source=" + m_NimbyGreetPendingSource + " manual=" + NimbyGreetLog.Bool(manual));
        g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(NimbyGreetRetryRequest, delayMs, false, manual);
    }

    protected void NimbyGreetRetryRequest(bool manual)
    {
        if (!m_NimbyGreetRetryPending)
        {
            return;
        }
        m_NimbyGreetRetryPending = false;
        NimbyGreetSendRequest(manual, m_NimbyGreetPendingSource);
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

    protected void NimbyGreetOpenWindow(string source)
    {
        if (!m_NimbyGreetPendingLanguage || !m_NimbyGreetPendingDone)
        {
            return;
        }
        if (m_NimbyGreetMenu)
        {
            m_NimbyGreetMenu.SetContent(m_NimbyGreetPendingLanguage, m_NimbyGreetPendingDone);
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
        m_NimbyGreetMenu = new NimbyGreetMenu();
        m_NimbyGreetMenu.Setup(m_NimbyGreetPendingLanguage, m_NimbyGreetPendingDone, m_NimbyGreetPrefs, m_NimbyGreetSeen, m_NimbyGreetManifest, source);
        GetGame().GetUIManager().ShowScriptedMenu(m_NimbyGreetMenu, null);
        string openLine = "open version=" + m_NimbyGreetPendingDone.version.ToString() + " source=" + source + " language=" + m_NimbyGreetPendingLanguage.language;
        openLine = openLine + " section=" + m_NimbyGreetMenu.GetSelectedSectionId() + " sections=" + m_NimbyGreetPendingLanguage.sections.Count().ToString() + " topMenu=" + NimbyGreetCurrentMenuId().ToString();
        NimbyGreetLog.Info(openLine);
    }

    // Asks the window to close with a reason; OnMenuClosed does the bookkeeping.
    protected void NimbyGreetCloseWindow(string reason)
    {
        if (!m_NimbyGreetMenu)
        {
            return;
        }
        m_NimbyGreetMenu.SetCloseReason(reason);
        m_NimbyGreetMenu.Close();
    }

    // Called from NimbyGreetMenu.OnHide, whoever closed it.
    void NimbyGreetOnMenuClosed(NimbyGreetMenu menu)
    {
        if (menu != m_NimbyGreetMenu)
        {
            return;
        }
        string reason = menu.GetCloseReason();
        if (reason == "")
        {
            reason = "vanilla";
            PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
            if (!player || !player.IsAlive())
            {
                reason = "death";
            }
        }
        bool checked = menu.IsNeverChecked();
        int version = 0;
        if (m_NimbyGreetPendingDone)
        {
            version = m_NimbyGreetPendingDone.version;
        }
        bool write = false;
        if (checked)
        {
            // G4, Q6: ticked at close writes the dismissal, from any open.
            if (!m_NimbyGreetSeen.neverShow || m_NimbyGreetSeen.dismissedVersion != version)
            {
                m_NimbyGreetSeen.neverShow = true;
                m_NimbyGreetSeen.dismissedVersion = version;
                write = true;
            }
        }
        else
        {
            // Q6: unticked at close after a dismissal clears it, the only in-game undo.
            if (m_NimbyGreetSeen.neverShow)
            {
                m_NimbyGreetSeen.neverShow = false;
                write = true;
            }
        }
        if (write)
        {
            NimbyGreetClientFiles.SaveSeen(m_NimbyGreetSeen);
        }
        string closeLine = "close reason=" + reason + " neverShow=" + NimbyGreetLog.Bool(m_NimbyGreetSeen.neverShow) + " dismissedVersion=" + m_NimbyGreetSeen.dismissedVersion.ToString();
        closeLine = closeLine + " written=" + NimbyGreetLog.Bool(write) + " section=" + menu.GetSelectedSectionId();
        NimbyGreetLog.Info(closeLine);
        m_NimbyGreetMenu = null;
    }

    // G5: a hit or unconsciousness closes the window at once; health drift never does.
    void NimbyGreetOnHit()
    {
        if (!m_NimbyGreetMenu)
        {
            return;
        }
        NimbyGreetCloseWindow("hit");
    }

    protected void NimbyGreetWatchWhileOpen()
    {
        if (!m_NimbyGreetMenu)
        {
            return;
        }
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
        {
            return;
        }
        if (player.IsAlive() && player.IsUnconscious())
        {
            NimbyGreetCloseWindow("unconscious");
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
        if (menuId == NimbyGreetRules.MENU_ID && m_NimbyGreetMenu)
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
