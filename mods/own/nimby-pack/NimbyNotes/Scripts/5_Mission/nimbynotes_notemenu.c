// -----------------------------------------------------------------------------
// NimbyNotes - note menu: input restore on close (N11, second cause) and the
// editor length cap (N12).
// Purpose : While the note menu is open, MissionGameplay.OnUpdate adds the
//           "inventory" input exclude and the INVENTORY input restriction
//           (missionGameplay.c:662-666) and nothing removes them when the menu
//           closes: HideInventory does that for the inventory menu
//           (missionGameplay.c:1216-1217), the note menu has no counterpart. The
//           player keeps WASD but loses mouse look and jump until the inventory is
//           opened and closed once (seen October 17, 2026, every read). This
//           override removes both when the note menu hides, the same two calls
//           HideInventory makes.
// Guard   : When the inventory menu is itself open underneath, the exclude and the
//           restriction belong to the inventory and HideInventory removes them
//           later; stripping them here would free mouse look and jump under an
//           open inventory (self-review S2). The N9 inventory write does not reach
//           this case: setting the write from the inventory closes the inventory
//           first (InventoryActionHandler.c:60). The guard stays for any menu that
//           does open over it.
// N12     : The editor holds at most NimbyNotesRules.NOTE_MAX (512) characters, the
//           same cut vanilla makes on send (NoteMenu.c:118) and on the server
//           (WrittenNoteData.c:80), so the player sees where the note ends instead
//           of losing the tail silently on confirm. MultilineEditBoxWidget has no
//           length limit and no placeholder or hint (EnWidgets.c:313-321), so the
//           cap is a truncation: every frame the text is read (GetText, :318) and,
//           if longer, set back to the first 512 characters (SetText, the call
//           InitNoteWrite uses at NoteMenu.c:78). No hint is shown: a counter
//           needs a new widget in the vanilla layout, which is editor work for N10.
//           Where the caret lands after SetText is not documented in the
//           reference; SC-2a-5 records it.
// Client  : UIScriptedMenu exists only on the client; the server never builds one.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

modded class NoteMenu
{
    protected bool m_NimbyNotesCapReported;

    // Runs before the vanilla Update, which may Close the menu on UAUIBack (NoteMenu.c:130-138).
    override void Update(float timeslice)
    {
        NimbyNotesCapEditor();
        super.Update(timeslice);
    }

    protected void NimbyNotesCapEditor()
    {
        if (!m_IsWriting || !m_edit)
        {
            return;
        }
        string text;
        m_edit.GetText(text);
        int length = text.Length();
        int cap = NimbyNotesRules.NOTE_MAX;
        if (length <= cap)
        {
            return;
        }
        m_edit.SetText(NimbyNotesRules.CapText(text, cap));
        if (!m_NimbyNotesCapReported)
        {
            m_NimbyNotesCapReported = true;
            NimbyNotesLog.Debug("editor capped from=" + length.ToString() + " to=" + cap.ToString());
        }
    }

    override void OnHide()
    {
        super.OnHide();
        Mission mission = g_Game.GetMission();
        if (!mission)
        {
            return;
        }
        UIManager ui = g_Game.GetUIManager();
        if (ui && ui.FindMenu(MENU_INVENTORY))
        {
            NimbyNotesLog.Debug("note menu closed over the inventory; input state left to HideInventory");
            return;
        }
        mission.RemoveActiveInputExcludes({"inventory"}, false);
        mission.RemoveActiveInputRestriction(EInputRestrictors.INVENTORY);
        NimbyNotesLog.Debug("note menu closed; inventory input exclude and restriction removed");
    }
}
