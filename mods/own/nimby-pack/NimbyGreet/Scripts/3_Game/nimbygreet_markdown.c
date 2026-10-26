// -----------------------------------------------------------------------------
// NimbyGreet - the markdown subset converter (plan v0.4, G13, G10, G15).
// Purpose : Turns one section body (markdown subset) into a block list the window renders:
//           heading, text, rule, table and media blocks, plus the section's link rows.
//           Engine-free: strings in, strings out, so the self-test runs it on the server
//           and Test-GreetText.ps1 mirrors it line for line.
// Honored : Phase 0 item 7 (NIMBY-T-040) proved RichTextWidget honors <b>, <i>, <color>,
//           <p>, <br/> and \n and nothing else, no entity works, a raw < swallows to the
//           next >, and an unterminated tag bleeds to the end of the text. So:
//           1. Every < and > becomes U+2039 and U+203A before anything else (the only
//              look-alikes with glyphs in sdf_MetronLight24).
//           2. Only <b>, <i>, <color hex> and <br/> are ever emitted.
//           3. Every tag opened is closed at the end of its block.
//           4. Headings are their own blocks (the window sets their size); <font> is not used.
//           5. Links are the label in the link color plus a [n] marker (no underline), and a
//              link row (G10). Only https:// links; the rest print as written and are noted.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyGreetBlockKind
{
    static const int TEXT = 0;
    static const int HEADING1 = 1;
    static const int HEADING2 = 2;
    static const int RULE = 3;
    static const int TABLE = 4;
    static const int MEDIA = 5;
}

class NimbyGreetBlock
{
    int kind = NimbyGreetBlockKind.TEXT;
    string markup = "";                 // TEXT, HEADING1, HEADING2: RichTextWidget markup
    string mediaId = "";                // MEDIA
    string alt = "";                    // MEDIA
    ref array<ref array<string>> rows;  // TABLE: two cells per row, first row is the header
    bool headerless;                    // TABLE: the continuation after a row printed as written; no header, not a new table

    void NimbyGreetBlock(int blockKind)
    {
        kind = blockKind;
        rows = new array<ref array<string>>;
    }
}

class NimbyGreetLink
{
    int index;      // 1-based, the [n] marker
    string label;
    string url;

    void NimbyGreetLink(int linkIndex, string linkLabel, string linkUrl)
    {
        index = linkIndex;
        label = linkLabel;
        url = linkUrl;
    }
}

class NimbyGreetConverted
{
    ref array<ref NimbyGreetBlock> blocks = new array<ref NimbyGreetBlock>;
    ref array<ref NimbyGreetLink> links = new array<ref NimbyGreetLink>;
    ref array<string> notes = new array<string>;   // one line per rejected link, unknown media id, dropped table
    int tables;
    int mediaBlocks;
}

class NimbyGreetMarkdown
{
    static const string LINK_COLOR = "0xFF8CC4FF";   // light blue on the dark pane, above 4.5:1
    static const string ANGLE_OPEN = "‹";            // U+2039
    static const string ANGLE_CLOSE = "›";           // U+203A
    static const string BULLET = "•";                // U+2022

    // ---- block level -------------------------------------------------------------------------

    // knownMediaIds may be null (no manifest); every media reference is then unknown.
    static NimbyGreetConverted Convert(string body, array<string> knownMediaIds)
    {
        NimbyGreetConverted result = new NimbyGreetConverted();
        string safe = Neutralize(body);
        array<string> lines;
        NimbyGreetRules.SplitLines(safe, lines);
        array<string> paragraph = new array<string>;
        int i = 0;
        while (i < lines.Count())
        {
            string line = lines.Get(i);
            string trimmed = line.Trim();
            if (trimmed.Length() == 0)
            {
                FlushParagraph(result, paragraph);
                i = i + 1;
                continue;
            }
            if (NimbyGreetRules.StartsWith(trimmed, "## "))
            {
                FlushParagraph(result, paragraph);
                AddHeading(result, NimbyGreetBlockKind.HEADING2, trimmed.Substring(3, trimmed.Length() - 3));
                i = i + 1;
                continue;
            }
            if (NimbyGreetRules.StartsWith(trimmed, "# "))
            {
                FlushParagraph(result, paragraph);
                AddHeading(result, NimbyGreetBlockKind.HEADING1, trimmed.Substring(2, trimmed.Length() - 2));
                i = i + 1;
                continue;
            }
            if (IsRule(trimmed))
            {
                FlushParagraph(result, paragraph);
                result.blocks.Insert(new NimbyGreetBlock(NimbyGreetBlockKind.RULE));
                i = i + 1;
                continue;
            }
            string mediaId;
            string alt;
            if (ParseMediaLine(trimmed, mediaId, alt))
            {
                FlushParagraph(result, paragraph);
                AddMedia(result, mediaId, alt, knownMediaIds);
                i = i + 1;
                continue;
            }
            if (NimbyGreetRules.StartsWith(trimmed, "|"))
            {
                FlushParagraph(result, paragraph);
                i = ParseTable(result, lines, i, paragraph);
                FlushParagraph(result, paragraph);
                continue;
            }
            if (NimbyGreetRules.StartsWith(trimmed, "- ") || NimbyGreetRules.StartsWith(trimmed, "* "))
            {
                paragraph.Insert(BULLET + " " + Inline(result, trimmed.Substring(2, trimmed.Length() - 2)));
                i = i + 1;
                continue;
            }
            int numberLength = NumberedPrefixLength(trimmed);
            if (numberLength > 0)
            {
                paragraph.Insert(trimmed.Substring(0, numberLength - 1) + " " + Inline(result, trimmed.Substring(numberLength, trimmed.Length() - numberLength)));
                i = i + 1;
                continue;
            }
            paragraph.Insert(Inline(result, trimmed));
            i = i + 1;
        }
        FlushParagraph(result, paragraph);
        return result;
    }

    // Rule 1: no < or > survives into the markup.
    static string Neutralize(string text)
    {
        string safe = text;
        safe.Replace("<", ANGLE_OPEN);
        safe.Replace(">", ANGLE_CLOSE);
        return safe;
    }

    protected static void FlushParagraph(NimbyGreetConverted result, array<string> paragraph)
    {
        if (paragraph.Count() == 0)
        {
            return;
        }
        NimbyGreetBlock block = new NimbyGreetBlock(NimbyGreetBlockKind.TEXT);
        block.markup = JoinLines(paragraph);
        result.blocks.Insert(block);
        paragraph.Clear();
    }

    protected static string JoinLines(array<string> lines)
    {
        string markup = "";
        int i = 0;
        while (i < lines.Count())
        {
            if (i > 0)
            {
                markup = markup + "<br/>";
            }
            markup = markup + lines.Get(i);
            i = i + 1;
        }
        return markup;
    }

    protected static void AddHeading(NimbyGreetConverted result, int kind, string text)
    {
        NimbyGreetBlock block = new NimbyGreetBlock(kind);
        block.markup = Inline(result, text.Trim());
        result.blocks.Insert(block);
    }

    protected static void AddMedia(NimbyGreetConverted result, string mediaId, string alt, array<string> knownMediaIds)
    {
        bool known = false;
        if (knownMediaIds)
        {
            known = knownMediaIds.Find(mediaId) >= 0;
        }
        if (!known || result.mediaBlocks >= NimbyGreetRules.MAX_MEDIA_PER_SECTION)
        {
            NimbyGreetBlock text = new NimbyGreetBlock(NimbyGreetBlockKind.TEXT);
            text.markup = "[" + alt + "]";
            result.blocks.Insert(text);
            if (!known)
            {
                result.notes.Insert("media unknown id=" + mediaId);
            }
            else
            {
                result.notes.Insert("media beyond " + NimbyGreetRules.MAX_MEDIA_PER_SECTION.ToString() + " per section dropped id=" + mediaId);
            }
            return;
        }
        NimbyGreetBlock block = new NimbyGreetBlock(NimbyGreetBlockKind.MEDIA);
        block.mediaId = mediaId;
        block.alt = alt;
        result.blocks.Insert(block);
        result.mediaBlocks = result.mediaBlocks + 1;
    }

    // "---" alone (three or more dashes, nothing else).
    static bool IsRule(string trimmed)
    {
        if (trimmed.Length() < 3)
        {
            return false;
        }
        int i = 0;
        while (i < trimmed.Length())
        {
            if (trimmed.Get(i) != "-")
            {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    // "![alt](id)" and nothing else on the line.
    static bool ParseMediaLine(string trimmed, out string mediaId, out string alt)
    {
        mediaId = "";
        alt = "";
        if (!NimbyGreetRules.StartsWith(trimmed, "!["))
        {
            return false;
        }
        int close = trimmed.IndexOf("](");
        if (close < 0)
        {
            return false;
        }
        int end = trimmed.Length() - 1;
        if (trimmed.Get(end) != ")")
        {
            return false;
        }
        alt = trimmed.Substring(2, close - 2);
        mediaId = trimmed.Substring(close + 2, end - close - 2);
        return true;
    }

    // Length of "N. " at the start of the line (digits, a dot, a space), or 0.
    static int NumberedPrefixLength(string trimmed)
    {
        int i = 0;
        int length = trimmed.Length();
        while (i < length)
        {
            int code = trimmed.Get(i).ToAscii();
            if (code < 48 || code > 57)
            {
                break;
            }
            i = i + 1;
        }
        if (i == 0 || i > 3)
        {
            return 0;
        }
        if (i + 1 >= length)
        {
            return 0;
        }
        if (trimmed.Get(i) != ".")
        {
            return 0;
        }
        if (trimmed.Get(i + 1) != " ")
        {
            return 0;
        }
        return i + 2;
    }

    // Consumes consecutive lines that start with "|". Rows of exactly two cells form a table block;
    // the "| --- | --- |" separator is skipped; any other row is printed as written (Rule: wider
    // tables are not supported). Returns the index of the first line not consumed.
    protected static int ParseTable(NimbyGreetConverted result, array<string> lines, int start, array<string> paragraph)
    {
        NimbyGreetBlock table = new NimbyGreetBlock(NimbyGreetBlockKind.TABLE);
        bool allowed = result.tables < NimbyGreetRules.MAX_TABLES_PER_SECTION;
        int i = start;
        while (i < lines.Count())
        {
            string trimmed = lines.Get(i).Trim();
            if (!NimbyGreetRules.StartsWith(trimmed, "|"))
            {
                break;
            }
            array<string> cells;
            SplitCells(trimmed, cells);
            if (IsSeparatorRow(cells))
            {
                i = i + 1;
                continue;
            }
            if (cells.Count() == 2 && allowed)
            {
                array<string> row = new array<string>;
                row.Insert(Inline(result, cells.Get(0)));
                row.Insert(Inline(result, cells.Get(1)));
                table.rows.Insert(row);
                i = i + 1;
                continue;
            }
            // Printed as written: flush any table rows so far, then the line as text. The rows after it
            // continue the same table (headerless, same count) so a stray wide row costs no second header.
            if (table.rows.Count() > 0)
            {
                result.blocks.Insert(table);
                if (!table.headerless)
                {
                    result.tables = result.tables + 1;
                }
                table = new NimbyGreetBlock(NimbyGreetBlockKind.TABLE);
                table.headerless = true;
            }
            paragraph.Insert(Inline(result, trimmed));
            FlushParagraph(result, paragraph);
            i = i + 1;
        }
        if (table.rows.Count() > 0)
        {
            result.blocks.Insert(table);
            if (!table.headerless)
            {
                result.tables = result.tables + 1;
            }
        }
        else
        {
            if (!allowed)
            {
                result.notes.Insert("table beyond " + NimbyGreetRules.MAX_TABLES_PER_SECTION.ToString() + " per section printed as written");
            }
        }
        return i;
    }

    // "| a | b |" to ["a", "b"]; a missing trailing pipe is tolerated.
    static void SplitCells(string trimmed, out array<string> cells)
    {
        cells = new array<string>;
        string inner = trimmed;
        if (NimbyGreetRules.StartsWith(inner, "|"))
        {
            inner = inner.Substring(1, inner.Length() - 1);
        }
        int innerLength = inner.Length();
        if (innerLength > 0)
        {
            if (inner.Get(innerLength - 1) == "|")
            {
                inner = inner.Substring(0, innerLength - 1);
            }
        }
        int startAt = 0;
        int length = inner.Length();
        while (startAt <= length)
        {
            int next = inner.IndexOfFrom(startAt, "|");
            if (next < 0)
            {
                next = length;
            }
            cells.Insert(inner.Substring(startAt, next - startAt).Trim());
            startAt = next + 1;
        }
    }

    // Every cell is dashes with optional colons, at least one dash.
    static bool IsSeparatorRow(array<string> cells)
    {
        if (cells.Count() == 0)
        {
            return false;
        }
        foreach (string cell : cells)
        {
            int dashes = 0;
            int i = 0;
            while (i < cell.Length())
            {
                string c = cell.Get(i);
                if (c == "-")
                {
                    dashes = dashes + 1;
                }
                else
                {
                    if (c != ":")
                    {
                        return false;
                    }
                }
                i = i + 1;
            }
            if (dashes == 0)
            {
                return false;
            }
        }
        return true;
    }

    // ---- inline level ------------------------------------------------------------------------

    // **bold**, *italic*, [label](https://url), \ escapes. Every tag opened is closed before return.
    static string Inline(NimbyGreetConverted result, string text)
    {
        string markup = "";
        array<string> open = new array<string>;   // tag names in open order: "b", "i"
        int i = 0;
        int length = text.Length();
        while (i < length)
        {
            string c = text.Get(i);
            if (c == "\\")
            {
                if (i + 1 < length)
                {
                    string next = text.Get(i + 1);
                    if (IsEscapable(next))
                    {
                        markup = markup + next;
                        i = i + 2;
                        continue;
                    }
                }
                markup = markup + c;
                i = i + 1;
                continue;
            }
            if (c == "*")
            {
                bool twin = false;
                if (i + 1 < length)
                {
                    twin = text.Get(i + 1) == "*";
                }
                if (twin)
                {
                    markup = markup + Toggle(open, "b");
                    i = i + 2;
                    continue;
                }
                if (ItalicMarker(text, i, open))
                {
                    markup = markup + Toggle(open, "i");
                    i = i + 1;
                    continue;
                }
                markup = markup + c;
                i = i + 1;
                continue;
            }
            if (c == "[")
            {
                int consumed = 0;
                string linkMarkup = TryLink(result, text, i, consumed);
                if (consumed > 0)
                {
                    markup = markup + linkMarkup;
                    i = i + consumed;
                    continue;
                }
            }
            markup = markup + c;
            i = i + 1;
        }
        markup = markup + CloseAll(open);
        return markup;
    }

    static bool IsEscapable(string c)
    {
        return c == "\\" || c == "*" || c == "[" || c == "]" || c == "(" || c == ")" || c == "!" || c == "#" || c == "|" || c == "-" || c == "_";
    }

    // A single * opens italic when followed by a non-space and closes it when preceded by a non-space.
    protected static bool ItalicMarker(string text, int i, array<string> open)
    {
        bool isOpen = open.Find("i") >= 0;
        if (isOpen)
        {
            if (i == 0)
            {
                return false;
            }
            return text.Get(i - 1) != " ";
        }
        if (i + 1 >= text.Length())
        {
            return false;
        }
        return text.Get(i + 1) != " ";
    }

    // Opens tag when closed, closes it when open; keeps the markup nested by closing and reopening
    // whatever was opened after it.
    protected static string Toggle(array<string> open, string tag)
    {
        int at = open.Find(tag);
        if (at < 0)
        {
            open.Insert(tag);
            return "<" + tag + ">";
        }
        string markup = "";
        array<string> reopen = new array<string>;
        int last = open.Count() - 1;
        while (last > at)
        {
            string inner = open.Get(last);
            markup = markup + "</" + inner + ">";
            reopen.Insert(inner);
            open.Remove(last);
            last = last - 1;
        }
        markup = markup + "</" + tag + ">";
        open.Remove(at);
        int r = reopen.Count() - 1;
        while (r >= 0)
        {
            string again = reopen.Get(r);
            markup = markup + "<" + again + ">";
            open.Insert(again);
            r = r - 1;
        }
        return markup;
    }

    protected static string CloseAll(array<string> open)
    {
        string markup = "";
        int last = open.Count() - 1;
        while (last >= 0)
        {
            markup = markup + "</" + open.Get(last) + ">";
            open.Remove(last);
            last = last - 1;
        }
        return markup;
    }

    // At text[i] == "[": "[label](url)". consumed is 0 when this is not a link. An accepted link
    // becomes the colored label plus " [n]" and a link row; a rejected one prints as written.
    protected static string TryLink(NimbyGreetConverted result, string text, int i, out int consumed)
    {
        consumed = 0;
        int mid = text.IndexOfFrom(i, "](");
        if (mid < 0)
        {
            return "";
        }
        int end = text.IndexOfFrom(mid + 2, ")");
        if (end < 0)
        {
            return "";
        }
        string label = text.Substring(i + 1, mid - i - 1);
        string url = text.Substring(mid + 2, end - mid - 2);
        if (label.IndexOf("[") >= 0)
        {
            return "";
        }
        consumed = end - i + 1;
        if (!NimbyGreetRules.IsAcceptedUrl(url))
        {
            result.notes.Insert("link rejected url=" + url);
            return label + " (" + url + ")";
        }
        if (result.links.Count() >= NimbyGreetRules.MAX_LINKS_PER_SECTION)
        {
            result.notes.Insert("link beyond " + NimbyGreetRules.MAX_LINKS_PER_SECTION.ToString() + " per section printed as text url=" + url);
            return label + " (" + url + ")";
        }
        int index = result.links.Count() + 1;
        result.links.Insert(new NimbyGreetLink(index, label, url));
        return "<color hex=\"" + LINK_COLOR + "\">" + label + "</color> [" + index.ToString() + "]";
    }
}
