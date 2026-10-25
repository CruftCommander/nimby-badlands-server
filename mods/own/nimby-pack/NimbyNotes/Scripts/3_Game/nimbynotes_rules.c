// -----------------------------------------------------------------------------
// NimbyNotes - engine-free rules.
// Purpose : The combine, split, and write predicates of Scrawl N2, the N12 length
//           cap, and the N15 write outcome as pure functions, so they are
//           unit-tested by the self-test and the 4_World and 5_Mission layers only
//           feed them the facts they read.
// N12     : The cap is vanilla's own. MiscGameplayFunctions.SanitizeString keeps the
//           first 512 characters of a note (MiscGameplayFunctions.c:930-937), on the
//           client before the text is sent (NoteMenu.c:118) and on the server when it
//           is stored (WrittenNoteData.c:80). Jeff ruled 512 on October 19, 2026, after
//           that was found: the 1000 ruled on October 17 could never take effect.
//           CapText cuts the same way (Length and Substring), so the editor, the
//           server log, and vanilla agree on where a note ends.
// N15     : WriteOutcome decides a RPC_WRITE_NOTE_CLIENT from four facts the server
//           reads (the text, whether a pen is bound, whether the stream read, whether
//           the sender is the writer) and returns what to store, whether to take ink,
//           and whether the text was cut, so the refusal, the cap, and the ink skip
//           are each pinned by a self-test case without an engine.
// -----------------------------------------------------------------------------

// N15: the verdict of one write, computed by NimbyNotesRules.WriteOutcome.
class NimbyNotesWriteOutcome
{
    int m_Verdict;
    string m_Text;
    bool m_TakeInk;
    bool m_Capped;

    void NimbyNotesWriteOutcome()
    {
        m_Verdict = NimbyNotesRules.WRITE_UNREADABLE;
        m_Text = "";
        m_TakeInk = false;
        m_Capped = false;
    }

    bool IsRefused()
    {
        return m_Verdict == NimbyNotesRules.WRITE_UNREADABLE || m_Verdict == NimbyNotesRules.WRITE_NOT_WRITER;
    }
}

class NimbyNotesRules
{
    static const int STACK_MAX = 5;
    static const int NOTE_MAX = 512;

    // N15 write verdicts.
    static const int WRITE_UNREADABLE = 0;
    static const int WRITE_NOT_WRITER = 1;
    static const int WRITE_STORED = 2;
    static const int WRITE_STORED_NO_INK = 3;

    // Two sheets may merge only when neither carries text (N2: a written note never merges, so
    // no text is ever lost to a stack).
    static bool CanCombine(bool thisHasText, bool otherHasText)
    {
        return !thisHasText && !otherHasText;
    }

    // A written note is one sheet by construction; splitting it would duplicate the text.
    static bool CanSplit(bool hasText, float quantity)
    {
        return !hasText && quantity > 1;
    }

    // Writing is allowed on exactly one sheet (split a stack first) that is not already written.
    // Rewriting a written note is vanilla behavior and stays allowed until N8: the write action
    // passes the old text to the menu (ActionWritePaper.c:31, WrittenNoteData.c:33).
    // Rounded (Math.Round, EnMath.c:413): every path that sets a Paper quantity today is an
    // integer, but a CE percentage spawn would hand this a float a hair off 1 (review S-7).
    static bool CanWrite(float quantity)
    {
        return Math.Round(quantity) == 1;
    }

    // N12: the first cap characters of text; text no longer than cap comes back unchanged.
    static string CapText(string text, int cap)
    {
        if (text.Length() <= cap)
        {
            return text;
        }
        return text.Substring(0, cap);
    }

    // N15: what the server does with one RPC_WRITE_NOTE_CLIENT. An unreadable stream and a
    // sender who is not the writer are refused and store nothing. Otherwise the text is capped
    // at NOTE_MAX (truncated, not refused: an unmodified client already cuts at 512 before
    // sending, NoteMenu.c:118) and stored; ink is taken only when a pen is bound, because
    // DepleteWritingImplement takes a notnull pen (WrittenNoteData.c:83).
    static NimbyNotesWriteOutcome WriteOutcome(string text, bool hasPen, bool readable, bool isWriter)
    {
        NimbyNotesWriteOutcome outcome = new NimbyNotesWriteOutcome();
        if (!readable)
        {
            outcome.m_Verdict = WRITE_UNREADABLE;
            return outcome;
        }
        if (!isWriter)
        {
            outcome.m_Verdict = WRITE_NOT_WRITER;
            return outcome;
        }
        outcome.m_Text = CapText(text, NOTE_MAX);
        outcome.m_Capped = outcome.m_Text.Length() < text.Length();
        outcome.m_TakeInk = hasPen;
        if (hasPen)
        {
            outcome.m_Verdict = WRITE_STORED;
        }
        else
        {
            outcome.m_Verdict = WRITE_STORED_NO_INK;
        }
        return outcome;
    }

    // Self-test helper: count copies of one character.
    protected static string Repeat(string character, int count)
    {
        string result = "";
        for (int i = 0; i < count; i++)
        {
            result += character;
        }
        return result;
    }

    // Engine-free validators for the $profile:NimbyNotes\selftest flag (shared rules 8).
    static int SelfTest(out array<string> results)
    {
        int failures = 0;
        failures += Case(results, "blank sheets combine", CanCombine(false, false));
        failures += Case(results, "written note refuses a blank sheet", !CanCombine(true, false));
        failures += Case(results, "blank sheet refuses a written note", !CanCombine(false, true));
        failures += Case(results, "two written notes refuse each other", !CanCombine(true, true));
        failures += Case(results, "stack of 2 splits", CanSplit(false, 2));
        failures += Case(results, "single sheet does not split", !CanSplit(false, 1));
        failures += Case(results, "empty stack does not split", !CanSplit(false, 0));
        failures += Case(results, "written note does not split", !CanSplit(true, 2));
        failures += Case(results, "written single sheet does not split", !CanSplit(true, 1));
        failures += Case(results, "write on one sheet", CanWrite(1));
        failures += Case(results, "write on a quantity that rounds to one", CanWrite(0.999));
        failures += Case(results, "no write on a stack", !CanWrite(2));
        failures += Case(results, "no write on a quantity that rounds to two", !CanWrite(1.6));
        failures += Case(results, "no write on an empty stack", !CanWrite(0));
        failures += Case(results, "stack max is 5", STACK_MAX == 5);
        failures += Case(results, "note max is vanilla 512", NOTE_MAX == 512);
        failures += Case(results, "cap keeps a shorter text", CapText("abc", 5) == "abc");
        failures += Case(results, "cap keeps a text of exactly the cap", CapText("abcde", 5) == "abcde");
        failures += Case(results, "cap cuts a longer text", CapText("abcdefg", 5) == "abcde");
        failures += Case(results, "cap keeps an empty text", CapText("", 5) == "");
        string longText = Repeat("x", NOTE_MAX + 1);
        string capped = CapText(longText, NOTE_MAX);
        failures += Case(results, "cap cuts 513 characters to 512", capped.Length() == NOTE_MAX);
        failures += SelfTestWriteOutcome(results, longText);
        return failures;
    }

    // N15 cases: the refusals store nothing, the cap flags a cut, the ink follows the pen.
    protected static int SelfTestWriteOutcome(out array<string> results, string longText)
    {
        int failures = 0;
        NimbyNotesWriteOutcome unreadable = WriteOutcome("abc", true, false, true);
        failures += Case(results, "write: unreadable stream is refused", unreadable.m_Verdict == WRITE_UNREADABLE && unreadable.IsRefused());
        failures += Case(results, "write: refused write stores nothing", unreadable.m_Text == "" && !unreadable.m_TakeInk);
        NimbyNotesWriteOutcome stranger = WriteOutcome("abc", true, true, false);
        failures += Case(results, "write: sender who is not the writer is refused", stranger.m_Verdict == WRITE_NOT_WRITER && stranger.m_Text == "");
        NimbyNotesWriteOutcome stored = WriteOutcome("abc", true, true, true);
        failures += Case(results, "write: text with a pen is stored and takes ink", stored.m_Verdict == WRITE_STORED && stored.m_Text == "abc" && stored.m_TakeInk);
        failures += Case(results, "write: text under the cap is not flagged capped", !stored.m_Capped && !stored.IsRefused());
        NimbyNotesWriteOutcome noPen = WriteOutcome("abc", false, true, true);
        failures += Case(results, "write: missing pen stores the text and skips the ink", noPen.m_Verdict == WRITE_STORED_NO_INK && noPen.m_Text == "abc" && !noPen.m_TakeInk);
        NimbyNotesWriteOutcome over = WriteOutcome(longText, true, true, true);
        failures += Case(results, "write: 513 characters are stored as 512 and flagged capped", over.m_Verdict == WRITE_STORED && over.m_Text.Length() == NOTE_MAX && over.m_Capped);
        return failures;
    }

    protected static int Case(out array<string> results, string name, bool passed)
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
