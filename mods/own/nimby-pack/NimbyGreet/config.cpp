// -----------------------------------------------------------------------------
// NimbyGreet - Greet: the welcome window. Server text in $profile:NimbyGreet\greet.json
// (markdown subset, one or more languages), fetched by the client after spawn, shown
// as a tabbed window with link buttons, media blocks, a hotkey, and a dismissal.
// Plan: docs/plans/2026-10-21-greet-plan.md (v0.3). Owner: Jeff Fabish.
// Stub on main so the pack builds with the pack.json row in place (the Ears E6
// precedent): rules constants and one self-test case, nothing else. The addon body
// lands from the feat/greet worktree. No save footprint, ever (shared rules 6).
// -----------------------------------------------------------------------------
class CfgPatches
{
    class NimbyGreet
    {
        requiredAddons[] = { "DZ_Data" };
        units[] = {};
        weapons[] = {};
    };
};

class CfgMods
{
    class NimbyGreet
    {
        type = "mod";
        dependencies[] = { "Game", "Mission" };
        class defs
        {
            class gameScriptModule
            {
                files[] = { "NimbyGreet/Scripts/3_Game" };
            };
            class missionScriptModule
            {
                files[] = { "NimbyGreet/Scripts/5_Mission" };
            };
        };
    };
};
