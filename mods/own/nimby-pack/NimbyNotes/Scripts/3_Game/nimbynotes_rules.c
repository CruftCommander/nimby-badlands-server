// -----------------------------------------------------------------------------
// NimbyNotes - engine-free rules.
// Purpose : The combine, split, and write predicates of Scrawl N2 and the N12 length
//           cap as pure functions, so they are unit-tested by the self-test and the
//           4_World and 5_Mission layers only feed them the facts they read.
// N12     : The cap is vanilla's own. MiscGameplayFunctions.SanitizeString keeps the
//           first 512 characters of a note (MiscGameplayFunctions.c:930-937), on the
//           client before the text is sent (NoteMenu.c:118) and on the server when it
//           is stored (WrittenNoteData.c:80). Jeff ruled 512 on October 19, 2026, after
//           that was found: the 1000 ruled on October 17 could never take effect.
//           CapText cuts the same way (Length and Substring), so the editor, the
//           server log, and vanilla agree on where a note ends.
// -----------------------------------------------------------------------------
class NimbyNotesRules
{
    static const int STACK_MAX = 5;
    static const int NOTE_MAX = 512;

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
    static bool CanWrite(float quantity)
    {
        return quantity == 1;
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
        failures += Case(results, "written note does not split", !CanSplit(true, 2));
        failures += Case(results, "write on one sheet", CanWrite(1));
        failures += Case(results, "no write on a stack", !CanWrite(2));
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
