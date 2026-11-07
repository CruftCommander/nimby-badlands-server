// -----------------------------------------------------------------------------
// NimbyDpv - DPV. Phase 1: the vehicle on its own model.
// NimbyDpv_Car is a config-only child of CarScript (rear-wheel drive, feasibility D-4; the
// RearDrive values it needs are written out in full, see the CfgVehicles note) with the DPV
// model, its own wheel item, the hatchback part set, three seats, no doors, no windows, no
// dashboard. The Phase 0 class NimbyDpv_TestCar (vanilla
// ADA model under a NIMBY name) stays until Phase 1's first in-game pass, then goes.
// Content addon (shared rules 6.1): the vehicle and wheel persist the vanilla way; no vanilla
// class gains saved data.
// Feasibility: docs/design/dpv-feasibility.md. Inventory: docs/design/2026-11-05-dpv-model-inventory.md.
// Owner: Jeff Fabish.
// Vanilla evidence (vendor/dayz-1.30-vehicles/): RearDrive wheeled.config.cpp:437; OffroadHatchback
// :1250 (crew :1348, SimulationModule :1371, Axles :1431, Cargo :1501, AnimationSources :1507,
// DamageSystem :1540); HatchbackWheel :853; ProxyNivaWheel :24023; ProxyCrew_Driver :23890;
// CfgSlots Slot_NivaWheel_1_1 D:\DayZProjects\scripts\config.cpp:1030 (selection="wheel_1_1").
// Scripts: 4_World/DayZ/Entities/Vehicles/InheritedCars/OffroadHatchback.c.
// -----------------------------------------------------------------------------
class CfgPatches
{
    class NimbyDpv
    {
        requiredAddons[] = { "DZ_Data", "DZ_Scripts", "DZ_Vehicles_Wheeled", "DZ_Sounds_Effects" };
        units[] = { "NimbyDpv_Car", "NimbyDpv_Wheel", "NimbyDpv_TestCar" };
        weapons[] = {};
    };
};

class CfgMods
{
    class NimbyDpv
    {
        type = "mod";
        dependencies[] = { "Game", "World", "Mission" };
        class defs
        {
            class gameScriptModule
            {
                files[] = { "NimbyDpv/Scripts/3_Game" };
            };
            class worldScriptModule
            {
                files[] = { "NimbyDpv/Scripts/4_World" };
            };
            class missionScriptModule
            {
                files[] = { "NimbyDpv/Scripts/5_Mission" };
            };
        };
    };
};

// Inventory slots on the vehicle: name -> the model selection that holds the proxy triangle
// (CfgSlots pattern: D:\DayZProjects\scripts\config.cpp:1030).
class CfgSlots
{
    class Slot_NimbyDpvWheel_1_1
    {
        name = "NimbyDpvWheel_1_1";
        displayName = "#STR_CfgWheel0";
        selection = "wheel_1_1";
        ghostIcon = "set:dayz_inventory image:wheel";
    };
    class Slot_NimbyDpvWheel_1_2 : Slot_NimbyDpvWheel_1_1
    {
        name = "NimbyDpvWheel_1_2";
        selection = "wheel_1_2";
    };
    class Slot_NimbyDpvWheel_2_1 : Slot_NimbyDpvWheel_1_1
    {
        name = "NimbyDpvWheel_2_1";
        selection = "wheel_2_1";
    };
    class Slot_NimbyDpvWheel_2_2 : Slot_NimbyDpvWheel_1_1
    {
        name = "NimbyDpvWheel_2_2";
        selection = "wheel_2_2";
    };
    class Slot_NimbyDpvWheel_Spare_1 : Slot_NimbyDpvWheel_1_1
    {
        name = "NimbyDpvWheel_Spare_1";
        selection = "wheel_spare_1";
    };
};

// The wheel proxy: the model placed at every proxy:\NimbyDpv\data\dpvwheel.NNN triangle,
// mapped to the slots it may fill (ProxyNivaWheel pattern, wheeled.config.cpp:24023).
// The class name is "Proxy" plus the proxy model's FILE NAME: the engine resolves a proxy
// triangle to its CfgNonAIVehicles class by that name alone (nivaWheel.p3d -> ProxyNivaWheel,
// battery_car.p3d -> ProxyBattery_Car, parts.config.cpp:1145; sparkplug.p3d -> Proxysparkplug,
// :1154). Named ProxyNimbyDpvWheel until November 6, 2026, the class was never found, so the
// Fire Geometry wheel proxies had no slot mapping and the wheel simulation created no hub
// (NIMBY-T-069: present=0, every hub <0,0,0>; the attached wheels still rendered because the
// CfgSlots selection placed them).
class CfgNonAIVehicles
{
    class ProxyVehiclePart;
    class ProxyDpvWheel : ProxyVehiclePart
    {
        model = "\NimbyDpv\data\dpvwheel.p3d";
        inventorySlot[] =
        {
            "NimbyDpvWheel_1_1",
            "NimbyDpvWheel_1_2",
            "NimbyDpvWheel_2_1",
            "NimbyDpvWheel_2_2",
            "NimbyDpvWheel_Spare_1"
        };
    };
};

class CfgVehicles
{
    class CarWheel;
    class OffroadHatchback;
    // NimbyDpv_Car inherits CarScript directly and writes Crew, SimulationModule and
    // AnimationSources out in full, the way the Arma2Quad does (its config.cpp forward-declares
    // only "class CarScript;"). A nested class written without ": Base" inside a derived class is
    // a new class, so nothing here depends on the nested-class declarations of another addon.
    // History: the RearDrive base with declared nested classes (builds of November 5, 2026)
    // resolved every inherited wheel field correctly for the script-side reader (self-test cases
    // car-wheel-*), yet the engine never initialised the wheel sim (NIMBY-T-069); the full
    // block removes that variable (probe 2e, November 6, 2026).
    class CarScript;
    // ---- the wheel item (HatchbackWheel pattern, wheeled.config.cpp:853)
    class NimbyDpv_Wheel : CarWheel
    {
        scope = 2;
        displayName = "DPV wheel";
        descriptionShort = "A wide off-road wheel for the DPV.";
        model = "\NimbyDpv\data\dpvwheel.p3d";
        weight = 18000;
        inventorySlot[] =
        {
            "NimbyDpvWheel_1_1",
            "NimbyDpvWheel_1_2",
            "NimbyDpvWheel_2_1",
            "NimbyDpvWheel_2_2",
            "NimbyDpvWheel_Spare_1"
        };
        rotationFlags = 4;
        repairableWithKits[] = { 6 };
        repairCosts[] = { 30 };
        radiusByDamage[] = { 0, 0.40, 0.3, 0.34, 0.9998, 0.28, 0.9999, 0.22 };
        radius = 0.40;            // rear wheel of the model, inventory D-8
        width = 0.29;
        tyreType = 0;
        tyreOffroadResistance = 0.75;   // ADA value; 0.55 until build 25
        tyreRollResistance = 0.018;
        // Build 28 (November 6, 2026, Phase 2 variable 2): grip 0.85, longitudinal 0.90, lateral
        // 0.92 until build 27, the passenger-car class (HatchbackWheel :853 is 0.80/0.85/0.95).
        // Rear-wheel drive (D-4) puts the whole climb on two contact patches, and builds 26 and 27
        // spun from rest on a 22 degree grass slope (4456 and 4868 rpm at under 4 km/h) while the
        // AWD ADA control climbed 187 m of 18 to 25 degrees at 27 to 39 km/h. Vanilla's own
        // off-roaders carry the values below: Offroad_02_Wheel (Humvee), Truck_01_Wheel (M3S) and
        // Hatchback_02_Wheel (Gunter) all run 1.5/1.8, grip 0.89 to 0.95.
        // Build 30 (variable 4): 0.95/1.5/1.8 on builds 28 and 29 climbed 25 to 37 degrees on the
        // move but spun from rest on 24 degrees every time (4830 rpm in first, under 1 km/h for
        // 15 s): once the tyre breaks loose at zero speed it is on sliding friction and 8.5 kN of
        // first-gear torque keeps it there. Twice the vanilla ceiling, lateral up for the rear
        // letting go on an uphill shift. Not clamped: build 30 restarted from rest on 23.5
        // degrees (2 to 40 km/h in 20 s) and climbed 32 degrees on the move; a restart on 26.5
        // still spun. Build 31 (variable 5): 3.0/2.5 to 4.5/3.0 for the restart ceiling.
        // Build 31 result (2026-11-06 21:06): restart from rest still spun on 25.5 degrees (4800 rpm,
        // under 1.3 km/h for 15 s), so tyre friction is exhausted as the restart lever. Jeff: pulls
        // right on every shift, drives like ice on dirt and sand (T-097 widened). Cause: every
        // intact vanilla wheel has lateral above longitudinal (0.85/0.95, 1.5/1.8, 2.5/2.8) and
        // 4.5/3.0 inverted that on a rear-drive car with a locked diff, so each torque step lets
        // the tail go. Build 32 (variable 6): longitudinal back to build 30's 3.0 (23.5 degree
        // restart proven), lateral above it by vanilla's 1.2x. Rollover is a separate cause: the
        // Geometry mass puts the centre of mass about 0.85 m up on a 1.88 m track (about 1.1 g
        // static threshold), which lateral grip above about 1.8 can exceed; builder change, later.
        tyreGrip = 1.0;
        tyreRoughness = 1.2;
        tyreLongitudinalFriction = 3.0;
        tyreLateralFriction = 3.6;
    };

    // ---- the ruined wheel. 1.30 CarWheelType::SetReplacementTypes looks up <wheel>_Ruined for
    // every wheel class and throws a VM exception when it is missing (seen at the first
    // CreateInInventory on 2026-11-05 20:29). Pattern HatchbackWheel_Ruined, wheeled.config.cpp:882.
    // Reuses dpvwheel.p3d until a flat-tyre mesh exists (open item M-8).
    class NimbyDpv_Wheel_Ruined : CarWheel
    {
        scope = 2;
        displayName = "DPV wheel";
        descriptionShort = "A wide off-road wheel for the DPV. Ruined.";
        model = "\NimbyDpv\data\dpvwheel.p3d";
        weight = 12000;
        inventorySlot[] =
        {
            "NimbyDpvWheel_1_1",
            "NimbyDpvWheel_1_2",
            "NimbyDpvWheel_2_1",
            "NimbyDpvWheel_2_2",
            "NimbyDpvWheel_Spare_1"
        };
        rotationFlags = 4;
        radius = 0.22;
        width = 0.29;
        tyreType = 0;
        tyreOffroadResistance = 0.1;
        tyreRollResistance = 0.3;
        tyreGrip = 0.2;
        tyreRoughness = 0.2;
        tyreLongitudinalFriction = 0.35;
        tyreLateralFriction = 4.95;
    };

    // ---- Phase 0 test class, kept until the Phase 1 first pass
    class NimbyDpv_TestCar : OffroadHatchback
    {
        scope = 2;
        displayName = "DPV test car (Phase 0, ADA model)";
        descriptionShort = "NIMBY Phase 0 test vehicle. Vanilla ADA 4x4 under a NIMBY class name.";
    };

    // ---- the DPV
    class NimbyDpv_Car : CarScript
    {
        scope = 2;
        displayName = "DPV";
        descriptionShort = "A fast, open, rear-engine patrol buggy. Three seats, no doors, no mercy.";
        model = "\NimbyDpv\data\dpv.p3d";
        attachments[] =
        {
            "CarBattery",
            "Reflector_1_1",
            "Reflector_2_1",
            "SparkPlug",
            "NimbyDpvWheel_1_1",
            "NimbyDpvWheel_1_2",
            "NimbyDpvWheel_2_1",
            "NimbyDpvWheel_2_2",
            "NimbyDpvWheel_Spare_1"
        };
        hiddenSelections[] =
        {
            "light_left",
            "light_right"
        };
        hiddenSelectionsTextures[] = { "", "" };
        hiddenSelectionsMaterials[] =
        {
            "NimbyDpv\data\dpv_body.rvmat",
            "NimbyDpv\data\dpv_body.rvmat"
        };
        frontReflectorMatOn = "dz\vehicles\wheeled\offroadhatchback\data\niva_lights.rvmat";
        frontReflectorMatOff = "NimbyDpv\data\dpv_body.rvmat";
        fuelCapacity = 60;
        fuelConsumption = 9;
        // Vehicle inventory groups, as on the ADA (wheeled.config.cpp:2307); without them the
        // vehicle screen has no group to refresh when a part leaves its slot (build 23: a
        // removed spark plug stayed drawn in the slot).
        class GUIInventoryAttachmentsProps
        {
            class Engine
            {
                name = "$STR_attachment_Engine0";
                description = "";
                icon = "set:dayz_inventory image:cat_vehicle_engine";
                attachmentSlots[] = { "CarBattery", "SparkPlug" };
            };
            class Body
            {
                name = "$STR_attachment_Body0";
                description = "";
                icon = "set:dayz_inventory image:cat_vehicle_body";
                attachmentSlots[] = { "Reflector_1_1", "Reflector_2_1", "NimbyDpvWheel_Spare_1" };
            };
            class Chassis
            {
                name = "$STR_attachment_Chassis0";
                description = "";
                icon = "set:dayz_inventory image:cat_vehicle_chassis";
                attachmentSlots[] = { "NimbyDpvWheel_1_1", "NimbyDpvWheel_2_1", "NimbyDpvWheel_1_2", "NimbyDpvWheel_2_2" };
            };
        };
        // Engine, tyre, skid, rain, damper and wind sound sets. Vanilla cars do not get these from
        // wheeled.config.cpp: the DZ_Sounds_Effects pbo binds them per vehicle class in
        // D:\DayZProjects\DZ\sounds\hpp\config.cpp (CfgVehicles.OffroadHatchback.Sounds :519352).
        // Without this block the car is silent (build 18, November 6, 2026). The ADA's Offroad
        // sets for now; Phase 2 may give the flat four its own voice.
        class Sounds
        {
            thrust = 0.6;
            thrustTurbo = 1;
            thrustGentle = 0.3;
            thrustSmoothCoef = 0.02;
            camposSmoothCoef = 0.03;
            soundSetsFilter[] =
            {
                "offroad_Engine_Offload_Ext_Rpm1_SoundSet",
                "offroad_Engine_Offload_Ext_Rpm2_SoundSet",
                "offroad_Engine_Offload_Ext_Rpm3_SoundSet",
                "offroad_Engine_Offload_Ext_Rpm4_SoundSet",
                "offroad_Engine_Offload_Ext_Rpm5_SoundSet",
                "offroad_Engine_Ext_Rpm0_SoundSet",
                "offroad_Engine_Ext_Rpm1_SoundSet",
                "offroad_Engine_Ext_Rpm2_SoundSet",
                "offroad_Engine_Ext_Rpm3_SoundSet",
                "offroad_Engine_Ext_Rpm4_SoundSet",
                "offroad_Engine_Ext_Rpm5_SoundSet",
                "offroad_Engine_Ext_Broken_SoundSet",
                "offroad_Tires_ice_slow_Ext_SoundSet",
                "offroad_Tires_ice_fast_Ext_SoundSet",
                "offroad_Tires_snow_slow_Ext_SoundSet",
                "offroad_Tires_snow_fast_Ext_SoundSet",
                "offroad_Tires_rock_slow_Ext_SoundSet",
                "offroad_Tires_rock_fast_Ext_SoundSet",
                "offroad_Tires_grass_slow_Ext_SoundSet",
                "offroad_Tires_grass_fast_Ext_SoundSet",
                "offroad_Tires_gravel_slow_Ext_SoundSet",
                "offroad_Tires_gravel_fast_Ext_SoundSet",
                "offroad_Tires_gravel_dust_fast_Ext_SoundSet",
                "offroad_Tires_asphalt_slow_Ext_SoundSet",
                "offroad_Tires_asphalt_fast_Ext_SoundSet",
                "offroad_Tires_water_slow_Ext_SoundSet",
                "offroad_Tires_water_fast_Ext_SoundSet",
                "Offroad_skid_dirt_SoundSet",
                "offroad_dirt_turn_SoundSet",
                "offroad_Rain_Ext_SoundSet",
                "offroad_damper_left_SoundSet",
                "offroad_damper_right_SoundSet"
            };
            // an open buggy has no cabin: the ADA's interior tyre and wind sets are the nearest fit
            soundSetsInt[] =
            {
                "Offroad_Tires_Asphalt_Fast_General_Int_SoundSet",
                "Offroad_Wind_SoundSet"
            };
        };
        class Crew
        {
            // vanilla CarScript Crew (wheeled.config.cpp:266-282)
            class Driver
            {
                actionSel = "seat_driver";
                proxyPos = "crewDriver";
                getInPos = "pos_driver";
                getInDir = "pos_driver_dir";
                isDriver = 1;
            };
            class CoDriver
            {
                actionSel = "seat_coDriver";
                proxyPos = "crewCoDriver";
                getInPos = "pos_coDriver";
                getInDir = "pos_coDriver_dir";
            };
            class Cargo1
            {
                actionSel = "seat_cargo1";
                proxyPos = "crewCargo1";
                getInPos = "pos_cargo1";
                getInDir = "pos_cargo1_dir";
            };
        };
        class SimulationModule
        {
            class Steering
            {
                maxSteeringAngle = 35;
                increaseSpeed[] = { 0, 50, 30, 30, 100, 10 };
                decreaseSpeed[] = { 0, 90, 60, 50, 90, 30 };
                centeringSpeed[] = { 0, 0, 15, 30, 60, 50, 100, 70 };
            };
            class Throttle
            {
                reactionTime = 0.6;
                defaultThrust = 0.9;
                gentleThrust = 0.6;
                turboCoef = 3.5;
                gentleCoef = 0.7;
            };
            class Brake
            {
                pressureBySpeed[] = { 0, 0.8, 10, 0.7, 20, 0.55, 40, 0.45, 60, 0.45, 80, 0.5, 100, 0.55, 120, 0.65 };
                gentleCoef = 0.65;
                minPressure = 0.2;
                reactionTime = 0.25;
                driverless = 0.1;
            };
            class Aerodynamics
            {
                frontalArea = 2.4;
                dragCoefficient = 0.62;
            };
            drive = "DRIVE_RWD";
            class Engine
            {
                // Builds 1 to 28 ran the vanilla RearDrive engine (wheeled.config.cpp:466-477):
                // { 400,0, 1000,157, 1700,183, 2500,187, 4500,147, 6000,0 }, redline 4700. The
                // flat-four block tried on November 5 and 6, 2026 (curve from 700 rpm, 120 Nm at
                // the 900 rpm idle against 110 Nm friction, inertia 0.10) started and stalled in
                // the same tick every time, so the start end of the curve (400,0 / 1000,160),
                // inertia, friction and the idle, min and clutch rpm are kept as proven.
                //
                // Build 29 (November 6, 2026, Phase 2 variable 3): the flat four. More torque where
                // the clutch bites (205 Nm at 1500, 220 peak at 2200; the Olga curve gave about 165
                // at the 1250 clutch rpm) so a standstill restart on a grade pulls away before the
                // revs run off; builds 26 to 28 stalled every steep restart at 3400 to 4900 rpm.
                // The curve reaches zero torque at 5400 and the redline sits above it at 5500, so
                // the engine cannot reach the vanilla redline damage tick (CarScript.c:876-883)
                // under load or free-revving: a configured rev limiter, the plan T-086 asked for.
                // Free-rev equilibrium (torque equals friction) is near 4700. Wheel-driven overrev
                // downhill in a low gear remains the driver's problem, as on every vanilla car.
                torqueCurve[] = { 400, 0, 1000, 160, 1500, 205, 2200, 220, 3200, 215, 4200, 180, 5000, 100, 5400, 0 };
                inertia = 0.45;
                frictionTorque = 130;
                rollingFriction = 1.5;
                viscousFriction = 0.5;
                rpmIdle = 900;
                rpmMin = 1000;
                rpmClutch = 1250;
                rpmRedline = 5500;
            };
            class Clutch
            {
                maxTorqueTransfer = 400;
                uncoupleTime = 0.3;
                coupleTime = 0.3;
            };
            class Gearbox
            {
                type = "GEARBOX_MANUAL";
                reverse = 3.3;
                ratios[] = { 3.5, 2.1, 1.4, 1.0 };
            };
            class Axles
            {
                class Front
                {
                    maxBrakeTorque = 2200;
                    maxHandbrakeTorque = 0;
                    wheelHubMass = 10;
                    wheelHubRadius = 0.16;
                    // Build 25 (November 6, 2026): builds 1 to 24 ran 28000/2600/6200 with 0.18/0.16 of
                    // travel against the ADA's 40000/2100/7500 and 0.088/0.083. That softness let the
                    // rear wheels unload on bumps; a rear-drive engine under throttle then free-revs
                    // to the redline, and CarScript.c:876-883 damages the Engine zone (and through it
                    // the 20-hitpoint spark plug) every tick at or above rpmRedline. It was also the
                    // wheel hop felt as ice on a gear change. ADA stiffness and damping, travel kept
                    // a third longer than the ADA's for the terrain.
                    class Suspension
                    {
                        stiffness = 40000;
                        compression = 2300;
                        damping = 7500;
                        travelMaxUp = 0.12;
                        travelMaxDown = 0.11;
                    };
                    class Wheels
                    {
                        class Left
                        {
                            inventorySlot = "NimbyDpvWheel_1_1";
                            animTurn = "turnfrontleft";
                            animRotation = "wheelfrontleft";
                            animDamper = "damper_1_1";
                            wheelHub = "wheel_1_1_damper_land";
                        };
                        class Right
                        {
                            inventorySlot = "NimbyDpvWheel_2_1";
                            animTurn = "turnfrontright";
                            animRotation = "wheelfrontright";
                            animDamper = "damper_2_1";
                            wheelHub = "wheel_2_1_damper_land";
                        };
                    };
                };
                class Rear
                {
                    maxBrakeTorque = 1900;
                    maxHandbrakeTorque = 6000;
                    wheelHubMass = 10;
                    wheelHubRadius = 0.16;
                    class Differential
                    {
                        // Build 27 (November 6, 2026, Phase 2 variable 1): 3.9 until build 26. On the
                        // 0.40 m tyre, 3.9 was an effective final drive about half the ADA's (6.15 on
                        // 0.35 m): build 26 telemetry showed second gear starving at 2100-2300 rpm on a
                        // 15-20 degree grade (about 3.8 kN at the wheels against 3.4 kN needed), third
                        // never reached, 77 km/h in second. 5.2 puts first at the ADA's first (39 km/h,
                        // about 8.5 kN) and second at about 5.1 kN; predicted redline speeds
                        // 39/65/98/137 km/h. Engine and gearbox ratios unchanged.
                        ratio = 5.2;
                        type = "DIFFERENTIAL_LOCKED";
                    };
                    class Suspension
                    {
                        stiffness = 42000;
                        compression = 2400;
                        damping = 7800;
                        travelMaxUp = 0.13;
                        travelMaxDown = 0.11;
                    };
                    class Wheels
                    {
                        class Left
                        {
                            inventorySlot = "NimbyDpvWheel_1_2";
                            animTurn = "turnbackleft";
                            animRotation = "wheelbackleft";
                            animDamper = "damper_1_2";
                            wheelHub = "wheel_1_2_damper_land";
                        };
                        class Right
                        {
                            inventorySlot = "NimbyDpvWheel_2_2";
                            animTurn = "turnbackright";
                            animRotation = "wheelbackright";
                            animDamper = "damper_2_2";
                            wheelHub = "wheel_2_2_damper_land";
                        };
                    };
                };
            };
        };
        class Cargo
        {
            itemsCargoSize[] = { 10, 20 };
            allowOwnedCargoManipulation = 1;
            openable = 0;
        };
        class AnimationSources
        {
            // phase at rest = travelMaxDown / (travelMaxDown + travelMaxUp)
            class damper_1_1 { source = "user"; initPhase = 0.48; animPeriod = 1; };   // 0.11 / 0.23
            class damper_2_1 : damper_1_1 {};
            class damper_1_2 { source = "user"; initPhase = 0.46; animPeriod = 1; };   // 0.11 / 0.24
            class damper_2_2 : damper_1_2 {};
        };
        class DamageSystem
        {
            class GlobalHealth
            {
                class Health
                {
                    hitpoints = 1000;
                    healthLevels[] = { {1,{}}, {0.7,{}}, {0.5,{}}, {0.3,{}}, {0,{}} };
                };
            };
            class DamageZones
            {
                class Chassis
                {
                    class Health { hitpoints = 1000; transferToGlobalCoef = 0; healthLevels[] = { {1,{}}, {0.7,{}}, {0.5,{}}, {0.3,{}}, {0,{}} }; };
                    componentNames[] = { "dmgZone_chassis" };
                    fatalInjuryCoef = -1;
                    inventorySlots[] = {};
                    transferToZonesNames[] = {};
                    transferToZonesCoefs[] = {};
                };
                class Front
                {
                    class Health { hitpoints = 1000; transferToGlobalCoef = 0; healthLevels[] = { {1,{}}, {0.7,{}}, {0.5,{}}, {0.3,{}}, {0,{}} }; };
                    componentNames[] = { "dmgZone_front" };
                    fatalInjuryCoef = -1;
                    inventorySlots[] = {};
                    transferToZonesNames[] = { "Engine" };
                    transferToZonesCoefs[] = { 0.1 };
                };
                class Back
                {
                    class Health { hitpoints = 1000; transferToGlobalCoef = 0; healthLevels[] = { {1,{}}, {0.7,{}}, {0.5,{}}, {0.3,{}}, {0,{}} }; };
                    componentNames[] = { "dmgZone_back" };
                    fatalInjuryCoef = -1;
                    inventorySlots[] = {};
                    // The ADA's Front zone passes 0.15 of its damage to Engine (wheeled.config.cpp
                    // Front :1540+). 0.5 here, behind a tail that scrapes on every bump, ruined
                    // the 20-hitpoint spark plug in minutes of off-road driving (build 22).
                    transferToZonesNames[] = { "Engine", "FuelTank" };
                    transferToZonesCoefs[] = { 0.15, 0.1 };
                };
                class Roof
                {
                    class Health { hitpoints = 1000; transferToGlobalCoef = 0; healthLevels[] = { {1,{}}, {0.7,{}}, {0.5,{}}, {0.3,{}}, {0,{}} }; };
                    componentNames[] = { "dmgZone_roof" };
                    fatalInjuryCoef = -1;
                    inventorySlots[] = {};
                    transferToZonesNames[] = {};
                    transferToZonesCoefs[] = {};
                };
                class Fender_1_1
                {
                    class Health { hitpoints = 500; transferToGlobalCoef = 0; healthLevels[] = { {1,{}}, {0.7,{}}, {0.5,{}}, {0.3,{}}, {0,{}} }; };
                    componentNames[] = { "dmgZone_fender_1_1" };
                    fatalInjuryCoef = -1;
                    inventorySlots[] = { "NimbyDpvWheel_1_1" };
                    transferToZonesNames[] = {};
                    transferToZonesCoefs[] = {};
                };
                class Fender_2_1 : Fender_1_1
                {
                    componentNames[] = { "dmgZone_fender_2_1" };
                    inventorySlots[] = { "NimbyDpvWheel_2_1" };
                };
                class Fender_1_2 : Fender_1_1
                {
                    componentNames[] = { "dmgZone_fender_1_2" };
                    inventorySlots[] = { "NimbyDpvWheel_1_2" };
                };
                class Fender_2_2 : Fender_1_1
                {
                    componentNames[] = { "dmgZone_fender_2_2" };
                    inventorySlots[] = { "NimbyDpvWheel_2_2" };
                };
                class Reflector_1_1
                {
                    class Health { hitpoints = 20; transferToGlobalCoef = 0; healthLevels[] = { {1,{}}, {0.7,{}}, {0.5,{}}, {0.3,{}}, {0,{}} }; };
                    componentNames[] = { "dmgZone_lights_1_1" };
                    fatalInjuryCoef = -1;
                    inventorySlots[] = { "Reflector_1_1" };
                    transferToZonesNames[] = {};
                    transferToZonesCoefs[] = {};
                };
                class Reflector_2_1 : Reflector_1_1
                {
                    componentNames[] = { "dmgZone_lights_2_1" };
                    inventorySlots[] = { "Reflector_2_1" };
                };
                class Engine
                {
                    class Health { hitpoints = 1000; transferToGlobalCoef = 1; healthLevels[] = { {1,{}}, {0.7,{}}, {0.5,{}}, {0.3,{}}, {0,{}} }; };
                    componentNames[] = { "dmgZone_engine" };
                    fatalInjuryCoef = 0.001;
                    // no CarRadiator: the DPV is air-cooled (ruled November 6, 2026; see
                    // NimbyDpv_Car.IsVitalRadiator)
                    inventorySlots[] = { "CarBattery", "SparkPlug" };
                    transferToZonesNames[] = {};
                    transferToZonesCoefs[] = {};
                };
                class FuelTank
                {
                    class Health { hitpoints = 600; transferToGlobalCoef = 0; healthLevels[] = { {1,{}}, {0.7,{}}, {0.5,{}}, {0.3,{}}, {0,{}} }; };
                    componentNames[] = { "dmgZone_fueltank" };
                    fatalInjuryCoef = -1;
                    inventorySlots[] = {};
                    transferToZonesNames[] = {};
                    transferToZonesCoefs[] = {};
                };
            };
        };
    };
};
