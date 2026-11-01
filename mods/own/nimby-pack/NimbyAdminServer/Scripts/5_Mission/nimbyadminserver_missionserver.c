// -----------------------------------------------------------------------------
// NimbyAdminServer - the MissionServer hooks (Plan 3 v0.8).
// Purpose : Starts the server half at mission start (config, Discord, permissions with the
//           first-owner bootstrap, bans), pumps the Discord queue every frame, and keeps the
//           session table: a session per connection, made at InvokeOnConnect and dropped at
//           PlayerDisconnected. Everything else lives in 4_World (nimbyadminserver_core.c), so
//           this file holds no state and queues no call (shared rules 7.1).
// Vanilla : MissionServer.OnInit, OnUpdate (5_Mission/DayZ/mission/missionServer.c:108),
//           InvokeOnConnect (:435), PlayerDisconnected (:690; "identity can be already deleted").
// Style   : One statement per line.
// -----------------------------------------------------------------------------

modded class MissionServer extends MissionBase
{
    override void OnInit()
    {
        super.OnInit();
        NimbyAdminServerCore.Init();
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        NimbyAdminServerCore.Tick(GetGame().GetTickTime());
    }

    override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
    {
        super.InvokeOnConnect(player, identity);
        NimbyAdminServerCore.OnConnect(player, identity);
    }

    override void PlayerDisconnected(PlayerBase player, PlayerIdentity identity, string uid)
    {
        NimbyAdminServerCore.OnDisconnect(uid);
        super.PlayerDisconnected(player, identity, uid);
    }
}
