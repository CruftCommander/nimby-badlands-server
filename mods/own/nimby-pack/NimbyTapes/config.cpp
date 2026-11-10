// -----------------------------------------------------------------------------
// NimbyTapes - Music. A cassette deck ("Cassette Player" in game), tapes, and a 9V battery:
// a switched-on deck plays its tape as positional sound that nearby players hear in step.
// Plan: docs/plans/2026-11-09-music-plan.md (Plan 13, v0.3). Owner: Jeff Fabish.
// Stub on main so the pack builds with the pack.json row in place (shared rules 3; the
// NimbyTack, NimbyGreet, and NimbyShared stub precedent). The addon body lands from the
// feat/nimby-music worktree (Plan 13 Phase 0, steps 2 to 4).
// The stub defines no CfgVehicles class, no slot, no sound, and nothing that is saved.
// Phase 0 step 3 adds them, and with them the requiredAddons the plan names (DZ_Radio,
// DZ_Gear_Tools, DZ_Gear_Consumables).
// Layers: 3_Game holds the engine-free rules and the self-test; 4_World the mission-start
// hook and, from Phase 0, the deck, the tape, and the actions. No 5_Mission module and no
// Inputs.xml: controls are action-menu entries only (T-3).
// Content addon once the body lands (shared rules 6.1 and 6.3).
// -----------------------------------------------------------------------------
class CfgPatches
{
    class NimbyTapes
    {
        requiredAddons[] = { "DZ_Data" };
        units[] = {};
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
