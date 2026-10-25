// -----------------------------------------------------------------------------
// NimbyNotes - Scrawl. Paper stacks to 5, written notes never merge (on the server
// and, through a net-synced flag, on the client), the 1.30 pen-and-paper write and
// read actions are reachable again in every order (pen in hands, sheet in hands, the
// inventory Combine drop), a note is capped at vanilla's 512 characters, and the
// server accepts a write only from the writer.
// Plan: docs/plans/2026-10-15-scrawl-plan.md (v0.9). Owner: Jeff Fabish.
// Content addon (shared rules 6.1): the Paper quantity override changes what an
// existing sheet saves, so the save-reload round trip and the removal boot gate it.
// The modded Paper.OnStoreSave writes no byte (decision N14).
// -----------------------------------------------------------------------------
class CfgPatches
{
    class NimbyNotes
    {
        requiredAddons[] = { "DZ_Data", "DZ_Gear_Consumables" };
        units[] = {};
        weapons[] = {};
    };
};

class CfgMods
{
    class NimbyNotes
    {
        type = "mod";
        dependencies[] = { "Game", "World", "Mission" };
        class defs
        {
            class gameScriptModule
            {
                files[] = { "NimbyNotes/Scripts/3_Game" };
            };
            class worldScriptModule
            {
                files[] = { "NimbyNotes/Scripts/4_World" };
            };
            class missionScriptModule
            {
                files[] = { "NimbyNotes/Scripts/5_Mission" };
            };
        };
    };
};

class CfgVehicles
{
    class Inventory_Base;
    // N2: vanilla Paper gains a quantity of 1-5 and splits like Rag (DZ/gear/consumables/config.cpp:343-346).
    // Sheets saved before Scrawl carry no quantity bit and load as 1 (ItemBase.c:3011-3014).
    // Everything else about Paper (model, size, slot, flammability, note data) is inherited unchanged.
    class Paper : Inventory_Base
    {
        canBeSplit = 1;
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 5;
        varQuantityDestroyOnMin = 1;
    };
};
