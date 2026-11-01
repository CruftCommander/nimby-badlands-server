// -----------------------------------------------------------------------------
// NimbyAdminServer - Central Scrutinizer, server half: the command router, the permissions
// and ban stores, the audit journal, the Discord delivery, and the command handlers. Loaded
// with -servermod= only; never signed for clients, never in the Workshop item (shared rules
// 8.1, plan N7). pack.json marks it "serverOnly": true, so Build-NimbyPack.ps1 writes it to
// build\@NIMBY-Server-<stamp>\.
// Plan: docs/plans/2026-10-15-nimby-admin-plan.md (v0.8). Owner: Jeff Fabish.
// Layers: 3_Game holds the engine-free rules, the stores, the audit journal, the Discord queue
// and the self-test; 4_World the sessions, the router and the handlers (PlayerBase.OnRPC);
// 5_Mission the MissionServer hooks. Reads the wire definition from the client half NimbyAdmin
// (requiredAddons[]), which also loads on the server; the reverse is forbidden (shared rules 8.1). Production log prefix is [NIMBY][Admin] like the client half
// (shared rules 4.1); the self-test component is AdminServer so the boot check's gate counts
// the two halves apart (shared rules 8, amended October 31, 2026). MAY use the NimbyShared
// codec (shared rules 8.1, v1.8); MUST NOT reference the window shell. No save footprint.
// -----------------------------------------------------------------------------
class CfgPatches
{
    class NimbyAdminServer
    {
        requiredAddons[] = { "DZ_Data", "NimbyShared", "NimbyAdmin" };
        units[] = {};
        weapons[] = {};
    };
};

class CfgMods
{
    class NimbyAdminServer
    {
        type = "mod";
        dependencies[] = { "Game", "World", "Mission" };
        class defs
        {
            class gameScriptModule
            {
                files[] = { "NimbyAdminServer/Scripts/3_Game" };
            };
            class worldScriptModule
            {
                files[] = { "NimbyAdminServer/Scripts/4_World" };
            };
            class missionScriptModule
            {
                files[] = { "NimbyAdminServer/Scripts/5_Mission" };
            };
        };
    };
};
