// -----------------------------------------------------------------------------
// NimbyNotes - player load backstop (N15).
// Purpose : After a player loads from the database, syncs the written flag of every
//           sheet in the player's inventory, so the client refuses Combine on a
//           written note carried over a restart (NIMBY-T-026).
// Vanilla : PlayerBase.AfterStoreLoad runs on the server after the player and its
//           inventory load (PlayerBase.c:7531-7533; DayZPlayerInventory.c:4289-4296,
//           "engine reaction to load from database"). GameInventory.EnumerateInventory
//           (Inventory.c:127, InventoryTraversalType at gameplay.c:5-11) walks the
//           player's inventory hierarchy; Test 23 keeps the note in a pocket (cargo).
// Backstop: Paper.AfterStoreLoad (nimbynotes_paper.c) should already have synced each
//           sheet, and NimbyNotesSyncWritten logs only on a change, so this prints
//           "from=player-load" only when the per-sheet event missed. Server only;
//           adds no field and writes no byte (shared rules 6.1).
// -----------------------------------------------------------------------------

modded class PlayerBase
{
    override void AfterStoreLoad()
    {
        super.AfterStoreLoad();
        if (!GetGame().IsServer())
        {
            return;
        }
        array<EntityAI> items = new array<EntityAI>;
        GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
        foreach (EntityAI item : items)
        {
            Paper sheet = Paper.Cast(item);
            if (sheet)
            {
                sheet.NimbyNotesSyncWritten("player-load");
            }
        }
    }
}
