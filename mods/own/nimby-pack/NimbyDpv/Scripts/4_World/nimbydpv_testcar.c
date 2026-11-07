// -----------------------------------------------------------------------------
// NimbyDpv - Phase 0 script class for NimbyDpv_TestCar. Every behavior is the vanilla
// OffroadHatchback's (4_World/DayZ/Entities/Vehicles/InheritedCars/OffroadHatchback.c:1);
// it only adds the Phase 1 diagnostics so the ADA serves as the baseline for the DPV.
// -----------------------------------------------------------------------------
class NimbyDpv_TestCar extends OffroadHatchback
{
    protected ref Timer m_DiagTimer;

    void ~NimbyDpv_TestCar()
    {
        if (m_DiagTimer)
        {
            m_DiagTimer.Stop();
            m_DiagTimer = null;
        }
    }

    override void EEInit()
    {
        super.EEInit();
        if ((g_Game.IsServer() || !g_Game.IsMultiplayer()) && NimbyDpvDiag.Enabled())
        {
            m_DiagTimer = new Timer(CALL_CATEGORY_SYSTEM);
            m_DiagTimer.Run(5.0, this, "LogDiag", null, true);
        }
    }

    void LogDiag()
    {
        NimbyDpvDiag.Log(this, "ada");
    }
}
