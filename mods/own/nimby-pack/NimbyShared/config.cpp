// -----------------------------------------------------------------------------
// NimbyShared - the shared client addon: the window shell every NIMBY menu subclasses
// (NimbySharedWindow: the section column, open and close with the input exclude and the
// zeroing guard, focus painting, the text-size cycle, the page keys), the pause-menu button
// registry, and the chunked server-to-client JSON codec (NimbySharedChunks,
// NimbySharedChunkReceiver, NimbySharedChunkSender). Extracted from Greet; consumed by Greet
// and Central Scrutinizer.
// Plan: docs/plans/2026-10-31-nimby-shared-plan.md (v0.2). Owner: Jeff Fabish.
// Layers: 3_Game holds the engine-free codec, the sender, the log helper and the self-test;
// 5_Mission the window base class, the pause-menu registry, and the self-test hook. No 4_World
// module: the deferred-call hook stays per addon (plan C2, shared rules 7.1). No save
// footprint, ever (shared rules 6). References no other addon (plan C3); a server-only PBO
// MAY use NimbySharedChunks and NimbySharedChunkSender and MUST NOT reference the shell or the
// pause registry (shared rules 8.1).
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
        inputs = "NimbyShared/Data/Inputs.xml";
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
