// -----------------------------------------------------------------------------
// NimbyTack - the pinned note (T3).
// Purpose : A Paper subclass per fastener that lives on a wall. Note text, health
//           levels, model, and the read action are inherited from Paper; the class
//           is the record of the fastener, so nothing is saved beyond vanilla Paper
//           data (shared rules 6.1: no OnStoreSave or OnStoreLoad override).
// Physics : Created without ECE_CREATEPHYSICS (CentralEconomy.c:17,28). The
//           0.01 s dynamic-physics lifetime is what vanilla StopItemDynamicPhysics
//           does (ItemBase.c:4530); it runs at init and again after load so a
//           physics body the engine adds settles instead of dropping the sheet.
//           DisableSimulation (Entity.c:3) is the plan's fallback if Phase 0 item 4
//           shows the sheet still falls.
// Inventory: a pinned note is never taken, stacked, or split; the take-down action
//           (T4) is the only way off the wall.
// -----------------------------------------------------------------------------
class NimbyTack_PinnedNote : Paper
{
    override void EEInit()
    {
        super.EEInit();
        NimbyTackFreeze();
    }

    override void EEOnAfterLoad()
    {
        super.EEOnAfterLoad();
        NimbyTackFreeze();
    }

    void NimbyTackFreeze()
    {
        SetDynamicPhysicsLifeTime(0.01);
    }

    // Overridden by each fastener class; -1 is the base, which never exists in the world.
    int GetNimbyTackFastener()
    {
        return -1;
    }

    override bool IsTakeable()
    {
        return false;
    }

    override bool CanPutInCargo(EntityAI parent)
    {
        return false;
    }

    override bool CanPutIntoHands(EntityAI parent)
    {
        return false;
    }

    override bool CanBeSplit()
    {
        return false;
    }

    override bool CanBeCombined(EntityAI other_item, bool reservation_check = true, bool stack_max_limit = false)
    {
        return false;
    }

    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionNimbyTackTakeDown);
    }
}

class NimbyTack_PinnedNote_Nail : NimbyTack_PinnedNote
{
    override int GetNimbyTackFastener()
    {
        return NimbyTackFastener.NAIL;
    }
}

class NimbyTack_PinnedNote_Tape : NimbyTack_PinnedNote
{
    override int GetNimbyTackFastener()
    {
        return NimbyTackFastener.TAPE;
    }
}

class NimbyTack_PinnedNote_Epoxy : NimbyTack_PinnedNote
{
    override int GetNimbyTackFastener()
    {
        return NimbyTackFastener.EPOXY;
    }
}

class NimbyTack_PinnedNote_Knife : NimbyTack_PinnedNote
{
    override int GetNimbyTackFastener()
    {
        return NimbyTackFastener.KNIFE;
    }
}
