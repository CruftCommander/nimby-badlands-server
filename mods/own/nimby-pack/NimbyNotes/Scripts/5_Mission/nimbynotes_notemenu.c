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
        mission.RemoveActiveInputExcludes({"inventory"}, false);
        mission.RemoveActiveInputRestriction(EInputRestrictors.INVENTORY);
        Print("[NIMBY][Notes] note menu closed; inventory input exclude and restriction removed");
    }
}
