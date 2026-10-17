// NimbyTack - Tack: pin a sheet of paper to a wall; pull it down by hand. Content addon.
// Stub on main so the pack builds with the pack.json row in place (Tack Phase 1 starts
// October 16, 2026). The addon body lands from the feat/tack worktree.
class CfgPatches
{
    class NimbyTack
    {
        requiredAddons[] = { "DZ_Data", "DZ_Gear_Consumables", "NimbyNotes" };
        units[] = {};
        weapons[] = {};
    };
};

class CfgMods
{
    class NimbyTack
    {
        type = "mod";
        dependencies[] = { "World" };
    };
};
