// -----------------------------------------------------------------------------
// NimbyNotes - Paper.
// Purpose : Applies the N2 rules to the vanilla Paper item: a sheet with text never
//           combines or splits, and the read action is reachable from the sheet.
// Vanilla : CanBeCombined ItemBase.c:2137, CanBeSplit ItemBase.c:1533,
//           note text WrittenNoteData.c:73, Paper.SetActions Paper.c.
// -----------------------------------------------------------------------------
modded class Paper
{
    bool NimbyNotesHasText()
    {
        WrittenNoteData data = GetWrittenNoteData();
        if (!data)
        {
            return false;
        }
        string text = data.GetNoteText();
        return text != "";
    }

    override bool CanBeCombined(EntityAI other_item, bool reservation_check = true, bool stack_max_limit = false)
    {
        Paper other = Paper.Cast(other_item);
        bool otherHasText = false;
        if (other)
        {
            otherHasText = other.NimbyNotesHasText();
        }
        if (!NimbyNotesRules.CanCombine(NimbyNotesHasText(), otherHasText))
        {
            return false;
        }
        return super.CanBeCombined(other_item, reservation_check, stack_max_limit);
    }

    override bool CanBeSplit()
    {
        if (!NimbyNotesRules.CanSplit(NimbyNotesHasText(), GetQuantity()))
        {
            return false;
        }
        return super.CanBeSplit();
    }

    override void SetActions()
    {
        super.SetActions();
        // N5: 1.30 ships ActionReadPaper but registers it nowhere (ActionConstructor.c lists
        // neither note action; Paper.SetActions adds only fireplace and attach actions).
        AddAction(ActionReadPaper);
    }
}
