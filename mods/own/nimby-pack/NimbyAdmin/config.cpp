// -----------------------------------------------------------------------------
// NimbyAdmin - Central Scrutinizer, client half: the admin menu (a NimbySharedWindow
// subclass with a page registry), the request sender, and the admin-state receiver.
// Decides nothing; hidden unless the server says the player has a role.
// Plan: docs/plans/2026-10-15-nimby-admin-plan.md (v0.8). Owner: Jeff Fabish.
// Layers: 3_Game holds the wire definition both halves read (RPC ids, the request envelope
// rules, the key=value args codec, the payload classes), the log helper, the client bridge and
// the self-test; 4_World the PlayerBase RPC plumbing (client only); 5_Mission the window, the
// page registry, the pause entry and the mission hooks. Server-only code lives in
// NimbyAdminServer (shared rules 8.1, plan N7); no script here MAY reference a class of that PBO.
// Requires NimbyShared (plan N16). No save footprint.
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
        inputs = "NimbyAdmin/Data/Inputs.xml";
        dependencies[] = { "Game", "World", "Mission" };
        class defs
        {
            class gameScriptModule
            {
                files[] = { "NimbyAdmin/Scripts/3_Game" };
            };
            class worldScriptModule
            {
                files[] = { "NimbyAdmin/Scripts/4_World" };
            };
            class missionScriptModule
            {
                files[] = { "NimbyAdmin/Scripts/5_Mission" };
            };
        };
    };
};
