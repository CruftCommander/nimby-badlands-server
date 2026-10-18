// -----------------------------------------------------------------------------
// NimbyEarplugs - engine-free level math and toggle state.
// Purpose : Everything about Ears that can be unit-tested without the engine:
//           the reduced level, the reset detector, the in/out state, the settle
//           window after an apply, the suppression while the player is dead or
//           unconscious, and the reapply decision. The mission layer owns the
//           engine calls; this class never touches g_Game.
// Plan    : Ears E3 (15 percent of the saved level, 0.5 s fade), E7-E9 (v0.6).
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

    // Earplugs come out. Returns the level to apply (the saved level, unchanged).
    float TakeOut(float savedLevel)
    {
        m_In = false;
        OnApplied();
        return savedLevel;
    }

    // The level Ears expects the scene to sit at for the given saved level (E8: the saved
    // level is read live, so a slider change while earplugs are in moves the target with it).
    float Expected(float savedLevel)
    {
        if (m_In)
        {
            return Reduced(savedLevel);
        }
        return savedLevel;
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
