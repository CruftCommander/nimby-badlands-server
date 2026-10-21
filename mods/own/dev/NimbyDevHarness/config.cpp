// -----------------------------------------------------------------------------
// NimbyDevHarness - the dev harness (shared rules 8). Built only with
// Build-NimbyPack.ps1 -Dev, signed with the separate nimby_dev key, never listed in
// pack.json "addons", never released. Runs forged requests against the real Legs
// and Hands handlers on the server and prints [NIMBY][DevHarness][SelfTest] lines
// the boot check reads. No client PBO may reference a class defined here.
// -----------------------------------------------------------------------------
class CfgPatches
{
    class NimbyDevHarness
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = { "DZ_Data", "DZ_Scripts", "NimbyAutorun", "NimbyHands" };
    };
};

class CfgMods
{
    class NimbyDevHarness
    {
        name = "NimbyDevHarness";
        dir = "NimbyDevHarness";
        author = "Jeff Fabish";
        type = "mod";
        hideName = 1;
        hidePicture = 1;
        dependencies[] = { "Game", "World", "Mission" };
        class defs
        {
            class worldScriptModule
            {
                value = "";
                files[] = { "NimbyDevHarness/scripts/4_world" };
            };
            class missionScriptModule
            {
                value = "";
                files[] = { "NimbyDevHarness/scripts/5_mission" };
            };
        };
    };
};
