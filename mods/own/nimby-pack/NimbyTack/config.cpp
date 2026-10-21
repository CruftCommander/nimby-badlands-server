// -----------------------------------------------------------------------------
// NimbyTack - Tack. Pin a sheet of paper to a wall with a hammer and nail (or a
// blunt tool), duct tape, epoxy putty, or a knife through the sheet; anyone pulls
// it down by hand at the cost of some damage to the sheet.
// Plan: docs/plans/2026-10-15-tack-plan.md (v0.13). Owner: Jeff Fabish.
// Content addon (shared rules 6.1): the pinned note is a persistent class of its
// own (T3) and nothing is written to the vanilla Paper class. Removing the addon
// drops only pinned notes; vanilla sheets and Scrawl stacks are unaffected.
// -----------------------------------------------------------------------------
class CfgPatches
{
    class NimbyTack
    {
        requiredAddons[] = { "DZ_Data", "DZ_Gear_Consumables", "NimbyNotes" };
        units[] = { "NimbyTack_PinnedNote_Nail", "NimbyTack_PinnedNote_Tape", "NimbyTack_PinnedNote_Epoxy", "NimbyTack_PinnedNote_Knife" };
        weapons[] = {};
    };
};

class CfgMods
{
    class NimbyTack
    {
        type = "mod";
        dependencies[] = { "Game", "World" };
        class defs
        {
            class gameScriptModule
            {
                files[] = { "NimbyTack/Scripts/3_Game" };
            };
            class worldScriptModule
            {
                files[] = { "NimbyTack/Scripts/4_World" };
            };
        };
    };
};

class CfgVehicles
{
    class Paper;
    // T3: the pinned note is a Paper subclass, so note text, health levels, model, and the
    // vanilla read action are inherited. The class records the fastener; no data is written
    // for it. scope 1 keeps the base out of the CE spawn list; the fastener variants are the
    // classes that exist in the world and in types.xml (server/mission-overrides/db/types.tack.patch.xml).
    class NimbyTack_PinnedNote : Paper
    {
        scope = 1;
        displayName = "Pinned note";
        descriptionShort = "A sheet of paper fixed to a wall. Pull it down to take it; the sheet tears a little each time.";
        // A pinned sheet is never a stack (Scrawl N2 gives Paper a 1-5 quantity; the pinned class
        // is always one sheet, T2).
        canBeSplit = 0;
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 1;
        varQuantityDestroyOnMin = 0;
    };
    class NimbyTack_PinnedNote_Nail : NimbyTack_PinnedNote
    {
        scope = 2;
        displayName = "Nailed note";
    };
    class NimbyTack_PinnedNote_Tape : NimbyTack_PinnedNote
    {
        scope = 2;
        displayName = "Taped note";
    };
    class NimbyTack_PinnedNote_Epoxy : NimbyTack_PinnedNote
    {
        scope = 2;
        displayName = "Glued note";
    };
    // T2 knife fastener: the knife that pinned the sheet lives in the Knife attachment slot of
    // the pinned note (knives carry inventorySlot[] = {"Knife"}, DZ/weapons/melee/blade/config.cpp:989),
    // so the knife entity and its health persist as vanilla cargo, not as new data.
    class NimbyTack_PinnedNote_Knife : NimbyTack_PinnedNote
    {
        scope = 2;
        displayName = "Knifed note";
        attachments[] = { "Knife" };
    };
};
