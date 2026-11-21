// -----------------------------------------------------------------------------
// NimbyTapes - test kit (Plan 13, T-11). Server only, flag-gated, never on by default.
// Purpose : While $profile:NimbyTapes\spawn-testkit exists on the server, a connecting player
//           gets the Phase 0 kit on the ground at their feet: a deck, the test tape, the probe
//           tape, a full 9V battery, and a nearly flat 9V battery. Without the flag file this
//           code gives nothing and prints nothing; the one thing it always does is create the
//           empty list below. The flag never exists on the live server.
// Once    : One kit per player per server boot, so the relog rows (T0-15) do not pile up kits.
// Flat 9V : Set to 1.2 energy units of the battery's 50. At the deck's 0.02 units a second
//           that is about one minute of play, counted in the energy manager's 15-second steps
//           (recon R1). Test row T0-10 records the real time.
// Hook    : MissionServer.InvokeOnConnect (5_Mission/DayZ/mission/missionServer.c:435), which
//           the new-character path (:336) and the ready path (:357) both call. Modeled on the
//           DPV test spawn (NimbyDpv/Scripts/5_Mission/nimbydpv_missionserver.c).
// Rule    : Nothing here is queued. Shared rules 7.1 forbids CallLater into a 5_Mission
//           function.
// API     : EntityAI.SpawnEntityOnGroundPos (3_Game/DayZ/Entities/EntityAI.c:2138),
//           ComponentEnergyManager.SetEnergy
//           (3_Game/DayZ/tools/Component/ComponentEnergyManager.c:521),
//           PlayerIdentity.GetId and GetPlainId (3_Game/DayZ/gameplay.c:368, :370).
//           Lines are relative to vendor/dayz-1.30-scripts/scripts/ (build 1.30.164289).
// Logging : shared rules 4.2, the player line carries GetId() and the Steam64 id.
// Style   : one statement per line; no ternary; no variable name is declared twice in one
//           function.
// -----------------------------------------------------------------------------
modded class MissionServer
{
    protected ref array<string> m_NimbyTapesKitGiven = new array<string>;

    override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
    {
        super.InvokeOnConnect(player, identity);
        NimbyTapesGiveTestKit(player, identity);
    }

    protected void NimbyTapesGiveTestKit(PlayerBase player, PlayerIdentity identity)
    {
        if (!player || !identity)
        {
            return;
        }
        if (!NimbyTapesLog.IsTestKit())
        {
            return;
        }
        string playerId = identity.GetId();
        if (m_NimbyTapesKitGiven.Find(playerId) >= 0)
        {
            return;
        }
        m_NimbyTapesKitGiven.Insert(playerId);
        vector position = player.GetPosition();
        int made = 0;
        made = made + NimbyTapesSpawnKitItem(player, "NimbyTapes_Deck", position, -1);
        made = made + NimbyTapesSpawnKitItem(player, "NimbyTapes_Tape_Test", position, -1);
        made = made + NimbyTapesSpawnKitItem(player, "NimbyTapes_Tape_Probe", position, -1);
        made = made + NimbyTapesSpawnKitItem(player, "Battery9V", position, -1);
        made = made + NimbyTapesSpawnKitItem(player, "Battery9V", position, 1.2);
        NimbyTapesLog.Info("test kit given; items=" + made.ToString() + " of 5 at=" + position.ToString() + " player=" + playerId + "/" + identity.GetPlainId());
    }

    // Returns 1 when the item was made, 0 when it was not. energy below zero leaves the item's
    // own charge alone.
    protected int NimbyTapesSpawnKitItem(PlayerBase player, string className, vector position, float energy)
    {
        EntityAI item = player.SpawnEntityOnGroundPos(className, position);
        if (!item)
        {
            NimbyTapesLog.Info("test kit item not made; class=" + className);
            return 0;
        }
        if (energy >= 0 && item.HasEnergyManager())
        {
            item.GetCompEM().SetEnergy(energy);
        }
        return 1;
    }
}
