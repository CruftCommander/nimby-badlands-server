// -----------------------------------------------------------------------------
// NimbyEarplugs - Ears. One key lowers the game's effect volume and restores it.
// Client-only: no server script, no RPC, no item, no save footprint. VOIP untouched.
// Plan: docs/plans/2026-10-15-earplugs-plan.md (v0.9). Owner: Jeff Fabish.
// Phase 1 toggle plus the release-1 fixes: N toggles 15 percent of the saved
// level, reapplies on drift (E9), yields to vanilla while dead or unconscious
// (E7), ignores the key while a menu is open (E12), reads the stored Effects
// slider value as the saved level (E14) with the captured fallback behind it (E13). Production logging per shared rules 4.3;
// debug lines only with $profile:NimbyEarplugs\debug.
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
