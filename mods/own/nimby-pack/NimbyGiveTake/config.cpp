// Decoded from the original rapified config.bin of Give&Take (Workshop 2903112334) on 2026-10-14.
// Content unchanged; only the binary form was converted to text.
class CfgPatches
{
    class GiveAndTake
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.10000000149011612;
        requiredAddons[] = {"DZ_Data", "DZ_Scripts"};
    };
};
class CfgMods
{
    class GiveAndTake
    {
        name = "GiveAndTake";
        dir = "GiveAndTake/Scripts";
        author = "affenb3rt";
        credits = "Wardog";
        type = "mod";
        hideName = 0;
        hidePicture = 0;
        picture = "GiveAndTake/Scripts/logo.paa";
        logoSmall = "GiveAndTake/Scripts/logo.paa";
        logo = "GiveAndTake/Scripts/logo.paa";
        logoOver = "GiveAndTake/Scripts/logo.paa";
        tooltip = "GiveAndTake";
        overview = "GiveAndTake";
        inputs = "GiveAndTake/Scripts/inputs.xml";
        dependencies[] = {"Game", "World", "Mission"};
        class defs
        {
            class gameScriptModule
            {
                value = "";
                files[] = {"GiveAndTake/Scripts/scripts/3_Game"};
            };
            class worldScriptModule
            {
                value = "";
                files[] = {"GiveAndTake/Scripts/scripts/4_World"};
            };
            class missionScriptModule
            {
                value = "";
                files[] = {"GiveAndTake/Scripts/scripts/5_Mission"};
            };
        };
    };
};
