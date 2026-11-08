modded class ItemBase
{
    bool NimbyHandsCanBeOffered()
    {
        return !IsHeavyBehaviour();
    }

    bool NimbyHandsCanBeGiven()
    {
        return !IsHeavyBehaviour();
    }

    // Completion signal 1. Build 13 (October 31, 2026, NIMBY-T-063): no NimbyHands script runs
    // inside this engine callback beyond one static write; eleven server crashes on 1.30.164200
    // followed a step 1 landing whose callback ran the registry, the trace lines and Advance. The
    // step 2 poll reads the flag and the item's location (NimbyHandsServer.ObserveLandings). All 21
    // vanilla overrides call super, so every item class reaches this.
    override void EEItemLocationChanged(notnull InventoryLocation oldLoc, notnull InventoryLocation newLoc)
    {
        super.EEItemLocationChanged(oldLoc, newLoc);
        NimbyHandsServer.OnItemLocationChanged();
    }
}
