// -----------------------------------------------------------------------------
// NimbyNotes - note menu close (N11, second cause).
// Purpose : While the note menu is open, MissionGameplay.OnUpdate adds the
//           "inventory" input exclude and the INVENTORY input restriction
//           (missionGameplay.c:662-666) and nothing removes them when the menu
//           closes: HideInventory does that for the inventory menu
//           (missionGameplay.c:1216-1217), the note menu has no counterpart. The
//           player keeps WASD but loses mouse look and jump until the inventory is
//           opened and closed once (seen October 17, 2026, every read). This
//           override removes both when the note menu hides, the same two calls
//           HideInventory makes.
// Guard   : When the inventory menu is itself open underneath (N9 will open the
//           note menu from a pen-and-paper combine in the inventory), the exclude
//           and the restriction belong to the inventory and HideInventory removes
//           them later; stripping them here would free mouse look and jump under
//           an open inventory (self-review S2).
// Client  : UIScriptedMenu exists only on the client; the server never builds one.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

modded class NoteMenu
{
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
