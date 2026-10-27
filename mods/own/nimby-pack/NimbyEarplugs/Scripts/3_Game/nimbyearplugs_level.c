// -----------------------------------------------------------------------------
// NimbyEarplugs - engine-free level math and toggle state.
// Purpose : Everything about Ears that can be unit-tested without the engine:
//           the reduced level, the reset detector, the in/out state, the settle
//           window after an apply, the suppression while the player is dead or
//           unconscious, the reapply decision, the saved-level resolution with
//           its captured fallback, and the two press gates (menu open, player
//           dead or unconscious). The mission layer owns the engine calls; this
//           class never touches g_Game.
// Plan    : Ears E3 (15 percent of the launch value, 0.5 s fade), E7, E9, E12-E14 (v0.11;
//           E8 superseded by E14: the saved level is the launch value of the channel).
// -----------------------------------------------------------------------------
class NimbyEarplugsLevel
{
    // Fraction of the player's saved effect volume that remains with earplugs in (E3).
    static const float REDUCED_FRACTION = 0.15;
    // Fade in seconds for both directions (E3).
    static const float FADE_SECONDS = 0.5;
    // How far the scene level may drift from the expected level before it counts as a reset.
    static const float RESET_TOLERANCE = 0.01;
    // Seconds after an apply during which drift is the fade itself, not a reset (E9).
    static const float SETTLE_SECONDS = 1.0;
    // Minimum seconds between two reapplies, so a reset that fires every frame is logged
    // and corrected once a second instead of fought every frame (E9).
    static const float REAPPLY_INTERVAL_SECONDS = 1.0;

    protected bool m_In;
    protected bool m_Suppressed;
    protected float m_SinceApplied;
    protected float m_SinceReapply;
    protected int m_ReapplyCount;
    // Scene level captured while earplugs were out, used as the saved level only when the
    // saved field reads 0 (E13). Never re-read from the scene while earplugs are in.
    protected float m_Fallback;

    bool IsIn()
    {
        return m_In;
    }

    bool IsSuppressed()
    {
        return m_Suppressed;
    }

    int ReapplyCount()
    {
        return m_ReapplyCount;
    }

    // Earplugs go in. Returns the level to apply for the given saved level.
    float PutIn(float savedLevel)
    {
        m_In = true;
        OnApplied();
        return Reduced(savedLevel);
    }

    // Earplugs come out. Returns the level to apply (the saved level, unchanged). Suppression
    // ends with the state it belonged to, so the object never rests at in=false suppressed=true.
    float TakeOut(float savedLevel)
    {
        m_In = false;
        m_Suppressed = false;
        OnApplied();
        return savedLevel;
    }

    // The level Ears expects the scene to sit at for the given saved level (E14: the launch
    // value of the channel, so a slider move never moves the target; E8 superseded).
    float Expected(float savedLevel)
    {
        if (m_In)
        {
            return Reduced(savedLevel);
        }
        return savedLevel;
    }

    // E15: the baseline is the value DayZGame.DeferredInit cached once per client process
    // (g_Game.m_volume_sound, DayZGame.c:1147), the same value vanilla writes back on respawn
    // and on waking. The live channel is the fallback for a cache that reads 0: the channel
    // is 0 after a death-screen or unconscious exit in the same process (review row 21), so
    // reading it first would start a rejoin on the E13 path with a reduced baseline.
    static float PickBaseline(float cached, float scene)
    {
        if (cached > 0)
        {
            return cached;
        }
        return scene;
    }

    // The saved level Ears works from (E13, E14). The populated saved field, the launch value
    // of the channel read once in OnInit, wins. When it reads 0
    // the scene level stands in: captured while earplugs are out and held while they are in,
    // so a sample never reads the reduced level that Ears itself wrote back as the baseline
    // (one step per second toward silence, review E-2). A player whose slider really is 0 gets 0.
    float ResolveSaved(float savedField, float sceneLevel)
    {
        if (savedField > 0)
        {
            return savedField;
        }
        if (m_In)
        {
            return m_Fallback;
        }
        m_Fallback = sceneLevel;
        return m_Fallback;
    }

    // E12: the toggle key is ignored while any scripted menu is open. Chat input is the case
    // that matters (typing an n toggled earplugs); the inventory is blocked with the rest.
    static bool PressAllowed(int menuId)
    {
        return menuId < 0;
    }

    // E7: a press while dead or unconscious would write over the silence vanilla owns; ignored.
    static bool CanToggle(bool alive, bool unconscious)
    {
        return alive && !unconscious;
    }

    // Vanilla owns the level while the player is unconscious (fades to 0, PlayerBase.c:3778)
    // and dead (0, DayZPlayerImplement.c:870). While suppressed, Ears never reapplies (E7).
    void SetSuppressed(bool suppressed)
    {
        if (suppressed == m_Suppressed)
        {
            return;
        }
        m_Suppressed = suppressed;
        if (!suppressed)
        {
            // Vanilla restores the saved level on waking (PlayerBase.c:3832) with a 1 s fade;
            // give it the settle window before judging drift.
            OnApplied();
        }
    }

    // Call after every SetSoundVolume Ears issues.
    void OnApplied()
    {
        m_SinceApplied = 0;
        m_SinceReapply = 0;
    }

    void Tick(float dt)
    {
        m_SinceApplied = m_SinceApplied + dt;
        m_SinceReapply = m_SinceReapply + dt;
    }

    bool IsSettled()
    {
        return m_SinceApplied >= SETTLE_SECONDS;
    }

    // True when Ears should write the expected level back: earplugs in, not suppressed,
    // past the settle window and the reapply interval, and the scene has drifted (E9).
    bool ShouldReapply(float currentLevel, float savedLevel)
    {
        if (!m_In || m_Suppressed)
        {
            return false;
        }
        if (!IsSettled() || m_SinceReapply < REAPPLY_INTERVAL_SECONDS)
        {
            return false;
        }
        return IsReset(currentLevel, Expected(savedLevel));
    }

    // Records a reapply and returns the level to write.
    float Reapply(float savedLevel)
    {
        m_ReapplyCount = m_ReapplyCount + 1;
        m_SinceReapply = 0;
        m_SinceApplied = 0;
        return Expected(savedLevel);
    }

    static float Reduced(float savedLevel)
    {
        return savedLevel * REDUCED_FRACTION;
    }

    // True when the scene level no longer matches what Ears last applied: something else wrote it.
    static bool IsReset(float currentLevel, float expectedLevel)
    {
        return Math.AbsFloat(currentLevel - expectedLevel) > RESET_TOLERANCE;
    }

    // Engine-free validators for the $profile:NimbyEarplugs\selftest flag (shared rules 8).
    // Returns the number of failures; the caller prints one PASS|FAIL line per case.
    static int SelfTest(out array<string> results)
    {
        int failures = 0;
        NimbyEarplugsLevel s = new NimbyEarplugsLevel();

        failures += SelfTestCase(results, "starts out", !s.IsIn() && !s.IsSuppressed());
        float applied = s.PutIn(0.8);
        failures += SelfTestCase(results, "put in applies 15 percent", Near(applied, 0.12) && s.IsIn());
        failures += SelfTestCase(results, "expected while in follows saved", Near(s.Expected(0.8), 0.12) && Near(s.Expected(0.4), 0.06));
        failures += SelfTestCase(results, "fade sample is not a reset", !IsReset(0.125, 0.12));
        failures += SelfTestCase(results, "saved level is a reset", IsReset(0.8, 0.12));
        failures += SelfTestCase(results, "no reapply inside settle window", !s.ShouldReapply(0.8, 0.8));
        s.Tick(1.5);
        failures += SelfTestCase(results, "reapply after settle on drift", s.ShouldReapply(0.8, 0.8));
        failures += SelfTestCase(results, "no reapply without drift", !s.ShouldReapply(0.12, 0.8));
        float reapplied = s.Reapply(0.8);
        failures += SelfTestCase(results, "reapply writes expected and counts", Near(reapplied, 0.12) && s.ReapplyCount() == 1);
        failures += SelfTestCase(results, "reapply interval holds", !s.ShouldReapply(0.8, 0.8));
        s.Tick(1.5);
        s.SetSuppressed(true);
        failures += SelfTestCase(results, "suppressed never reapplies", !s.ShouldReapply(0.0, 0.8));
        s.SetSuppressed(false);
        failures += SelfTestCase(results, "unsuppress reopens settle window", !s.IsSettled() && !s.ShouldReapply(0.8, 0.8));
        s.Tick(1.5);
        failures += SelfTestCase(results, "reapply resumes after unsuppress", s.ShouldReapply(0.8, 0.8));
        float restored = s.TakeOut(0.8);
        failures += SelfTestCase(results, "take out restores saved", Near(restored, 0.8) && !s.IsIn());
        s.Tick(1.5);
        failures += SelfTestCase(results, "out never reapplies", !s.ShouldReapply(0.3, 0.8));
        failures += SelfTestCase(results, "zero saved stays zero", Near(Reduced(0), 0));

        // E13: saved-level resolution and the captured fallback.
        NimbyEarplugsLevel f = new NimbyEarplugsLevel();
        failures += SelfTestCase(results, "populated saved field wins", Near(f.ResolveSaved(0.8, 0.12), 0.8));
        float fallbackOut = f.ResolveSaved(0, 0.6);
        failures += SelfTestCase(results, "fallback while out reads scene", Near(fallbackOut, 0.6));
        f.PutIn(fallbackOut);
        float fallbackIn = f.ResolveSaved(0, 0.09);
        failures += SelfTestCase(results, "fallback while in keeps captured level", Near(fallbackIn, 0.6));
        float restoredFallback = f.TakeOut(f.ResolveSaved(0, 0.09));
        failures += SelfTestCase(results, "take out restores captured fallback", Near(restoredFallback, 0.6));
        NimbyEarplugsLevel z = new NimbyEarplugsLevel();
        float zeroOut = z.ResolveSaved(0, 0);
        float zeroApplied = z.PutIn(zeroOut);
        float zeroIn = z.ResolveSaved(0, 0);
        failures += SelfTestCase(results, "legitimate zero stays zero", Near(zeroOut, 0) && Near(zeroApplied, 0) && Near(zeroIn, 0));

        // E15: the cached launch value wins over a channel that death left at 0; a cache of 0 falls back to the channel.
        failures += SelfTestCase(results, "cached launch value wins over a silenced channel", Near(PickBaseline(1.0, 0), 1.0) && Near(PickBaseline(0.8, 0.12), 0.8));
        failures += SelfTestCase(results, "zero cache falls back to the channel", Near(PickBaseline(0, 0.6), 0.6) && Near(PickBaseline(0, 0), 0));

        // E12 and E7: the press gates. Menu ids: chat input 17, inventory 11 (constants.c:192, :186).
        failures += SelfTestCase(results, "press blocked while a menu is open", !PressAllowed(17) && !PressAllowed(11) && !PressAllowed(0));
        failures += SelfTestCase(results, "press allowed with no menu", PressAllowed(-1));
        failures += SelfTestCase(results, "toggle only while alive and conscious", CanToggle(true, false) && !CanToggle(false, false) && !CanToggle(true, true));
        NimbyEarplugsLevel u = new NimbyEarplugsLevel();
        u.PutIn(0.8);
        u.SetSuppressed(true);
        u.TakeOut(0.8);
        failures += SelfTestCase(results, "take out clears suppressed", !u.IsSuppressed() && !u.IsIn());

        // E14: the launch value of the channel is the saved level; in writes 0.15 of it, the
        // reduced channel read while in never moves it, and take-out writes it back.
        NimbyEarplugsLevel l = new NimbyEarplugsLevel();
        float launch = l.ResolveSaved(1, 0.5);
        float launchIn = l.PutIn(launch);
        float launchHeld = l.ResolveSaved(1, launchIn);
        float launchOut = l.TakeOut(launchHeld);
        failures += SelfTestCase(results, "launch value restored on take out", Near(launch, 1) && Near(launchIn, 0.15) && Near(launchHeld, 1) && Near(launchOut, 1));
        return failures;
    }

    protected static bool Near(float a, float b)
    {
        return Math.AbsFloat(a - b) < 0.0001;
    }

    protected static int SelfTestCase(out array<string> results, string name, bool passed)
    {
        if (passed)
        {
            results.Insert("PASS " + name);
            return 0;
        }
        results.Insert("FAIL " + name);
        return 1;
    }
}
