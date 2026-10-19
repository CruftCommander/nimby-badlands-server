// G3: pockets close on respawn. MissionServer.OnClientRespawnEvent (missionServer.c:611)
// receives the character being left behind; its pockets close before vanilla may kill it.
// The new character starts closed because nothing is persisted.
modded class MissionServer
{
    override void OnClientRespawnEvent( PlayerIdentity identity, PlayerBase player )
    {
        if ( player )
        {
            player.NimbyClosePockets( "respawn" );
        }

        super.OnClientRespawnEvent( identity, player );
    }
}
