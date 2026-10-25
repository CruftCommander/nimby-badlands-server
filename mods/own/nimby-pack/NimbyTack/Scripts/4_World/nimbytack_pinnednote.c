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
//           Knife slot drops to the ground, and the note is deleted. Since T18
//           (NIMBY-T-022, October 24, 2026) the only fire path is the Tack burn
//           (ActionNimbyTackBurn), which calls OnIgnitedThis directly; vanilla Ignite is
//           refused here (CanBeIgnitedBy) because its target condition cancelled the
//           strike at the edge of reach.
// Doors   : T21, NIMBY-T-029 (Jeff, October 25, 2026). A note pinned on a closed door
//           leaf stays, on the server, where the closed door holds it; that is what
//           storage saves, so nothing new is written. The door index, the door's bone
//           pivot, and the note's transform in the pivot's frame are net-synced
//           variables (shared rules 6: allowed, never persisted), and each client
//           parents its own copy to the pivot (Object.AddArrow precedent,
//           Object.c:1387-1415), so the note swings with the door. After a restart the
//           server finds the door again with a ray through the note, the doors being
//           closed then, and syncs the same data.
// -----------------------------------------------------------------------------
class NimbyTack_PinnedNote : Paper
{
    // T21: door attachment, net-synced only.
    protected int m_NimbyDoorIndex = -1;
    protected int m_NimbyDoorPivot = -1;
    protected float m_NimbyDoorPosX;
    protected float m_NimbyDoorPosY;
    protected float m_NimbyDoorPosZ;
    protected float m_NimbyDoorYaw;
    protected float m_NimbyDoorPitch;
    protected float m_NimbyDoorRoll;
    // Client only: this copy is parented to the door pivot.
    protected bool m_NimbyDoorAttached;
    // Server only: seconds after a load before the door is looked for, so buildings spawned by
    // the mission (the test range) exist first.
    static const int DOOR_LOAD_DELAY_MS = 5000;

    void NimbyTack_PinnedNote()
    {
        RegisterNetSyncVariableInt("m_NimbyDoorIndex", -1, 63);
        RegisterNetSyncVariableInt("m_NimbyDoorPivot", -1, 1023);
        RegisterNetSyncVariableFloat("m_NimbyDoorPosX", -20, 20, 4);
        RegisterNetSyncVariableFloat("m_NimbyDoorPosY", -20, 20, 4);
        RegisterNetSyncVariableFloat("m_NimbyDoorPosZ", -20, 20, 4);
        RegisterNetSyncVariableFloat("m_NimbyDoorYaw", -180, 180, 3);
        RegisterNetSyncVariableFloat("m_NimbyDoorPitch", -180, 180, 3);
        RegisterNetSyncVariableFloat("m_NimbyDoorRoll", -180, 180, 3);
    }

    override void EEInit()
    {
        super.EEInit();
        NimbyTackFreeze();
        if (GetGame().IsClient())
        {
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(NimbyTackTryAttachDoor, 500, false);
        }
    }

    override void EEOnAfterLoad()
    {
        super.EEOnAfterLoad();
        NimbyTackFreeze();
        if (GetGame().IsServer())
        {
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(NimbyTackRederiveDoor, DOOR_LOAD_DELAY_MS, false);
        }
    }

    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();
        NimbyTackTryAttachDoor();
    }

    // T21, server: record the door this note hangs on. The door must be closed (the pin and the
    // load both check), so the transform in the pivot's frame is the closed door's. Returns the
    // pin line's door= token.
    string NimbyTackAttachDoorServer(Object wall, int doorIndex, int component, string caller)
    {
        string levelName;
        int pivot = NimbyTackPlace.DoorPivot(wall, component, levelName);
        if (pivot < 0)
        {
            string refused = "door attach-server from=" + caller + " result=no-pivot door=" + doorIndex.ToString() + " component=" + component.ToString();
            refused = refused + " wall=" + NimbyTackWorld.ClassOf(wall) + " " + NimbyTackWorld.PositionTag(GetPosition());
            NimbyTackLog.Debug(refused);
            return doorIndex.ToString() + "/no-pivot";
        }
        vector pivotTransform[4];
        wall.GetBoneTransformWS(pivot, pivotTransform);
        vector noteTransform[4];
        GetTransform(noteTransform);
        vector localTransform[4];
        Math3D.MatrixInvMultiply4(pivotTransform, noteTransform, localTransform);
        // As Object.AddArrow: orthogonalize (the parent might be skewed) and undo the scale.
        Math3D.MatrixOrthogonalize4(localTransform);
        float scale = wall.GetScale();
        if (scale > 0.0)
        {
            scale = 1.0 / (scale * scale);
            localTransform[3] = localTransform[3] * scale;
        }
        vector rotation[3];
        rotation[0] = localTransform[0];
        rotation[1] = localTransform[1];
        rotation[2] = localTransform[2];
        vector angles = Math3D.MatrixToAngles(rotation);
        vector offset = localTransform[3];
        m_NimbyDoorIndex = doorIndex;
        m_NimbyDoorPivot = pivot;
        m_NimbyDoorPosX = offset[0];
        m_NimbyDoorPosY = offset[1];
        m_NimbyDoorPosZ = offset[2];
        m_NimbyDoorYaw = angles[0];
        m_NimbyDoorPitch = angles[1];
        m_NimbyDoorRoll = angles[2];
        SetSynchDirty();
        if (NimbyTackLog.IsDebug())
        {
            string attached = "door attach-server from=" + caller + " result=ok door=" + doorIndex.ToString() + " component=" + component.ToString();
            attached = attached + " pivot=" + pivot.ToString() + " level=" + levelName + " local=" + offset.ToString();
            attached = attached + " angles=" + angles.ToString() + " wall=" + NimbyTackWorld.ClassOf(wall) + " " + NimbyTackWorld.PositionTag(GetPosition());
            NimbyTackLog.Debug(attached);
        }
        return doorIndex.ToString() + "/pivot=" + pivot.ToString() + "/" + levelName;
    }

    // T21, server, after a load: find the door under the note again (doors are closed after a
    // restart) and sync it. A note on no door does nothing.
    void NimbyTackRederiveDoor()
    {
        if (!GetGame().IsServer() || IsSetForDeletion())
        {
            return;
        }
        Object wall;
        int component;
        if (!NimbyTackPlace.FindDoorUnder(this, wall, component))
        {
            return;
        }
        int doorIndex = NimbyTackPlace.DoorIndexOf(wall, component);
        if (doorIndex < 0)
        {
            return;
        }
        if (!NimbyTackPlace.DoorClosed(wall, doorIndex))
        {
            NimbyTackLog.Info("door load result=open door=" + doorIndex.ToString() + " wall=" + NimbyTackWorld.ClassOf(wall) + " " + NimbyTackWorld.PositionTag(GetPosition()));
            return;
        }
        string tag = NimbyTackAttachDoorServer(wall, doorIndex, component, "load");
        NimbyTackLog.Info("door load class=" + GetType() + " door=" + tag + " wall=" + NimbyTackWorld.ClassOf(wall) + " " + NimbyTackWorld.PositionTag(GetPosition()));
    }

    // T21, client: parent this copy to the door pivot once the door data has arrived. The
    // transform is set to the pivot-local one first, as Object.AddArrow does; if the engine
    // refuses the child, the world transform is put back.
    void NimbyTackTryAttachDoor()
    {
        if (!GetGame().IsClient() || m_NimbyDoorAttached || IsSetForDeletion())
        {
            return;
        }
        if (m_NimbyDoorIndex < 0 || m_NimbyDoorPivot < 0)
        {
            return;
        }
        vector world[4];
        GetTransform(world);
        Building building = NimbyTackPlace.FindDoorBuilding(world[3], m_NimbyDoorIndex);
        if (!building)
        {
            NimbyTackLog.Debug("door attach-client result=no-building door=" + m_NimbyDoorIndex.ToString() + " " + NimbyTackWorld.PositionTag(world[3]));
            return;
        }
        vector rotation[3];
        Math3D.YawPitchRollMatrix(Vector(m_NimbyDoorYaw, m_NimbyDoorPitch, m_NimbyDoorRoll), rotation);
        vector localTransform[4];
        localTransform[0] = rotation[0];
        localTransform[1] = rotation[1];
        localTransform[2] = rotation[2];
        localTransform[3] = Vector(m_NimbyDoorPosX, m_NimbyDoorPosY, m_NimbyDoorPosZ);
        SetTransform(localTransform);
        if (!building.AddChild(this, m_NimbyDoorPivot))
        {
            SetTransform(world);
            NimbyTackLog.Debug("door attach-client result=refused door=" + m_NimbyDoorIndex.ToString() + " pivot=" + m_NimbyDoorPivot.ToString() + " building=" + building.GetType() + " " + NimbyTackWorld.PositionTag(world[3]));
            return;
        }
        m_NimbyDoorAttached = true;
        NimbyTackLog.Debug("door attach-client result=ok door=" + m_NimbyDoorIndex.ToString() + " pivot=" + m_NimbyDoorPivot.ToString() + " building=" + building.GetType() + " open=" + NimbyTackLog.Bool(!building.IsDoorClosed(m_NimbyDoorIndex)) + " " + NimbyTackWorld.PositionTag(GetPosition()));
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

    // T18: vanilla ActionLightItemOnFire offers when the target CanBeIgnitedBy the igniter
    // (ActionLightItemOnFire.c:77; its other branch needs Paper.CanIgniteItem, false at
    // Paper.c:48-51). Paper answers !GetHierarchyParent() (Paper.c:43-46), true on a wall; a
    // pinned note says no, so only ActionNimbyTackBurn offers.
    override bool CanBeIgnitedBy(EntityAI igniter = null)
    {
        return false;
    }

    // Fire: always succeeds; the burn is the deletion. Server side only: ActionNimbyTackBurn
    // calls OnIgnitedThis from OnFinishProgressServer. IsThisIgnitionSuccessful stays true for
    // any other caller of vanilla's ignition path.
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
