// -----------------------------------------------------------------------------
// NimbyGreet - the pause-menu button (plan v0.4, G7).
// Purpose : Adds a "Server welcome" button to the vanilla pause menu (InGameMenu, layout
//           gui/layouts/day_z_ingamemenu.layout) at runtime, under the existing button column
//           (the "top" WrapSpacer, :314), from NimbyGreet/GUI/nimbygreet_pause_button.layout,
//           which copies the shape of optionsbtn (:458-496) so the vanilla hover code
//           (ButtonSetColor finds "<name>_panel", InGameMenu.c:403-416) paints it like the rest.
//           A click continues the game and sends a manual request (source=menu); the window
//           opens when the payload arrives and the pause menu is gone (G5 deferral).
//           Not "Server info": vanilla uses that for its IP panel (InGameMenu.c:50).
// Style   : One statement per line.
// -----------------------------------------------------------------------------

modded class InGameMenu extends UIScriptedMenu
{
    protected Widget m_NimbyGreetButton;

    override Widget Init()
    {
        Widget root = super.Init();
        if (!root)
        {
            return root;
        }
        Widget column = root.FindAnyWidget("top");
        if (!column)
        {
            NimbyGreetLog.Info("pause menu: button column 'top' not found; no Server welcome button");
            return root;
        }
        m_NimbyGreetButton = g_Game.GetWorkspace().CreateWidgets("NimbyGreet/GUI/nimbygreet_pause_button.layout", column);
        if (!m_NimbyGreetButton)
        {
            NimbyGreetLog.Info("pause menu: nimbygreet_pause_button.layout failed to load");
            return root;
        }
        TextWidget label = TextWidget.Cast(m_NimbyGreetButton.FindAnyWidget("nimbygreetbtn_label"));
        if (label)
        {
            label.SetText(NimbyGreetMenu.Tr("#STR_NimbyGreet_PauseButton", "Server welcome"));
        }
        return root;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && w == m_NimbyGreetButton)
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
            return true;
        }
        return super.OnClick(w, x, y, button);
    }
}
