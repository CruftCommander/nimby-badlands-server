// -----------------------------------------------------------------------------
// NimbyGreet - Phase 0 probe payloads (plan Phase 0 items 2 and 8). Engine-free.
// Purpose : The server builds the text a request asks for from the request's language
//           field, which the probe overloads as a command:
//             "english" (or anything else)  the fixed welcome text (item 2)
//             "probe:ascii:N"               N bytes of ASCII in ONE chunk, whatever CHUNK_BYTES says
//             "probe:cyr:N"                 N bytes of Cyrillic (2 bytes a character) in one chunk
//             "probe:cjk:N"                 N bytes of CJK (3 bytes a character) in one chunk
//             "probe:big"                   16 sections, about 40,000 Russian characters, chunked
//                                           at CHUNK_BYTES (the G9 maximum language, item 8)
//           The single-chunk forms find the RPC string ceiling; the client verifies each
//           arrival by byte count and hash (NimbyGreetFetch.Verify).
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyGreetProbePayload
{
    static const string ASCII_UNIT = "NIMBY probe 0123456789 ";
    static const string CYR_UNIT = "дайз";
    static const string CJK_UNIT = "字";
    static const string RU_SENTENCE = "Добро пожаловать в Бэдлендс, где умирает всякая надежда. ";

    static string FixedWelcome()
    {
        string text = "Welcome to NIMBY | HARDCORE | BADLANDS | 1pp.";
        text = text + "<br/>Where all hope dies and the Badlands begin.";
        text = text + "<br/><br/>This is the Phase 0 probe text, sent by the server after you spawned.";
        text = text + "<br/>The one rule: don't be a dick. Discord: https://discord.gg/kH3eJ5mdEv";
        text = text + "<br/>Restarts every 12 hours. Fresh world dated October 2026.";
        return text;
    }

    // One section: a heading line plus about 2,500 Russian characters.
    static string BigSection(int number)
    {
        string text = "## Раздел " + number.ToString() + "\n";
        text = text + NimbyGreetRules.Repeat(RU_SENTENCE, 44);
        return text;
    }

    static string Big()
    {
        string text = "";
        int i = 1;
        while (i <= NimbyGreetRules.MAX_SECTIONS)
        {
            text = text + BigSection(i) + "\n\n";
            i = i + 1;
        }
        return text;
    }

    // Builds the payload for a request. singleChunk is true for the ceiling forms, which must go in one RPC.
    static string Build(string language, out bool singleChunk, out string info)
    {
        singleChunk = false;
        info = "fixed";
        if (language.IndexOf("probe:") != 0)
        {
            return FixedWelcome();
        }
        array<string> parts = new array<string>;
        language.Split(":", parts);
        if (parts.Count() < 2)
        {
            return FixedWelcome();
        }
        string kind = parts.Get(1);
        if (kind == "big")
        {
            info = "big";
            return Big();
        }
        if (parts.Count() < 3)
        {
            return FixedWelcome();
        }
        int bytes = parts.Get(2).ToInt();
        if (bytes <= 0)
        {
            return FixedWelcome();
        }
        singleChunk = true;
        if (kind == "ascii")
        {
            info = "ascii " + bytes.ToString();
            return NimbyGreetRules.FillBytes(ASCII_UNIT, bytes);
        }
        if (kind == "cyr")
        {
            info = "cyr " + bytes.ToString();
            return NimbyGreetRules.FillBytes(CYR_UNIT, bytes);
        }
        if (kind == "cjk")
        {
            info = "cjk " + bytes.ToString();
            return NimbyGreetRules.FillBytes(CJK_UNIT, bytes);
        }
        singleChunk = false;
        return FixedWelcome();
    }
}
