// -----------------------------------------------------------------------------
// NimbyTapes - hand pose (Plan 13, Architecture, Hand pose; test row T0-24).
// Purpose : Without a profile an own item class sits in the hand with the default pose. The
//           deck takes the vanilla Radio's profile and every tape the vanilla Cassette's, the
//           classes whose models they use.
// Source  : The two vanilla lines, copied with only the class name changed
//           (4_World/DayZ/Entities/ManBase/DayZPlayer/DayZPlayerCfgBase.c:1304 Radio,
//           :738 Cassette; both pass the one-handed tool behavior, toolsOneHanded).
// Hook    : ModItemRegisterCallbacks.RegisterOneHanded (DayZPlayerCfgBase.c:305-309), which
//           vanilla calls with that same behavior (:1552).
//           Lines are relative to vendor/dayz-1.30-scripts/scripts/ (build 1.30.164289).
// Note    : The tape is registered by its base class; whether the classes derived from it in
//           config take the profile is what T0-24 shows.
// -----------------------------------------------------------------------------
modded class ModItemRegisterCallbacks
{
    override void RegisterOneHanded(DayZPlayerType pType, DayzPlayerItemBehaviorCfg pBehavior)
    {
        super.RegisterOneHanded(pType, pBehavior);
        pType.AddItemInHandsProfileIK("NimbyTapes_Deck", "dz/anims/workspaces/player/player_main/props/player_main_1h_radio.asi", pBehavior, "dz/anims/anm/player/ik/gear/Radio.anm");
        pType.AddItemInHandsProfileIK("NimbyTapes_Tape_Base", "dz/anims/workspaces/player/player_main/player_main_1h.asi", pBehavior, "dz/anims/anm/player/ik/gear/cassette.anm");
    }
}
