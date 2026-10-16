// -----------------------------------------------------------------------------
// NimbyEarplugs - client side, Phase 0 probe.
// Purpose : Reads UANimbyEarplugsToggle, applies or restores the scene effect
//           volume, and logs what the engine does to that level afterwards so the
//           vanilla reset points (Ears plan, "Vanilla reset points") are recorded
//           from a real client rather than from reading script.
// Logs    : [NIMBY][Earplugs] lines in the client script log (Print).
// Behavior: Observation only. The probe never reapplies the level after a reset;
//           Phase 1 decides whether and where to reapply from what this records.
// -----------------------------------------------------------------------------

modded class MissionGameplay extends MissionBase
{
    protected ref NimbyEarplugsLevel m_NimbyEarplugs;
    protected float m_NimbyEarplugsSampleClock;
    protected int m_NimbyEarplugsLastMenuId = -2;
    protected bool m_NimbyEarplugsResetLogged;

    override void OnInit()
    {
        super.OnInit();
        m_NimbyEarplugs = new NimbyEarplugsLevel();
        NimbyEarplugsLog("probe loaded; input=UANimbyEarplugsToggle saved_sound=" + g_Game.m_volume_sound.ToString()
            + " scene_sound=" + g_Game.GetSoundScene().GetSoundVolume().ToString()
            + " scene_voip=" + g_Game.GetSoundScene().GetVOIPVolume().ToString());
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        NimbyEarplugsTrackMenu();

        if (NimbyEarplugsPressed())
        {
            NimbyEarplugsToggle();
        }

        if (!m_NimbyEarplugs.IsIn())
        {
            return;
        }

        m_NimbyEarplugsSampleClock += timeslice;
        if (m_NimbyEarplugsSampleClock < 1.0)
        {
            return;
        }
        m_NimbyEarplugsSampleClock = 0;
        NimbyEarplugsSample();
    }

    // Vanilla restores m_volume_sound here (missionGameplay.c:1690). Logged so the test record can
    // tie a reset to the respawn rather than to whatever menu was open.
    override void OnPlayerRespawned(Man player)
    {
        super.OnPlayerRespawned(player);
        NimbyEarplugsLog("vanilla OnPlayerRespawned ran; scene_sound=" + g_Game.GetSoundScene().GetSoundVolume().ToString()
            + " earplugs_in=" + m_NimbyEarplugs.IsIn().ToString());
    }

    protected bool NimbyEarplugsPressed()
    {
        if (!GetUApi())
        {
            return false;
        }
        UAInput inp = GetUApi().GetInputByName("UANimbyEarplugsToggle");
        return inp && inp.LocalPress();
    }

    protected void NimbyEarplugsToggle()
    {
        float before = g_Game.GetSoundScene().GetSoundVolume();
        float voip = g_Game.GetSoundScene().GetVOIPVolume();
        float target;
        string detail;

        if (m_NimbyEarplugs.IsIn())
        {
            target = m_NimbyEarplugs.TakeOut();
            detail = "Earplugs out";
        }
        else
        {
            // E3: the saved level is the baseline and is never written. Fall back to the live scene
            // level if the saved field is not populated yet (DayZGame.c:1147 runs on option refresh).
            float saved = g_Game.m_volume_sound;
            if (saved <= 0)
            {
                saved = before;
            }
            target = m_NimbyEarplugs.PutIn(saved);
            detail = "Earplugs in";
        }

        g_Game.GetSoundScene().SetSoundVolume(target, NimbyEarplugsLevel.FADE_SECONDS);
        m_NimbyEarplugsResetLogged = false;
        m_NimbyEarplugsSampleClock = 0;

        NimbyEarplugsLog("toggle " + detail + "; scene_sound before=" + before.ToString() + " target=" + target.ToString()
            + " saved_sound=" + m_NimbyEarplugs.SavedLevel().ToString() + " scene_voip=" + voip.ToString()
            + " menu=" + NimbyEarplugsMenuName(NimbyEarplugsCurrentMenuId()));
        NotificationSystem.AddNotificationExtended(3, "Ears", detail);
    }

    // Once a second while earplugs are in: the scene level, VOIP, and the open menu. The first
    // sample that drifts from the expected level is flagged RESET with the menu that was open.
    protected void NimbyEarplugsSample()
    {
        float current = g_Game.GetSoundScene().GetSoundVolume();
        float expected = m_NimbyEarplugs.Expected();
        int menuId = NimbyEarplugsCurrentMenuId();
        string line = "sample scene_sound=" + current.ToString() + " expected=" + expected.ToString()
            + " scene_voip=" + g_Game.GetSoundScene().GetVOIPVolume().ToString()
            + " saved_sound=" + g_Game.m_volume_sound.ToString() + " menu=" + NimbyEarplugsMenuName(menuId);

        if (NimbyEarplugsLevel.IsReset(current, expected))
        {
            if (!m_NimbyEarplugsResetLogged)
            {
                m_NimbyEarplugsResetLogged = true;
                NimbyEarplugsLog("RESET " + line);
            }
            return;
        }
        if (m_NimbyEarplugsResetLogged)
        {
            m_NimbyEarplugsResetLogged = false;
            NimbyEarplugsLog("level back at expected; " + line);
        }
        NimbyEarplugsLog(line);
    }

    // Logs every change of the top UI menu so a RESET line can be matched to a menu open or close.
    protected void NimbyEarplugsTrackMenu()
    {
        int menuId = NimbyEarplugsCurrentMenuId();
        if (menuId == m_NimbyEarplugsLastMenuId)
        {
            return;
        }
        if (m_NimbyEarplugsLastMenuId != -2)
        {
            NimbyEarplugsLog("menu " + NimbyEarplugsMenuName(m_NimbyEarplugsLastMenuId) + " -> " + NimbyEarplugsMenuName(menuId)
                + "; scene_sound=" + g_Game.GetSoundScene().GetSoundVolume().ToString()
                + " earplugs_in=" + m_NimbyEarplugs.IsIn().ToString());
        }
        m_NimbyEarplugsLastMenuId = menuId;
    }

    protected int NimbyEarplugsCurrentMenuId()
    {
        UIScriptedMenu menu = GetGame().GetUIManager().GetMenu();
        if (!menu)
        {
            return -1;
        }
        return menu.GetID();
    }

    protected string NimbyEarplugsMenuName(int menuId)
    {
        if (menuId < 0)
        {
            return "none";
        }
        switch (menuId)
        {
            case MENU_INGAME: return "pause";
            case MENU_INVENTORY: return "inventory";
            case MENU_OPTIONS: return "options";
            case MENU_KEYBINDINGS: return "keybindings";
        }
        return "id" + menuId.ToString();
    }

    protected void NimbyEarplugsLog(string message)
    {
        Print("[NIMBY][Earplugs] " + message);
    }
}
