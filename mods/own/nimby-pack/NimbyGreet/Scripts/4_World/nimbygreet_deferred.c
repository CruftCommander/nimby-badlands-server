// -----------------------------------------------------------------------------
// NimbyGreet - deferred calls (NIMBY-T-052).
// Purpose : The one function every Greet CallLater queues. The 5_Mission script module is
//           compiled afresh for every mission ("Module: Mission; loaded ..." at each mission
//           start, the main menu included), so a call queued from 5_Mission code, instance or
//           static, that fires after the player leaves points into a discarded module: an
//           access violation in DayZGame.OnUpdate (dayzgame.c:3044, the GUI call queue tick),
//           three times in Phase 1 run 5, about 20 s after a request (the fetch give-up).
//           ScriptCallQueue.Remove did not prevent it. 4_World is not recompiled per mission
//           (NimbyHands queues its static OnTimeout from 4_World the same way), so Fire lives
//           here and reaches the mission through a hook on MissionBaseWorld (4_World,
//           MissionBaseWorld.c:1); the engine class Mission cannot be modded (build
//           20261026-092027: "Engine class 'Mission' cannot be modded").
// Contract: Greet's MissionGameplay overrides NimbyGreetDeferredCall and drops a call whose
//           connection number is not the current one; every other mission ignores it.
// -----------------------------------------------------------------------------

class NimbyGreetDeferred
{
    static const int AUTO_REQUEST = 1;
    static const int FETCH_TIMEOUT = 2;
    static const int RETRY_REQUEST = 3;
    static const int TRY_OPEN = 4;

    // Bumped by MissionGameplay in OnInit and OnMissionFinish; kept here so it survives the reload.
    static int s_Connection;

    static void Fire(int kind, int connection, int param)
    {
        if (!g_Game)
        {
            return;
        }
        MissionBaseWorld mission = MissionBaseWorld.Cast(g_Game.GetMission());
        if (mission)
        {
            mission.NimbyGreetDeferredCall(kind, connection, param);
        }
    }
}

modded class MissionBaseWorld
{
    // Overridden by Greet's MissionGameplay; every other mission (the main menu included) ignores it.
    void NimbyGreetDeferredCall(int kind, int connection, int param)
    {
    }
}
