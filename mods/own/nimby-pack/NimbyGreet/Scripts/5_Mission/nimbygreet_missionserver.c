// -----------------------------------------------------------------------------
// NimbyGreet - server side (plan v0.4, G3, G5, G9, G12, G14).
// Purpose : Owns the text. At mission start it reads $profile:NimbyGreet\greet.json with
//           JsonFileLoader<T>.LoadFile (3_Game/DayZ/tools/JsonFileLoader.c:7): a missing
//           file gets the G3 default (temp-validate-copy-delete, Current state "File
//           writes"); a malformed file logs one "greet.json invalid: <error>" line and the
//           last good copy (.lkg) is served instead; with neither there is no text and every
//           request gets a bare done. Every successful parse refreshes the .lkg copy.
//           A request re-reads the file when the copy is older than rereadSeconds, so an
//           edit reaches the next joiner without a restart and a burst of joins costs one read.
//           Each request is rate-limited per identity (requestCooldownSeconds), answered
//           with the picked language's JSON in CHUNK_BYTES chunks and a done record, or
//           with a bare done when the client is dismissed (G4) or there is no text.
// Note    : NIMBY-T-042: the engine prints its own "SCRIPT (E): JSON ERROR" on a malformed
//           file no matter what this file does. scripts/Test-GreetText.ps1 is what keeps a
//           malformed file off the host; this code keeps players served meanwhile.
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
                    CopyFile(NimbyGreetRules.FILE_TEXT, NimbyGreetRules.FILE_TEXT_LKG);
                    NimbyGreetAdopt(created, "default");
                    return;
                }
            }
            NimbyGreetLoadLkg("missing");
            return;
        }
        NimbyGreetFile data = new NimbyGreetFile();
        string error;
        bool loaded = JsonFileLoader<NimbyGreetFile>.LoadFile(NimbyGreetRules.FILE_TEXT, data, error);
        if (!loaded)
        {
            NimbyGreetLog.Info("greet.json invalid: " + error);
            NimbyGreetLoadLkg("invalid");
            return;
        }
        CopyFile(NimbyGreetRules.FILE_TEXT, NimbyGreetRules.FILE_TEXT_LKG);
        NimbyGreetAdopt(data, "greet.json");
    }

    protected void NimbyGreetLoadLkg(string because)
    {
        if (m_NimbyGreetFile && m_NimbyGreetSource != "none")
        {
            NimbyGreetLog.Info("greet.json " + because + "; serving the copy loaded earlier this boot (source=" + m_NimbyGreetSource + " version=" + m_NimbyGreetFile.version.ToString() + ")");
            return;
        }
        if (!FileExist(NimbyGreetRules.FILE_TEXT_LKG))
        {
            NimbyGreetLog.Info("greet.json " + because + " and no greet.json.lkg; no text until the file is fixed");
            m_NimbyGreetFile = null;
            m_NimbyGreetSource = "none";
            m_NimbyGreetCodes.Clear();
            m_NimbyGreetSerialized.Clear();
            return;
        }
        NimbyGreetFile data = new NimbyGreetFile();
        string error;
        if (!JsonFileLoader<NimbyGreetFile>.LoadFile(NimbyGreetRules.FILE_TEXT_LKG, data, error))
        {
            NimbyGreetLog.Info("greet.json " + because + " and greet.json.lkg invalid: " + error + "; no text until the file is fixed");
            m_NimbyGreetFile = null;
            m_NimbyGreetSource = "none";
            m_NimbyGreetCodes.Clear();
            m_NimbyGreetSerialized.Clear();
            return;
        }
        NimbyGreetAdopt(data, "greet.json.lkg");
    }

    // Applies the caps and the language rules, then makes the file the one being served.
    protected void NimbyGreetAdopt(NimbyGreetFile data, string source)
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
        m_NimbyGreetSerialized.Clear();
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
            string json;
            JsonSerializer serializer = new JsonSerializer();
            serializer.WriteToString(language, false, json);
            if (json.Length() > NimbyGreetRules.MAX_PAYLOAD_BYTES)
            {
                NimbyGreetLog.Info("greet.json language " + code + " serializes to " + json.Length().ToString() + " bytes, above the payload cap; not served");
                continue;
            }
            m_NimbyGreetSerialized.Insert(code, json);
            totalBytes = totalBytes + json.Length();
        }
        m_NimbyGreetCodes.Clear();
        foreach (string served : codes)
        {
            if (m_NimbyGreetSerialized.Contains(served))
            {
                m_NimbyGreetCodes.Insert(served);
            }
        }
        if (m_NimbyGreetCodes.Count() == 0)
        {
            NimbyGreetLog.Info("greet.json has no servable language (source=" + source + "); no text until the file is fixed");
            m_NimbyGreetFile = null;
            m_NimbyGreetSource = "none";
            return;
        }
        if (m_NimbyGreetCodes.Find(data.defaultLanguage) < 0)
        {
            NimbyGreetLog.Info("greet.json defaultLanguage " + data.defaultLanguage + " is not served; using " + m_NimbyGreetCodes.Get(0));
            data.defaultLanguage = m_NimbyGreetCodes.Get(0);
        }
        m_NimbyGreetFile = data;
        m_NimbyGreetSource = source;
        string line = "greet.json loaded source=" + source + " version=" + data.version.ToString() + " forceShow=" + NimbyGreetLog.Bool(data.forceShow);
        line = line + " languages=" + string.Join(",", m_NimbyGreetCodes) + " default=" + data.defaultLanguage;
        line = line + " bytes=" + totalBytes.ToString() + " truncations=" + totalCuts.ToString();
        line = line + " openDelay=" + data.openDelaySeconds.ToString() + " cooldown=" + data.requestCooldownSeconds.ToString() + " reread=" + data.rereadSeconds.ToString();
        NimbyGreetLog.Info(line);
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
            NimbyGreetLog.Info("serve id=" + requestId.ToString() + " chunks=0 reason=no-text asked=" + language + " manual=" + NimbyGreetLog.Bool(manual) + " " + who);
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
            dismissedLine = dismissedLine + " dismissedVersion=" + dismissedVersion.ToString() + " asked=" + language + " " + who;
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
        string serveLine = "serve id=" + requestId.ToString() + " asked=" + language + " served=" + served + " version=" + m_NimbyGreetFile.version.ToString();
        serveLine = serveLine + " chunks=" + count.ToString() + " bytes=" + payload.Length().ToString() + " manual=" + NimbyGreetLog.Bool(manual) + " " + who;
        NimbyGreetLog.Info(serveLine);
    }
}
