// NimbyEarplugs - Ears: one key lowers effect volume and restores it. Client-only.
// Stub on main so the pack builds with the pack.json row in place (Ears plan E6).
// The addon body lands from the feat/ears worktree.
class CfgPatches
{
    class NimbyEarplugs
    {
        requiredAddons[] = { "DZ_Data" };
        units[] = {};
        weapons[] = {};
    };
};

class CfgMods
{
    class NimbyEarplugs
    {
        type = "mod";
        dependencies[] = { "Mission" };
    };
};
