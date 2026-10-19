// Decoded from the original rapified config.bin of Give&Take (Workshop 2903112334) on 2026-10-14.
// Changed since import: logo entries removed; addon, CfgMods class, and paths renamed, first to
// the October 14 fork name and on October 19, 2026 to NimbyHands (Hands plan G1, Phase 2); input
// bindings moved to Data/Inputs.xml (see NOTICE). Original author credits kept.
class CfgPatches
{
    class NimbyHands
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.10000000149011612;
        requiredAddons[] = {"DZ_Data", "DZ_Scripts"};
    };
};
class CfgMods
{
    class NimbyHands
    {
        name = "NimbyHands";
        dir = "NimbyHands";
        author = "affenb3rt";
        credits = "Wardog";
        type = "mod";
        hideName = 0;
        hidePicture = 0;
        tooltip = "NimbyHands";
        overview = "NimbyHands";
        inputs = "NimbyHands/Data/Inputs.xml";
        dependencies[] = {"Game", "World", "Mission"};
        class defs
        {
            class gameScriptModule
            {
                value = "";
                files[] = {"NimbyHands/scripts/3_game"};
            };
            class worldScriptModule
            {
                value = "";
                files[] = {"NimbyHands/scripts/4_world"};
            };
            class missionScriptModule
            {
                value = "";
                files[] = {"NimbyHands/scripts/5_mission"};
            };
        };
    };
};
