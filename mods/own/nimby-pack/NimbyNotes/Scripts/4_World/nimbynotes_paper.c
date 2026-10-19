// -----------------------------------------------------------------------------
// NimbyNotes - Paper.
// Purpose : Applies the N2 rules to the vanilla Paper item: a sheet with text never
//           combines or splits, and the read action is reachable from the sheet.
// Vanilla : CanBeCombined ItemBase.c:2137, CanBeSplit ItemBase.c:1533,
//           note text WrittenNoteData.c:73, Paper.SetActions Paper.c.
// Save    : Paper.OnStoreSave writes the whole WrittenNoteData object (Paper.c:27),
//           pen and paper references included. ActionWritePaper fills those two
//           references when the write menu opens (ActionWritePaper.c:70-88) and
//           nothing in vanilla clears them, so a written sheet saves the pen's
//           entity id; after a restart that id belongs to something else and
//           ctx.Read throws "Instance ContaminatedTrigger is not inherited from
//           ItemBase" at Paper.c:17 (live-copy world, October 19, 2026). Vanilla
//           never hit it because the write action is unregistered in 1.30. The
//           references are cleared when a write completes and again right before
//           every save; the saved layout is unchanged (same object, null
//           references), so shared rules 6.1 are kept. Window left open: a save
//           that lands while a write menu is open clears the pen before the write
//           RPC depletes it; the text still lands, the pen keeps its ink once.
// -----------------------------------------------------------------------------
modded class WrittenNoteData
{
    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, rpc_type, ctx);
        if (rpc_type == ERPCs.RPC_WRITE_NOTE_CLIENT)
        {
            InitNoteInfo(null, null);
            NimbyNotesLog.Debug("write finished; pen and paper references cleared");
        }
    }
}

modded class Paper
{
    override void OnStoreSave(ParamsWriteContext ctx)
    {
        WrittenNoteData data = GetWrittenNoteData();
        if (data)
        {
            data.InitNoteInfo(null, null);
        }
        super.OnStoreSave(ctx);
    }

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
