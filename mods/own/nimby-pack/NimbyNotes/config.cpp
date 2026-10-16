// NimbyNotes - Scrawl: Paper stacks to 5, written notes never merge. Content addon.
// Stub on main so the pack builds with the pack.json row in place (Scrawl Phase 1 starts
// October 16, 2026). The addon body lands from the feat/scrawl worktree.
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
        dependencies[] = { "World" };
    };
};
