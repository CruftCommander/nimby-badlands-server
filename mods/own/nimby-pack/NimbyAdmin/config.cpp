// -----------------------------------------------------------------------------
// NimbyAdmin - Central Scrutinizer, client half: the admin menu (a NimbySharedWindow
// subclass with a page registry), the request sender, and the admin-state receiver.
// Decides nothing; hidden unless the server says the player has a role.
// Plan: docs/plans/2026-10-15-nimby-admin-plan.md (v0.7). Owner: Jeff Fabish.
// Stub on main so the pack builds with the pack.json row in place (shared rules 3, the
// NimbyTack, NimbyGreet, and NimbyShared stub precedent). The body lands from the
// feat/nimby-admin worktree. Layers: 3_Game holds the engine-free rules (RPC ids, request
// envelope validation) and the self-test; 5_Mission the window and the mission hooks.
// Server-only code lives in NimbyAdminServer (shared rules 8.1, plan N7); no script here
// MAY reference a class of that PBO. Requires NimbyShared (plan N16). No save footprint.
// -----------------------------------------------------------------------------
class CfgPatches
{
    class NimbyAdmin
    {
        requiredAddons[] = { "DZ_Data", "NimbyShared" };
        units[] = {};
        weapons[] = {};
    };
};

class CfgMods
{
    class NimbyAdmin
    {
        type = "mod";
        dependencies[] = { "Game", "Mission" };
        class defs
        {
            class gameScriptModule
            {
                files[] = { "NimbyAdmin/Scripts/3_Game" };
            };
            class missionScriptModule
            {
                files[] = { "NimbyAdmin/Scripts/5_Mission" };
            };
        };
    };
};
