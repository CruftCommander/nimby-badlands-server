// NimbyAutorun - toggleable autorun for NIMBY | HARDCORE | BADLANDS | 1pp.
// Fork of SchanaModAutorun (Apache-2.0); see NOTICE for origin and changes.
class CfgPatches
{
    class NimbyAutorun
    {
        requiredAddons[] = {};
        units[] = {};
        weapons[] = {};
    };
};

class CfgMods
{
    class NimbyAutorun
    {
        type = "mod";
        inputs = "NimbyAutorun/Data/Inputs.xml";
        dependencies[] =
        {
            "World",
            "Mission"
        };
        class defs
        {
            class worldScriptModule
            {
                files[] = { "NimbyAutorun/Scripts/4_world" };
            };
            class missionScriptModule
            {
                files[] = { "NimbyAutorun/Scripts/5_mission" };
            };
        };
    };
};
