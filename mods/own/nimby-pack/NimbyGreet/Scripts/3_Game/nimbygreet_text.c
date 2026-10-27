// -----------------------------------------------------------------------------
// NimbyGreet - the JSON shapes (plan v0.4, Data format) and the cap pass (G9).
// Purpose : The classes JsonFileLoader and JsonSerializer read and write: greet.json on
//           the server, the per-language payload in the chunks, seen.json and prefs.json
//           on the client, and Media/manifest.json in the PBO. Field names are the file
//           format; a rename here is a format change.
// Shape   : "languages" is an array of Language, each carrying its "language" code, not an
//           object keyed by code: no vanilla JSON class carries a map (grep of 3_Game,
//           October 26, 2026), so the serializer's map support is unproven and the array
//           is the safe shape. The server builds the code to Language lookup after load.
// Caps    : ApplyCaps truncates, never rejects (G9), and reports every cut so the server
//           logs "greet.json truncated" once per load and the validator prints the same.
// Body    : NIMBY-T-055: "body" is an array of lines, never one string, because the engine's
//           JSON reader keeps at most 1,023 bytes of a string value (ENGINE_STRING_BYTES) on
//           the server's load and again on the client's read of the payload. BodyText joins
//           the lines with "\n" for the converter and the caps; SetBodyText splits them back.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyGreetSection
{
    string id = "";
    string heading = "";
    ref array<string> body = new array<string>;   // lines, joined with "\n" (NIMBY-T-055)
}

class NimbyGreetLanguage
{
    string language = "";
    string title = "";
    string tagline = "";
    ref array<ref NimbyGreetSection> sections = new array<ref NimbyGreetSection>;
}

class NimbyGreetFile
{
    int version = 1;
    bool forceShow = false;
    string defaultLanguage = "english";
    ref array<ref NimbyGreetLanguage> languages = new array<ref NimbyGreetLanguage>;
    float openDelaySeconds = 1.0;
    float openRetrySeconds = 30.0;
    float requestCooldownSeconds = 5.0;
    float rereadSeconds = 10.0;
}

class NimbyGreetSeen
{
    int dismissedVersion = 0;
    bool neverShow = false;
}

class NimbyGreetPrefs
{
    float textScale = 1.0;
    bool playAnimations = true;
    string language = "";
}

class NimbyGreetMediaEntry
{
    string id = "";
    string kind = "still";   // still | clip
    int width = 0;
    int height = 0;
    int frames = 1;
    int frameMs = 0;
    string source = "";
    string license = "";
}

class NimbyGreetManifest
{
    ref array<ref NimbyGreetMediaEntry> media = new array<ref NimbyGreetMediaEntry>;
}

class NimbyGreetText
{
    // Applies the G9 caps to one language in place. Returns the number of cuts; notes gets one line per cut.
    static int ApplyCaps(NimbyGreetLanguage language, out array<string> notes)
    {
        notes = new array<string>;
        int cuts = 0;
        if (!language)
        {
            return 0;
        }
        if (language.title.LengthUtf8() > NimbyGreetRules.MAX_TITLE_CHARS)
        {
            language.title = NimbyGreetRules.CapText(language.title, NimbyGreetRules.MAX_TITLE_CHARS);
            notes.Insert("title cut to " + NimbyGreetRules.MAX_TITLE_CHARS.ToString());
            cuts = cuts + 1;
        }
        if (language.tagline.LengthUtf8() > NimbyGreetRules.MAX_TAGLINE_CHARS)
        {
            language.tagline = NimbyGreetRules.CapText(language.tagline, NimbyGreetRules.MAX_TAGLINE_CHARS);
            notes.Insert("tagline cut to " + NimbyGreetRules.MAX_TAGLINE_CHARS.ToString());
            cuts = cuts + 1;
        }
        if (!language.sections)
        {
            language.sections = new array<ref NimbyGreetSection>;
        }
        int removed = 0;
        while (language.sections.Count() > NimbyGreetRules.MAX_SECTIONS)
        {
            language.sections.Remove(language.sections.Count() - 1);
            removed = removed + 1;
        }
        if (removed > 0)
        {
            notes.Insert(removed.ToString() + " section(s) beyond " + NimbyGreetRules.MAX_SECTIONS.ToString() + " dropped");
            cuts = cuts + 1;
        }
        int total = 0;
        int i = 0;
        while (i < language.sections.Count())
        {
            NimbyGreetSection section = language.sections.Get(i);
            if (!section)
            {
                language.sections.Remove(i);
                notes.Insert("empty section entry removed");
                cuts = cuts + 1;
                continue;
            }
            if (!NimbyGreetRules.IsValidId(section.id))
            {
                section.id = "section-" + (i + 1).ToString();
                notes.Insert("section " + (i + 1).ToString() + " id replaced with " + section.id);
                cuts = cuts + 1;
            }
            if (section.heading.LengthUtf8() > NimbyGreetRules.MAX_HEADING_CHARS)
            {
                section.heading = NimbyGreetRules.CapText(section.heading, NimbyGreetRules.MAX_HEADING_CHARS);
                notes.Insert("section " + section.id + " heading cut to " + NimbyGreetRules.MAX_HEADING_CHARS.ToString());
                cuts = cuts + 1;
            }
            if (!section.body)
            {
                section.body = new array<string>;
            }
            int lineIndex = 0;
            while (lineIndex < section.body.Count())
            {
                int lineBytes = section.body.Get(lineIndex).Length();
                if (lineBytes >= NimbyGreetRules.ENGINE_STRING_WARN_BYTES)
                {
                    notes.Insert("section " + section.id + " line " + (lineIndex + 1).ToString() + " is " + lineBytes.ToString() + " bytes, at the engine's " + NimbyGreetRules.ENGINE_STRING_BYTES.ToString() + "-byte string limit; it may have been cut on load");
                    cuts = cuts + 1;
                }
                lineIndex = lineIndex + 1;
            }
            string bodyText = BodyText(section);
            int allowed = NimbyGreetRules.MAX_BODY_CHARS;
            int remaining = NimbyGreetRules.MAX_TOTAL_CHARS - total;
            if (remaining < allowed)
            {
                allowed = remaining;
            }
            if (allowed < 0)
            {
                allowed = 0;
            }
            if (bodyText.LengthUtf8() > allowed)
            {
                bodyText = NimbyGreetRules.CapText(bodyText, allowed);
                SetBodyText(section, bodyText);
                notes.Insert("section " + section.id + " body cut to " + allowed.ToString());
                cuts = cuts + 1;
            }
            total = total + bodyText.LengthUtf8();
            i = i + 1;
        }
        return cuts;
    }

    // The language codes a file carries, in file order; unknown and duplicate codes are skipped and noted.
    static void LanguageCodes(NimbyGreetFile file, out array<string> codes, out array<string> notes)
    {
        codes = new array<string>;
        notes = new array<string>;
        if (!file)
        {
            return;
        }
        if (!file.languages)
        {
            return;
        }
        foreach (NimbyGreetLanguage language : file.languages)
        {
            if (!language)
            {
                notes.Insert("empty language entry skipped");
                continue;
            }
            if (!NimbyGreetRules.IsKnownLanguage(language.language))
            {
                notes.Insert("unknown language key skipped: " + language.language);
                continue;
            }
            if (NimbyGreetRules.IsCjkLanguage(language.language))
            {
                notes.Insert("CJK language skipped (no font proven, plan G14): " + language.language);
                continue;
            }
            if (codes.Find(language.language) >= 0)
            {
                notes.Insert("duplicate language key skipped: " + language.language);
                continue;
            }
            codes.Insert(language.language);
        }
    }

    // The Language entry for a code, or null.
    static NimbyGreetLanguage FindLanguage(NimbyGreetFile file, string code)
    {
        if (!file)
        {
            return null;
        }
        if (!file.languages)
        {
            return null;
        }
        foreach (NimbyGreetLanguage language : file.languages)
        {
            if (!language)
            {
                continue;
            }
            if (language.language == code)
            {
                return language;
            }
        }
        return null;
    }

    // Clamps the timing fields a server operator may mistype (G5).
    static void ClampTiming(NimbyGreetFile file)
    {
        if (!file)
        {
            return;
        }
        file.openDelaySeconds = NimbyGreetRules.ClampFloat(file.openDelaySeconds, NimbyGreetRules.MIN_OPEN_DELAY_SECONDS, NimbyGreetRules.MAX_OPEN_DELAY_SECONDS);
        file.openRetrySeconds = NimbyGreetRules.ClampFloat(file.openRetrySeconds, 0.0, NimbyGreetRules.MAX_OPEN_RETRY_SECONDS);
        file.requestCooldownSeconds = NimbyGreetRules.ClampFloat(file.requestCooldownSeconds, NimbyGreetRules.MIN_COOLDOWN_SECONDS, 60.0);
        file.rereadSeconds = NimbyGreetRules.ClampFloat(file.rereadSeconds, NimbyGreetRules.MIN_REREAD_SECONDS, 3600.0);
    }

    // The G3 default greet.json: english only, the four placeholder sections. Real text is the repository draft (G12).
    static NimbyGreetFile DefaultFile()
    {
        NimbyGreetFile file = new NimbyGreetFile();
        NimbyGreetLanguage english = new NimbyGreetLanguage();
        english.language = "english";
        english.title = "NIMBY | HARDCORE | BADLANDS | 1pp";
        english.tagline = "Where all hope dies and the Badlands begin";
        english.sections.Insert(MakeSection("welcome", "Welcome", "# Welcome\n\nThis is the default welcome text. The server owner edits $profile:NimbyGreet\\greet.json to replace it."));
        english.sections.Insert(MakeSection("rules", "Rules", "# The one rule\n\nDon't be a dick."));
        english.sections.Insert(MakeSection("discord", "Discord", "# Discord\n\nJoin us: [NIMBY Discord](https://discord.gg/kH3eJ5mdEv)"));
        english.sections.Insert(MakeSection("restarts", "Restarts", "# Restarts\n\nThe server restarts every 12 hours. See docs/operations/restart-schedule.md for the exact times."));
        file.languages.Insert(english);
        return file;
    }

    // body is the text with "\n" line breaks; it is stored as lines (NIMBY-T-055).
    static NimbyGreetSection MakeSection(string id, string heading, string body)
    {
        NimbyGreetSection section = new NimbyGreetSection();
        section.id = id;
        section.heading = heading;
        SetBodyText(section, body);
        return section;
    }

    // The body's lines joined with "\n"; "" for a missing section or body.
    static string BodyText(NimbyGreetSection section)
    {
        if (!section)
        {
            return "";
        }
        if (!section.body)
        {
            return "";
        }
        string text = "";
        int i = 0;
        while (i < section.body.Count())
        {
            if (i > 0)
            {
                text = text + "\n";
            }
            text = text + section.body.Get(i);
            i = i + 1;
        }
        return text;
    }

    // Replaces the body with text split on "\n" (SplitLines keeps empty lines).
    static void SetBodyText(NimbyGreetSection section, string text)
    {
        if (!section)
        {
            return;
        }
        array<string> lines;
        NimbyGreetRules.SplitLines(text, lines);
        section.body = lines;
    }
}
