// -----------------------------------------------------------------------------
// NimbyTack - server configuration (T6).
// Purpose : The numeric tunables of Tack, read once at mission start from
//           $profile:NimbyTack\config.json on the server. The file is written with
//           defaults when it does not exist, so the operator edits a real file.
//           Class lists (hammer substitutes, knives) are code constants in
//           NimbyTackRules, not config: the client evaluates the same action
//           conditions and has no server profile folder (T8).
// Safety  : The file controls nothing about access or admission, so a plain
//           write of the defaults is enough (shared rules 6, atomic-write clause
//           does not apply). Values are clamped by NimbyTackRules.Sanitize so a
//           bad edit degrades to a bound, never to a crash.
// Logging : [NIMBY][Tack] prefix (shared rules 4.1); debug lines only when
//           $profile:NimbyTack\debug exists (shared rules 4.3).
// -----------------------------------------------------------------------------
class NimbyTackConfig
{
    // 2 since October 20, 2026 (roughSurfaceOffsetMeters). A file written at version 1 is
    // upgraded once on load and written back, see NimbyTackRules.UpgradeConfig.
    int version = 2;
    // T2: a wall hit by the raycast within this many meters of the player's head.
    float reachMeters = 1.5;
    // T2: the surface normal is within this many degrees of horizontal.
    float wallAngleDegrees = 25.0;
    // T3: how far off the wall the sheet is created, along the outward normal. This initializer
    // is the only place the default lives (self-test and client read it through a fresh
    // instance). The October 17, 2026 clipping was not relief depth: the offset was applied along
    // the ray, into the wall (see NimbyTackWorld). Re-measured with the physics normal on
    // October 18, 2026; moves only once a value is confirmed in game.
    float wallOffsetMeters = 0.03;
    // T3: the offset used instead when the wall reports no surface material from either
    // lookup (view and fire geometry). NIMBY-T-008, October 19, 2026: Land_TradTent_Small reports
    // none, and its physics shell sits 3 to 5 cm behind the visible cloth, so 1 cm from the view
    // hit showed through to the inside. Jeff ruled tents stay pinnable with a larger offset.
    float unknownSurfaceOffsetMeters = 0.06;
    // T3: the offset used instead on a rough material (NimbyTackRules.IsRoughSurface, a code list
    // starting with Takistan mud brick). NIMBY-T-013, October 20, 2026: the stones stand 3 to 11 cm
    // proud of the point the sheet is placed from, so a taped note at 3 cm clipped into them. Jeff
    // ruled a per-material offset (option (a) of the T-013 study); every other surface keeps 3 cm.
    float roughSurfaceOffsetMeters = 0.06;
    // T4: health taken from the sheet per take-down, by fastener. Paper has 15 hitpoints.
    float takeDownDamageNail = 5.0;
    float takeDownDamageTape = 3.0;
    float takeDownDamageEpoxy = 7.0;
    float takeDownDamageKnife = 5.0;
    // T2: quantity consumed from the roll or tube per pin (both are 0-100 items).
    float tapeQuantityPerPin = 10.0;
    float epoxyQuantityPerPin = 10.0;
    // T6: at most this many pinned notes within the radius; 0 disables the cap.
    int areaCapCount = 20;
    float areaCapRadiusMeters = 50.0;
}

class NimbyTackLog
{
    static const string PREFIX = "[NIMBY][Tack]";
    protected static bool s_Debug = false;

    static void ReadDebugFlag()
    {
        s_Debug = FileExist("$profile:NimbyTack\\debug");
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

    static void Info(string line)
    {
        Print(PREFIX + " " + line);
    }

    static void Debug(string line)
    {
        if (!s_Debug)
        {
            return;
        }
        Print(PREFIX + "[Debug] " + line);
    }
}

class NimbyTackConfigLoader
{
    static const string FOLDER = "$profile:NimbyTack";
    static const string PATH = "$profile:NimbyTack\\config.json";
    protected static ref NimbyTackConfig s_Config;

    // The active configuration. On the server this is the sanitized file; on a client, or
    // before Load ran, it is the defaults, which is what the client-side action conditions use.
    static NimbyTackConfig Get()
    {
        if (!s_Config)
        {
            s_Config = new NimbyTackConfig();
        }
        return s_Config;
    }

    static void Load()
    {
        NimbyTackConfig loaded = new NimbyTackConfig();
        if (FileExist(PATH))
        {
            // LoadFile reports a parse failure; the legacy JsonLoadFile swallowed it and the
            // server ran on defaults while logging "config loaded". The operator's file is left
            // untouched so the typo can be fixed; the defaults run until the next restart.
            string error;
            if (!JsonFileLoader<NimbyTackConfig>.LoadFile(PATH, loaded, error))
            {
                loaded = new NimbyTackConfig();
                NimbyTackLog.Info("config invalid; defaults in use; path=" + PATH + " error=" + error);
            }
            else
            {
                NimbyTackLog.Info("config loaded; path=" + PATH);
                // A file from an older version gains the keys added since, once, and is written
                // back so the operator sees them. Out-of-range values are written as the operator
                // left them; Sanitize below clamps only the running copy.
                int fromVersion = loaded.version;
                if (NimbyTackRules.UpgradeConfig(loaded, new NimbyTackConfig()))
                {
                    JsonFileLoader<NimbyTackConfig>.JsonSaveFile(PATH, loaded);
                    NimbyTackLog.Info("config upgraded; from=" + fromVersion.ToString() + " to=" + loaded.version.ToString() + " path=" + PATH);
                }
            }
        }
        else
        {
            MakeDirectory(FOLDER);
            JsonFileLoader<NimbyTackConfig>.JsonSaveFile(PATH, loaded);
            NimbyTackLog.Info("config missing; defaults written; path=" + PATH);
        }
        int clamped = NimbyTackRules.Sanitize(loaded);
        if (clamped > 0)
        {
            NimbyTackLog.Info("config clamped; fields=" + clamped.ToString());
        }
        s_Config = loaded;
        string line = "config reach=" + loaded.reachMeters.ToString() + " angle=" + loaded.wallAngleDegrees.ToString() + " offset=" + loaded.wallOffsetMeters.ToString();
        line = line + " unknown-offset=" + loaded.unknownSurfaceOffsetMeters.ToString() + " rough-offset=" + loaded.roughSurfaceOffsetMeters.ToString();
        line = line + " cap=" + loaded.areaCapCount.ToString() + "/" + loaded.areaCapRadiusMeters.ToString() + "m version=" + loaded.version.ToString();
        NimbyTackLog.Debug(line);
    }
}
