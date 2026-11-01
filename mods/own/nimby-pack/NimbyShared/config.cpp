// -----------------------------------------------------------------------------
// NimbyShared - the shared client addon: the window shell every NIMBY menu subclasses
// (NimbySharedWindow, the section column, open and close with the input exclude and the
// zeroing guard, focus painting, text-size cycle, pause-menu buttons) and the chunked
// server-to-client JSON codec (NimbySharedChunks, NimbySharedChunkReceiver,
// NimbySharedChunkSender). Extracted from Greet; consumed by Greet and Central Scrutinizer.
// Plan: docs/plans/2026-10-31-nimby-shared-plan.md (v0.1). Owner: Jeff Fabish.
// Stub on main so the pack builds with the pack.json row in place (shared rules 3, the
// NimbyTack and NimbyGreet stub precedent). The addon body lands from the feat/nimby-shared
// worktree. Layers: 3_Game holds the engine-free codec and the self-test; 5_Mission the
// window base class, the pause-menu registry, and the self-test hook. No 4_World module:
// the deferred-call hook stays per addon (plan C2, shared rules 7.1). No save footprint,
// ever (shared rules 6). References no other addon (plan C3).
// -----------------------------------------------------------------------------
class CfgPatches
{
    class NimbyShared
    {
        requiredAddons[] = { "DZ_Data" };
        units[] = {};
        weapons[] = {};
    };
};

class CfgMods
{
    class NimbyShared
    {
        type = "mod";
        dependencies[] = { "Game", "Mission" };
        class defs
        {
            class gameScriptModule
            {
                files[] = { "NimbyShared/Scripts/3_Game" };
            };
            class missionScriptModule
            {
                files[] = { "NimbyShared/Scripts/5_Mission" };
            };
        };
    };
};
