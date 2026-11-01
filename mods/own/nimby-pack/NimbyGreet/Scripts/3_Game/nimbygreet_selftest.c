// -----------------------------------------------------------------------------
// NimbyGreet - self-test (shared rules 8).
// Purpose : The engine-free validators, run at mission start when $profile:NimbyGreet\selftest
//           exists (MissionBase hook in 5_Mission). One case per construct, cap, dismissal
//           state and language pick; the Phase 0 cap cases stay first. The chunk, UTF-8 cut and
//           receiver cases moved to the NimbyShared self-test in Plan 11 Phase 1 (October 31,
//           2026), names unchanged.
//           The boot check reads "[NIMBY][Greet] selftest complete; cases=N failures=M".
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyGreetSelfTest
{
    // Returns the number of failures; the caller prints one PASS|FAIL line per result.
    static int Run(out array<string> results)
    {
        results = new array<string>;
        int failures = 0;
        failures += Phase0Cases(results);
        failures += ConverterCases(results);
        failures += CapCases(results);
        failures += RuleCases(results);
        failures += FetchCases(results);
        return failures;
    }

    // ---- Phase 0 (October 25, 2026): caps -------------------------------------------------------

    protected static int Phase0Cases(array<string> results)
    {
        int failures = 0;
        failures += Case(results, "cap: short text unchanged", NimbyGreetRules.CapText("abc", 10) == "abc");
        failures += Case(results, "cap: long text cut to cap", NimbyGreetRules.CapText("abcdef", 3) == "abc");
        failures += Case(results, "cap: zero cap gives empty", NimbyGreetRules.CapText("abc", 0) == "");
        failures += Case(results, "cap: multibyte text cut by characters", NimbyGreetRules.CapText("дайз", 2).LengthUtf8() == 2);
        return failures;
    }

    // ---- G13 converter -----------------------------------------------------------------------

    protected static int ConverterCases(array<string> results)
    {
        int failures = 0;
        array<string> media = {"map", "clip"};
        NimbyGreetConverted c;

        c = NimbyGreetMarkdown.Convert("hello", media);
        failures += Case(results, "md: one paragraph is one text block", c.blocks.Count() == 1 && c.blocks.Get(0).kind == NimbyGreetBlockKind.TEXT && c.blocks.Get(0).markup == "hello");

        c = NimbyGreetMarkdown.Convert("one\ntwo\n\nthree", media);
        failures += Case(results, "md: blank line splits paragraphs, lines join with br", c.blocks.Count() == 2 && c.blocks.Get(0).markup == "one<br/>two" && c.blocks.Get(1).markup == "three");

        c = NimbyGreetMarkdown.Convert("a **b** c", media);
        failures += Case(results, "md: bold", c.blocks.Get(0).markup == "a <b>b</b> c");

        c = NimbyGreetMarkdown.Convert("a *b* c", media);
        failures += Case(results, "md: italic", c.blocks.Get(0).markup == "a <i>b</i> c");

        c = NimbyGreetMarkdown.Convert("**a *b* c**", media);
        failures += Case(results, "md: nested italic in bold", c.blocks.Get(0).markup == "<b>a <i>b</i> c</b>");

        c = NimbyGreetMarkdown.Convert("**a *b** c*", media);
        failures += Case(results, "md: interleaved emphasis stays nested", c.blocks.Get(0).markup == "<b>a <i>b</i></b><i> c</i>");

        c = NimbyGreetMarkdown.Convert("**open to the end\n\nnext", media);
        failures += Case(results, "md: unterminated bold closes at its line", c.blocks.Get(0).markup == "<b>open to the end</b>" && c.blocks.Get(1).markup == "next");

        c = NimbyGreetMarkdown.Convert("**a\nb**", media);
        failures += Case(results, "md: emphasis never crosses a line break", c.blocks.Get(0).markup == "<b>a</b><br/>b<b></b>");

        c = NimbyGreetMarkdown.Convert("10. ten\n#x\n2026. year", media);
        failures += Case(results, "md: two-digit list number kept, no-space hash and four-digit number are text", c.blocks.Get(0).markup == "10. ten<br/>#x<br/>2026. year");

        failures += Case(results, "md: rule needs three dashes", NimbyGreetMarkdown.IsRule("----") && !NimbyGreetMarkdown.IsRule("--") && !NimbyGreetMarkdown.IsRule("- - -"));

        c = NimbyGreetMarkdown.Convert("2 * 3 * 4", media);
        failures += Case(results, "md: a spaced star is not italic", c.blocks.Get(0).markup == "2 * 3 * 4");

        c = NimbyGreetMarkdown.Convert("\\*not italic\\* and \\# not a heading", media);
        failures += Case(results, "md: backslash escapes print the character", c.blocks.Get(0).markup == "*not italic* and # not a heading");

        c = NimbyGreetMarkdown.Convert("typed <b>tag</b> here", media);
        failures += Case(results, "md: typed tag becomes angle quotes", c.blocks.Get(0).markup == "typed ‹b›tag‹/b› here");

        c = NimbyGreetMarkdown.Convert("a < b and c > d", media);
        failures += Case(results, "md: raw brackets become angle quotes, line intact", c.blocks.Get(0).markup == "a ‹ b and c › d");

        c = NimbyGreetMarkdown.Convert("# Big\n\n## Small\n\nbody", media);
        bool headingsOk = c.blocks.Count() == 3;
        headingsOk = headingsOk && c.blocks.Get(0).kind == NimbyGreetBlockKind.HEADING1 && c.blocks.Get(0).markup == "Big";
        headingsOk = headingsOk && c.blocks.Get(1).kind == NimbyGreetBlockKind.HEADING2 && c.blocks.Get(1).markup == "Small";
        failures += Case(results, "md: headings are their own blocks", headingsOk);

        c = NimbyGreetMarkdown.Convert("above\n\n---\n\nbelow", media);
        failures += Case(results, "md: rule block", c.blocks.Count() == 3 && c.blocks.Get(1).kind == NimbyGreetBlockKind.RULE);

        c = NimbyGreetMarkdown.Convert("- one\n- two", media);
        failures += Case(results, "md: bullet list", c.blocks.Get(0).markup == "• one<br/>• two");

        c = NimbyGreetMarkdown.Convert("1. one\n2. two", media);
        failures += Case(results, "md: numbered list", c.blocks.Get(0).markup == "1. one<br/>2. two");

        c = NimbyGreetMarkdown.Convert("join [Discord](https://discord.gg/x) now", media);
        failures += Case(results, "md: link is colored label plus marker", c.blocks.Get(0).markup == "join <color hex=\"0xFF8CC4FF\">Discord</color> [1] now");
        failures += Case(results, "md: link row recorded", c.links.Count() == 1 && c.links.Get(0).index == 1 && c.links.Get(0).label == "Discord" && c.links.Get(0).url == "https://discord.gg/x");

        c = NimbyGreetMarkdown.Convert("[plain](http://x.y)", media);
        failures += Case(results, "md: non-https link prints as written and is noted", c.blocks.Get(0).markup == "plain (http://x.y)" && c.links.Count() == 0 && c.notes.Count() == 1);

        string many = "";
        int n = 1;
        while (n <= 9)
        {
            many = many + "[l" + n.ToString() + "](https://a.b/" + n.ToString() + ") ";
            n = n + 1;
        }
        c = NimbyGreetMarkdown.Convert(many, media);
        failures += Case(results, "md: ninth link beyond the cap prints as text", c.links.Count() == 8 && c.notes.Count() == 1);

        c = NimbyGreetMarkdown.Convert("| When | What |\n| --- | --- |\n| 06:00 | Restart |\n| 18:00 | Restart |", media);
        bool tableOk = c.blocks.Count() == 1 && c.blocks.Get(0).kind == NimbyGreetBlockKind.TABLE && c.blocks.Get(0).rows.Count() == 3;
        tableOk = tableOk && c.blocks.Get(0).rows.Get(0).Get(1) == "What" && c.blocks.Get(0).rows.Get(2).Get(0) == "18:00";
        failures += Case(results, "md: two-column table", tableOk);

        c = NimbyGreetMarkdown.Convert("| a | b |\n| --- | --- |\n| 1 | 2 | 3 |\n| 4 | 5 |", media);
        bool wideOk = c.blocks.Count() == 3 && c.blocks.Get(0).kind == NimbyGreetBlockKind.TABLE;
        wideOk = wideOk && c.blocks.Get(1).kind == NimbyGreetBlockKind.TEXT && c.blocks.Get(1).markup == "| 1 | 2 | 3 |";
        wideOk = wideOk && c.blocks.Get(2).kind == NimbyGreetBlockKind.TABLE;
        wideOk = wideOk && c.blocks.Get(2).headerless && !c.blocks.Get(0).headerless && c.tables == 1;
        failures += Case(results, "md: three-cell row prints as written between table parts, one table, no second header", wideOk);

        c = NimbyGreetMarkdown.Convert("![The map](map)", media);
        failures += Case(results, "md: known media id is a media block", c.blocks.Count() == 1 && c.blocks.Get(0).kind == NimbyGreetBlockKind.MEDIA && c.blocks.Get(0).mediaId == "map" && c.blocks.Get(0).alt == "The map");

        c = NimbyGreetMarkdown.Convert("![Nope](missing)", media);
        failures += Case(results, "md: unknown media id prints alt in brackets and is noted", c.blocks.Get(0).kind == NimbyGreetBlockKind.TEXT && c.blocks.Get(0).markup == "[Nope]" && c.notes.Count() == 1);

        c = NimbyGreetMarkdown.Convert("![x](map)", null);
        failures += Case(results, "md: no manifest means every media id is unknown", c.blocks.Get(0).kind == NimbyGreetBlockKind.TEXT);

        c = NimbyGreetMarkdown.Convert("дайз **Бэдлендс**", media);
        failures += Case(results, "md: multibyte text passes through emphasis", c.blocks.Get(0).markup == "дайз <b>Бэдлендс</b>");

        array<string> lines;
        NimbyGreetRules.SplitLines("a\n\nb\r\n", lines);
        failures += Case(results, "lines: empty lines kept, CR stripped", lines.Count() == 4 && lines.Get(1) == "" && lines.Get(2) == "b" && lines.Get(3) == "");
        return failures;
    }

    // ---- G9 caps -----------------------------------------------------------------------------

    protected static int CapCases(array<string> results)
    {
        int failures = 0;
        NimbyGreetLanguage language = new NimbyGreetLanguage();
        language.language = "english";
        language.title = NimbyGreetRules.Repeat("t", 70);
        language.tagline = "fine";
        int s = 0;
        while (s < 17)
        {
            language.sections.Insert(NimbyGreetText.MakeSection("s" + s.ToString(), "Heading", NimbyGreetRules.Repeat("b", 100)));
            s = s + 1;
        }
        // Five lines of 900: over the 4,000 cap but every line under the engine's string limit (NIMBY-T-055).
        string longLine = NimbyGreetRules.Repeat("x", 900);
        NimbyGreetText.SetBodyText(language.sections.Get(0), longLine + "\n" + longLine + "\n" + longLine + "\n" + longLine + "\n" + longLine);
        language.sections.Get(1).id = "Bad Id!";
        language.sections.Get(2).heading = NimbyGreetRules.Repeat("h", 50);
        array<string> notes;
        int cuts = NimbyGreetText.ApplyCaps(language, notes);
        failures += Case(results, "caps: title cut to 60", language.title.LengthUtf8() == 60);
        failures += Case(results, "caps: sections cut to 16", language.sections.Count() == 16);
        failures += Case(results, "caps: body cut to 4000", NimbyGreetText.BodyText(language.sections.Get(0)).LengthUtf8() == 4000);
        failures += Case(results, "caps: invalid id replaced", language.sections.Get(1).id == "section-2");
        failures += Case(results, "caps: heading cut to 40", language.sections.Get(2).heading.LengthUtf8() == 40);
        failures += Case(results, "caps: every cut is noted", cuts == 5 && notes.Count() == 5);

        NimbyGreetLanguage total = new NimbyGreetLanguage();
        int t = 0;
        while (t < 11)
        {
            total.sections.Insert(NimbyGreetText.MakeSection("t" + t.ToString(), "H", NimbyGreetRules.Repeat("y", 4000)));
            t = t + 1;
        }
        NimbyGreetText.ApplyCaps(total, notes);
        int sum = 0;
        foreach (NimbyGreetSection section : total.sections)
        {
            sum = sum + NimbyGreetText.BodyText(section).LengthUtf8();
        }
        failures += Case(results, "caps: total held to 40000 characters", sum == 40000 && NimbyGreetText.BodyText(total.sections.Get(10)).Length() == 0);
        failures += BodyLineCases(results);

        NimbyGreetFile file = new NimbyGreetFile();
        file.openDelaySeconds = 99.0;
        file.openRetrySeconds = -5.0;
        NimbyGreetText.ClampTiming(file);
        failures += Case(results, "caps: timing clamped", file.openDelaySeconds == 30.0 && file.openRetrySeconds == 0.0);

        NimbyGreetFile def = NimbyGreetText.DefaultFile();
        failures += Case(results, "default: english with four sections", def.languages.Count() == 1 && def.languages.Get(0).language == "english" && def.languages.Get(0).sections.Count() == 4);
        return failures;
    }

    // ---- G4, G10, G14, ids -------------------------------------------------------------------

    protected static int RuleCases(array<string> results)
    {
        int failures = 0;
        failures += Case(results, "dismiss: not dismissed opens", NimbyGreetRules.ShouldAutoOpen(false, 0, false, 1) == true);
        failures += Case(results, "dismiss: dismissed stays closed", NimbyGreetRules.ShouldAutoOpen(true, 1, false, 2) == false);
        failures += Case(results, "dismiss: forced newer version opens", NimbyGreetRules.ShouldAutoOpen(true, 1, true, 2) == true);
        failures += Case(results, "dismiss: forced same version stays closed", NimbyGreetRules.ShouldAutoOpen(true, 2, true, 2) == false);
        failures += Case(results, "open wait: pause menu not counted", NimbyGreetRules.OpenWaitCounts(true, true, false) == false);
        failures += Case(results, "open wait: other menu counted", NimbyGreetRules.OpenWaitCounts(false, true, false) == true);
        failures += Case(results, "open wait: unconscious behind pause counted", NimbyGreetRules.OpenWaitCounts(true, true, true) == true);

        array<string> available = {"english", "german"};
        failures += Case(results, "language: requested present is served", NimbyGreetRules.PickLanguage("german", available, "english") == "german");
        failures += Case(results, "language: requested absent falls back", NimbyGreetRules.PickLanguage("czech", available, "english") == "english");
        failures += Case(results, "language: 14 known columns", NimbyGreetRules.IsKnownLanguage("chinesesimp") && !NimbyGreetRules.IsKnownLanguage("klingon"));
        failures += Case(results, "language: cjk flagged", NimbyGreetRules.IsCjkLanguage("japanese") && !NimbyGreetRules.IsCjkLanguage("russian"));

        NimbyGreetFile file = new NimbyGreetFile();
        file.languages.Insert(MakeLanguage("english"));
        file.languages.Insert(MakeLanguage("klingon"));
        file.languages.Insert(MakeLanguage("chinese"));
        file.languages.Insert(MakeLanguage("english"));
        file.languages.Insert(MakeLanguage("german"));
        array<string> codes;
        array<string> notes;
        NimbyGreetText.LanguageCodes(file, codes, notes);
        failures += Case(results, "language: codes skip unknown, cjk and duplicate", codes.Count() == 2 && codes.Get(1) == "german" && notes.Count() == 3);
        failures += Case(results, "language: find by code", NimbyGreetText.FindLanguage(file, "german") != null && NimbyGreetText.FindLanguage(file, "czech") == null);

        failures += Case(results, "url: https accepted", NimbyGreetRules.IsAcceptedUrl("https://discord.gg/kH3eJ5mdEv"));
        failures += Case(results, "url: http rejected", !NimbyGreetRules.IsAcceptedUrl("http://discord.gg/x"));
        failures += Case(results, "url: bare scheme rejected", !NimbyGreetRules.IsAcceptedUrl("https://"));
        failures += Case(results, "url: whitespace and quotes rejected", !NimbyGreetRules.IsAcceptedUrl("https://a.b/c d") && !NimbyGreetRules.IsAcceptedUrl("https://a.b/\"c"));

        failures += Case(results, "id: lower, digits, dash accepted", NimbyGreetRules.IsValidId("how-to-join-2"));
        failures += Case(results, "id: upper case rejected", !NimbyGreetRules.IsValidId("Welcome"));
        failures += Case(results, "id: leading or trailing dash rejected", !NimbyGreetRules.IsValidId("-a") && !NimbyGreetRules.IsValidId("a-"));
        failures += Case(results, "id: 33 characters rejected", !NimbyGreetRules.IsValidId(NimbyGreetRules.Repeat("a", 33)) && NimbyGreetRules.IsValidId(NimbyGreetRules.Repeat("a", 32)));
        failures += Case(results, "media id: trailing -digit rejected", !NimbyGreetRules.IsValidMediaId("clip-0") && NimbyGreetRules.IsValidMediaId("clip") && NimbyGreetRules.IsValidMediaId("map-2024-a"));

        // Patch 1 (October 27, 2026): log identity, gates, refusal replies, timing floors.
        failures += Case(results, "who: ids first, name quoted", NimbyGreetLog.WhoFrom("Road Dog", "BIHASH=", "76561190000000000") == "player=(BIHASH=, 76561190000000000) \"Road Dog\"");
        failures += Case(results, "who: a hostile name cannot forge the id pair", NimbyGreetLog.WhoFrom("A (B, 76561198354705597)", "REAL=", "76561190000000001") == "player=(REAL=, 76561190000000001) \"A (B, 76561198354705597)\"");
        failures += Case(results, "escape: backslash and quote", NimbyGreetLog.Escape("a\\\"b") == "a\\\\\\\"b");
        failures += Case(results, "escape: line break flattened", NimbyGreetLog.Escape("a\nb") == "a b");
        failures += Case(results, "gate: first line passes", NimbyGreetRules.GateAllows(-1000.0, 5.0, NimbyGreetRules.LOG_GATE_SECONDS));
        failures += Case(results, "gate: line inside the window suppressed", !NimbyGreetRules.GateAllows(5.0, 5.0 + NimbyGreetRules.LOG_GATE_SECONDS - 0.5, NimbyGreetRules.LOG_GATE_SECONDS));
        failures += Case(results, "gate: line at the window passes", NimbyGreetRules.GateAllows(5.0, 5.0 + NimbyGreetRules.LOG_GATE_SECONDS, NimbyGreetRules.LOG_GATE_SECONDS));
        failures += Case(results, "gate: clock rewind passes", NimbyGreetRules.GateAllows(9.0, 8.0, NimbyGreetRules.LOG_GATE_SECONDS));
        failures += Case(results, "refusal reply: the first replies are answered", NimbyGreetRules.RefusalReplyAllowed(0) && NimbyGreetRules.RefusalReplyAllowed(NimbyGreetRules.MAX_REFUSAL_REPLIES_PER_WINDOW - 1));
        failures += Case(results, "refusal reply: the cap drops the rest", !NimbyGreetRules.RefusalReplyAllowed(NimbyGreetRules.MAX_REFUSAL_REPLIES_PER_WINDOW));
        NimbyGreetFile timing = new NimbyGreetFile();
        timing.requestCooldownSeconds = 0.0;
        timing.rereadSeconds = 0.0;
        NimbyGreetText.ClampTiming(timing);
        failures += Case(results, "timing: cooldown and reread floor at one second", timing.requestCooldownSeconds == NimbyGreetRules.MIN_COOLDOWN_SECONDS && timing.rereadSeconds == NimbyGreetRules.MIN_REREAD_SECONDS);
        return failures;
    }

    // ---- G5 done record (the receiver cases moved to the NimbyShared self-test, Plan 11) ------

    protected static int FetchCases(array<string> results)
    {
        int failures = 0;
        string payload = NimbySharedChunks.FillBytes("д", 2500);
        NimbyGreetDone done = new NimbyGreetDone();
        done.version = 3;
        done.forceShow = true;
        done.servedLanguage = "russian";
        done.languages.Insert("english");
        done.languages.Insert("russian");
        done.openDelaySeconds = 1.0;
        done.openRetrySeconds = 30.0;
        done.totalBytes = payload.Length();
        done.hash = payload.Hash();
        string json = NimbyGreetDone.ToJson(done);
        string error;
        NimbyGreetDone back = NimbyGreetDone.FromJson(json, error);
        bool roundTrip = back != null;
        if (roundTrip)
        {
            roundTrip = back.version == 3 && back.forceShow && back.servedLanguage == "russian";
            roundTrip = roundTrip && back.languages.Count() == 2 && back.totalBytes == payload.Length() && back.hash == payload.Hash();
        }
        failures += Case(results, "done: json round trip", roundTrip);
        // No malformed-JSON case: the engine prints its own "SCRIPT (E): JSON ERROR" for any parse
        // failure, including ReadFromString (NIMBY-T-042), and the boot check fails on it
        // (build 20261025-231313). FromJson's null return is covered by the client's live path.

        failures += Case(results, "fetch: Greet payload cap is 164 chunks", NimbySharedChunks.MaxChunkCount(NimbyGreetRules.MAX_PAYLOAD_BYTES) == 164);
        NimbyGreetDone blank = NimbyGreetDone.FromJson("{}", error);
        failures += Case(results, "done: empty object parses to defaults", blank != null && blank.version == 0 && blank.languages != null && blank.languages.Count() == 0 && blank.reason == "");
        return failures;
    }

    // ---- NIMBY-T-055: the body is an array of lines ----------------------------------------

    protected static int BodyLineCases(array<string> results)
    {
        int failures = 0;
        NimbyGreetSection joined = NimbyGreetText.MakeSection("j", "J", "a\n\nb");
        failures += Case(results, "body: text stored as lines, empty line kept", joined.body.Count() == 3 && joined.body.Get(1) == "");
        failures += Case(results, "body: lines join with a newline", NimbyGreetText.BodyText(joined) == "a\n\nb");
        NimbyGreetSection none = new NimbyGreetSection();
        none.body = null;
        failures += Case(results, "body: missing body reads as empty", NimbyGreetText.BodyText(none) == "");

        NimbyGreetLanguage spread = new NimbyGreetLanguage();
        NimbyGreetSection five = new NimbyGreetSection();
        five.id = "five";
        five.heading = "Five";
        int k = 0;
        while (k < 5)
        {
            five.body.Insert(NimbyGreetRules.Repeat("x", 1000));
            k = k + 1;
        }
        spread.sections.Insert(five);
        array<string> notes;
        int cuts = NimbyGreetText.ApplyCaps(spread, notes);
        failures += Case(results, "caps: body cut across lines to 4000", cuts == 1 && five.body.Count() == 4 && five.body.Get(3).Length() == 997 && NimbyGreetText.BodyText(five).LengthUtf8() == 4000);

        NimbyGreetLanguage limit = new NimbyGreetLanguage();
        NimbyGreetSection near = NimbyGreetText.MakeSection("near", "Near", NimbyGreetRules.Repeat("z", NimbyGreetRules.ENGINE_STRING_WARN_BYTES));
        limit.sections.Insert(near);
        cuts = NimbyGreetText.ApplyCaps(limit, notes);
        failures += Case(results, "caps: a line of 1020 bytes (the warning threshold) is noted", cuts == 1 && notes.Count() == 1 && notes.Get(0).IndexOf("engine") >= 0);
        NimbyGreetLanguage under = new NimbyGreetLanguage();
        under.sections.Insert(NimbyGreetText.MakeSection("under", "Under", NimbyGreetRules.Repeat("z", NimbyGreetRules.ENGINE_STRING_WARN_BYTES - 1)));
        failures += Case(results, "caps: a line of 1019 bytes is not noted", NimbyGreetText.ApplyCaps(under, notes) == 0);
        failures += Case(results, "caps: the warning threshold is the limit less one four-byte character", NimbyGreetRules.ENGINE_STRING_WARN_BYTES == 1020 && NimbyGreetRules.ENGINE_STRING_BYTES - 3 == NimbyGreetRules.ENGINE_STRING_WARN_BYTES);

        // The engine behavior itself (G-P1-34, G-P1-35): a guard for a future DayZ that changes it.
        NimbyGreetLanguage wire = new NimbyGreetLanguage();
        wire.language = "english";
        NimbyGreetSection lines = new NimbyGreetSection();
        lines.id = "wire";
        lines.heading = "Wire";
        lines.body.Insert(NimbyGreetRules.Repeat("a", 1000));
        lines.body.Insert("");
        lines.body.Insert(NimbyGreetRules.Repeat("b", 1000));
        lines.body.Insert(NimbyGreetRules.Repeat("c", 1100));
        wire.sections.Insert(lines);
        JsonSerializer serializer = new JsonSerializer();
        string json;
        serializer.WriteToString(wire, false, json);
        NimbyGreetLanguage back = new NimbyGreetLanguage();
        string error;
        bool read = serializer.ReadFromString(back, json, error);
        bool intact = read && back.sections.Count() == 1 && back.sections.Get(0).body.Count() == 4;
        if (intact)
        {
            array<string> got = back.sections.Get(0).body;
            intact = got.Get(0).Length() == 1000 && got.Get(1) == "" && got.Get(2).Length() == 1000;
        }
        failures += Case(results, "body: lines of 1000 bytes survive a JSON round trip", intact);
        bool cutAtLimit = read && back.sections.Count() == 1 && back.sections.Get(0).body.Count() == 4;
        if (cutAtLimit)
        {
            cutAtLimit = back.sections.Get(0).body.Get(3).Length() == NimbyGreetRules.ENGINE_STRING_BYTES;
        }
        failures += Case(results, "engine: a JSON string value is kept to 1023 bytes", cutAtLimit);
        return failures;
    }

    // ---- helpers -----------------------------------------------------------------------------

    protected static NimbyGreetLanguage MakeLanguage(string code)
    {
        NimbyGreetLanguage language = new NimbyGreetLanguage();
        language.language = code;
        language.title = code;
        return language;
    }

    protected static int Case(array<string> results, string name, bool passed)
    {
        if (passed)
        {
            results.Insert("PASS " + name);
            return 0;
        }
        results.Insert("FAIL " + name);
        return 1;
    }
}
