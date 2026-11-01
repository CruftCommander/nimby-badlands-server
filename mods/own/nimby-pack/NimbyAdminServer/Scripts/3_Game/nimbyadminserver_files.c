// -----------------------------------------------------------------------------
// NimbyAdminServer - the profile stores (Plan 3 v0.8 N8, recon 5, rulings 3 to 5).
// Purpose : config.json, permissions.json and bans.json, each read as text first (the
//           JsonFileLoader.c:11-21 steps) and parsed with JsonFileLoader<T>.LoadData
//           (3_Game/DayZ/tools/JsonFileLoader.c:69). An unchanged good text is not parsed again
//           and an unchanged bad text is not parsed again (NIMBY-T-056), so a bad file costs one
//           engine "JSON ERROR" line per distinct text. Every adopted parse refreshes the .lkg
//           copy from the text just parsed. A corrupt file falls back to the set already in
//           memory, then to .lkg, then to nothing; any fallback refuses edits until a clean
//           re-read (N8). Writes are copy-based (recon 5.3, R4): there is no rename in 1.30.
// Engine  : FileExist, OpenFile, ReadFile, FPrint, CloseFile, MakeDirectory, DeleteFile, CopyFile
//           (1_Core/DayZ/proto/EnSystem.c:397,417,425,462,443,525,528,531); JsonFileLoader.SaveFile
//           (:42) and LoadFile (:7).
// Never   : logs the webhook URL or any file content.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyAdminServerFiles
{
    static bool ReadText(string path, out string text)
    {
        text = "";
        if (!FileExist(path))
        {
            return false;
        }
        FileHandle handle = OpenFile(path, FileMode.READ);
        if (handle == 0)
        {
            return false;
        }
        ReadFile(handle, text, 1000000);
        CloseFile(handle);
        return true;
    }

    // tmp, then CopyFile over target, then delete tmp; the caller validated text already.
    static bool WriteText(string path, string text)
    {
        string tmp = path + ".tmp";
        FileHandle handle = OpenFile(tmp, FileMode.WRITE);
        if (handle == 0)
        {
            return false;
        }
        FPrint(handle, text);
        CloseFile(handle);
        string check;
        if (!ReadText(tmp, check) || check != text)
        {
            DeleteFile(tmp);
            return false;
        }
        bool copied = CopyFile(tmp, path);
        DeleteFile(tmp);
        return copied;
    }

    static void EnsureDirs()
    {
        if (!FileExist(NimbyAdminServerRules.DIR))
        {
            MakeDirectory(NimbyAdminServerRules.DIR);
        }
        if (!FileExist(NimbyAdminServerRules.DIR_AUDIT))
        {
            MakeDirectory(NimbyAdminServerRules.DIR_AUDIT);
        }
    }

    static NimbyAdminServerConfig LoadConfig()
    {
        NimbyAdminServerConfig config = new NimbyAdminServerConfig();
        string error;
        if (!FileExist(NimbyAdminServerRules.FILE_CONFIG))
        {
            if (JsonFileLoader<NimbyAdminServerConfig>.SaveFile(NimbyAdminServerRules.FILE_CONFIG, config, error))
            {
                NimbyAdminLog.Info("config.json missing; defaults written (no bootstrap owner, webhook unset)");
            }
            else
            {
                NimbyAdminLog.Info("config.json missing and could not be written; defaults used");
            }
            return config;
        }
        if (!JsonFileLoader<NimbyAdminServerConfig>.LoadFile(NimbyAdminServerRules.FILE_CONFIG, config, error))
        {
            NimbyAdminLog.Info("config.json invalid; defaults used with no bootstrap owner and no webhook");
            return new NimbyAdminServerConfig();
        }
        array<string> notes;
        config.Normalize(notes);
        foreach (string note : notes)
        {
            NimbyAdminLog.Info("config.json: " + note);
        }
        return config;
    }
}

// The common shape of the two access stores (N8).
class NimbyAdminServerStore
{
    string m_Name;
    string m_Path;
    string m_LkgPath;
    string m_Source = "none";
    bool m_EditsRefused;
    float m_LoadedAt = -1000.0;
    ref map<string, string> m_Active = new map<string, string>;
    protected string m_LastGoodText = "";
    protected string m_LastBadText = "";
    protected bool m_HaveGood;
    protected bool m_HaveBad;

    // Parse text into an active set; overridden per store.
    protected bool ParseText(string text, out map<string, string> active, out array<string> notes, out string error)
    {
        active = new map<string, string>;
        notes = new array<string>;
        error = "not implemented";
        return false;
    }

    // Reads the file and adopts it, or falls back (N8). Returns true when the active set changed.
    // problem is set to a one-line description when a fallback happened this time.
    bool Load(float now, out string problem)
    {
        problem = "";
        m_LoadedAt = now;
        string text;
        bool exists = NimbyAdminServerFiles.ReadText(m_Path, text);
        if (!exists)
        {
            return false;
        }
        if (m_HaveGood && text == m_LastGoodText)
        {
            return false;
        }
        if (m_HaveBad && text == m_LastBadText)
        {
            return false;
        }
        map<string, string> active;
        array<string> notes;
        string error;
        bool fileOk = ParseText(text, active, notes, error);
        if (fileOk)
        {
            foreach (string note : notes)
            {
                NimbyAdminLog.Info(m_Name + ": " + note);
            }
            bool changed = !SameSet(active, m_Active);
            m_Active = active;
            m_LastGoodText = text;
            m_HaveGood = true;
            m_HaveBad = false;
            m_Source = NimbyAdminServerSource.FILE;
            m_EditsRefused = false;
            if (!NimbyAdminServerFiles.WriteText(m_LkgPath, text))
            {
                NimbyAdminLog.Info(m_Name + ".lkg not refreshed: write failed");
            }
            NimbyAdminLog.Info(m_Name + " loaded entries=" + m_Active.Count().ToString() + " source=file");
            return changed;
        }
        m_LastBadText = text;
        m_HaveBad = true;
        bool lkgOk = false;
        map<string, string> lkgActive;
        if (!m_HaveGood)
        {
            string lkgText;
            if (NimbyAdminServerFiles.ReadText(m_LkgPath, lkgText))
            {
                array<string> lkgNotes;
                string lkgError;
                lkgOk = ParseText(lkgText, lkgActive, lkgNotes, lkgError);
            }
        }
        string source = NimbyAdminServerSource.Decide(false, m_HaveGood, lkgOk);
        bool before = m_Active.Count() > 0;
        if (source == NimbyAdminServerSource.LKG)
        {
            m_Active = lkgActive;
        }
        else if (source == NimbyAdminServerSource.NONE)
        {
            m_Active = new map<string, string>;
        }
        m_Source = source;
        m_EditsRefused = NimbyAdminServerSource.EditsRefused(source);
        problem = m_Name + " invalid (" + NimbyAdminRules.CutBytes(error, 200) + "); using " + source + "; edits refused until repaired";
        NimbyAdminLog.Info(problem);
        return before || m_Active.Count() > 0;
    }

    bool Exists()
    {
        return FileExist(m_Path);
    }

    static bool SameSet(map<string, string> a, map<string, string> b)
    {
        if (a.Count() != b.Count())
        {
            return false;
        }
        foreach (string key, string value : a)
        {
            string other;
            if (!b.Find(key, other) || other != value)
            {
                return false;
            }
        }
        return true;
    }
}

class NimbyAdminServerPermissionStore extends NimbyAdminServerStore
{
    string m_Bootstrap = "";
    bool m_BootstrapActive;

    void NimbyAdminServerPermissionStore()
    {
        m_Name = "permissions.json";
        m_Path = NimbyAdminServerRules.FILE_PERMISSIONS;
        m_LkgPath = NimbyAdminServerRules.FILE_PERMISSIONS_LKG;
    }

    override protected bool ParseText(string text, out map<string, string> active, out array<string> notes, out string error)
    {
        NimbyAdminServerPermissions data = new NimbyAdminServerPermissions();
        error = "";
        if (!JsonFileLoader<NimbyAdminServerPermissions>.LoadData(text, data, error))
        {
            active = new map<string, string>;
            notes = new array<string>;
            return false;
        }
        if (!data.Build(active, notes))
        {
            error = "version";
            return false;
        }
        return true;
    }

    // The role of steam64 now; "none" for anyone not in the active set or the bootstrap overlay.
    string RoleOf(string steam64)
    {
        string role;
        if (m_Active.Find(steam64, role))
        {
            return role;
        }
        if (m_BootstrapActive && steam64 == m_Bootstrap)
        {
            return NimbyAdminRules.ROLE_OWNER;
        }
        return NimbyAdminRules.ROLE_NONE;
    }

    // Plan, config.json table: the bootstrap owner is an owner while the active set has none.
    void ApplyBootstrap()
    {
        m_BootstrapActive = m_Bootstrap != "" && !NimbyAdminServerPermissions.HasOwner(m_Active);
    }

    // First boot only (recon 5.4): the file does not exist and a bootstrap owner is configured.
    // Written through the copy-based write; never over an existing file; refused in N8 state.
    bool WriteBootstrapFile(out string why)
    {
        why = "";
        if (Exists())
        {
            why = "permissions.json exists";
            return false;
        }
        if (m_EditsRefused)
        {
            why = "permissions.json needs repair";
            return false;
        }
        NimbyAdminServerPermissions data = new NimbyAdminServerPermissions();
        NimbyAdminServerAdmin owner = new NimbyAdminServerAdmin();
        owner.steam64 = m_Bootstrap;
        owner.name = "bootstrap owner";
        owner.role = NimbyAdminRules.ROLE_OWNER;
        data.admins.Insert(owner);
        string text;
        string error;
        if (!JsonFileLoader<NimbyAdminServerPermissions>.MakeData(data, text, error, true))
        {
            why = "serialize failed";
            return false;
        }
        if (!NimbyAdminServerFiles.WriteText(m_Path, text))
        {
            why = "write failed";
            return false;
        }
        return true;
    }
}

class NimbyAdminServerBanStore extends NimbyAdminServerStore
{
    void NimbyAdminServerBanStore()
    {
        m_Name = "bans.json";
        m_Path = NimbyAdminServerRules.FILE_BANS;
        m_LkgPath = NimbyAdminServerRules.FILE_BANS_LKG;
    }

    override protected bool ParseText(string text, out map<string, string> active, out array<string> notes, out string error)
    {
        NimbyAdminServerBans data = new NimbyAdminServerBans();
        error = "";
        if (!JsonFileLoader<NimbyAdminServerBans>.LoadData(text, data, error))
        {
            active = new map<string, string>;
            notes = new array<string>;
            return false;
        }
        if (!data.Build(active, notes))
        {
            error = "version";
            return false;
        }
        return true;
    }

    // Ruling 3: no usable ban list means the file exists but neither it nor .lkg loaded.
    bool NoUsableList()
    {
        return Exists() && m_Source == NimbyAdminServerSource.NONE;
    }
}
