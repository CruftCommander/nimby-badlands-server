// -----------------------------------------------------------------------------
// NimbyNotes - engine-free rules.
// Purpose : The combine, split, and write predicates of Scrawl N2 as pure functions,
//           so they are unit-tested by the self-test and the 4_World layer only
//           feeds them the facts it reads from the items.
// -----------------------------------------------------------------------------
class NimbyNotesRules
{
    static const int STACK_MAX = 5;

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
    // Rewriting a written note is vanilla behavior and stays allowed: the recipe passes the old
    // text to the menu (WrittenNoteData.c:33).
    static bool CanWrite(float quantity)
    {
        return quantity == 1;
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
