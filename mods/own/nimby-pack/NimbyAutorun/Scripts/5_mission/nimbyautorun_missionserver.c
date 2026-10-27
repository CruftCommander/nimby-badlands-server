// -----------------------------------------------------------------------------
// NimbyAutorun - Legs. Server mission hook for the logout window.
// Purpose : Clears the server-side override the moment a client leaves, not when
//           its logout timer ends. Vanilla puts a living player's body into the
//           logout list in OnClientDisconnectedEvent (missionServer.c:641-668) and
//           calls PlayerBase.OnDisconnect only when UpdateLogoutPlayers fires it
//           logoutTime later (:277-300, :690-703; 15 s by default). Until October 27,
//           2026 the body stood for those seconds with OverrideMovementSpeed still
//           applied (review LEGS-R2-01; the October 19 record shows the 15 s gap).
// Vanilla : MissionServer.OnClientDisconnectedEvent(identity, player, logoutTime,
//           authFailed), missionServer.c:641. No deferred call (shared rules 7.1).
// -----------------------------------------------------------------------------
modded class MissionServer
{
    override void OnClientDisconnectedEvent(PlayerIdentity identity, PlayerBase player, int logoutTime, bool authFailed)
    {
        if (player)
        {
            player.NimbyAutorunOnLogoutStart();
        }
        super.OnClientDisconnectedEvent(identity, player, logoutTime, authFailed);
    }
}
