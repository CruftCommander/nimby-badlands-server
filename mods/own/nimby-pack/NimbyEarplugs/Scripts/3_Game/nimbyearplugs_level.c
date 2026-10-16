// -----------------------------------------------------------------------------
// NimbyEarplugs - engine-free level math and toggle state.
// Purpose : Everything about Ears that can be unit-tested without the engine:
//           the reduced level, the reset detector, and the in/out state machine.
//           The mission layer owns the engine calls; this class never touches g_Game.
// Plan    : Ears E3 (15 percent of the player's saved level, 0.5 s fade).
// -----------------------------------------------------------------------------
class NimbyEarplugsLevel
{
    // Fraction of the player's saved effect volume that remains with earplugs in (E3).
    static const float REDUCED_FRACTION = 0.15;
    // Fade in seconds for both directions (E3).
    static const float FADE_SECONDS = 0.5;
    // How far the scene level may drift from the expected level before it counts as a reset.
    // SetSoundVolume fades, so a sample taken inside the fade window is not a reset.
    static const float RESET_TOLERANCE = 0.01;

    protected bool m_In;
    protected float m_SavedLevel;

    bool IsIn()
    {
        return m_In;
    }

    float SavedLevel()
    {
        return m_SavedLevel;
    }

    // Returns the level to apply when earplugs go in; remembers the saved level for TakeOut.
    float PutIn(float savedLevel)
    {
        m_In = true;
        m_SavedLevel = savedLevel;
        return Reduced(savedLevel);
    }

    // Returns the level to apply when earplugs come out.
    float TakeOut()
    {
        m_In = false;
        return m_SavedLevel;
    }

    float Expected()
    {
        if (m_In)
        {
            return Reduced(m_SavedLevel);
        }
        return m_SavedLevel;
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

        failures += SelfTestCase(results, "starts out", !s.IsIn());
        float applied = s.PutIn(0.8);
        failures += SelfTestCase(results, "put in applies 15 percent", Math.AbsFloat(applied - 0.12) < 0.0001 && s.IsIn());
        failures += SelfTestCase(results, "expected while in is reduced", Math.AbsFloat(s.Expected() - 0.12) < 0.0001);
        failures += SelfTestCase(results, "fade sample is not a reset", !IsReset(0.125, 0.12));
        failures += SelfTestCase(results, "saved level is a reset", IsReset(0.8, 0.12));
        float restored = s.TakeOut();
        failures += SelfTestCase(results, "take out restores saved", Math.AbsFloat(restored - 0.8) < 0.0001 && !s.IsIn());
        failures += SelfTestCase(results, "zero saved stays zero", Math.AbsFloat(Reduced(0)) < 0.0001);
        return failures;
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
