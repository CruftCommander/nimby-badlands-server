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
// N15     : WriteOutcome decides a RPC_WRITE_NOTE_CLIENT from five facts the server
//           reads (the text, whether a pen is bound, whether the stream read, whether
//           the sender is the writer, the sheet quantity) and returns what to store,
//           whether to take ink, and whether the text was cut, so the refusal, the
//           cap, and the ink skip are each pinned by a self-test case without an engine.
// N17     : The writer of a sheet is the identity whose write action started on it
//           (recorded by the server at ActionWritePaper.OnStartServer), not the root
//           player of a pen reference that any later start rebinds. One write is open
//           per sheet: a second player's start is refused while it is open (Jeff,
//           October 27, 2026). The write ends on the writer's OK (the text is stored),
//           on the action ending (Cancel, Escape, walking away, disconnect: OnEndServer),
//           or after WRITE_GRACE_MS (Jeff: 300 s). A write RPC is still accepted from
//           the writer for WRITE_LATE_MS after the action ended, because the OK sends
//           the RPC and then closes the menu, and the menu close is what ends the action.
// N18     : The target of a write must be in the writer's own inventory or within
//           WRITE_REACH_METERS of the writer's head or feet (vanilla's TARGET_DISTANCE,
//           ActionWritePaper.c:40, whose check vanilla commented out at :54). Measured
//           on the server from object positions, never from the client's cursor.
// Gate    : Info lines a client can trigger per packet are written at most once per
//           LOG_GATE_MS per sender (shared rules 4.3); GateAllows is the pure rule.
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
        return m_Verdict == NimbyNotesRules.WRITE_UNREADABLE || m_Verdict == NimbyNotesRules.WRITE_NOT_WRITER || m_Verdict == NimbyNotesRules.WRITE_NOT_SINGLE;
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
    static const int WRITE_NOT_SINGLE = 4;

    // N17: one open write per sheet; how long it stays open without the action ending, and
    // how long after the action ended the writer's RPC is still accepted.
    static const int WRITE_GRACE_MS = 300000;
    static const int WRITE_LATE_MS = 5000;

    // N18: vanilla's TARGET_DISTANCE (ActionWritePaper.c:40).
    static const float WRITE_REACH_METERS = 2.0;

    // Shared rules 4.3: one Info line per sender per window for client-triggered lines.
    static const int LOG_GATE_MS = 10000;

    // N17: a write is open on a sheet (and blocks every other player's start) while a writer is
    // recorded, the action has not ended, and the grace has not run out. endedMs is 0 while the
    // action runs. A clock that moved backwards counts as expired, never as open forever.
    static bool IsWriteOpen(string writerId, int openedMs, int endedMs, int nowMs)
    {
        if (writerId == "")
        {
            return false;
        }
        if (endedMs != 0)
        {
            return false;
        }
        int age = nowMs - openedMs;
        if (age < 0)
        {
            return false;
        }
        return age < WRITE_GRACE_MS;
    }

    // N17: whether a write RPC from senderId is the recorded writer's: the action is still
    // running inside the grace, or it ended less than WRITE_LATE_MS ago.
    static bool AcceptsWriteFrom(string writerId, int openedMs, int endedMs, int nowMs, string senderId)
    {
        if (writerId == "" || senderId == "")
        {
            return false;
        }
        if (writerId != senderId)
        {
            return false;
        }
        if (endedMs == 0)
        {
            return IsWriteOpen(writerId, openedMs, endedMs, nowMs);
        }
        int sinceEnd = nowMs - endedMs;
        if (sinceEnd < 0)
        {
            return false;
        }
        return sinceEnd < WRITE_LATE_MS;
    }

    // N17: a start is allowed when no write is open, or the open write is this sender's own
    // (a re-start by the same player replaces their own record).
    static bool CanStartWrite(string writerId, int openedMs, int endedMs, int nowMs, string senderId)
    {
        if (!IsWriteOpen(writerId, openedMs, endedMs, nowMs))
        {
            return true;
        }
        return writerId == senderId;
    }

    // N18: a target held by the writer is always in reach; a world target must be within
    // WRITE_REACH_METERS of the writer's head or feet, whichever is closer.
    static bool WriteTargetInReach(bool heldByWriter, float distanceFromFeet, float distanceFromHead)
    {
        if (heldByWriter)
        {
            return true;
        }
        if (distanceFromFeet <= WRITE_REACH_METERS)
        {
            return true;
        }
        return distanceFromHead <= WRITE_REACH_METERS;
    }

    // 4.3: the first line in a window passes; later lines in the same window are suppressed.
    // lastMs 0 means nothing was written yet. A clock that moved backwards passes.
    static bool GateAllows(int lastMs, int nowMs, int windowMs)
    {
        if (lastMs == 0)
        {
            return true;
        }
        int since = nowMs - lastMs;
        if (since < 0)
        {
            return true;
        }
        return since >= windowMs;
    }

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

    // N15: what the server does with one RPC_WRITE_NOTE_CLIENT. An unreadable stream, a
    // sender who is not the recorded writer (N17), and a sheet that is not a single sheet
    // (N2; the action condition checks it on the honest path, the RPC path re-checks it) are
    // refused and store nothing. Otherwise the text is capped at NOTE_MAX (truncated, not
    // refused: an unmodified client already cuts at 512 before sending, NoteMenu.c:118) and
    // stored; ink is taken only when a pen is bound, because DepleteWritingImplement takes a
    // notnull pen (WrittenNoteData.c:83).
    static NimbyNotesWriteOutcome WriteOutcome(string text, bool hasPen, bool readable, bool isWriter, float quantity)
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
        if (!CanWrite(quantity))
        {
            outcome.m_Verdict = WRITE_NOT_SINGLE;
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
        failures += Case(results, "cap keeps a shorter text", CapText("abc", 5) == "abc");
        failures += Case(results, "cap keeps a text of exactly the cap", CapText("abcde", 5) == "abcde");
        failures += Case(results, "cap cuts a longer text", CapText("abcdefg", 5) == "abcde");
        failures += Case(results, "cap keeps an empty text", CapText("", 5) == "");
        string longText = Repeat("x", NOTE_MAX + 1);
        string capped = CapText(longText, NOTE_MAX);
        failures += Case(results, "cap cuts 513 characters to 512", capped.Length() == NOTE_MAX);
        failures += SelfTestWriteOutcome(results, longText);
        failures += SelfTestWriteOpen(results);
        failures += SelfTestReachAndGate(results);
        return failures;
    }

    // N15 cases: the refusals store nothing, the cap flags a cut, the ink follows the pen.
    protected static int SelfTestWriteOutcome(out array<string> results, string longText)
    {
        int failures = 0;
        NimbyNotesWriteOutcome unreadable = WriteOutcome("abc", true, false, true, 1);
        failures += Case(results, "write: unreadable stream is refused", unreadable.m_Verdict == WRITE_UNREADABLE && unreadable.IsRefused());
        failures += Case(results, "write: refused write stores nothing", unreadable.m_Text == "" && !unreadable.m_TakeInk);
        NimbyNotesWriteOutcome stranger = WriteOutcome("abc", true, true, false, 1);
        failures += Case(results, "write: sender who is not the writer is refused", stranger.m_Verdict == WRITE_NOT_WRITER && stranger.m_Text == "");
        NimbyNotesWriteOutcome stack = WriteOutcome("abc", true, true, true, 5);
        failures += Case(results, "write: a stack of five is refused on the rpc path", stack.m_Verdict == WRITE_NOT_SINGLE && stack.IsRefused() && stack.m_Text == "");
        NimbyNotesWriteOutcome empty = WriteOutcome("abc", true, true, true, 0);
        failures += Case(results, "write: an empty stack is refused on the rpc path", empty.m_Verdict == WRITE_NOT_SINGLE);
        NimbyNotesWriteOutcome stored = WriteOutcome("abc", true, true, true, 1);
        failures += Case(results, "write: text with a pen is stored and takes ink", stored.m_Verdict == WRITE_STORED && stored.m_Text == "abc" && stored.m_TakeInk);
        failures += Case(results, "write: text under the cap is not flagged capped", !stored.m_Capped && !stored.IsRefused());
        NimbyNotesWriteOutcome noPen = WriteOutcome("abc", false, true, true, 1);
        failures += Case(results, "write: missing pen stores the text and skips the ink", noPen.m_Verdict == WRITE_STORED_NO_INK && noPen.m_Text == "abc" && !noPen.m_TakeInk);
        NimbyNotesWriteOutcome over = WriteOutcome(longText, true, true, true, 1);
        failures += Case(results, "write: 513 characters are stored as 512 and flagged capped", over.m_Verdict == WRITE_STORED && over.m_Text.Length() == NOTE_MAX && over.m_Capped);
        return failures;
    }

    // N17 cases: one open write per sheet, the grace, the late window, the same-writer restart.
    protected static int SelfTestWriteOpen(out array<string> results)
    {
        int failures = 0;
        failures += Case(results, "open: no writer means no open write", !IsWriteOpen("", 1000, 0, 2000));
        failures += Case(results, "open: a running write inside the grace is open", IsWriteOpen("A", 1000, 0, 1000 + WRITE_GRACE_MS - 1));
        failures += Case(results, "open: a running write at the grace is expired", !IsWriteOpen("A", 1000, 0, 1000 + WRITE_GRACE_MS));
        failures += Case(results, "open: an ended write is not open", !IsWriteOpen("A", 1000, 1500, 1600));
        failures += Case(results, "open: a clock rewind counts as expired", !IsWriteOpen("A", 5000, 0, 4000));
        failures += Case(results, "start: anyone may start on a sheet with no writer", CanStartWrite("", 0, 0, 1000, "B"));
        failures += Case(results, "start: a second player is refused while a write is open", !CanStartWrite("A", 1000, 0, 2000, "B"));
        failures += Case(results, "start: the writer may restart their own open write", CanStartWrite("A", 1000, 0, 2000, "A"));
        failures += Case(results, "start: a second player may start after the action ended", CanStartWrite("A", 1000, 1500, 1600, "B"));
        failures += Case(results, "start: a second player may start after the grace", CanStartWrite("A", 1000, 0, 1000 + WRITE_GRACE_MS, "B"));
        failures += Case(results, "accept: the writer's rpc during the action is accepted", AcceptsWriteFrom("A", 1000, 0, 2000, "A"));
        failures += Case(results, "accept: a stranger's rpc during the action is refused", !AcceptsWriteFrom("A", 1000, 0, 2000, "B"));
        failures += Case(results, "accept: the writer's rpc just after the action ended is accepted", AcceptsWriteFrom("A", 1000, 1500, 1500 + WRITE_LATE_MS - 1, "A"));
        failures += Case(results, "accept: the writer's rpc long after the action ended is refused", !AcceptsWriteFrom("A", 1000, 1500, 1500 + WRITE_LATE_MS, "A"));
        failures += Case(results, "accept: an rpc with no writer recorded is refused", !AcceptsWriteFrom("", 0, 0, 1000, "A"));
        failures += Case(results, "accept: an rpc from an empty sender id is refused", !AcceptsWriteFrom("A", 1000, 0, 2000, ""));
        return failures;
    }

    // N18 and 4.3 cases.
    protected static int SelfTestReachAndGate(out array<string> results)
    {
        int failures = 0;
        failures += Case(results, "reach: a sheet the writer holds is in reach at any distance", WriteTargetInReach(true, 300, 300));
        failures += Case(results, "reach: a world sheet at the feet is in reach", WriteTargetInReach(false, 0.5, 1.9));
        failures += Case(results, "reach: a wall note at head height is in reach", WriteTargetInReach(false, 2.4, 1.6));
        failures += Case(results, "reach: a world sheet at two metres is in reach", WriteTargetInReach(false, 2.0, 2.3));
        failures += Case(results, "reach: a world sheet past two metres is out of reach", !WriteTargetInReach(false, 2.01, 2.2));
        failures += Case(results, "reach: a sheet across the field is out of reach", !WriteTargetInReach(false, 300, 300));
        failures += Case(results, "gate: the first line passes", GateAllows(0, 5000, LOG_GATE_MS));
        failures += Case(results, "gate: a line inside the window is suppressed", !GateAllows(5000, 5000 + LOG_GATE_MS - 1, LOG_GATE_MS));
        failures += Case(results, "gate: a line at the window passes", GateAllows(5000, 5000 + LOG_GATE_MS, LOG_GATE_MS));
        failures += Case(results, "gate: a clock rewind passes", GateAllows(9000, 8000, LOG_GATE_MS));
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
