// -----------------------------------------------------------------------------
// NimbyTapes - Music. A cassette deck ("Cassette Player" in game), tapes, and a 9V battery:
// a switched-on deck plays its tape as positional sound that nearby players hear in step.
// Plan: docs/plans/2026-11-09-music-plan.md (Plan 13, v0.3). Owner: Jeff Fabish.
// Phase 0 step 3 (config): the tape slot, the deck, the tape base, the two Phase 0 tapes,
// and one sound shader and one sound set per timing track. The script side (deck logic,
// actions, hand pose, tape memory, test kit) is step 4; until it lands the classes below
// are plain inventory items and the deck plays nothing.
// Layers: 3_Game holds the engine-free rules and the self-test; 4_World the mission-start
// hook and, from step 4, the deck, the tape, and the actions. No 5_Mission module and no
// Inputs.xml: controls are action-menu entries only (T-3).
// Content addon (shared rules 6.1 and 6.3): the deck and the tapes persist the vanilla way.
// Nothing here touches a vanilla class.
// File and line references are to vendor/dayz-1.30-scripts/ (build 1.30.164200).
// -----------------------------------------------------------------------------
class CfgPatches
{
    class NimbyTapes
    {
        // DZ_Scripts: CfgSlots lives there (scripts/config.cpp:11). DZ_Radio and DZ_Gear_Tools:
        // the Wilga and cassette models and their damage materials. DZ_Gear_Consumables: the 9V
        // battery that fills the BatteryD slot. DZ_Sounds_Effects: the 3D processor and the
        // volume curve the sound sets name (DZ/sounds/hpp/config.cpp:2614, 781).
        requiredAddons[] = { "DZ_Data", "DZ_Scripts", "DZ_Radio", "DZ_Gear_Tools", "DZ_Gear_Consumables", "DZ_Sounds_Effects" };
        units[] = { "NimbyTapes_Deck", "NimbyTapes_Tape_Test", "NimbyTapes_Tape_Probe" };
        weapons[] = {};
    };
};

class CfgMods
{
    class NimbyTapes
    {
        type = "mod";
        dependencies[] = { "Game", "World" };
        class defs
        {
            class gameScriptModule
            {
                files[] = { "NimbyTapes/Scripts/3_Game" };
            };
            class worldScriptModule
            {
                files[] = { "NimbyTapes/Scripts/4_World" };
            };
        };
    };
};

// The deck's tape slot (T-1). Shape of the vanilla BatteryD slot (scripts/config.cpp:461-466).
// The Wilga model has no proxy for it, so an attached tape is not drawn; the slot shows in
// the inventory. The icon is the vanilla placeholder (scripts/config.cpp:159-163); an own
// icon is Phase 1 work.
class CfgSlots
{
    class Slot_NimbyTape
    {
        name = "NimbyTape";
        displayName = "Tape";
        ghostIcon = "set:dayz_inventory image:missing";
    };
};

class CfgVehicles
{
    class Inventory_Base;

    // The deck (T-13: "Cassette Player"). Model, size, weight, damage, and the energy block are
    // the vanilla Radio's (DZ/gear/radio/config.cpp:256-333). Not derived from Radio: its
    // ItemRadio simulation is a tuner, and its Channels block is left out with it.
    class NimbyTapes_Deck : Inventory_Base
    {
        scope = 2;
        displayName = "Cassette Player";
        descriptionShort = "A portable radio-cassette recorder. It plays one tape and runs on a 9V battery.";
        model = "\dz\gear\radio\unitra_wilga.p3d";
        animClass = "Knife";
        weight = 1700;
        itemSize[] = {2, 3};
        fragility = 0.01;
        absorbency = 0.5;
        attachments[] =
        {
            "BatteryD",
            "NimbyTape"
        };
        repairableWithKits[] = {7};
        repairCosts[] = {25};
        // The Radio's values (DZ/gear/radio/config.cpp:275-282). No plugType: an undefined plug
        // is compatible (ComponentEnergyManager.c:972-977). No autoSwitchOffWhenInCargo: a deck
        // keeps playing in a backpack (T-2). No updateInterval: the manager's own tick stays at
        // its 15-second default (recon R1); the deck's 1-second timer is script (step 4).
        class EnergyManager
        {
            hasIcon = 1;
            autoSwitchOff = 1;
            energyUsagePerSecond = 0.02;
            attachmentAction = 1;
            wetnessExposure = 0.1;
        };
        class DamageSystem
        {
            class GlobalHealth
            {
                class Health
                {
                    hitpoints = 50;
                    healthLevels[] =
                    {
                        {1, {"DZ\gear\radio\data\unitra_wilga.rvmat"}},
                        {0.7, {"DZ\gear\radio\data\unitra_wilga.rvmat"}},
                        {0.5, {"DZ\gear\radio\data\unitra_wilga_damage.rvmat"}},
                        {0.3, {"DZ\gear\radio\data\unitra_wilga_damage.rvmat"}},
                        {0, {"DZ\gear\radio\data\unitra_wilga_destruct.rvmat"}}
                    };
                };
            };
        };
        class MeleeModes
        {
            class Default
            {
                ammo = "MeleeLightBlunt";
                range = 1;
            };
            class Heavy
            {
                ammo = "MeleeLightBlunt_Heavy";
                range = 1;
            };
            class Sprint
            {
                ammo = "MeleeLightBlunt_Heavy";
                range = 2.8;
            };
        };
        soundImpactType = "plastic";
    };

    // Every tape derives from this class. Model, size, weight, and damage are the vanilla
    // Cassette's (DZ/gear/tools/config.cpp:1200-1248). tracks[] holds sound set names in play
    // order and trackSeconds[] the length of each in whole seconds; the two lists have the same
    // length and at most 63 entries (T-10). tools\Test-TapesAssets.ps1 checks them against the
    // audio before a build.
    class NimbyTapes_Tape_Base : Inventory_Base
    {
        scope = 0;
        displayName = "Tape";
        descriptionShort = "A compact cassette.";
        model = "\dz\gear\tools\cassette.p3d";
        rotationFlags = 17;
        weight = 80;
        absorbency = 0.5;
        itemSize[] = {2, 1};
        inventorySlot[] =
        {
            "NimbyTape"
        };
        soundImpactType = "plastic";
        tracks[] = {};
        trackSeconds[] = {};
        class DamageSystem
        {
            class GlobalHealth
            {
                class Health
                {
                    hitpoints = 10;
                    healthLevels[] =
                    {
                        {1, {"DZ\gear\tools\data\cassette.rvmat"}},
                        {0.7, {"DZ\gear\tools\data\cassette.rvmat"}},
                        {0.5, {"DZ\gear\tools\data\cassette_damage.rvmat"}},
                        {0.3, {"DZ\gear\tools\data\cassette_damage.rvmat"}},
                        {0, {"DZ\gear\tools\data\cassette_destruct.rvmat"}}
                    };
                };
            };
        };
        class MeleeModes
        {
            class Default
            {
                ammo = "MeleeLightBlunt";
                range = 1;
            };
            class Heavy
            {
                ammo = "MeleeLightBlunt_Heavy";
                range = 1;
            };
            class Sprint
            {
                ammo = "MeleeLightBlunt_Heavy";
                range = 2.8;
            };
        };
    };

    // The Phase 0 test tape (T-12, T-13): the two timing tracks, 240 and 60 seconds.
    class NimbyTapes_Tape_Test : NimbyTapes_Tape_Base
    {
        scope = 2;
        displayName = "Test Tape";
        descriptionShort = "Two timing tracks: a beep every 5 seconds and a counted mark every 30.";
        tracks[] = { "NimbyTapes_TimingA_SoundSet", "NimbyTapes_TimingB_SoundSet" };
        trackSeconds[] = { 240, 60 };
    };

    // The texture probe (T-12, test row T0-25): a flat magenta texture on the cassette model's
    // zbytek selection shows which faces a tape label can cover. It carries no track, so a deck
    // holding it refuses to start.
    class NimbyTapes_Tape_Probe : NimbyTapes_Tape_Base
    {
        scope = 2;
        displayName = "Probe Tape";
        descriptionShort = "A tape with a flat test texture. It has no track.";
        hiddenSelections[] =
        {
            "zbytek"
        };
        hiddenSelectionsTextures[] =
        {
            "NimbyTapes\data\probe_co.paa"
        };
    };
};

// One shader per timing track. The .ogg files are generated by tools\New-TimingTrack.ps1 into
// sound\ and are git-ignored (plan P4). range is the plan's starting value; T0-16 tunes it.
class CfgSoundShaders
{
    class NimbyTapes_Timing_SoundShader_Base
    {
        samples[] = {};
        frequency = 1;
        volume = 1;
        range = 25;
    };
    class NimbyTapes_TimingA_SoundShader : NimbyTapes_Timing_SoundShader_Base
    {
        samples[] =
        {
            {"NimbyTapes\sound\timing_a", 1}
        };
    };
    class NimbyTapes_TimingB_SoundShader : NimbyTapes_Timing_SoundShader_Base
    {
        samples[] =
        {
            {"NimbyTapes\sound\timing_b", 1}
        };
    };
};

// One set per track: positional, mono, not looping. Processor and curve are the vanilla
// ChristmasMusic_SoundSet's (DZ/sounds/hpp/config.cpp:194396-194407; recon question 4).
class CfgSoundSets
{
    class NimbyTapes_Timing_SoundSet_Base
    {
        sound3DProcessingType = "smallItem3DProcessingType";
        volumeCurve = "LinearCurve";
        volumeFactor = 1;
        spatial = 1;
        doppler = 0;
        loop = 0;
    };
    class NimbyTapes_TimingA_SoundSet : NimbyTapes_Timing_SoundSet_Base
    {
        soundShaders[] =
        {
            "NimbyTapes_TimingA_SoundShader"
        };
    };
    class NimbyTapes_TimingB_SoundSet : NimbyTapes_Timing_SoundSet_Base
    {
        soundShaders[] =
        {
            "NimbyTapes_TimingB_SoundShader"
        };
    };
};
