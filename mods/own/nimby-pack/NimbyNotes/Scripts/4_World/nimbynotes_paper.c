// -----------------------------------------------------------------------------
// NimbyNotes - Paper.
// Purpose : Applies the N2 rules to the vanilla Paper item: a sheet with text never
//           combines or splits, and the read and write actions are reachable from
//           the sheet (N5, N9). Caps the stored note text on the server (N12).
//           Carries the net-synced written flag (N15) so the client refuses the
//           combine too, and accepts a write only from the writer (N16).
// Vanilla : CanBeCombined ItemBase.c:2137, CanBeSplit ItemBase.c:1533,
//           note text WrittenNoteData.c:73, Paper.SetActions Paper.c:65-73,
//           ItemBase.OnRPC hands every item RPC to the note data (ItemBase.c:2831-2834).
//           The server re-runs CanBeCombined before any quantity moves: CombineItems
//           opens with it (ItemBase.c:2243-2246) ahead of AddQuantity (:2262-2263),
//           and both server entries reach it, the inventory manipulation input
//           (PlayerBase.c:6665-6670) and the attachment-slot combine
//           (Inventory.c:1109-1111). 1.30 script has no separate quick-stack path.
// Save    : Paper.OnStoreSave writes the whole WrittenNoteData object (Paper.c:27),
//           pen and paper references included. ActionWritePaper fills those two
//           references when the write menu opens (ActionWritePaper.c:78-90) and
//           nothing in vanilla clears them, so a written sheet saves the pen's
//           entity id; after a restart that id belongs to something else and
//           ctx.Read throws "Instance ContaminatedTrigger is not inherited from
//           ItemBase" at Paper.c:17 (live-copy world, October 19, 2026). Vanilla
//           never hit it because the write action is unregistered in 1.30. The
//           override below nulls both references before super and restores them
//           after; the bytes written are vanilla's (same object, null references).
//           Shared rules 6.1 intent kept; letter excepted by N14. Restoring after
//           super closes the window the October 19 fix left open: a save that
//           landed while a write menu was open cleared the pen before the write
//           RPC arrived (the October 21 "no pen reference" line, review S-3).
// Load    : The flag follows the loaded text from two vanilla events. AfterStoreLoad
//           runs for an entity loaded from the player database or the world storage,
//           after its children (EntityAI.c:1418-1419); vanilla restores every item's
//           saved quantity there (ItemBase.c:3218-3231), so carried items should too.
//           EEOnAfterLoad is the "connected system" event of the world storage
//           (EntityAI.c:1380-1381) and never reached a note restored with its player
//           (NIMBY-T-026, October 24, 2026). Both stay; the sync logs only on a change.
//           nimbynotes_playerbase.c syncs again from PlayerBase.AfterStoreLoad as a
//           backstop and logs from=player-load only if this file's events missed.
// Owner   : The vanilla m_Paper reference always names the sheet the data belongs
//           to when vanilla binds it (ActionWritePaper.c:84,88); Scrawl binds it
//           from construction and again after load, so modded SetNoteText can
//           reach the owning sheet for the flag sync from any caller (Tack's pin
//           and take-down call SetNoteText with no action bound). It never reaches
//           the save because OnStoreSave nulls both references before super.
// N12     : Scrawl handles RPC_WRITE_NOTE_CLIENT itself instead of passing it to
//           vanilla (WrittenNoteData.c:37-45), because the text has to be measured
//           before it is stored and the stream can be read only once. The body is
//           vanilla's three steps (read, SetNoteText, DepleteWritingImplement) plus
//           the cap, the writer check, and the log. A text over NimbyNotesRules.NOTE_MAX
//           is truncated, not refused: an unmodified client already cuts at 512 before
//           sending (NoteMenu.c:118) and vanilla SetNoteText would cut it again
//           (WrittenNoteData.c:80), so only a modified client can send more, and
//           truncating is what vanilla does with it anyway; the Info line records
//           the attempt. A missing pen reference (a pen deleted mid-write, or a
//           forged RPC on a sheet the sender holds) skips the ink instead of passing
//           null to the notnull parameter of DepleteWritingImplement
//           (WrittenNoteData.c:83) and is a production audit line.
// N16     : The writer is the root player of the bound pen, else of the sheet
//           (EntityAI.c:882). The three supported orders keep at least one of them
//           on the writer: pen in hands with the sheet anywhere, sheet in hands with
//           the pen in the inventory, sheet in hands with the pen on the ground.
//           A sender who is neither is refused at Info with both ids.
// Ink     : DepleteWritingImplement takes one quantity per character added
//           (WrittenNoteData.c:83-88), once per confirmed write.
// -----------------------------------------------------------------------------

// This class MUST NOT gain a member variable: vanilla Paper.OnStoreSave serializes the
// object whole (Paper.c:27; the October 19, 2026 incident proved that entity references in
// it reach the save), so any field added here is saved data on a vanilla class, which
// shared rules 6.1 forbids. State that belongs to the sheet goes on modded Paper below, as a
// net-synced variable or in-memory only.
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

    // N15: every text change, from Scrawl's write or from another addon's SetNoteText, syncs
    // the owning sheet's written flag. Vanilla calls this on the client too (RPC_WRITE_NOTE and
    // RPC_READ_NOTE, WrittenNoteData.c:22,50); the sync returns at once there.
    override void SetNoteText(string text)
    {
        super.SetNoteText(text);
        Paper owner = Paper.Cast(m_Paper);
        if (owner)
        {
            owner.NimbyNotesSyncWritten("text");
        }
    }

    // Binds the vanilla owner reference without touching a bound pen.
    void NimbyNotesBindOwner(ItemBase owner)
    {
        InitNoteInfo(m_WritingImplement, owner);
    }

    // Clears the pen after a write or a refusal; the owner reference stays.
    void NimbyNotesClearPen()
    {
        InitNoteInfo(null, m_Paper);
    }

    // N16: the writer is the root player of the pen when the pen is held or carried, else the
    // root player of the sheet (the pen-on-the-ground order).
    protected bool NimbyNotesSenderIsWriter(PlayerIdentity sender)
    {
        if (!sender)
        {
            return false;
        }
        Man writer = null;
        if (m_WritingImplement)
        {
            writer = m_WritingImplement.GetHierarchyRootPlayer();
        }
        if (!writer && m_Paper)
        {
            writer = m_Paper.GetHierarchyRootPlayer();
        }
        if (!writer)
        {
            return false;
        }
        PlayerIdentity identity = writer.GetIdentity();
        if (!identity)
        {
            return false;
        }
        return identity.GetId() == sender.GetId();
    }

    protected string NimbyNotesSheetType()
    {
        if (!m_Paper)
        {
            return "none";
        }
        return m_Paper.GetType();
    }

    protected void NimbyNotesWrite(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<string> param = new Param1<string>("");
        bool readable = ctx.Read(param);
        bool hasPen = false;
        if (m_WritingImplement)
        {
            hasPen = true;
        }
        bool isWriter = NimbyNotesSenderIsWriter(sender);
        string tag = NimbyNotesLog.PlayerTag(sender);
        string sheetType = NimbyNotesSheetType();
        NimbyNotesWriteOutcome outcome = NimbyNotesRules.WriteOutcome(param.param1, hasPen, readable, isWriter);
        if (outcome.m_Verdict == NimbyNotesRules.WRITE_UNREADABLE)
        {
            NimbyNotesClearPen();
            NimbyNotesLog.Info("write refused: text unreadable sheet=" + sheetType + " " + tag);
            return;
        }
        if (outcome.m_Verdict == NimbyNotesRules.WRITE_NOT_WRITER)
        {
            NimbyNotesClearPen();
            NimbyNotesLog.Info("write refused: sender is not the writer sheet=" + sheetType + " " + tag);
            return;
        }
        int sent = param.param1.Length();
        int kept = outcome.m_Text.Length();
        if (outcome.m_Capped)
        {
            NimbyNotesLog.Info("write capped " + tag + " from=" + sent.ToString() + " to=" + kept.ToString());
        }
        string oldText = m_SimpleText;
        SetNoteText(outcome.m_Text);
        int oldLength = oldText.Length();
        int newLength = m_SimpleText.Length();
        if (outcome.m_TakeInk)
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
            string noPenLine = "write ink: no pen reference; text stored, ink not taken; sheet=" + sheetType;
            noPenLine = noPenLine + " length " + oldLength.ToString() + " -> " + newLength.ToString() + " " + tag;
            NimbyNotesLog.Info(noPenLine);
        }
        NimbyNotesClearPen();
        NimbyNotesLog.Debug("write finished; pen reference cleared");
    }
}

modded class Paper
{
    // N15: net-synced, never persisted (shared rules 6, second bullet). The server sets it
    // from the note text; the client reads it in CanBeCombined, where the text itself never
    // arrives (review S-1). Registered the way ItemBase registers its own in its constructor
    // (ItemBase.c:125,203-205); Hands registers two on PlayerBase the same way.
    protected bool m_NimbyNotesWritten;

    void Paper()
    {
        RegisterNetSyncVariableBool("m_NimbyNotesWritten");
        NimbyNotesBindOwner();
    }

    // Entity event after a load from the player database or the world storage, after the
    // children (EntityAI.c:1418-1419, ItemBase.c:3218), not OnStoreLoad: the loaded note data
    // carries null references, so the owner is bound again and the flag follows the loaded
    // text. This is the event expected to reach a note restored in a player's inventory
    // (NIMBY-T-026; Test 23 proves it).
    override void AfterStoreLoad()
    {
        super.AfterStoreLoad();
        NimbyNotesBindOwner();
        NimbyNotesSyncWritten("store-load");
    }

    // World-storage event (EntityAI.c:1380-1381, ItemBase.c:3234). Kept: when AfterStoreLoad
    // has already synced the sheet, this binds again and logs nothing.
    override void EEOnAfterLoad()
    {
        super.EEOnAfterLoad();
        NimbyNotesBindOwner();
        NimbyNotesSyncWritten("load");
    }

    protected void NimbyNotesBindOwner()
    {
        WrittenNoteData data = GetWrittenNoteData();
        if (data)
        {
            data.NimbyNotesBindOwner(this);
        }
    }

    // Server only. Dirty only on change, so an unchanged text costs no sync.
    void NimbyNotesSyncWritten(string from)
    {
        if (!GetGame().IsServer())
        {
            return;
        }
        bool written = NimbyNotesHasText();
        if (written == m_NimbyNotesWritten)
        {
            return;
        }
        m_NimbyNotesWritten = written;
        SetSynchDirty();
        NimbyNotesLog.Debug("written flag synced from=" + from + " written=" + NimbyNotesLog.Bool(written) + " sheet=" + GetType());
    }

    // N14: writes no byte, adds no field; nulls two in-memory references before super and
    // restores them after. Never add a ctx.Write here.
    override void OnStoreSave(ParamsWriteContext ctx)
    {
        WrittenNoteData data = GetWrittenNoteData();
        ItemBase pen = null;
        ItemBase sheet = null;
        if (data)
        {
            data.GetNoteInfo(pen, sheet);
            if (pen)
            {
                NimbyNotesLog.Debug("save cleared a live write binding on " + GetType() + " pen=" + pen.GetType());
            }
            data.InitNoteInfo(null, null);
        }
        super.OnStoreSave(ctx);
        if (data)
        {
            data.InitNoteInfo(pen, sheet);
        }
    }

    // The flag on the client, the text itself on the server.
    bool NimbyNotesHasText()
    {
        if (!GetGame().IsServer())
        {
            return m_NimbyNotesWritten;
        }
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
            if (GetGame().IsServer() && other_item)
            {
                NimbyNotesLog.Info("combine refused: a sheet carries text; this=" + GetType() + " other=" + other_item.GetType());
            }
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
