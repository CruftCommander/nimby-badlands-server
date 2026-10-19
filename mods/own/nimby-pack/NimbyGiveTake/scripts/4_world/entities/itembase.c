modded class ItemBase
{
    bool NimbyCanBeOffered()
    {
        return !IsHeavyBehaviour();
    }

    bool NimbyCanBeGiven()
    {
        return !IsHeavyBehaviour();
    }

    // Completion signal 1 (Phase 0 item 4). Vanilla calls OnItemLocationChanged and, through it,
    // OnInventoryExit then OnInventoryEnter from inside this event (EntityAI.c:976-980,
    // ItemBase.c:1162-1244), so the begin and end markers bracket them in the debug trace. All
    // 21 vanilla overrides call super, so every item class reaches this.
    override void EEItemLocationChanged(notnull InventoryLocation oldLoc, notnull InventoryLocation newLoc)
    {
        NimbyGiveTakeServer.OnItemLocationChangedBegin(this, oldLoc, newLoc);
        super.EEItemLocationChanged(oldLoc, newLoc);
        NimbyGiveTakeServer.OnItemLocationChangedEnd(this, oldLoc, newLoc);
    }

    // Completion signal candidates 3a and 3b: traced behind the debug flag only.
    override void OnInventoryExit(Man player)
    {
        super.OnInventoryExit(player);
        NimbyGiveTakeServer.OnInventoryExitSignal(this, player);
    }

    override void OnInventoryEnter(Man player)
    {
        super.OnInventoryEnter(player);
        NimbyGiveTakeServer.OnInventoryEnterSignal(this, player);
    }
}
