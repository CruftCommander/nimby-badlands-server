// -----------------------------------------------------------------------------
// NimbyNotes - Paper.
// Purpose : Applies the N2 rules to the vanilla Paper item: a sheet with text never
//           combines or splits, and the read and write actions are reachable from
//           the sheet (N5, N9). Caps the stored note text on the server (N12).
// Vanilla : CanBeCombined ItemBase.c:2137, CanBeSplit ItemBase.c:1533,
//           note text WrittenNoteData.c:73, Paper.SetActions Paper.c:65-73,
//           ItemBase.OnRPC hands every item RPC to the note data (ItemBase.c:2831-2834).
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
// N12     : Scrawl handles RPC_WRITE_NOTE_CLIENT itself instead of passing it to
//           vanilla (WrittenNoteData.c:37-45), because the text has to be measured
//           before it is stored and the stream can be read only once. The body is
//           vanilla's three steps (read, SetNoteText, DepleteWritingImplement) plus
//           the cap and the log. A text over NimbyNotesRules.NOTE_MAX is truncated,
//           not refused: an unmodified client already cuts at 512 before sending
//           (NoteMenu.c:118) and vanilla SetNoteText would cut it again
//           (WrittenNoteData.c:80), so only a modified client can send more, and
//           truncating is what vanilla does with it anyway; the Info line records
//           the attempt. A missing pen reference (the save window above, or a
//           forged RPC) skips the ink instead of passing null to the notnull
//           parameter of DepleteWritingImplement (WrittenNoteData.c:83).
// Ink     : DepleteWritingImplement takes one quantity per character added
//           (WrittenNoteData.c:83-88), once per confirmed write.
// -----------------------------------------------------------------------------
modded class WrittenNoteData
{
    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        if (rpc_type != ERPCs.RPC_WRITE_NOTE_CLIENT)
        {
            super.OnRPC(sender, rpc_type, ctx);
            return;
        }
        NimbyNotesWrite(sender, ctx);
    }

    protected void NimbyNotesWrite(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<string> param = new Param1<string>("");
        if (!ctx.Read(param))
        {
            InitNoteInfo(null, null);
            NimbyNotesLog.Info("write refused: text unreadable " + NimbyNotesLog.PlayerTag(sender));
            return;
        }
        string text = param.param1;
        int sent = text.Length();
        string capped = NimbyNotesRules.CapText(text, NimbyNotesRules.NOTE_MAX);
        int kept = capped.Length();
        if (kept < sent)
        {
            NimbyNotesLog.Info("write capped " + NimbyNotesLog.PlayerTag(sender) + " from=" + sent.ToString() + " to=" + kept.ToString());
        }
        string oldText = m_SimpleText;
        SetNoteText(capped);
        int oldLength = oldText.Length();
        int newLength = m_SimpleText.Length();
        if (m_WritingImplement)
        {
            float inkBefore = m_WritingImplement.GetQuantity();
            DepleteWritingImplement(m_WritingImplement, oldText, m_SimpleText);
            float inkAfter = m_WritingImplement.GetQuantity();
            string inkLine = "write ink: pen=" + m_WritingImplement.GetType() + " quantity " + inkBefore.ToString() + " -> " + inkAfter.ToString();
            inkLine = inkLine + " health=" + m_WritingImplement.GetHealth().ToString() + " length " + oldLength.ToString() + " -> " + newLength.ToString();
            NimbyNotesLog.Debug(inkLine);
        }
        else
        {
            NimbyNotesLog.Debug("write ink: no pen reference; text stored, ink not taken; length " + oldLength.ToString() + " -> " + newLength.ToString());
        }
        InitNoteInfo(null, null);
        NimbyNotesLog.Debug("write finished; pen and paper references cleared");
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
        // N9: the write action on the sheet too, for the sheet-in-hands order. The inventory
        // Combine zone always targets the item in hands (Icon.c:719-720) and offers only
        // that item's actions (ActionManagerClient.c:1199-1282), so a pen dropped on a sheet
        // in hands needs the action here. Vanilla wrote the condition and the client start
        // for this order already (ActionWritePaper.c:59 second clause, :68-70). It also
        // gives the world order: sheet in hands, looking at a pen on the ground.
        AddAction(ActionWritePaper);
    }
}
