// -----------------------------------------------------------------------------
// NimbyEarplugs - client side.
// Purpose : Reads UANimbyEarplugsToggle, applies or restores the scene effect
//           volume, reapplies it when something else writes the level (E9), and
//           stands down while vanilla owns the level (dead or unconscious, E7).
//           The key is ignored while any scripted menu is open (E12) and while
//           the player is dead or unconscious; earplugs come out when the
//           mission ends so a disconnect never carries the reduced level into
//           the main menu.
// Logs    : [NIMBY][Earplugs] lines in the client script log (Print).
//           Production: reapplies, the death take-out, and the mission-finish
//           take-out (shared rules 4.3: forced clears and audit records only).
//           Debug ($profile:NimbyEarplugs\debug): loaded, toggles, ignored
//           presses, menu changes, the first drifted sample as RESET, one sample
//           per second while earplugs are in, and the vanilla respawn restore.
//           The sample line is built only when the flag is set.
// Vanilla : GetUApi().GetInputByName, UAInput.GetPersistentWrapper, UAIDWrapper.InputP, and
//           UAInput.LocalPress (UAInput.c:218, :99, :24-29, :60);
//           UIManager.GetMenu and UIScriptedMenu.GetID (UIManager.c:59,
//           UIScriptedMenu.c:151); SoundScene GetSoundVolume/SetSoundVolume
//           (Sound.c:71-72) is the scene effect channel: a multiplier the engine
//           applies on top of the Options Effects slider, read at 1 on every
//           launch, cached once by DayZGame.DeferredInit into g_Game.m_volume_sound
//           (DayZGame.c:1147, scheduled at :1093) and restored from that cache on
//           respawn (missionGameplay.c:1690) and on waking (PlayerBase.c:3832).
//           The Effects slider is a separate quantity: its option store
//           (OptionsMenuSounds.c:363-366) reads on its own 0 to 10 range
//           (NumericOptionsAccess.GetMin/GetMax, gameplay.c:1425-1426) and Options
//           Apply never writes the channel (test record, NIMBY-T-018 and
//           NIMBY-T-019), so Ears never reads the slider (E14). OnMissionFinish
//           (gameplay.c:702, overridden at missionGameplay.c:266);
//           Man.IsUnconscious (Man.c:63, PlayerBase.c:3925), Object.IsAlive
//           (Object.c:517).
// Style   : One statement per line and no multi-line expressions. The Enforce
//           compiler rejected a string built across continuation lines
//           ("Invalid statement ')'", build 20261016-162750).
// -----------------------------------------------------------------------------

modded class MissionGameplay extends MissionBase
{
    protected ref NimbyEarplugsLevel m_NimbyEarplugs;
    // Resolved once in OnInit as the persistent wrapper, the handle vanilla keeps across an input
    // rebind (UAInput.c GetPersistentWrapper; UAIDWrapper.InputP re-resolves the UAInput). A bare
    // UAInput cached in OnInit went stale after an Options > Controls rebind (EP-005 re-run).
    protected UAIDWrapper m_NimbyEarplugsInput;
    // The launch value of the scene effect channel (E14, E15): the DeferredInit cache when it is
    // populated, else the channel itself, read once in OnInit before any press.
    // Earplugs in writes 0.15 of it; take-out writes it back. The Options Effects slider is the
    // player's own setting, applied by the engine on top of this channel, and is never read.
    protected float m_NimbyEarplugsLaunch;
    protected bool m_NimbyEarplugsDebug;
    protected float m_NimbyEarplugsSampleClock;
    protected int m_NimbyEarplugsLastMenuId = -2;
    protected bool m_NimbyEarplugsResetLogged;

    override void OnInit()
    {
        super.OnInit();
        m_NimbyEarplugs = new NimbyEarplugsLevel();
        m_NimbyEarplugsDebug = FileExist("$profile:NimbyEarplugs\\debug");
        if (GetUApi())
        {
            UAInput toggle = GetUApi().GetInputByName("UANimbyEarplugsToggle");
            if (toggle)
            {
                m_NimbyEarplugsInput = toggle.GetPersistentWrapper();
            }
        }
        float cachedLaunch = g_Game.m_volume_sound;
        float sceneLaunch = g_Game.GetSoundScene().GetSoundVolume();
        m_NimbyEarplugsLaunch = NimbyEarplugsLevel.PickBaseline(cachedLaunch, sceneLaunch);
        string launchFrom = "scene";
        if (cachedLaunch > 0)
        {
            launchFrom = "cache";
        }
        bool bound = false;
        if (m_NimbyEarplugsInput)
        {
            bound = true;
        }
        string line = "loaded; input=UANimbyEarplugsToggle bound=" + NimbyEarplugsBool(bound);
        line = line + NimbyEarplugsLaunchToken();
        line = line + " launch_from=" + launchFrom;
        line = line + " debug=" + NimbyEarplugsBool(m_NimbyEarplugsDebug);
        line = line + NimbyEarplugsLevels();
        NimbyEarplugsDebugLog(line);
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        NimbyEarplugsTrackMenu();

        bool pressed = NimbyEarplugsPressed();
        if (!pressed && !m_NimbyEarplugs.IsIn())
        {
            return;
        }

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        bool alive = true;
        bool unconscious = false;
        if (player)
        {
            alive = player.IsAlive();
            unconscious = player.IsUnconscious();
        }

        if (pressed)
        {
            if (NimbyEarplugsLevel.CanToggle(alive, unconscious))
            {
                NimbyEarplugsToggle();
            }
            else
            {
                NimbyEarplugsDebugLog("press ignored: player dead or unconscious; vanilla owns the level");
            }
        }

        if (!m_NimbyEarplugs.IsIn())
        {
            return;
        }

        m_NimbyEarplugs.Tick(timeslice);

        if (player)
        {
            if (!alive)
            {
                // E7: vanilla silences everything at death and restores the saved level on
                // respawn; earplugs come out so the two agree.
                m_NimbyEarplugs.TakeOut(NimbyEarplugsSavedLevel());
                NimbyEarplugsLog("earplugs out: player died; vanilla owns the level until respawn");
                return;
            }
            m_NimbyEarplugs.SetSuppressed(unconscious);
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

    // Nothing in vanilla restores the scene level between leaving a server and the main menu's
    // own write (MainMenuVideo.c:32-42), so earplugs come out here. The Relog row of the
    // release-1 record (October 21, 2026) saw this hook run on the exit path.
    override void OnMissionFinish()
    {
        if (m_NimbyEarplugs && m_NimbyEarplugs.IsIn())
        {
            float target = m_NimbyEarplugs.TakeOut(NimbyEarplugsSavedLevel());
            g_Game.GetSoundScene().SetSoundVolume(target, 0);
            string line = "earplugs out: mission finished; target=" + target.ToString();
            line = line + NimbyEarplugsLaunchToken();
            line = line + NimbyEarplugsLevels();
            NimbyEarplugsLog(line);
        }
        super.OnMissionFinish();
    }

    // E12: a press counts only when no scripted menu is open. An ignored press is logged (debug)
    // so the chat row has positive evidence that the key reached the input and was refused.
    protected bool NimbyEarplugsPressed()
    {
        if (!m_NimbyEarplugsInput)
        {
            return false;
        }
        UAInput toggle = m_NimbyEarplugsInput.InputP();
        if (!toggle)
        {
            return false;
        }
        if (!toggle.LocalPress())
        {
            return false;
        }
        int menuId = NimbyEarplugsCurrentMenuId();
        if (NimbyEarplugsLevel.PressAllowed(menuId))
        {
            return true;
        }
        NimbyEarplugsDebugLog("press ignored: menu=" + NimbyEarplugsMenuName(menuId) + " open");
        return false;
    }

    // E3, E13, E14: the saved level is the launch value of the channel and is never written.
    // When that read was 0 (E13), the scene level captured while earplugs were out stands in.
    protected float NimbyEarplugsSavedLevel()
    {
        float scene = g_Game.GetSoundScene().GetSoundVolume();
        return m_NimbyEarplugs.ResolveSaved(m_NimbyEarplugsLaunch, scene);
    }

    // " launch_sound=<channel value read in OnInit>"
    protected string NimbyEarplugsLaunchToken()
    {
        return " launch_sound=" + m_NimbyEarplugsLaunch.ToString();
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
        line = line + NimbyEarplugsLaunchToken();
        line = line + NimbyEarplugsLevels();
        NimbyEarplugsDebugLog(line);
        NotificationSystem.AddNotificationExtended(3, "Ears", detail);
    }

    // Once a second while earplugs are in: write the expected level back when the state machine
    // says so (production), and with the debug flag log the sample, flag the first drift as
    // RESET, and note when the level is back. The sample line is built only under the flag.
    protected void NimbyEarplugsSampleAndReapply()
    {
        float current = g_Game.GetSoundScene().GetSoundVolume();
        float saved = NimbyEarplugsSavedLevel();
        float expected = m_NimbyEarplugs.Expected(saved);
        bool drifted = NimbyEarplugsLevel.IsReset(current, expected);

        string line = "";
        if (m_NimbyEarplugsDebug)
        {
            line = "expected=" + expected.ToString();
            line = line + NimbyEarplugsLaunchToken();
            line = line + NimbyEarplugsLevels();
            line = line + " menu=" + NimbyEarplugsMenuName(NimbyEarplugsCurrentMenuId());
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
        }

        if (m_NimbyEarplugs.ShouldReapply(current, saved))
        {
            float target = m_NimbyEarplugs.Reapply(saved);
            g_Game.GetSoundScene().SetSoundVolume(target, NimbyEarplugsLevel.FADE_SECONDS);
            int count = m_NimbyEarplugs.ReapplyCount();
            string reapply = "reapply #" + count.ToString() + " from=" + current.ToString() + " to=" + target.ToString();
            reapply = reapply + NimbyEarplugsLaunchToken();
            reapply = reapply + NimbyEarplugsLevels();
            reapply = reapply + " menu=" + NimbyEarplugsMenuName(NimbyEarplugsCurrentMenuId());
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

    // Menu ids from 3_Game/DayZ/constants.c:184-209 (MENU_LOGOUT at :201).
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
        if (menuId == MENU_CHAT_INPUT)
        {
            return "chat";
        }
        if (menuId == MENU_NOTE)
        {
            return "note";
        }
        if (menuId == MENU_LOGOUT)
        {
            return "logout";
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
