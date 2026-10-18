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
// Fire    : A lit match burns the note off the wall (Jeff, October 17, 2026). Vanilla
//           Paper turns a lit sheet into a fireplace and refuses that on a wall, so the
//           Ignite prompt did nothing. Here ignition always succeeds, a knife in the
//           Knife slot drops to the ground, and the note is deleted.
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

    // Fire: always succeeds; the burn is the deletion. Server side only (the ignite action
    // calls these from OnFinishProgressServer, ActionLightItemOnFire.c:115,129).
    override bool IsThisIgnitionSuccessful(EntityAI item_source = null)
    {
        return true;
    }

    override void OnIgnitedThis(EntityAI fire_source)
    {
        if (!GetGame().IsServer())
        {
            return;
        }
        string knifeNote = "";
        EntityAI knife = FindAttachmentBySlotName("Knife");
        if (knife)
        {
            GetInventory().DropEntity(InventoryMode.SERVER, this, knife);
            knifeNote = " knife=ground";
        }
        string source = "unknown";
        PlayerBase burner = null;
        if (fire_source)
        {
            source = fire_source.GetType();
            // The match is in the burner's hands, so its hierarchy root is the player.
            burner = PlayerBase.Cast(fire_source.GetHierarchyRootPlayer());
        }
        NimbyTackLog.Info("burn class=" + GetType() + " source=" + source + knifeNote + " " + NimbyTackWorld.PlayerTag(burner) + " " + NimbyTackWorld.PositionTag(GetPosition()));
        GetGame().ObjectDelete(this);
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

    // Read first so hold F reads by default and take-down is one scroll away (T3, T4). The
    // inherited ActionReadPaper stays registered but never offers: it has no target.
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionNimbyTackRead);
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
