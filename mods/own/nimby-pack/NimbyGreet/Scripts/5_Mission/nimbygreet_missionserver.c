// -----------------------------------------------------------------------------
// NimbyGreet - server side (plan v0.4, G3, G5, G9, G12, G14).
// Purpose : Owns the text. At mission start it reads $profile:NimbyGreet\greet.json with
//           JsonFileLoader<T>.LoadFile (3_Game/DayZ/tools/JsonFileLoader.c:7): a missing
//           file gets the G3 default (temp-validate-copy-delete, Current state "File
//           writes"); a malformed file logs one "greet.json invalid: <error>" line and the
//           last good copy (.lkg) is served instead; with neither there is no text and every
//           request gets a bare done. Every adopted parse refreshes the .lkg copy, written
//           from the text just parsed (not copied from disk, which may have changed again).
//           A request re-reads the file when the copy is older than rereadSeconds, so an
//           edit reaches the next joiner without a restart and a burst of joins costs one read.
//           Each request is rate-limited per identity (requestCooldownSeconds), answered
//           with the picked language's JSON in CHUNK_BYTES chunks and a done record, or
//           with a bare done when the client is dismissed (G4) or there is no text.
// Note    : NIMBY-T-042: the engine prints its own "SCRIPT (E): JSON ERROR" on a malformed
//           file no matter what this file does. scripts/Test-GreetText.ps1 is what keeps a
//           malformed file off the host; this code keeps players served meanwhile.
//           NIMBY-T-056: that error came once per re-read of the same bad file (G-P1-36, -37),
//           so one per join. The file is read as text first (the JsonFileLoader.c:11-21 steps)
//           and an unchanged copy of the last bad text is not parsed again: one engine error
//           per distinct bad file. The fix build before release 1 (plan Phase 2, Jeff, October
//           26, 2026) extends that: an unchanged bad .lkg is not parsed again either, a
//           greet.json problem is logged when it changes rather than on every re-read, and an
//           unchanged good text is not parsed, capped and logged again.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

modded class MissionServer extends MissionBase
{
    protected ref NimbyGreetFile m_NimbyGreetFile;
    protected ref array<string> m_NimbyGreetCodes;
    protected ref map<string, string> m_NimbyGreetSerialized;     // language code to Language JSON
    protected ref map<string, float> m_NimbyGreetLastServed;      // identity id to tick time
    protected ref map<string, float> m_NimbyGreetLastRefusedLog;  // identity id to tick time
    protected float m_NimbyGreetLoadedAt = -1.0;
    protected string m_NimbyGreetSource = "none";                 // greet.json | greet.json.lkg | default | none
    protected string m_NimbyGreetLastError = "";                  // last greet.json problem logged ("invalid: <error>" or "cannot be opened for reading"); "" once a read parses again
    protected string m_NimbyGreetLastFallback = "";               // last fallback line logged, so a re-read logs only a change
    protected string m_NimbyGreetLastBadText = "";                // NIMBY-T-056: the text of the last greet.json that did not parse or was not servable
    protected string m_NimbyGreetLastBadReason = "";              // "invalid" or "unservable" for that text
    protected bool m_NimbyGreetHaveBadText;
    protected string m_NimbyGreetLastGoodText = "";               // the greet.json text adopted last, so an unchanged file is not parsed again
    protected string m_NimbyGreetLastBadLkgText = "";             // the text of the last greet.json.lkg that did not parse or was not servable
    protected bool m_NimbyGreetHaveBadLkgText;

    override void OnInit()
    {
        super.OnInit();
        m_NimbyGreetCodes = new array<string>;
        m_NimbyGreetSerialized = new map<string, string>;
        m_NimbyGreetLastServed = new map<string, float>;
        m_NimbyGreetLastRefusedLog = new map<string, float>;
        if (!FileExist(NimbyGreetRules.DIR))
        {
            MakeDirectory(NimbyGreetRules.DIR);
        }
        NimbyGreetLoadText(true);
        NimbyGreetClient.Request().Insert(NimbyGreetOnRequest);
    }

    override void OnMissionFinish()
    {
        NimbyGreetClient.Request().Remove(NimbyGreetOnRequest);
        super.OnMissionFinish();
    }

    override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
    {
        super.InvokeOnConnect(player, identity);
        NimbyGreetLog.Debug("ClientReady: " + NimbyGreetLog.Who(identity));
    }

    // ---- file ------------------------------------------------------------------------------

    // Reads greet.json. firstBoot writes the default when the file is missing; a later re-read never does.
    // A file that parses but has no servable language (self-review) keeps the copy already served and
    // never overwrites the .lkg. A problem ("invalid: <error>", "cannot be opened for reading") is logged
    // when it changes, not on every re-read; the text of a bad file is kept so an unchanged bad file is
    // not parsed again (NIMBY-T-056); the text of the adopted file is kept so an unchanged good file is
    // not parsed, capped and logged again.
    protected void NimbyGreetLoadText(bool firstBoot)
    {
        m_NimbyGreetLoadedAt = GetGame().GetTickTime();
        if (!FileExist(NimbyGreetRules.FILE_TEXT))
        {
            if (firstBoot)
            {
                NimbyGreetFile created = NimbyGreetText.DefaultFile();
                string writeError;
                bool written = NimbyGreetWriteAtomic(created, writeError);
                NimbyGreetLog.Info("greet.json missing; default written=" + NimbyGreetLog.Bool(written) + " error=" + writeError);
                if (written)
                {
                    string createdRaw;
                    bool createdRead = NimbyGreetReadRaw(NimbyGreetRules.FILE_TEXT, createdRaw);
                    if (NimbyGreetAdopt(created, "default") && createdRead)
                    {
                        NimbyGreetRememberGood(createdRaw);
                    }
                    return;
                }
            }
            NimbyGreetLoadLkg("missing");
            return;
        }
        string raw;
        if (!NimbyGreetReadRaw(NimbyGreetRules.FILE_TEXT, raw))
        {
            NimbyGreetLogProblem("cannot be opened for reading");
            NimbyGreetLoadLkg("unreadable");
            return;
        }
        if (m_NimbyGreetHaveBadText && raw == m_NimbyGreetLastBadText)
        {
            NimbyGreetLoadLkg(m_NimbyGreetLastBadReason);
            return;
        }
        bool servingFile = m_NimbyGreetSource == "greet.json" || m_NimbyGreetSource == "default";
        if (servingFile && raw == m_NimbyGreetLastGoodText)
        {
            // The text being served, unchanged: nothing to parse, cap or log. A problem logged since
            // the last adoption (a bad edit since reverted, a read that failed) is closed with one line.
            if (m_NimbyGreetHaveBadText || m_NimbyGreetLastError != "")
            {
                NimbyGreetLog.Info("greet.json readable again and unchanged; serving version=" + m_NimbyGreetFile.version.ToString());
            }
            NimbyGreetForgetBad();
            return;
        }
        NimbyGreetFile data = new NimbyGreetFile();
        string error;
        JsonSerializer serializer = new JsonSerializer();
        bool loaded = serializer.ReadFromString(data, raw, error);
        if (!loaded)
        {
            NimbyGreetRememberBad(raw, "invalid");
            NimbyGreetLogProblem("invalid: " + error);
            NimbyGreetLoadLkg("invalid");
            return;
        }
        if (!NimbyGreetAdopt(data, "greet.json"))
        {
            NimbyGreetRememberBad(raw, "unservable");
            NimbyGreetLoadLkg("unservable");
            return;
        }
        NimbyGreetForgetBad();
        NimbyGreetRememberGood(raw);
    }

    // A greet.json problem is logged once per change; a read that parses clears it (NimbyGreetForgetBad).
    protected void NimbyGreetLogProblem(string problem)
    {
        if (problem == m_NimbyGreetLastError)
        {
            return;
        }
        m_NimbyGreetLastError = problem;
        NimbyGreetLog.Info("greet.json " + problem);
    }

    // A fallback outcome is logged once per change; an adoption clears it (NimbyGreetAdopt).
    protected void NimbyGreetLogFallback(string line)
    {
        if (line == m_NimbyGreetLastFallback)
        {
            return;
        }
        m_NimbyGreetLastFallback = line;
        NimbyGreetLog.Info(line);
    }

    protected void NimbyGreetRememberBad(string raw, string reason)
    {
        m_NimbyGreetLastBadText = raw;
        m_NimbyGreetLastBadReason = reason;
        m_NimbyGreetHaveBadText = true;
    }

    protected void NimbyGreetForgetBad()
    {
        m_NimbyGreetLastBadText = "";
        m_NimbyGreetLastBadReason = "";
        m_NimbyGreetHaveBadText = false;
        m_NimbyGreetLastError = "";
    }

    // Keeps the adopted text and refreshes the .lkg from it.
    protected void NimbyGreetRememberGood(string raw)
    {
        m_NimbyGreetLastGoodText = raw;
        NimbyGreetWriteLkg(raw);
    }

    // The .lkg is written from the text that was just parsed and adopted, not copied from greet.json,
    // which may have been edited again between the read and the copy. Temp, read back, copy, delete,
    // the NimbyGreetWriteAtomic steps (there is no rename, EnSystem.c:525-531).
    protected void NimbyGreetWriteLkg(string raw)
    {
        FileHandle handle = OpenFile(NimbyGreetRules.FILE_TEXT_LKG_TMP, FileMode.WRITE);
        if (handle == 0)
        {
            NimbyGreetLog.Info("greet.json.lkg not refreshed: cannot open " + NimbyGreetRules.FILE_TEXT_LKG_TMP + " for writing");
            return;
        }
        FPrint(handle, raw);
        CloseFile(handle);
        string check;
        bool readBack = NimbyGreetReadRaw(NimbyGreetRules.FILE_TEXT_LKG_TMP, check);
        if (!readBack || check != raw)
        {
            NimbyGreetLog.Info("greet.json.lkg not refreshed: the temp file did not read back as written");
            DeleteFile(NimbyGreetRules.FILE_TEXT_LKG_TMP);
            return;
        }
        if (!CopyFile(NimbyGreetRules.FILE_TEXT_LKG_TMP, NimbyGreetRules.FILE_TEXT_LKG))
        {
            NimbyGreetLog.Info("greet.json.lkg not refreshed: CopyFile failed");
            DeleteFile(NimbyGreetRules.FILE_TEXT_LKG_TMP);
            return;
        }
        DeleteFile(NimbyGreetRules.FILE_TEXT_LKG_TMP);
        m_NimbyGreetLastBadLkgText = "";
        m_NimbyGreetHaveBadLkgText = false;
    }

    // The whole file as text, the way JsonFileLoader.LoadFile reads it (JsonFileLoader.c:11-21).
    protected bool NimbyGreetReadRaw(string path, out string raw)
    {
        raw = "";
        FileHandle handle = OpenFile(path, FileMode.READ);
        if (handle == 0)
        {
            return false;
        }
        ReadFile(handle, raw, 100000000);
        CloseFile(handle);
        return true;
    }

    // Serves the copy already in memory, else the .lkg. The .lkg is read as text first and an unchanged
    // copy of the last bad .lkg is not parsed again (one engine JSON ERROR per distinct bad file, the
    // NIMBY-T-056 rule for greet.json); every outcome is logged once per change.
    protected void NimbyGreetLoadLkg(string because)
    {
        if (m_NimbyGreetFile && m_NimbyGreetSource != "none")
        {
            NimbyGreetLogFallback("greet.json " + because + "; serving the copy loaded earlier this boot (source=" + m_NimbyGreetSource + " version=" + m_NimbyGreetFile.version.ToString() + ")");
            return;
        }
        if (!FileExist(NimbyGreetRules.FILE_TEXT_LKG))
        {
            NimbyGreetLogFallback("greet.json " + because + " and no greet.json.lkg; no text until the file is fixed");
            return;
        }
        string raw;
        if (!NimbyGreetReadRaw(NimbyGreetRules.FILE_TEXT_LKG, raw))
        {
            NimbyGreetLogFallback("greet.json " + because + " and greet.json.lkg cannot be opened for reading; no text until the file is fixed");
            return;
        }
        if (m_NimbyGreetHaveBadLkgText && raw == m_NimbyGreetLastBadLkgText)
        {
            return;
        }
        NimbyGreetFile data = new NimbyGreetFile();
        string error;
        JsonSerializer serializer = new JsonSerializer();
        if (!serializer.ReadFromString(data, raw, error))
        {
            m_NimbyGreetLastBadLkgText = raw;
            m_NimbyGreetHaveBadLkgText = true;
            NimbyGreetLogFallback("greet.json " + because + " and greet.json.lkg invalid: " + error + "; no text until the file is fixed");
            return;
        }
        if (!NimbyGreetAdopt(data, "greet.json.lkg"))
        {
            m_NimbyGreetLastBadLkgText = raw;
            m_NimbyGreetHaveBadLkgText = true;
            NimbyGreetLogFallback("greet.json " + because + " and greet.json.lkg has no servable language; no text until the file is fixed");
        }
    }

    // Applies the caps and the language rules. True when at least one language is servable; the file
    // then replaces the one being served. False leaves the served state untouched.
    protected bool NimbyGreetAdopt(NimbyGreetFile data, string source)
    {
        NimbyGreetText.ClampTiming(data);
        array<string> codes;
        array<string> languageNotes;
        NimbyGreetText.LanguageCodes(data, codes, languageNotes);
        foreach (string note : languageNotes)
        {
            NimbyGreetLog.Info("greet.json " + note);
        }
        int totalCuts = 0;
        int totalBytes = 0;
        map<string, string> serialized = new map<string, string>;
        array<string> servable = new array<string>;
        foreach (string code : codes)
        {
            NimbyGreetLanguage language = NimbyGreetText.FindLanguage(data, code);
            array<string> capNotes;
            int cuts = NimbyGreetText.ApplyCaps(language, capNotes);
            foreach (string capNote : capNotes)
            {
                NimbyGreetLog.Info("greet.json truncated " + code + ": " + capNote);
            }
            totalCuts = totalCuts + cuts;
            if (language.sections.Count() == 0)
            {
                NimbyGreetLog.Info("greet.json language " + code + " has no section; not served");
                continue;
            }
            string json;
            JsonSerializer serializer = new JsonSerializer();
            serializer.WriteToString(language, false, json);
            if (json.Length() > NimbyGreetRules.MAX_PAYLOAD_BYTES)
            {
                NimbyGreetLog.Info("greet.json language " + code + " serializes to " + json.Length().ToString() + " bytes, above the payload cap; not served");
                continue;
            }
            serialized.Insert(code, json);
            servable.Insert(code);
            totalBytes = totalBytes + json.Length();
        }
        if (servable.Count() == 0)
        {
            NimbyGreetLog.Info("greet.json has no servable language (source=" + source + "); the file is not adopted");
            return false;
        }
        if (servable.Find(data.defaultLanguage) < 0)
        {
            NimbyGreetLog.Info("greet.json defaultLanguage " + data.defaultLanguage + " is not served; using " + servable.Get(0));
            data.defaultLanguage = servable.Get(0);
        }
        m_NimbyGreetSerialized = serialized;
        m_NimbyGreetCodes = servable;
        m_NimbyGreetFile = data;
        m_NimbyGreetSource = source;
        m_NimbyGreetLastFallback = "";
        string line = "greet.json loaded source=" + source + " version=" + data.version.ToString() + " forceShow=" + NimbyGreetLog.Bool(data.forceShow);
        line = line + " languages=" + string.Join(",", m_NimbyGreetCodes) + " default=" + data.defaultLanguage;
        line = line + " bytes=" + totalBytes.ToString() + " truncations=" + totalCuts.ToString();
        line = line + " openDelay=" + data.openDelaySeconds.ToString() + " cooldown=" + data.requestCooldownSeconds.ToString() + " reread=" + data.rereadSeconds.ToString();
        NimbyGreetLog.Info(line);
        return true;
    }

    // Write the temp file, read it back, copy over the target, delete the temp (there is no rename, EnSystem.c:525-531).
    protected bool NimbyGreetWriteAtomic(NimbyGreetFile data, out string error)
    {
        error = "";
        if (!JsonFileLoader<NimbyGreetFile>.SaveFile(NimbyGreetRules.FILE_TEXT_TMP, data, error))
        {
            return false;
        }
        NimbyGreetFile check = new NimbyGreetFile();
        if (!JsonFileLoader<NimbyGreetFile>.LoadFile(NimbyGreetRules.FILE_TEXT_TMP, check, error))
        {
            DeleteFile(NimbyGreetRules.FILE_TEXT_TMP);
            return false;
        }
        if (!CopyFile(NimbyGreetRules.FILE_TEXT_TMP, NimbyGreetRules.FILE_TEXT))
        {
            error = "CopyFile failed";
            DeleteFile(NimbyGreetRules.FILE_TEXT_TMP);
            return false;
        }
        DeleteFile(NimbyGreetRules.FILE_TEXT_TMP);
        return true;
    }

    // ---- requests ----------------------------------------------------------------------------

    void NimbyGreetOnRequest(PlayerBase player, PlayerIdentity sender, int requestId, string language, int dismissedVersion, bool neverShow, bool manual)
    {
        if (!player || !sender)
        {
            return;
        }
        float now = GetGame().GetTickTime();
        string who = NimbyGreetLog.Who(sender);
        // The language string comes from the client; only a known column name reaches the log (no forged lines).
        string asked = language;
        if (!NimbyGreetRules.IsKnownLanguage(language))
        {
            asked = "<unknown:" + language.Length().ToString() + "b>";
        }
        string key = sender.GetId();
        float cooldown = NimbyGreetRules.DEFAULT_COOLDOWN_SECONDS;
        float reread = NimbyGreetRules.DEFAULT_REREAD_SECONDS;
        if (m_NimbyGreetFile)
        {
            cooldown = m_NimbyGreetFile.requestCooldownSeconds;
            reread = m_NimbyGreetFile.rereadSeconds;
        }
        float last;
        if (m_NimbyGreetLastServed.Find(key, last))
        {
            if (now - last < cooldown)
            {
                // Phase 1 run 1: a silent refusal left the language selector waiting on nothing. The refusal
                // is now a bare done with the time left, and a manual request retries itself once after it.
                float lastLog;
                bool logged = m_NimbyGreetLastRefusedLog.Find(key, lastLog);
                if (!logged || now - lastLog >= cooldown)
                {
                    m_NimbyGreetLastRefusedLog.Set(key, now);
                    NimbyGreetLog.Info("request refused reason=cooldown id=" + requestId.ToString() + " " + who);
                }
                NimbyGreetDone refused = new NimbyGreetDone();
                refused.reason = "cooldown";
                refused.retryAfterSeconds = cooldown - (now - last);
                player.NimbyGreetSendDone(sender, requestId, 0, NimbyGreetDone.ToJson(refused));
                return;
            }
        }
        m_NimbyGreetLastServed.Set(key, now);
        if (now - m_NimbyGreetLoadedAt >= reread)
        {
            NimbyGreetLoadText(false);
        }
        NimbyGreetDone done = new NimbyGreetDone();
        if (!m_NimbyGreetFile)
        {
            done.reason = "no-text";
            player.NimbyGreetSendDone(sender, requestId, 0, NimbyGreetDone.ToJson(done));
            NimbyGreetLog.Info("serve id=" + requestId.ToString() + " chunks=0 reason=no-text asked=" + asked + " manual=" + NimbyGreetLog.Bool(manual) + " " + who);
            return;
        }
        done.version = m_NimbyGreetFile.version;
        done.forceShow = m_NimbyGreetFile.forceShow;
        done.openDelaySeconds = m_NimbyGreetFile.openDelaySeconds;
        done.openRetrySeconds = m_NimbyGreetFile.openRetrySeconds;
        foreach (string code : m_NimbyGreetCodes)
        {
            done.languages.Insert(code);
        }
        string served = NimbyGreetRules.PickLanguage(language, m_NimbyGreetCodes, m_NimbyGreetFile.defaultLanguage);
        done.servedLanguage = served;
        bool open = manual || NimbyGreetRules.ShouldAutoOpen(neverShow, dismissedVersion, m_NimbyGreetFile.forceShow, m_NimbyGreetFile.version);
        if (!open)
        {
            done.reason = "dismissed";
            player.NimbyGreetSendDone(sender, requestId, 0, NimbyGreetDone.ToJson(done));
            string dismissedLine = "serve id=" + requestId.ToString() + " chunks=0 reason=dismissed version=" + m_NimbyGreetFile.version.ToString();
            dismissedLine = dismissedLine + " dismissedVersion=" + dismissedVersion.ToString() + " asked=" + asked + " " + who;
            NimbyGreetLog.Info(dismissedLine);
            return;
        }
        string payload;
        if (!m_NimbyGreetSerialized.Find(served, payload))
        {
            done.reason = "no-text";
            player.NimbyGreetSendDone(sender, requestId, 0, NimbyGreetDone.ToJson(done));
            NimbyGreetLog.Info("serve id=" + requestId.ToString() + " chunks=0 reason=no-text (language " + served + " not serialized) " + who);
            return;
        }
        array<string> chunks;
        int count = NimbyGreetRules.SplitChunks(payload, NimbyGreetRules.CHUNK_BYTES, chunks);
        int i = 0;
        while (i < count)
        {
            player.NimbyGreetSendChunk(sender, requestId, i, chunks.Get(i));
            i = i + 1;
        }
        done.totalBytes = payload.Length();
        done.hash = payload.Hash();
        string doneJson = NimbyGreetDone.ToJson(done);
        if (doneJson.Length() > NimbyGreetRules.CHUNK_BYTES)
        {
            NimbyGreetLog.Info("done record is " + doneJson.Length().ToString() + " bytes, above CHUNK_BYTES; the client will drop it");
        }
        player.NimbyGreetSendDone(sender, requestId, count, doneJson);
        string serveLine = "serve id=" + requestId.ToString() + " asked=" + asked + " served=" + served + " version=" + m_NimbyGreetFile.version.ToString();
        serveLine = serveLine + " chunks=" + count.ToString() + " bytes=" + payload.Length().ToString() + " manual=" + NimbyGreetLog.Bool(manual) + " " + who;
        NimbyGreetLog.Info(serveLine);
    }
}
