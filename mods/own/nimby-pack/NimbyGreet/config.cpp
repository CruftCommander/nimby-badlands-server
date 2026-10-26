// -----------------------------------------------------------------------------
// NimbyGreet - Greet: the welcome window. Server text in $profile:NimbyGreet\greet.json
// (markdown subset, one or more languages), fetched by the client after spawn, shown
// as a tabbed window with link buttons, media blocks, a hotkey, and a dismissal.
// Plan: docs/plans/2026-10-21-greet-plan.md (v0.3). Owner: Jeff Fabish.
// Phase 0 probe (October 2026): the stub plus the probe that settles the eleven engine
// questions in plan Phase 0. 4_World holds the PlayerBase RPC plumbing, 5_Mission the
// probe window, the client orchestration, and the server request handler. Probe code is
// replaced in Phase 1. No save footprint, ever (shared rules 6).
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
        inputs = "NimbyGreet/Data/Inputs.xml";
        dependencies[] = { "Game", "World", "Mission" };
        class defs
        {
            class gameScriptModule
            {
                files[] = { "NimbyGreet/Scripts/3_Game" };
            };
            class worldScriptModule
            {
                files[] = { "NimbyGreet/Scripts/4_World" };
            };
            class missionScriptModule
            {
                files[] = { "NimbyGreet/Scripts/5_Mission" };
            };
        };
    };
};
