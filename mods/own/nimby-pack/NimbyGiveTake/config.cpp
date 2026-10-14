// Decoded from the original rapified config.bin of Give&Take (Workshop 2903112334) on 2026-10-14.
// Changed since import: logo entries removed; addon, CfgMods class, and paths renamed to
// NimbyGiveTake (see NOTICE). Original author credits kept.
class CfgPatches
{
    class NimbyGiveTake
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.10000000149011612;
        requiredAddons[] = {"DZ_Data", "DZ_Scripts"};
    };
};
class CfgMods
{
    class NimbyGiveTake
    {
        name = "NimbyGiveTake";
        dir = "NimbyGiveTake";
        author = "affenb3rt";
        credits = "Wardog";
        type = "mod";
        hideName = 0;
        hidePicture = 0;
        tooltip = "NimbyGiveTake";
        overview = "NimbyGiveTake";
        inputs = "NimbyGiveTake/inputs.xml";
        dependencies[] = {"Game", "World", "Mission"};
        class defs
        {
            class gameScriptModule
            {
                value = "";
                files[] = {"NimbyGiveTake/scripts/3_game"};
            };
            class worldScriptModule
            {
                value = "";
                files[] = {"NimbyGiveTake/scripts/4_world"};
            };
            class missionScriptModule
            {
                value = "";
                files[] = {"NimbyGiveTake/scripts/5_mission"};
            };
        };
    };
};
