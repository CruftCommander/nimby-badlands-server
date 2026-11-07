// -----------------------------------------------------------------------------
// NimbyDpv - test spawn (server only, flag-gated, never on by default).
// Purpose : The local test server has no vehicle spawn (NIMBY-T-003, open since
//           October 14, 2026; Central Scrutinizer spawning is its Phase 2). While the
//           file $profile:NimbyDpv\spawn-testcar exists, the first player to connect
//           after each boot gets one vehicle six metres ahead, with the debug-spawn
//           fill (wheels, battery, plug, fluids; no radiator, the DPV is air-cooled). Each non-empty line of the
//           file names one class, optionally followed by a space and a delay in
//           milliseconds ("NimbyDpv_Car 500"): the car is created at once, its debug-spawn
//           fill runs after the delay (NIMBY-T-069 probe 2a, November 6, 2026: does a wheel
//           attached a few frames after the body exists become present?). An empty file
//           means NimbyDpv_Car. One set of cars per boot. Delete the flag file to turn it
//           off; the release gate boots have no flag file. The flag state is logged once per
//           boot, at the first connect. Each NimbyDpv_Car gets the diag tag
//           "car<n>/delay<ms>" so several DPVs in one run can be told apart in the log.
//           Build 33 (November 6, 2026): a line "@ <x> <z>" moves the connecting player to that
//           map position (height from the road surface) before the cars are placed, so a test
//           can start on tarmac; the cars then face north from there. The surface type at the
//           point is logged. Without the line the player's own position is used, as before.
// Hook    : MissionServer.InvokeOnConnect (5_Mission/DayZ/mission/missionServer.c:435),
//           which both the new-character path (:336, ClientNewEventTypeID) and the
//           ready path (:357, ClientReadyEventTypeID) call; OnClientReadyEvent (:599)
//           is NOT called for a new character (NIMBY-T-067, November 5, 2026).
// API     : CGame.CreateObjectEx (3_Game/DayZ/Global/Game.c:703),
//           ECE_PLACE_ON_SURFACE (3_Game/DayZ/CE/CentralEconomy.c:38),
//           Object.GetDirection (3_Game/DayZ/Entities/Object.c:314),
//           EntityAI.OnDebugSpawn (3_Game/DayZ/Entities/EntityAI.c:3955),
//           CGame.GetCallQueue (3_Game/DayZ/Global/Game.c:1549),
//           ScriptCallQueue.CallLater (2_GameLib/DayZ/tools.c:61),
//           string.Split / ToInt / ToFloat (1_Core/DayZ/proto/EnString.c:396, :20, :46),
//           CGame.SurfaceRoadY / SurfaceGetType (3_Game/DayZ/Global/Game.c:1177, :1180),
//           Object.SetPosition (3_Game/DayZ/Entities/Object.c:294).
// Logging : shared rules 4.2, the player line carries GetId() and the Steam64 id.
// -----------------------------------------------------------------------------
modded class MissionServer extends MissionBase
{
    protected bool m_NimbyDpvTestCarSpawned;
    protected bool m_NimbyDpvTestCarFlagLogged;

    override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
    {
        super.InvokeOnConnect(player, identity);
        NimbyDpvSpawnTestCarIfFlagged(identity, player);
    }

    protected void NimbyDpvSpawnTestCarIfFlagged(PlayerIdentity identity, PlayerBase player)
    {
        if (m_NimbyDpvTestCarSpawned || !player || !identity)
        {
            return;
        }
        string flagPath = "$profile:NimbyDpv\\spawn-testcar";
        bool flagged = FileExist(flagPath);
        // one "<class> [<debug-spawn delay ms>]" per line; an empty file means NimbyDpv_Car
        array<string> classNames = new array<string>();
        array<int> spawnDelays = new array<int>();
        bool hasSpawnPos = false;
        vector spawnPos = vector.Zero;
        if (flagged)
        {
            FileHandle fh = OpenFile(flagPath, FileMode.READ);
            if (fh != 0)
            {
                string line;
                while (FGets(fh, line) >= 0)
                {
                    line.Trim();
                    if (line.Length() > 0)
                    {
                        array<string> parts = new array<string>();
                        line.Split(" ", parts);
                        if (parts[0] == "@" && parts.Count() >= 3)
                        {
                            spawnPos[0] = parts[1].ToFloat();
                            spawnPos[2] = parts[2].ToFloat();
                            spawnPos[1] = GetGame().SurfaceRoadY(spawnPos[0], spawnPos[2]);
                            hasSpawnPos = true;
                            continue;
                        }
                        classNames.Insert(parts[0]);
                        int delayMs = 0;
                        if (parts.Count() > 1)
                        {
                            delayMs = parts[1].ToInt();
                        }
                        spawnDelays.Insert(delayMs);
                    }
                }
                CloseFile(fh);
            }
        }
        if (classNames.Count() == 0)
        {
            classNames.Insert("NimbyDpv_Car");
            spawnDelays.Insert(0);
        }
        if (!m_NimbyDpvTestCarFlagLogged)
        {
            m_NimbyDpvTestCarFlagLogged = true;
            Print("[NIMBY][Dpv] test-car flag=" + flagged.ToString() + " classes=" + classNames.Count().ToString() + " first=" + classNames[0] + " path=" + flagPath);
        }
        if (!flagged)
        {
            return;
        }
        m_NimbyDpvTestCarSpawned = true;

        // each car 6 m further ahead and 5 m to the right of the previous one
        vector forward = player.GetDirection();
        vector origin = player.GetPosition();
        if (hasSpawnPos)
        {
            string surfaceType;
            GetGame().SurfaceGetType(spawnPos[0], spawnPos[2], surfaceType);
            player.SetPosition(spawnPos);
            origin = spawnPos;
            forward = "0 0 1";
            Print("[NIMBY][Dpv] test-car spawn position override at=" + spawnPos.ToString() + " surface=" + surfaceType + " player=" + identity.GetId() + "/" + identity.GetPlainId());
        }
        vector right = forward.Perpend();
        for (int i = 0; i < classNames.Count(); i++)
        {
            string className = classNames[i];
            vector position = origin + forward * 6.0 + right * (5.0 * i);
            Object spawned = GetGame().CreateObjectEx(className, position, ECE_PLACE_ON_SURFACE);
            EntityAI car = EntityAI.Cast(spawned);
            if (!car)
            {
                Print("[NIMBY][Dpv] test-car spawn FAILED class=" + className + " at=" + position.ToString() + " player=" + identity.GetId() + "/" + identity.GetPlainId());
                continue;
            }
            int delay = spawnDelays[i];
            string diagTag = "car" + i.ToString() + "/delay" + delay.ToString();
            NimbyDpv_Car dpv = NimbyDpv_Car.Cast(car);
            if (dpv)
            {
                dpv.NimbyDpvSetDiagTag(diagTag);
            }
            Print("[NIMBY][Dpv] test-car spawned class=" + className + " tag=" + diagTag + " at=" + car.GetPosition().ToString() + " debugSpawnDelayMs=" + delay.ToString() + " player=" + identity.GetId() + "/" + identity.GetPlainId());
            if (delay > 0)
            {
                GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.NimbyDpvDebugSpawnLater, delay, false, car, diagTag);
            }
            else
            {
                NimbyDpvDebugSpawnLater(car, diagTag);
            }
        }
    }

    // the debug-spawn fill, now or after the flag line's delay, followed by one diag snapshot
    // so the wheel state right after the attach can be compared with the 5 s timer lines
    protected void NimbyDpvDebugSpawnLater(EntityAI car, string diagTag)
    {
        if (!car)
        {
            Print("[NIMBY][Dpv] test-car debug-spawn SKIPPED tag=" + diagTag + " (car gone)");
            return;
        }
        car.OnDebugSpawn();
        Print("[NIMBY][Dpv] test-car debug-spawn done tag=" + diagTag + " class=" + car.GetType());
        CarScript carScript = CarScript.Cast(car);
        if (carScript && NimbyDpvDiag.Enabled())
        {
            NimbyDpvDiag.Log(carScript, diagTag + "/post-debugspawn");
        }
    }
}
