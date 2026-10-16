// -----------------------------------------------------------------------------
// NimbyEarplugs - Ears. One key lowers the game's effect volume and restores it.
// Client-only: no server script, no RPC, no item, no save footprint. VOIP untouched.
// Plan: docs/plans/2026-10-15-earplugs-plan.md (v0.5). Owner: Jeff Fabish.
// Phase 0 probe: binds N, toggles the level, and logs every observation to the
// client script log so the vanilla reset points can be recorded.
// -----------------------------------------------------------------------------
class CfgPatches
{
    class NimbyEarplugs
    {
        requiredAddons[] = { "DZ_Data" };
        units[] = {};
        weapons[] = {};
    };
};

class CfgMods
{
    class NimbyEarplugs
    {
        type = "mod";
        inputs = "NimbyEarplugs/Data/Inputs.xml";
        dependencies[] = { "Game", "Mission" };
        class defs
        {
            class gameScriptModule
            {
                files[] = { "NimbyEarplugs/Scripts/3_Game" };
            };
            class missionScriptModule
            {
                files[] = { "NimbyEarplugs/Scripts/5_Mission" };
            };
        };
    };
};
