// -----------------------------------------------------------------------------
// NimbyDpv - NimbyDpv_Car, the DPV script class (Phase 1).
// Mirrors the vanilla ADA (4_World/DayZ/Entities/Vehicles/InheritedCars/OffroadHatchback.c)
// where the DPV shares a mechanism, and drops what the DPV does not have: doors, windows, a
// hood, seat-back animations. Sounds reuse the ADA sound sets until the DPV gets its own.
// Seats: 0 driver, 1 co-driver, 2 rear (config Crew Cargo1). Seat reach goes through the
// seat_con_* memory points (CarScript.CanReachSeatFromDoors, CarScript.c:2177).
// Build 34 (November 7, 2026): server-side traction control for the locked rear axle, see the
// traction-control section below.
// -----------------------------------------------------------------------------
class NimbyDpv_Car extends CarScript
{
    protected ref UniversalTemperatureSource m_UTSource;
    protected ref UniversalTemperatureSourceSettings m_UTSSettings;
    protected ref UniversalTemperatureSourceLambdaEngine m_UTSLEngine;

    // Phase 1 test diagnostics: while $profile:NimbyDpv\spawn-testcar exists (the test-car
    // flag, nimbydpv_missionserver.c) the server logs the car's state every 5 s so seat
    // heights, ground sink and engine-start refusals can be read from the script log.
    protected ref Timer m_DiagTimer;
    // diag tag, so several DPVs in one test run can be told apart in the log
    // (set by nimbydpv_missionserver.c from the spawn flag line: "car<n>/delay<ms>")
    protected string m_NimbyDpvDiagTag = "car";

    void NimbyDpv_Car()
    {
        m_EngineStartOK = "offroad_engine_start_SoundSet";
        m_EngineStartBattery = "offroad_engine_failed_start_battery_SoundSet";
        m_EngineStartPlug = "offroad_engine_failed_start_sparkplugs_SoundSet";
        m_EngineStartFuel = "offroad_engine_failed_start_fuel_SoundSet";
        m_EngineStop = "offroad_engine_stop_SoundSet";
        m_EngineStopFuel = "offroad_engine_stop_fuel_SoundSet";
        m_CarSeatShiftInSound = "Offroad_SeatShiftIn_SoundSet";
        m_CarSeatShiftOutSound = "Offroad_SeatShiftOut_SoundSet";

        m_LightsComponent.RegisterLight("Front", new VehicleLightData(new OffroadHatchbackLightProfileFront()));

        m_HornComponent.RegisterSound(VehicleHornMode.SHORT, "Offroad_Horn_Short_SoundSet");
        m_HornComponent.RegisterSound(VehicleHornMode.LONG, "Offroad_Horn_SoundSet");

        SetEnginePos("0 0.6 -1.1");     // rear engine: model space x, y up, z forward
    }

    void ~NimbyDpv_Car()
    {
        if (m_DiagTimer)
        {
            m_DiagTimer.Stop();
            m_DiagTimer = null;
        }
        if (m_NimbyDpvRpmTimer)
        {
            m_NimbyDpvRpmTimer.Stop();
            m_NimbyDpvRpmTimer = null;
        }
        m_UTSource = null;
        m_UTSSettings = null;
        m_UTSLEngine = null;
    }

    override void EEInit()
    {
        super.EEInit();
        if (g_Game.IsServer() || !g_Game.IsMultiplayer())
        {
            m_UTSSettings = new UniversalTemperatureSourceSettings();
            m_UTSSettings.m_ManualUpdate = true;
            m_UTSSettings.m_TemperatureItemCap = GameConstants.ITEM_TEMPERATURE_NEUTRAL_ZONE_MIDDLE;
            m_UTSSettings.m_TemperatureCap = 0;
            m_UTSSettings.m_RangeFull = 0.5;
            m_UTSSettings.m_RangeMax = 2;
            m_UTSLEngine = new UniversalTemperatureSourceLambdaEngine();
            m_UTSource = new UniversalTemperatureSource(this, m_UTSSettings, m_UTSLEngine);

            if (NimbyDpvDiag.Enabled())
            {
                m_DiagTimer = new Timer(CALL_CATEGORY_SYSTEM);
                m_DiagTimer.Run(5.0, this, "LogDiag", null, true);
            }
        }
    }

    // ---- diagnostics (test builds only, NimbyDpvDiag)
    void NimbyDpvSetDiagTag(string tag)
    {
        m_NimbyDpvDiagTag = tag;
    }

    string NimbyDpvGetDiagTag()
    {
        return m_NimbyDpvDiagTag;
    }

    void LogDiag()
    {
        NimbyDpvDiag.Log(this, m_NimbyDpvDiagTag);
    }

    override bool OnBeforeEngineStart()
    {
        bool ok = super.OnBeforeEngineStart();
        if (NimbyDpvDiag.Enabled())
        {
            int reqState = CheckOperationalRequirements();
            bool server = g_Game.IsServer();
            Print("[NIMBY][Dpv][Diag] OnBeforeEngineStart ok=" + ok.ToString() + " reqState=" + reqState.ToString() + " server=" + server.ToString());
        }
        return ok;
    }

    override bool NeedElectricitySourceDevice()
    {
        return true;
    }

    override EntityAI GetElectricitySourceDevice()
    {
        super.GetElectricitySourceDevice();
        return GetInventory().FindAttachment(CarBattery.SLOT_ID);
    }

    override string GetElectricitySourceDeviceType()
    {
        return "CarBattery";
    }

    override void OnEngineStart()
    {
        super.OnEngineStart();
        if ((g_Game.IsServer() || !g_Game.IsMultiplayer()) && m_UTSource)
        {
            m_UTSource.SetDefferedActive(true, 20.0);
        }
        NimbyDpvLogEngineEvent("OnEngineStart");
    }

    override void OnEngineStop()
    {
        super.OnEngineStop();
        if ((g_Game.IsServer() || !g_Game.IsMultiplayer()) && m_UTSource)
        {
            m_UTSource.SetDefferedActive(false, 10.0);
        }
        NimbyDpvLogEngineEvent("OnEngineStop");
    }

    // test builds only: the engine's state at every start and stop, so a start that dies in
    // the same tick (NIMBY-T-069 follow-up, November 6, 2026) shows its rpm and gear
    protected ref Timer m_NimbyDpvRpmTimer;
    protected int m_NimbyDpvRpmSamples;

    // 50 ms rpm samples for 1.5 s after each start: the trajectory between the start at idle
    // and the stop at rpm 0 (every start of November 6, 2026 died within a second in neutral)
    void NimbyDpvSampleRpm()
    {
        m_NimbyDpvRpmSamples++;
        float rpm = EngineGetRPM();
        float throttle = GetThrottle();
        int clutch = GetClutch();
        float brake = GetBrake();
        float handbrake = GetHandbrake();
        bool on = EngineIsOn();
        float w0 = WheelGetAngularVelocity(0);
        float w2 = WheelGetAngularVelocity(2);
        Print("[NIMBY][Dpv][Diag] " + m_NimbyDpvDiagTag + " rpm-sample " + m_NimbyDpvRpmSamples.ToString() + " on=" + on.ToString() + " rpm=" + rpm.ToString() + " throttle=" + throttle.ToString() + " clutch=" + clutch.ToString() + " brake=" + brake.ToString() + " handbrake=" + handbrake.ToString() + " gear=" + GetGear().ToString() + " w0=" + w0.ToString() + " w2=" + w2.ToString());
        if (m_NimbyDpvRpmSamples >= 30 && m_NimbyDpvRpmTimer)
        {
            m_NimbyDpvRpmTimer.Stop();
        }
    }

    protected void NimbyDpvLogEngineEvent(string what)
    {
        if (!NimbyDpvDiag.Enabled())
            return;
        if (what == "OnEngineStart")
        {
            if (!m_NimbyDpvRpmTimer)
            {
                m_NimbyDpvRpmTimer = new Timer(CALL_CATEGORY_SYSTEM);
            }
            m_NimbyDpvRpmSamples = 0;
            m_NimbyDpvRpmTimer.Run(0.05, this, "NimbyDpvSampleRpm", null, true);
        }
        float rpm = EngineGetRPM();
        int gear = GetGear();
        int neutral = GetNeutralGear();
        bool on = EngineIsOn();
        float speed = GetSpeedometer();
        int reqState = CheckOperationalRequirements();
        int present = WheelCountPresent();
        float engineHealth = GetHealth("Engine", "Health");
        // NIMBY-T-069 follow-up (build 18): the periodic stop at CarScript.c:848 keys on the cached
        // m_EngineHealth = GetHealthLevelValue(GetHealthLevel("Engine"), "Engine"), which is 0 when
        // the zone declares no healthLevels[]; log the level math and the plug lookup it checks
        int engineLevel = GetHealthLevel("Engine");
        float engineLevelValue = GetHealthLevelValue(engineLevel, "Engine");
        bool destroyed = IsDamageDestroyed();
        float fuel = GetFluidFraction(CarFluid.FUEL);
        EntityAI plug = GetInventory().FindAttachment(SparkPlug.SLOT_ID);
        string plugName = "null";
        if (plug)
            plugName = plug.GetType() + "/h=" + plug.GetHealth().ToString();
        Print("[NIMBY][Dpv][Diag] " + m_NimbyDpvDiagTag + " " + what + " engineLevel=" + engineLevel.ToString() + " engineLevelValue=" + engineLevelValue.ToString() + " destroyed=" + destroyed.ToString() + " fuel=" + fuel.ToString() + " vitalSparkPlug=" + IsVitalSparkPlug().ToString() + " plugSlot=" + SparkPlug.SLOT_ID.ToString() + " plug=" + plugName);
        Print("[NIMBY][Dpv][Diag] " + m_NimbyDpvDiagTag + " " + what + " on=" + on.ToString() + " rpm=" + rpm.ToString() + " idle=" + EngineGetRPMIdle().ToString() + " min=" + EngineGetRPMMin().ToString() + " gear=" + gear.ToString() + " neutral=" + neutral.ToString() + " speed=" + speed.ToString() + " reqState=" + reqState.ToString() + " present=" + present.ToString() + " engineHealth=" + engineHealth.ToString() + " server=" + g_Game.IsServer().ToString());
    }

    // ---- traction control (build 34, November 7, 2026; NIMBY-T-096 hill restart, NIMBY-T-099
    // airborne spin-up). Server side only. The rear axle is DIFFERENTIAL_LOCKED (config.cpp,
    // class Rear/Differential), so once a rear tyre breaks loose at zero speed it is on sliding
    // friction and the engine pins at the rev limiter (build 31: 4800 rpm, under 1.3 km/h for
    // 15 s on 25.5 degrees), and in the air both rear wheels spin up under throttle and land at
    // tyre speed. Each physics step compares the rear wheels' surface speed with the car's ground
    // speed; when a rear wheel runs ahead by more than the margin, or a present rear wheel has no
    // contact while the throttle is open, the throttle is capped.
    //
    // Where the cap is applied: Transport.OnInput (Transport.c:348-356) is "called after every
    // input simulation step" and the vanilla comment says other systems can change the internal
    // state at any time and recommends calling setters there; vanilla's own debug driver sets the
    // throttle from CarScript.OnInput (CarScript.c:1202). The cap is therefore applied in OnInput
    // and, as a fallback that the first run measures separately, in EOnPostSimulate as well.
    // The drive line (nimbydpv_diag.c) reports both counters so the run shows which hook holds.
    // Build 34 run (November 6, 22:24 to 22:37): EOnPostSimulate holds (throttle reads 0.3 with W
    // held, tcapplypost equals tc every window); OnInput never ran on the server with a driver
    // aboard (tcin only counted while the car was empty), so the OnInput path is a no-op there.
    // Build 35 (November 6, 22:45): reverse gear covered as well (NIMBY-T-100, Jeff's ruling).
    //
    // API: Car.c:102 GetSpeedometer (km/h), :188 GetThrottle, :191 SetThrottle ("future throttle
    // value"), :279 WheelGetAngularVelocity ("raw angular velocity, unstable value"; unit assumed
    // rad/s, calibrated by the w2=/w3= fields on the drive line against speed), :286
    // WheelHasContact, :248 GetGear (CarGear: 0 reverse, 1 neutral, 2 first).
    static const float TC_WHEEL_RADIUS_M = 0.40;      // NimbyDpv_Wheel radius (config.cpp)
    static const float TC_SLIP_RATIO = 0.25;          // wheel ahead of ground by 25 per cent...
    static const float TC_SLIP_FLOOR_MPS = 3.0;       // ...or by 3 m/s, whichever is larger
    static const float TC_THROTTLE_CAP = 0.3;         // throttle while a cut is active
    static const int TC_FIRST_REAR_WHEEL = 2;         // wheels are counted left-front to rear-right
    protected bool m_NimbyDpvTcWantCap;               // this step asked for the cap
    protected int m_NimbyDpvTcCuts;                   // steps that asked for the cap (window)
    protected int m_NimbyDpvTcAirCuts;                // of those, a present rear wheel had no contact
    protected float m_NimbyDpvTcSlipMax;              // largest wheel-minus-ground speed seen, m/s (window)
    protected int m_NimbyDpvTcOnInputCalls;           // OnInput calls on the server (window)
    protected int m_NimbyDpvTcAppliedOnInput;         // SetThrottle calls made from OnInput (window)
    protected int m_NimbyDpvTcAppliedPostSim;         // SetThrottle calls made from EOnPostSimulate (window)

    // one physics step: decide whether the cap is wanted, keep the diag counters
    protected void NimbyDpvTractionControlStep()
    {
        m_NimbyDpvTcWantCap = false;
        // build 35 (NIMBY-T-100): reverse is covered too; the speeds below are absolute values,
        // so only neutral is excluded (nothing is driven there)
        if (!EngineIsOn() || GetGear() == CarGear.NEUTRAL)
            return;
        float throttle = GetThrottle();
        if (throttle <= TC_THROTTLE_CAP)
            return;

        float ground = Math.AbsFloat(GetSpeedometer()) / 3.6;
        float margin = Math.Max(TC_SLIP_FLOOR_MPS, ground * TC_SLIP_RATIO);
        bool air = false;
        bool slipping = false;
        int wheelCount = WheelCount();
        for (int i = TC_FIRST_REAR_WHEEL; i < wheelCount; i++)
        {
            if (!WheelGetEntity(i))
                continue;   // a missing wheel has no contact and no speed; not a spin-up
            if (!WheelHasContact(i))
            {
                air = true;
                continue;
            }
            float wheel = Math.AbsFloat(WheelGetAngularVelocity(i)) * TC_WHEEL_RADIUS_M;
            float slip = wheel - ground;
            if (slip > m_NimbyDpvTcSlipMax)
                m_NimbyDpvTcSlipMax = slip;
            if (slip > margin)
                slipping = true;
        }
        if (air || slipping)
        {
            m_NimbyDpvTcWantCap = true;
            m_NimbyDpvTcCuts++;
            if (air)
                m_NimbyDpvTcAirCuts++;
        }
    }

    // apply the cap decided by the last step; returns true when SetThrottle was called
    protected bool NimbyDpvTractionControlApply()
    {
        if (!m_NimbyDpvTcWantCap)
            return false;
        if (GetThrottle() <= TC_THROTTLE_CAP)
            return false;
        SetThrottle(TC_THROTTLE_CAP);
        return true;
    }

    // diag (test builds): the window's counters as drive-line fields, then the window resets
    string NimbyDpvTcTakeWindow()
    {
        string s = " tc=" + m_NimbyDpvTcCuts.ToString() + " tcair=" + m_NimbyDpvTcAirCuts.ToString() + " tcslip=" + m_NimbyDpvTcSlipMax.ToString() + " tcin=" + m_NimbyDpvTcOnInputCalls.ToString() + " tcapplyin=" + m_NimbyDpvTcAppliedOnInput.ToString() + " tcapplypost=" + m_NimbyDpvTcAppliedPostSim.ToString();
        m_NimbyDpvTcCuts = 0;
        m_NimbyDpvTcAirCuts = 0;
        m_NimbyDpvTcSlipMax = 0;
        m_NimbyDpvTcOnInputCalls = 0;
        m_NimbyDpvTcAppliedOnInput = 0;
        m_NimbyDpvTcAppliedPostSim = 0;
        return s;
    }

    override void OnInput(float dt)
    {
        super.OnInput(dt);
        if (g_Game.IsServer() || !g_Game.IsMultiplayer())
        {
            m_NimbyDpvTcOnInputCalls++;
            if (NimbyDpvTractionControlApply())
                m_NimbyDpvTcAppliedOnInput++;
        }
    }

    override void EOnPostSimulate(IEntity other, float timeSlice)
    {
        if (g_Game.IsServer() || !g_Game.IsMultiplayer())
        {
            if (m_UTSource && m_UTSource.IsActive())
            {
                m_UTSource.Update(m_UTSSettings, m_UTSLEngine);
            }
            NimbyDpvTractionControlStep();
            if (NimbyDpvTractionControlApply())
                m_NimbyDpvTcAppliedPostSim++;
        }
    }

    override int GetAnimInstance()
    {
        return VehicleAnimInstances.HATCHBACK;
    }

    override float GetTransportCameraDistance()
    {
        return 4.0;
    }

    override int GetSeatAnimationType(int posIdx)
    {
        switch (posIdx)
        {
            case 0:
                return DayZPlayerConstants.VEHICLESEAT_DRIVER;
            case 1:
                return DayZPlayerConstants.VEHICLESEAT_CODRIVER;
            case 2:
                return DayZPlayerConstants.VEHICLESEAT_PASSENGER_R;
        }
        return 0;
    }

    override string GetDoorConditionPointFromSelection(string selection)
    {
        switch (selection)
        {
            case "seat_driver":
                return "seat_con_1_1";
            case "seat_codriver":
                return "seat_con_2_1";
            case "seat_cargo1":
                return "seat_con_1_2";
        }
        return "";
    }

    // The get-in prompt needs the player within pDistance (1.0 m, ActionGetInTransport.c:64)
    // of the seat's condition point, measured flat (CarScript.c:2177). The rear seat's point is
    // on the centre line (seat_con_1_2, builder), 1.4 m from a player standing at either rear
    // wheel and 1.35 m from one standing behind the car, so that seat gets 1.8 m. The front
    // seats keep the vanilla reach from their own side.
    override bool CanReachSeatFromDoors(string pSeatSelection, vector pFromPos, float pDistance = 1.0)
    {
        if (pSeatSelection == "seat_cargo1")
        {
            return super.CanReachSeatFromDoors(pSeatSelection, pFromPos, 1.8);
        }
        return super.CanReachSeatFromDoors(pSeatSelection, pFromPos, pDistance);
    }

    // The spare rides in the open on the hood, so it can always be taken. CarScript's default
    // CanManipulateSpareWheel is false (CarScript.c:670) and each vanilla car gates it on a
    // door or trunk animation; without this override the spare could never leave the slot
    // (build 22, November 6, 2026).
    override protected bool CanManipulateSpareWheel(string slotSelectionName)
    {
        return true;
    }

    // an open buggy: every seat is reachable from outside, nothing to get through
    override bool CrewCanGetThrough(int posIdx)
    {
        return true;
    }

    // Only the two front seats are reachable from each other. The hatchback animation set the
    // DPV uses (VehicleAnimInstances.HATCHBACK) has seat changes for front<->front and
    // rear<->rear only, as vanilla OffroadHatchback.CanReachSeatFromSeat encodes; with any
    // seat allowed, a switch to the rear seat took the crew slot but could not play a move,
    // leaving the body in the passenger seat (build 22, November 6, 2026). The rear seat is
    // entered and left from outside.
    override bool CanReachSeatFromSeat(int currentSeat, int nextSeat)
    {
        return (currentSeat == 0 && nextSeat == 1) || (currentSeat == 1 && nextSeat == 0);
    }

    // Air-cooled flat four, as on the real vehicle (Jeff's ruling, November 6, 2026): no
    // radiator slot, and the coolant checks in CarScript (leak at CarScript.c:894, engine
    // damage below half coolant at :915, steam at :693, radiator spawn at :2511) are all
    // gated on this returning true, so returning false switches the whole system off.
    override bool IsVitalRadiator()
    {
        return false;
    }

    override bool IsVitalGlowPlug()
    {
        return false;
    }

    override bool IsVitalSparkPlug()
    {
        return true;
    }

    override void OnDebugSpawn()
    {
        SpawnUniversalParts();
        SpawnAdditionalItems();
        FillUpCarFluids();

        GameInventory inventory = GetInventory();
        inventory.CreateInInventory("NimbyDpv_Wheel");
        inventory.CreateInInventory("NimbyDpv_Wheel");
        inventory.CreateInInventory("NimbyDpv_Wheel");
        inventory.CreateInInventory("NimbyDpv_Wheel");
        inventory.CreateInInventory("NimbyDpv_Wheel");
    }
}
