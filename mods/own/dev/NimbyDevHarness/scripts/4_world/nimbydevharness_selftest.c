// -----------------------------------------------------------------------------
// NimbyDevHarness - mission-start hook (shared rules 8). Server only: the boot
// cases run when $profile:NimbyDevHarness\selftest exists, so the Build-NimbyPack
// boot check sees the [NIMBY][DevHarness][SelfTest] lines and the completion line.
// -----------------------------------------------------------------------------
modded class MissionBaseWorld
{
    override void OnInit()
    {
        super.OnInit();
        NimbyDevHarness.OnMissionStart();
    }
}
