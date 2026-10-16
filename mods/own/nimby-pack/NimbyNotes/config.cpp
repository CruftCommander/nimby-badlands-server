// -----------------------------------------------------------------------------
// NimbyNotes - Scrawl. Paper stacks to 5, written notes never merge, and the
// 1.30 pen-and-paper write and read actions are reachable again.
// Plan: docs/plans/2026-10-15-scrawl-plan.md (v0.3). Owner: Jeff Fabish.
// Content addon (shared rules 6.1): the Paper quantity override changes what an
// existing sheet saves, so the save-reload round trip and the removal boot gate it.
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
        dependencies[] = { "Game", "World" };
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
