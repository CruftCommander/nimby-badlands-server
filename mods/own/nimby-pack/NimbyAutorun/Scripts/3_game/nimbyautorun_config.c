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
    bool stopOnSwim = true;
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

    // "player=<name> (<bi-id>, <steam64>)", the form of the plan's logging table.
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
            return "player=unknown (unknown, unknown)";
        }
        return "player=" + name + " (" + biId + ", " + plainId + ")";
    }

    static void Info(string line)
    {
        Print(PREFIX + " " + line);
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
                    JsonFileLoader<NimbyAutorunConfig>.JsonSaveFile(PATH, loaded);
                    NimbyAutorunLog.Info("config upgraded; from=" + oldVersion.ToString() + " to=" + loaded.version.ToString() + " path=" + PATH);
                }
            }
        }
        else
        {
            MakeDirectory(FOLDER);
            JsonFileLoader<NimbyAutorunConfig>.JsonSaveFile(PATH, loaded);
            NimbyAutorunLog.Info("config missing; defaults written; path=" + PATH);
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
}
