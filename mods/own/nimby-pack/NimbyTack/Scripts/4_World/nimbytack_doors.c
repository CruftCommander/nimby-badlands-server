// -----------------------------------------------------------------------------
// NimbyTack - notes on doors follow their doors (T21b, NIMBY-T-029).
// Purpose : The server keeps a list of pinned notes that hang on a door leaf. When a
//           building reports a door starting to move (Building.c:102-128 events,
//           fired on the server; Bunker.c:73 and Land_WarheadStorage_Main.c:137 act
//           on them server-side), each note on that door follows it every 50 ms
//           until the door reports it has settled, capped at 3 s. The note moves on
//           the server, so the normal position updates reach every player.
// Server  : Everything here returns at once on a client.
// -----------------------------------------------------------------------------
class NimbyTackDoors
{
    protected static ref array<NimbyTack_PinnedNote> s_Notes;

    static void Register(NimbyTack_PinnedNote note)
    {
        if (!note || !GetGame().IsServer())
        {
            return;
        }
        if (!s_Notes)
        {
            s_Notes = new array<NimbyTack_PinnedNote>;
        }
        if (s_Notes.Find(note) < 0)
        {
            s_Notes.Insert(note);
        }
    }

    static void Unregister(NimbyTack_PinnedNote note)
    {
        if (!s_Notes || !note)
        {
            return;
        }
        int index = s_Notes.Find(note);
        if (index >= 0)
        {
            s_Notes.Remove(index);
        }
    }

    // A door of building started to open, open ajar, or close.
    static void OnDoorMoving(Building building, int doorIndex)
    {
        if (!s_Notes || !building || !GetGame().IsServer())
        {
            return;
        }
        foreach (NimbyTack_PinnedNote note : s_Notes)
        {
            if (note && note.NimbyTackDoorBuilding() == building && note.NimbyTackDoorIndex() == doorIndex)
            {
                note.NimbyTackFollowDoor();
            }
        }
    }

    // A door of building finished moving.
    static void OnDoorSettled(Building building, int doorIndex)
    {
        if (!s_Notes || !building || !GetGame().IsServer())
        {
            return;
        }
        foreach (NimbyTack_PinnedNote note : s_Notes)
        {
            if (note && note.NimbyTackDoorBuilding() == building && note.NimbyTackDoorIndex() == doorIndex)
            {
                note.NimbyTackDoorSettled();
            }
        }
    }
}

// The engine raises these on Building (3_Game, Building.c:102-128), which a 4_World addon cannot mod
// (build of October 25, 2026: "Unknown type 'Building'"). BuildingBase (4_World,
// Entities/Game/Super/Building.c:8) is the script base of House (:380), and the map houses are
// HouseNoDestruct : House in config (DZ/data/config.cpp:3119). Each override calls super first, so
// vanilla behaviour (BuildingBase's own construction hooks, :363-376) is unchanged.
modded class BuildingBase
{
    override void OnDoorOpenStart(DoorStartParams params)
    {
        super.OnDoorOpenStart(params);
        NimbyTackDoors.OnDoorMoving(this, params.param1);
    }

    override void OnDoorOpenAjarStart(DoorStartParams params)
    {
        super.OnDoorOpenAjarStart(params);
        NimbyTackDoors.OnDoorMoving(this, params.param1);
    }

    override void OnDoorCloseStart(DoorStartParams params)
    {
        super.OnDoorCloseStart(params);
        NimbyTackDoors.OnDoorMoving(this, params.param1);
    }

    override void OnDoorOpenFinish(DoorFinishParams params)
    {
        super.OnDoorOpenFinish(params);
        NimbyTackDoors.OnDoorSettled(this, params.param1);
    }

    override void OnDoorOpenAjarFinish(DoorFinishParams params)
    {
        super.OnDoorOpenAjarFinish(params);
        NimbyTackDoors.OnDoorSettled(this, params.param1);
    }

    override void OnDoorCloseFinish(DoorFinishParams params)
    {
        super.OnDoorCloseFinish(params);
        NimbyTackDoors.OnDoorSettled(this, params.param1);
    }
}
