// -----------------------------------------------------------------------------
// NimbyNotes - action registration and the write rule.
// Purpose : (N5) Registers ActionWritePaper and ActionReadPaper with the action
//           manager so AddAction on Paper and Pen_ColorBase resolves them; (N2)
//           restricts writing to a single sheet.
// Vanilla : ActionConstructor.RegisterActions ActionConstructor.c:27;
//           ActionWritePaper.ActionCondition ActionWritePaper.c:57-59.
// -----------------------------------------------------------------------------
modded class ActionConstructor
{
    override void RegisterActions(TTypenameArray actions)
    {
        super.RegisterActions(actions);
        actions.Insert(ActionWritePaper);
        actions.Insert(ActionReadPaper);
    }
}

modded class ActionWritePaper
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (!super.ActionCondition(player, target, item))
        {
            return false;
        }
        // The sheet is whichever side is not the pen (ActionWritePaper.c:57).
        ItemBase sheet = item;
        if (item && item.ConfigIsExisting("writingColor"))
        {
            sheet = ItemBase.Cast(target.GetObject());
        }
        if (!sheet)
        {
            return false;
        }
        return NimbyNotesRules.CanWrite(sheet.GetQuantity());
    }
}
