// -----------------------------------------------------------------------------
// NimbyGreet - the pause-menu button (plan v0.4, G7).
// Purpose : The "Server welcome" entry in the NimbyShared pause-menu registry (Plan 11 C6,
//           NimbyShared/Scripts/5_Mission/nimbyshared_pausebuttons.c), registered by
//           MissionGameplay.OnInit. NimbyShared adds the button under the vanilla pause menu's
//           button column; a click continues the game and sends a manual request (source=menu);
//           the window opens when the payload arrives and the pause menu is gone (G5 deferral).
//           Not "Server info": vanilla uses that for its IP panel (InGameMenu.c:50).
// Origin  : the handler body of nimbygreet_ingamemenu.c (October 25, 2026), whose button
//           creation moved to NimbyShared in Plan 11 Phase 1.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyGreetPauseEntry extends NimbySharedPauseEntry
{
    override string Id()
    {
        return "greet";
    }

    override string LabelKey()
    {
        return "#STR_NimbyGreet_PauseButton";
    }

    override string LabelFallback()
    {
        return "Server welcome";
    }

    override void OnClick()
    {
        NimbyGreetLog.Debug("pause menu: Server welcome clicked");
        Mission mission = g_Game.GetMission();
        if (mission)
        {
            mission.Continue();
        }
        MissionGameplay gameplay = MissionGameplay.Cast(g_Game.GetMission());
        if (gameplay)
        {
            gameplay.NimbyGreetSendRequest(true, "menu");
        }
    }
}
