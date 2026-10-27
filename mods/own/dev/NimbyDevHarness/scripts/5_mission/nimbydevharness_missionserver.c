// -----------------------------------------------------------------------------
// NimbyDevHarness - the online cases need a connected identity, so they start from
// the server's connect hook (missionServer.c:435-440) and wait for the player to be
// in the world. They run only when $profile:NimbyDevHarness\forge says "online".
// -----------------------------------------------------------------------------
modded class MissionServer
{
    override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
    {
        super.InvokeOnConnect(player, identity);
        NimbyDevHarness.OnPlayerConnected(player);
    }
}
