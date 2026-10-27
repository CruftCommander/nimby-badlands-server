// -----------------------------------------------------------------------------
// NimbyAutorun - Legs. Server configuration and logging.
// Purpose : The tunables of Legs, the per-sender request budget and the swim rule
//           (stopOnSwim), read once at mission start from
//           $profile:NimbyAutorun\config.json on the server.
//           The file is written with defaults when it does not exist, so the
//           operator edits a real file.
// Safety  : The file controls nothing about access or admission, so a plain
//           write of the defaults is enough (shared rules 6; the atomic-write
//           clause does not apply). NimbyAutorunRules.Sanitize clamps the value,
//           so a bad edit degrades to a bound, never to a crash.
// Logging : [NIMBY][Autorun] prefix (shared rules 4.1). Debug lines only when
//           $profile:NimbyAutorun\debug exists (shared rules 4.3). Every line that
//           names a player carries GetId() and GetPlainId() (shared rules 4.2).
// -----------------------------------------------------------------------------
class NimbyAutorunConfig
{
    // 2 since round 3 (stopOnSwim). A version 1 file is rewritten once with the new key.
    int version = 2;
    // Phase 1: requests a sender may make in one second before the rest are dropped. An
    // authenticated stop is never counted or dropped.
    int requestsPerSecond = 10;
    // Round 3, NIMBY-T-006 experiment: true stops autorun when the swim command starts and
    // refuses a start while swimming (plan v0.6 behavior); false leaves the override applied in
    // water. The server sends it to the owning client with RPC 6550103 on every start.
    // Default false since October 19, 2026 (Jeff, after the R4 dam run proved the override drives
    // the swim command): autorun carries into water. Servers that want the old rule set true.
    bool stopOnSwim = false;
}

class NimbyAutorunLog
{
    static const string PREFIX = "[NIMBY][Autorun]";
    protected static bool s_Debug = false;

    static void ReadDebugFlag()
    {
        s_Debug = FileExist("$profile:NimbyAutorun\\debug");
    }

    static bool IsDebug()
    {
        return s_Debug;
    }

    static string Bool(bool value)
    {
        if (value)
        {
            return "true";
        }
        return "false";
    }

    // "<bi-id>, <steam64>". GetId() is the BI hash the .ADM log uses; GetPlainId() is the
    // Steam64 (gameplay.c:362-370; the in-game run proves the second).
    static string Ids(PlayerIdentity identity)
    {
        if (!identity)
        {
            return "unknown, unknown";
        }
        return identity.GetId() + ", " + identity.GetPlainId();
    }

    // "player=(<bi-id>, <steam64>) "<name>"": the ids first, the launcher name last, quoted and
    // escaped. The name is player-chosen and may contain parentheses, commas and quotes; before
    // October 27, 2026 it came first and unquoted, so a name like "A (B, 76561198000000000)"
    // forged a second id pair in front of the real one (review LEGS-R2-02, prior item 15).
    static string Who(PlayerIdentity identity)
    {
        if (!identity)
        {
            return WhoFrom("", "", "");
        }
        return WhoFrom(identity.GetName(), identity.GetId(), identity.GetPlainId());
    }

    // Who() from strings, for a line written after the identity is gone (NIMBY-T-005): the
    // player caches its name and both ids while the identity is attached. An empty BI id means
    // nothing was cached.
    static string WhoFrom(string name, string biId, string plainId)
    {
        if (biId == "")
        {
            return "player=(unknown, unknown) \"\"";
        }
        return "player=(" + biId + ", " + plainId + ") \"" + Escape(name) + "\"";
    }

    // A quoted field: backslash and double quote escaped, line breaks replaced, so the field
    // ends at the first unescaped quote whatever the player typed.
    static string Escape(string text)
    {
        string result = text;
        result.Replace("\\", "\\\\");
        result.Replace("\"", "\\\"");
        result.Replace("\n", " ");
        result.Replace("\r", " ");
        return result;
    }

    static void Info(string line)
    {
        Print(PREFIX + " " + line);
    }

    // One gate per sender id, for lines a stranger can trigger on another player's object: the
    // ownership refusal used to go through the target's gate and let one forged request per
    // window silence the target's own refusal lines (review LEGS-R2-03, prior item 18).
    protected static ref map<string, ref NimbyAutorunLogGate> s_SenderGates;

    static bool AllowForSender(string key, float now)
    {
        if (!s_SenderGates)
        {
            s_SenderGates = new map<string, ref NimbyAutorunLogGate>();
        }
        NimbyAutorunLogGate gate = s_SenderGates.Get(key);
        if (!gate)
        {
            gate = new NimbyAutorunLogGate();
            s_SenderGates.Set(key, gate);
        }
        return gate.Allow(now, NimbyAutorunRules.LOG_INTERVAL_SECONDS);
    }

    // Same prefix as Info, so the plan's "Toggle player=..." line reads exactly as specified.
    static void Debug(string line)
    {
        if (!s_Debug)
        {
            return;
        }
        Print(PREFIX + " " + line);
    }
}

class NimbyAutorunConfigLoader
{
    static const string FOLDER = "$profile:NimbyAutorun";
    static const string PATH = "$profile:NimbyAutorun\\config.json";
    protected static ref NimbyAutorunConfig s_Config;

    // The active configuration. On the server this is the sanitized file; before Load ran it is
    // the defaults. The client never reads it.
    static NimbyAutorunConfig Get()
    {
        if (!s_Config)
        {
            s_Config = new NimbyAutorunConfig();
        }
        return s_Config;
    }

    static void Load()
    {
        NimbyAutorunConfig loaded = new NimbyAutorunConfig();
        if (FileExist(PATH))
        {
            // LoadFile reports a parse failure (JsonFileLoader.c:7); the operator's file is left
            // untouched so the typo can be fixed, and the defaults run until the next restart.
            string error;
            if (!JsonFileLoader<NimbyAutorunConfig>.LoadFile(PATH, loaded, error))
            {
                loaded = new NimbyAutorunConfig();
                NimbyAutorunLog.Info("config invalid; defaults in use; path=" + PATH + " error=" + error);
            }
            else
            {
                NimbyAutorunLog.Info("config loaded; path=" + PATH);
                // A file from an older build keeps its values and gains the new keys at their
                // defaults (absent keys keep the class default on load), so the operator edits a
                // file that names every key.
                if (loaded.version < NimbyAutorunRules.CONFIG_VERSION)
                {
                    int oldVersion = loaded.version;
                    loaded.version = NimbyAutorunRules.CONFIG_VERSION;
                    if (Save(loaded))
                    {
                        NimbyAutorunLog.Info("config upgraded; from=" + oldVersion.ToString() + " to=" + loaded.version.ToString() + " path=" + PATH);
                    }
                }
            }
        }
        else
        {
            MakeDirectory(FOLDER);
            if (Save(loaded))
            {
                NimbyAutorunLog.Info("config missing; defaults written; path=" + PATH);
            }
        }
        int clamped = NimbyAutorunRules.Sanitize(loaded);
        if (clamped > 0)
        {
            NimbyAutorunLog.Info("config clamped; fields=" + clamped.ToString());
        }
        s_Config = loaded;
        // Always on since round 3: the line shows which way the swim experiment runs (R3-6).
        NimbyAutorunLog.Info("config requestsPerSecond=" + loaded.requestsPerSecond.ToString() + " stopOnSwim=" + NimbyAutorunLog.Bool(loaded.stopOnSwim));
    }

    // SaveFile reports a failed open or serialization (JsonFileLoader.c:42-60); the deprecated
    // void JsonSaveFile swallowed both and the loader then logged a success that never happened
    // (review LEGS-R2-06). The file is not written atomically: it controls no admission, so a
    // half-written file costs one "config invalid; defaults in use" at the next boot.
    protected static bool Save(NimbyAutorunConfig config)
    {
        string error;
        if (JsonFileLoader<NimbyAutorunConfig>.SaveFile(PATH, config, error))
        {
            return true;
        }
        NimbyAutorunLog.Info("config write failed; path=" + PATH + " error=" + error);
        return false;
    }
}
