// -----------------------------------------------------------------------------
// NimbyGreet - self-test (shared rules 8).
// Purpose : The engine-free validators, run at mission start when $profile:NimbyGreet\selftest
//           exists (MissionBase hook in 5_Mission). One case per construct, cap, chunk
//           boundary, dismissal state and language pick; the 22 Phase 0 cases stay first.
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

    // ---- Phase 0 (October 25, 2026): caps, chunk math, UTF-8 cut --------------------------

    protected static int Phase0Cases(array<string> results)
    {
        int failures = 0;
        failures += Case(results, "cap: short text unchanged", NimbyGreetRules.CapText("abc", 10) == "abc");
        failures += Case(results, "cap: long text cut to cap", NimbyGreetRules.CapText("abcdef", 3) == "abc");
        failures += Case(results, "cap: zero cap gives empty", NimbyGreetRules.CapText("abc", 0) == "");
        failures += Case(results, "chunks: 0 chars is 0 chunks", NimbyGreetRules.ChunkCount(0) == 0);
        failures += Case(results, "chunks: one char is 1 chunk", NimbyGreetRules.ChunkCount(1) == 1);
        failures += Case(results, "chunks: exact multiple", NimbyGreetRules.ChunkCount(NimbyGreetRules.CHUNK_BYTES * 2) == 2);
        failures += Case(results, "chunks: one over rounds up", NimbyGreetRules.ChunkCount(NimbyGreetRules.CHUNK_BYTES * 2 + 1) == 3);
        failures += Case(results, "chunks: 40000 bytes is 40 chunks", NimbyGreetRules.ChunkCount(40000) == 40);
        failures += Case(results, "cap: multibyte text cut by characters", NimbyGreetRules.CapText("дайз", 2).LengthUtf8() == 2);
        failures += Case(results, "utf8: ascii byte is not a continuation byte", NimbyGreetRules.IsContinuationByte("abc", 1) == false);
        failures += Case(results, "utf8: second byte of a cyrillic char is a continuation byte", NimbyGreetRules.IsContinuationByte("дайз", 1) == true);
        failures += Case(results, "utf8: lead byte of a cyrillic char is not a continuation byte", NimbyGreetRules.IsContinuationByte("дайз", 2) == false);
        failures += Case(results, "utf8: chunk end backs off a split 2-byte char", NimbyGreetRules.ChunkEnd("дайз", 0, 3) == 2);
        failures += Case(results, "utf8: chunk end keeps an exact boundary", NimbyGreetRules.ChunkEnd("дайз", 0, 4) == 4);
        failures += Case(results, "utf8: chunk end past the end is the length", NimbyGreetRules.ChunkEnd("abc", 0, 10) == 3);
        failures += Case(results, "fill: ascii gives exact bytes", NimbyGreetRules.FillBytes("a", 10).Length() == 10);
        failures += Case(results, "fill: cyrillic rounds down to a whole char", NimbyGreetRules.FillBytes("д", 5).Length() == 4);
        failures += Case(results, "fill: cjk exact multiple of 3 bytes", NimbyGreetRules.FillBytes("字", 9).Length() == 9);
        array<string> parts;
        int count = NimbyGreetRules.SplitChunks(NimbyGreetRules.FillBytes("a", 80), 32, parts);
        failures += Case(results, "split: 80 ascii bytes at 32 is 3 chunks", count == 3);
        failures += Case(results, "split: chunks rejoin to the input", Rejoin(parts) == NimbyGreetRules.FillBytes("a", 80));
        count = NimbyGreetRules.SplitChunks(NimbyGreetRules.FillBytes("д", 20), 7, parts);
        failures += Case(results, "split: cyrillic at 7 bytes is 4 chunks of 6 bytes", count == 4 && parts.Get(0).Length() == 6);
        failures += Case(results, "split: cyrillic chunks never split a char", ChunksAreWholeChars(parts, 2));
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
        failures += Case(results, "md: unterminated bold closes at its paragraph", c.blocks.Get(0).markup == "<b>open to the end</b>" && c.blocks.Get(1).markup == "next");

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
        failures += Case(results, "md: three-cell row prints as written between table parts", wideOk);

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
        language.sections.Get(0).body = NimbyGreetRules.Repeat("x", 4500);
        language.sections.Get(1).id = "Bad Id!";
        language.sections.Get(2).heading = NimbyGreetRules.Repeat("h", 50);
        array<string> notes;
        int cuts = NimbyGreetText.ApplyCaps(language, notes);
        failures += Case(results, "caps: title cut to 60", language.title.LengthUtf8() == 60);
        failures += Case(results, "caps: sections cut to 16", language.sections.Count() == 16);
        failures += Case(results, "caps: body cut to 4000", language.sections.Get(0).body.LengthUtf8() == 4000);
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
            sum = sum + section.body.LengthUtf8();
        }
        failures += Case(results, "caps: total held to 40000 characters", sum == 40000 && total.sections.Get(10).body.Length() == 0);

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
        return failures;
    }

    // ---- G5 fetch validation -----------------------------------------------------------------

    protected static int FetchCases(array<string> results)
    {
        int failures = 0;
        string why;
        string payload = NimbyGreetRules.FillBytes("д", 2500);
        array<string> chunks;
        int count = NimbyGreetRules.SplitChunks(payload, NimbyGreetRules.CHUNK_BYTES, chunks);
        failures += Case(results, "fetch: 2500 cyrillic bytes are 3 chunks", count == 3);

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
        failures += Case(results, "done: malformed json is null with an error", NimbyGreetDone.FromJson("{ nope", error) == null && error.Length() > 0);

        NimbyGreetFetch fetch = new NimbyGreetFetch();
        fetch.Begin(7, "russian", false, 0.0);
        failures += Case(results, "fetch: chunk for another request dropped", !fetch.AcceptChunk(6, 0, chunks.Get(0), 0.1, why) && why == "other request");
        failures += Case(results, "fetch: oversize chunk dropped", !fetch.AcceptChunk(7, 0, NimbyGreetRules.FillBytes("a", NimbyGreetRules.CHUNK_BYTES + 1), 0.1, why) && why == "chunk above CHUNK_BYTES");
        failures += Case(results, "fetch: negative index dropped", !fetch.AcceptChunk(7, -1, chunks.Get(0), 0.1, why));
        failures += Case(results, "fetch: chunk 2 accepted before done", fetch.AcceptChunk(7, 2, chunks.Get(2), 0.2, why));
        failures += Case(results, "fetch: duplicate index dropped", !fetch.AcceptChunk(7, 2, chunks.Get(2), 0.2, why) && why == "duplicate index");
        failures += Case(results, "fetch: stray chunk 9 accepted before done", fetch.AcceptChunk(7, 9, "stray", 0.2, why));
        failures += Case(results, "fetch: not complete without done", !fetch.IsComplete());
        failures += Case(results, "fetch: done accepted and drops the stray chunk", fetch.AcceptDone(7, 3, back, 0.3, why) && fetch.m_Received == 1);
        failures += Case(results, "fetch: index beyond chunkCount dropped after done", !fetch.AcceptChunk(7, 3, "late", 0.3, why) && why == "index beyond chunkCount");
        fetch.AcceptChunk(7, 0, chunks.Get(0), 0.4, why);
        failures += Case(results, "fetch: still incomplete at 2 of 3", !fetch.IsComplete());
        fetch.AcceptChunk(7, 1, chunks.Get(1), 0.5, why);
        failures += Case(results, "fetch: complete at 3 of 3 in any order", fetch.IsComplete());
        string assembled = fetch.Assemble();
        failures += Case(results, "fetch: assembled payload intact", assembled == payload && fetch.Verify(assembled) == "intact");
        failures += Case(results, "fetch: elapsed from request to done", fetch.ElapsedMs() == 500);

        NimbyGreetFetch bad = new NimbyGreetFetch();
        bad.Begin(8, "english", true, 0.0);
        failures += Case(results, "fetch: chunkCount above the cap refuses the response", !bad.AcceptDone(8, NimbyGreetRules.MaxChunkCount() + 1, back, 0.1, why) && bad.IsRefused());
        failures += Case(results, "fetch: refused response takes no chunk", !bad.AcceptChunk(8, 0, "x", 0.1, why) && why == "response refused");

        NimbyGreetFetch empty = new NimbyGreetFetch();
        empty.Begin(9, "english", false, 0.0);
        NimbyGreetDone none = new NimbyGreetDone();
        none.reason = "dismissed";
        failures += Case(results, "fetch: done with zero chunks is complete and empty", empty.AcceptDone(9, 0, none, 0.1, why) && empty.IsComplete() && empty.Assemble() == "");

        failures += Case(results, "fetch: payload cap is 164 chunks", NimbyGreetRules.MaxChunkCount() == 164);
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

    protected static string Rejoin(array<string> parts)
    {
        string result = "";
        foreach (string part : parts)
        {
            result = result + part;
        }
        return result;
    }

    protected static bool ChunksAreWholeChars(array<string> parts, int bytesPerChar)
    {
        foreach (string part : parts)
        {
            if (part.Length() != part.LengthUtf8() * bytesPerChar)
            {
                return false;
            }
        }
        return true;
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
