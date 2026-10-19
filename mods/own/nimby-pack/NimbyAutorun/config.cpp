// NimbyAutorun - Legs, toggleable autorun for NIMBY | HARDCORE | BADLANDS | 1pp.
// Fork of SchanaModAutorun (Apache-2.0); see NOTICE for origin and changes.
// Plan: docs/plans/2026-10-15-autorun-plan.md. Utility addon (shared rules 6): no save footprint.
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
            "Game",
            "World",
            "Mission"
        };
        class defs
        {
            // Engine-free rules, config, and logging, covered by the self-test (shared rules 8).
            class gameScriptModule
            {
                files[] = { "NimbyAutorun/Scripts/3_game" };
            };
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
