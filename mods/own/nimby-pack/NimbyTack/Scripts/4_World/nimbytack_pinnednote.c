// -----------------------------------------------------------------------------
// NimbyTack - the pinned note (T3).
// Purpose : A Paper subclass per fastener that lives on a wall. Note text, health
//           levels, model, and the read action are inherited from Paper; the class
//           is the record of the fastener. Since T21b (October 25, 2026) the class
//           saves its door fields after Paper's data, with a version guard (shared
//           rules 6.1: allowed on a class the addon defines, field list in the plan).
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
// Doors   : T21b, NIMBY-T-029 (Jeff, October 25, 2026, after the client-side
//           attachment was overwritten by the server's position updates). The server
//           records the note's door, bone pivot, and transform in the pivot's frame,
//           saves them on this class (version guard, shared rules 6.1), and moves the
//           note with the door while it swings (modded Building door events,
//           nimbytack_doors.c); the normal position updates carry it to players.
//           After a restart the note goes where its door holds it now.
// -----------------------------------------------------------------------------
class NimbyTack_PinnedNote : Paper
{
    // T21b: door attachment, server only, saved on this class (shared rules 6.1, version guard).
    // Field list: store version (int), door index (int), bone pivot (int), position in the pivot's
    // frame (vector), yaw/pitch/roll in the pivot's frame (vector).
    static const int STORE_VERSION = 1;
    protected int m_NimbyDoorIndex = -1;
    protected int m_NimbyDoorPivot = -1;
    protected vector m_NimbyDoorLocalPos;
    protected vector m_NimbyDoorLocalAngles;
    protected Building m_NimbyDoorBuilding;
    // Server: the note follows its door until this time (ms, game tick) while the door swings.
    protected int m_NimbyFollowUntil;
    protected bool m_NimbyFollowing;
    // Server only: seconds after a load before the door is looked for, so buildings spawned by
    // the mission (the test range) exist first.
    static const int DOOR_LOAD_DELAY_MS = 5000;
    static const int FOLLOW_STEP_MS = 50;
    static const int FOLLOW_CAP_MS = 3000;

    override void EEInit()
    {
        super.EEInit();
        NimbyTackFreeze();
    }

    override void EEOnAfterLoad()
    {
        super.EEOnAfterLoad();
        NimbyTackFreeze();
        if (GetGame().IsServer())
        {
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(NimbyTackRestoreDoor, DOOR_LOAD_DELAY_MS, false);
        }
    }

    override void EEDelete(EntityAI parent)
    {
        NimbyTackDoors.Unregister(this);
        super.EEDelete(parent);
    }

    // T21b: version 1 adds the door fields after Paper's own data. A note saved before this build
    // has none: the first read fails and the note loads as it was (its door, if any, is found again
    // by a ray after the load).
    override void OnStoreSave(ParamsWriteContext ctx)
    {
        super.OnStoreSave(ctx);
        ctx.Write(STORE_VERSION);
        ctx.Write(m_NimbyDoorIndex);
        ctx.Write(m_NimbyDoorPivot);
        ctx.Write(m_NimbyDoorLocalPos);
        ctx.Write(m_NimbyDoorLocalAngles);
    }

    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
        {
            return false;
        }
        int storeVersion;
        if (!ctx.Read(storeVersion))
        {
            return true;
        }
        if (storeVersion < 1)
        {
            return true;
        }
        if (!ctx.Read(m_NimbyDoorIndex))
        {
            return false;
        }
        if (!ctx.Read(m_NimbyDoorPivot))
        {
            return false;
        }
        if (!ctx.Read(m_NimbyDoorLocalPos))
        {
            return false;
        }
        if (!ctx.Read(m_NimbyDoorLocalAngles))
        {
            return false;
        }
        return true;
    }

    int NimbyTackDoorIndex()
    {
        return m_NimbyDoorIndex;
    }

    Building NimbyTackDoorBuilding()
    {
        return m_NimbyDoorBuilding;
    }

    // T21b, server: record the door this note hangs on and the note's transform in the door pivot's
    // frame, then follow the door from now on. Returns the pin line's door= token.
    string NimbyTackAttachDoorServer(Object wall, int doorIndex, int component, string caller)
    {
        Building building = Building.Cast(wall);
        string levelName;
        int pivot = NimbyTackPlace.DoorPivot(wall, component, levelName);
        if (!building || pivot < 0)
        {
            string refused = "door attach-server from=" + caller + " result=no-pivot door=" + doorIndex.ToString() + " component=" + component.ToString();
            refused = refused + " wall=" + NimbyTackWorld.ClassOf(wall) + " " + NimbyTackWorld.PositionTag(GetPosition());
            NimbyTackLog.Debug(refused);
            return doorIndex.ToString() + "/no-pivot";
        }
        vector pivotTransform[4];
        building.GetBoneTransformWS(pivot, pivotTransform);
        vector noteTransform[4];
        GetTransform(noteTransform);
        vector localTransform[4];
        Math3D.MatrixInvMultiply4(pivotTransform, noteTransform, localTransform);
        Math3D.MatrixOrthogonalize4(localTransform);
        vector rotation[3];
        rotation[0] = localTransform[0];
        rotation[1] = localTransform[1];
        rotation[2] = localTransform[2];
        m_NimbyDoorIndex = doorIndex;
        m_NimbyDoorPivot = pivot;
        m_NimbyDoorLocalPos = localTransform[3];
        m_NimbyDoorLocalAngles = Math3D.MatrixToAngles(rotation);
        m_NimbyDoorBuilding = building;
        NimbyTackDoors.Register(this);
        if (NimbyTackLog.IsDebug())
        {
            string attached = "door attach-server from=" + caller + " result=ok door=" + doorIndex.ToString() + " component=" + component.ToString();
            attached = attached + " pivot=" + pivot.ToString() + " level=" + levelName + " local=" + m_NimbyDoorLocalPos.ToString();
            attached = attached + " angles=" + m_NimbyDoorLocalAngles.ToString() + " wall=" + NimbyTackWorld.ClassOf(wall) + " " + NimbyTackWorld.PositionTag(GetPosition());
            NimbyTackLog.Debug(attached);
        }
        return doorIndex.ToString() + "/pivot=" + pivot.ToString() + "/" + levelName;
    }

    // T21b, server: put the note where its door holds it now (pivot transform times the note's
    // transform in the pivot's frame).
    void NimbyTackSnapToDoor()
    {
        if (!m_NimbyDoorBuilding || m_NimbyDoorPivot < 0)
        {
            return;
        }
        vector pivotTransform[4];
        m_NimbyDoorBuilding.GetBoneTransformWS(m_NimbyDoorPivot, pivotTransform);
        vector rotation[3];
        Math3D.YawPitchRollMatrix(m_NimbyDoorLocalAngles, rotation);
        vector localTransform[4];
        localTransform[0] = rotation[0];
        localTransform[1] = rotation[1];
        localTransform[2] = rotation[2];
        localTransform[3] = m_NimbyDoorLocalPos;
        vector world[4];
        Math3D.MatrixMultiply4(pivotTransform, localTransform, world);
        SetTransform(world);
    }

    // T21b, server: the door started to move; follow it every FOLLOW_STEP_MS until it settles or
    // FOLLOW_CAP_MS passes.
    void NimbyTackFollowDoor()
    {
        m_NimbyFollowUntil = GetGame().GetTime() + FOLLOW_CAP_MS;
        if (m_NimbyFollowing)
        {
            return;
        }
        m_NimbyFollowing = true;
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(NimbyTackFollowTick, FOLLOW_STEP_MS, true);
    }

    void NimbyTackFollowTick()
    {
        if (IsSetForDeletion())
        {
            NimbyTackStopFollow();
            return;
        }
        NimbyTackSnapToDoor();
        if (GetGame().GetTime() >= m_NimbyFollowUntil)
        {
            NimbyTackStopFollow();
        }
    }

    // T21b, server: the door settled; one last snap and stop.
    void NimbyTackDoorSettled()
    {
        NimbyTackSnapToDoor();
        NimbyTackStopFollow();
        if (NimbyTackLog.IsDebug())
        {
            NimbyTackLog.Debug("door settled door=" + m_NimbyDoorIndex.ToString() + " closed=" + NimbyTackLog.Bool(m_NimbyDoorBuilding && m_NimbyDoorBuilding.IsDoorClosed(m_NimbyDoorIndex)) + " " + NimbyTackWorld.PositionTag(GetPosition()));
        }
    }

    protected void NimbyTackStopFollow()
    {
        m_NimbyFollowing = false;
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(NimbyTackFollowTick);
    }

    // T21b, server, after a load: a note with saved door data finds its building by the door's
    // position and goes where the door holds it now (doors load closed, so a note saved on an open
    // door returns to the closed door). A note without data (saved before this build) is looked up
    // with a ray through it, as before.
    void NimbyTackRestoreDoor()
    {
        if (!GetGame().IsServer() || IsSetForDeletion())
        {
            return;
        }
        // Notes saved before T21b were read with the next entity's bytes as door data (October 25,
        // 2026: door=1065353216); clear what cannot be a door so the ray lookup below runs instead.
        if (!NimbyTackRules.DoorDataValid(m_NimbyDoorIndex, m_NimbyDoorPivot))
        {
            NimbyTackLog.Info("door load result=invalid-cleared door=" + m_NimbyDoorIndex.ToString() + " pivot=" + m_NimbyDoorPivot.ToString() + " " + NimbyTackWorld.PositionTag(GetPosition()));
            m_NimbyDoorIndex = -1;
            m_NimbyDoorPivot = -1;
        }
        if (m_NimbyDoorIndex >= 0 && m_NimbyDoorPivot >= 0)
        {
            Building building = NimbyTackPlace.FindDoorBuilding(GetPosition(), m_NimbyDoorIndex);
            if (!building)
            {
                NimbyTackLog.Info("door load result=no-building door=" + m_NimbyDoorIndex.ToString() + " " + NimbyTackWorld.PositionTag(GetPosition()));
                return;
            }
            m_NimbyDoorBuilding = building;
            vector before = GetPosition();
            NimbyTackSnapToDoor();
            NimbyTackDoors.Register(this);
            string restored = "door load result=restored class=" + GetType() + " door=" + m_NimbyDoorIndex.ToString() + " wall=" + building.GetType();
            restored = restored + " closed=" + NimbyTackLog.Bool(building.IsDoorClosed(m_NimbyDoorIndex)) + " moved=" + vector.Distance(before, GetPosition()).ToString() + " " + NimbyTackWorld.PositionTag(GetPosition());
            NimbyTackLog.Info(restored);
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
        NimbyTackLog.Info("door load result=found class=" + GetType() + " door=" + tag + " wall=" + NimbyTackWorld.ClassOf(wall) + " " + NimbyTackWorld.PositionTag(GetPosition()));
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
