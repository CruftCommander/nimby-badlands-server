// -----------------------------------------------------------------------------
// NimbyAdmin - the pause-menu entry (Plan 3 v0.8 component table, recon 8.5).
// Purpose : "Central Scrutinizer" in the NimbyShared pause-menu registry (Plan 11 C6,
//           NimbyShared/Scripts/5_Mission/nimbyshared_pausebuttons.c). Visible only while the
//           server has told this client it holds a role; the server still authorizes every
//           request. A click continues the game and asks MissionGameplay to open the window once
//           no menu is up (the pause menu closes after this frame), with no deferred call.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyAdminPauseEntry extends NimbySharedPauseEntry
{
    override string Id()
    {
        return "nimbyadmin";
    }

    override string LabelFallback()
    {
        return "Central Scrutinizer";
    }

    override bool IsVisible()
    {
        MissionGameplay gameplay = MissionGameplay.Cast(g_Game.GetMission());
        if (!gameplay)
        {
            return false;
        }
        return gameplay.NimbyAdminHasRole();
    }

    override void OnClick()
    {
        NimbyAdminLog.Debug("pause menu: Central Scrutinizer clicked");
        Mission mission = g_Game.GetMission();
        if (mission)
        {
            mission.Continue();
        }
        MissionGameplay gameplay = MissionGameplay.Cast(g_Game.GetMission());
        if (gameplay)
        {
            gameplay.NimbyAdminOpenWhenClear("menu");
        }
    }
}
