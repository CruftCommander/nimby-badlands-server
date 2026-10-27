// -----------------------------------------------------------------------------
// NimbyGreet - the two client files (plan v0.4, G4, G17).
// Purpose : $profile:NimbyGreet\seen.json (the dismissal) and prefs.json (text scale,
//           animations, language override). Both are client-local and never leave the
//           client. A missing or malformed file means the defaults (never dismissed,
//           normal scale, animations on, detected language). Writes use the
//           temp-validate-copy-delete pattern (Current state "File writes": there is no
//           rename, EnSystem.c:525-531), so a crash mid-write leaves the old file intact.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyGreetClientFiles
{
    static NimbyGreetSeen LoadSeen()
    {
        NimbyGreetSeen seen = new NimbyGreetSeen();
        if (!FileExist(NimbyGreetRules.FILE_SEEN))
        {
            return seen;
        }
        string error;
        if (!JsonFileLoader<NimbyGreetSeen>.LoadFile(NimbyGreetRules.FILE_SEEN, seen, error))
        {
            NimbyGreetLog.Info("seen.json invalid, treated as never dismissed: " + error);
            return new NimbyGreetSeen();
        }
        return seen;
    }

    static bool SaveSeen(NimbyGreetSeen seen)
    {
        EnsureDir();
        string error;
        if (!JsonFileLoader<NimbyGreetSeen>.SaveFile(NimbyGreetRules.FILE_SEEN_TMP, seen, error))
        {
            NimbyGreetLog.Info("seen.json write failed: " + error);
            return false;
        }
        NimbyGreetSeen check = new NimbyGreetSeen();
        if (!JsonFileLoader<NimbyGreetSeen>.LoadFile(NimbyGreetRules.FILE_SEEN_TMP, check, error))
        {
            NimbyGreetLog.Info("seen.json verify failed: " + error);
            DeleteFile(NimbyGreetRules.FILE_SEEN_TMP);
            return false;
        }
        bool copied = CopyFile(NimbyGreetRules.FILE_SEEN_TMP, NimbyGreetRules.FILE_SEEN);
        DeleteFile(NimbyGreetRules.FILE_SEEN_TMP);
        if (!copied)
        {
            NimbyGreetLog.Info("seen.json copy failed");
        }
        return copied;
    }

    static NimbyGreetPrefs LoadPrefs()
    {
        NimbyGreetPrefs prefs = new NimbyGreetPrefs();
        if (!FileExist(NimbyGreetRules.FILE_PREFS))
        {
            return prefs;
        }
        string error;
        if (!JsonFileLoader<NimbyGreetPrefs>.LoadFile(NimbyGreetRules.FILE_PREFS, prefs, error))
        {
            NimbyGreetLog.Info("prefs.json invalid, defaults used: " + error);
            return new NimbyGreetPrefs();
        }
        prefs.textScale = NormalizeScale(prefs.textScale);
        if (prefs.language.Length() > 0 && !NimbyGreetRules.IsKnownLanguage(prefs.language))
        {
            prefs.language = "";
        }
        return prefs;
    }

    static bool SavePrefs(NimbyGreetPrefs prefs)
    {
        EnsureDir();
        string error;
        if (!JsonFileLoader<NimbyGreetPrefs>.SaveFile(NimbyGreetRules.FILE_PREFS_TMP, prefs, error))
        {
            NimbyGreetLog.Info("prefs.json write failed: " + error);
            return false;
        }
        NimbyGreetPrefs check = new NimbyGreetPrefs();
        if (!JsonFileLoader<NimbyGreetPrefs>.LoadFile(NimbyGreetRules.FILE_PREFS_TMP, check, error))
        {
            NimbyGreetLog.Info("prefs.json verify failed: " + error);
            DeleteFile(NimbyGreetRules.FILE_PREFS_TMP);
            return false;
        }
        bool copied = CopyFile(NimbyGreetRules.FILE_PREFS_TMP, NimbyGreetRules.FILE_PREFS);
        DeleteFile(NimbyGreetRules.FILE_PREFS_TMP);
        if (!copied)
        {
            NimbyGreetLog.Info("prefs.json copy failed");
        }
        return copied;
    }

    // Snaps a stored scale to one of the three G16 steps.
    static float NormalizeScale(float scale)
    {
        if (scale >= 1.4)
        {
            return NimbyGreetRules.TEXT_SCALE_LARGER;
        }
        if (scale >= 1.15)
        {
            return NimbyGreetRules.TEXT_SCALE_LARGE;
        }
        return NimbyGreetRules.TEXT_SCALE_NORMAL;
    }

    protected static void EnsureDir()
    {
        if (!FileExist(NimbyGreetRules.DIR))
        {
            MakeDirectory(NimbyGreetRules.DIR);
        }
    }
}
