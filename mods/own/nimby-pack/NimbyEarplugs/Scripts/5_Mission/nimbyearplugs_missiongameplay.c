// -----------------------------------------------------------------------------
// NimbyEarplugs - client side.
// Purpose : Reads UANimbyEarplugsToggle, applies or restores the scene effect
//           volume, reapplies it when something else writes the level (E9), and
//           stands down while vanilla owns the level (dead or unconscious, E7).
// Logs    : [NIMBY][Earplugs] lines in the client script log (Print).
//           Production: reapplies and the death take-out only (shared rules 4.3).
//           Debug ($profile:NimbyEarplugs\debug): toggles, menu changes, the first
//           drifted sample as RESET, one sample per second while earplugs are in,
//           and the vanilla respawn restore. These are the Phase 0 evidence.
// Style   : One statement per line and no multi-line expressions. The Enforce
//           compiler rejected a string built across continuation lines
//           ("Invalid statement ')'", build 20261016-162750).
// -----------------------------------------------------------------------------

modded class MissionGameplay extends MissionBase
{
    protected ref NimbyEarplugsLevel m_NimbyEarplugs;
    protected bool m_NimbyEarplugsDebug;
    protected float m_NimbyEarplugsSampleClock;
    protected int m_NimbyEarplugsLastMenuId = -2;
    protected bool m_NimbyEarplugsResetLogged;

    override void OnInit()
    {
        super.OnInit();
        m_NimbyEarplugs = new NimbyEarplugsLevel();
        m_NimbyEarplugsDebug = FileExist("$profile:NimbyEarplugs\\debug");
        string line = "loaded; input=UANimbyEarplugsToggle debug=" + NimbyEarplugsBool(m_NimbyEarplugsDebug);
        line = line + NimbyEarplugsLevels();
        NimbyEarplugsDebugLog(line);
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

        m_NimbyEarplugs.Tick(timeslice);

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (player)
        {
            if (!player.IsAlive())
            {
                // E7: vanilla silences everything at death and restores the saved level on
                // respawn; earplugs come out so the two agree.
                m_NimbyEarplugs.TakeOut(NimbyEarplugsSavedLevel());
                NimbyEarplugsLog("earplugs out: player died; vanilla owns the level until respawn");
                return;
            }
            m_NimbyEarplugs.SetSuppressed(player.IsUnconscious());
        }

        m_NimbyEarplugsSampleClock = m_NimbyEarplugsSampleClock + timeslice;
        if (m_NimbyEarplugsSampleClock < 1.0)
        {
            return;
        }
        m_NimbyEarplugsSampleClock = 0;
        NimbyEarplugsSampleAndReapply();
    }

    // Vanilla restores m_volume_sound here (missionGameplay.c:1690). Logged so the test record can
    // tie a reset to the respawn rather than to whatever menu was open.
    override void OnPlayerRespawned(Man player)
    {
        super.OnPlayerRespawned(player);
        string line = "vanilla OnPlayerRespawned ran;";
        line = line + NimbyEarplugsLevels();
        line = line + " earplugs_in=" + NimbyEarplugsBool(m_NimbyEarplugs.IsIn());
        NimbyEarplugsDebugLog(line);
    }

    protected bool NimbyEarplugsPressed()
    {
        if (!GetUApi())
        {
            return false;
        }
        UAInput inp = GetUApi().GetInputByName("UANimbyEarplugsToggle");
        if (!inp)
        {
            return false;
        }
        return inp.LocalPress();
    }

    // E3: the saved level is the baseline and is never written. Falls back to the live scene
    // level if the saved field is not populated yet (DayZGame.c:1147 runs on option refresh).
    protected float NimbyEarplugsSavedLevel()
    {
        float saved = g_Game.m_volume_sound;
        if (saved <= 0)
        {
            saved = g_Game.GetSoundScene().GetSoundVolume();
        }
        return saved;
    }

    protected void NimbyEarplugsToggle()
    {
        float before = g_Game.GetSoundScene().GetSoundVolume();
        float saved = NimbyEarplugsSavedLevel();
        float target;
        string detail;

        if (m_NimbyEarplugs.IsIn())
        {
            target = m_NimbyEarplugs.TakeOut(saved);
            detail = "Earplugs out";
        }
        else
        {
            target = m_NimbyEarplugs.PutIn(saved);
            detail = "Earplugs in";
        }

        g_Game.GetSoundScene().SetSoundVolume(target, NimbyEarplugsLevel.FADE_SECONDS);
        m_NimbyEarplugsResetLogged = false;
        m_NimbyEarplugsSampleClock = 0;

        string line = "toggle " + detail + "; before=" + before.ToString() + " target=" + target.ToString();
        line = line + NimbyEarplugsLevels();
        line = line + " menu=" + NimbyEarplugsMenuName(NimbyEarplugsCurrentMenuId());
        NimbyEarplugsDebugLog(line);
        NotificationSystem.AddNotificationExtended(3, "Ears", detail);
    }

    // Once a second while earplugs are in: log the sample (debug), flag the first drift as RESET
    // (debug), and write the expected level back when the state machine says so (production).
    protected void NimbyEarplugsSampleAndReapply()
    {
        float current = g_Game.GetSoundScene().GetSoundVolume();
        float saved = NimbyEarplugsSavedLevel();
        float expected = m_NimbyEarplugs.Expected(saved);
        string menu = NimbyEarplugsMenuName(NimbyEarplugsCurrentMenuId());
        string line = "expected=" + expected.ToString();
        line = line + NimbyEarplugsLevels();
        line = line + " menu=" + menu;

        bool drifted = NimbyEarplugsLevel.IsReset(current, expected);
        if (drifted && !m_NimbyEarplugsResetLogged)
        {
            m_NimbyEarplugsResetLogged = true;
            NimbyEarplugsDebugLog("RESET " + line);
        }
        if (!drifted && m_NimbyEarplugsResetLogged)
        {
            m_NimbyEarplugsResetLogged = false;
            NimbyEarplugsDebugLog("level back at expected; " + line);
        }

        if (m_NimbyEarplugs.ShouldReapply(current, saved))
        {
            float target = m_NimbyEarplugs.Reapply(saved);
            g_Game.GetSoundScene().SetSoundVolume(target, NimbyEarplugsLevel.FADE_SECONDS);
            int count = m_NimbyEarplugs.ReapplyCount();
            string reapply = "reapply #" + count.ToString() + " from=" + current.ToString() + " to=" + target.ToString() + " menu=" + menu;
            NimbyEarplugsLog(reapply);
            return;
        }

        NimbyEarplugsDebugLog("sample " + line);
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
            string line = "menu " + NimbyEarplugsMenuName(m_NimbyEarplugsLastMenuId) + " -> " + NimbyEarplugsMenuName(menuId) + ";";
            line = line + NimbyEarplugsLevels();
            line = line + " earplugs_in=" + NimbyEarplugsBool(m_NimbyEarplugs.IsIn());
            NimbyEarplugsDebugLog(line);
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
        if (menuId == MENU_INGAME)
        {
            return "pause";
        }
        if (menuId == MENU_INVENTORY)
        {
            return "inventory";
        }
        if (menuId == MENU_OPTIONS)
        {
            return "options";
        }
        if (menuId == MENU_KEYBINDINGS)
        {
            return "keybindings";
        }
        return "id" + menuId.ToString();
    }

    // " saved_sound=<g_Game.m_volume_sound> scene_sound=<scene level> scene_voip=<VOIP level>"
    protected string NimbyEarplugsLevels()
    {
        float savedSound = g_Game.m_volume_sound;
        float sceneSound = g_Game.GetSoundScene().GetSoundVolume();
        float sceneVoip = g_Game.GetSoundScene().GetVOIPVolume();
        string line = " saved_sound=" + savedSound.ToString();
        line = line + " scene_sound=" + sceneSound.ToString();
        line = line + " scene_voip=" + sceneVoip.ToString();
        return line;
    }

    protected string NimbyEarplugsBool(bool value)
    {
        if (value)
        {
            return "true";
        }
        return "false";
    }

    protected void NimbyEarplugsLog(string message)
    {
        Print("[NIMBY][Earplugs] " + message);
    }

    protected void NimbyEarplugsDebugLog(string message)
    {
        if (!m_NimbyEarplugsDebug)
        {
            return;
        }
        Print("[NIMBY][Earplugs] " + message);
    }
}
