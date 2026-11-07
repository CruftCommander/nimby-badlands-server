// -----------------------------------------------------------------------------
// NimbyDpv - self-test (shared rules 8). Phase 1: config facts of the Phase 0 test class, the
// DPV, its wheel item, the wheel proxy class and the slots. Runs only when
// $profile:NimbyDpv\selftest exists (the boot gate creates it).
// API     : CGame.ConfigIsExisting (3_Game/DayZ/Global/Game.c:612),
//           CGame.ConfigGetInt (:539), CGame.ConfigGetFloat (:524),
//           CGame.ConfigGetText (:449).
// Vanilla : fuelCapacity 42 and Crew Driver actionSel "seat_driver" are read from
//           vendor/dayz-1.30-vehicles/wheeled.config.cpp:1346 and :269.
// -----------------------------------------------------------------------------
class NimbyDpvSelfTest
{
    static const string TEST_PATH = "CfgVehicles NimbyDpv_TestCar";
    static const string CAR_PATH = "CfgVehicles NimbyDpv_Car";
    static const string WHEEL_PATH = "CfgVehicles NimbyDpv_Wheel";

    // Returns the failure count; one line per case in results.
    static int Run(out array<string> results)
    {
        int failures = 0;
        string text;
        failures += Case(results, "class-exists", GetGame().ConfigIsExisting(TEST_PATH));
        failures += Case(results, "scope-is-2", GetGame().ConfigGetInt(TEST_PATH + " scope") == 2);
        failures += Case(results, "inherits-ada-fuel-42", GetGame().ConfigGetFloat(TEST_PATH + " fuelCapacity") == 42);
        GetGame().ConfigGetText(TEST_PATH + " Crew Driver actionSel", text);
        failures += Case(results, "inherits-ada-driver-seat", text == "seat_driver");

        failures += Case(results, "car-class-exists", GetGame().ConfigIsExisting(CAR_PATH));
        GetGame().ConfigGetText(CAR_PATH + " model", text);
        failures += Case(results, "car-model-is-dpv", text == "\\NimbyDpv\\data\\dpv.p3d");
        GetGame().ConfigGetText(CAR_PATH + " SimulationModule drive", text);
        failures += Case(results, "car-rear-drive", text == "DRIVE_RWD");
        GetGame().ConfigGetText(CAR_PATH + " Crew Cargo1 actionSel", text);
        failures += Case(results, "car-rear-seat", text == "seat_cargo1");
        GetGame().ConfigGetText(CAR_PATH + " SimulationModule Axles Front Wheels Left inventorySlot", text);
        failures += Case(results, "car-front-left-slot", text == "NimbyDpvWheel_1_1");

        failures += Case(results, "wheel-class-exists", GetGame().ConfigIsExisting(WHEEL_PATH));
        failures += Case(results, "wheel-radius", GetGame().ConfigGetFloat(WHEEL_PATH + " radius") > 0.39);
        GetGame().ConfigGetText("CfgSlots Slot_NimbyDpvWheel_1_1 selection", text);
        failures += Case(results, "wheel-slot-selection", text == "wheel_1_1");
        GetGame().ConfigGetText("CfgNonAIVehicles ProxyDpvWheel model", text);
        failures += Case(results, "wheel-proxy-model", text == "\\NimbyDpv\\data\\dpvwheel.p3d");
        // 1.30 wheel replacement needs <wheel>_Ruined (CarWheelType::SetReplacementTypes)
        failures += Case(results, "wheel-ruined-class-exists", GetGame().ConfigIsExisting(WHEEL_PATH + "_Ruined"));

        // NIMBY-T-069 guard (November 6, 2026): the engine's wheel sim is wired from the Wheels
        // classes' wheelHub / animRotation / animDamper / inventorySlot. The DPV sets animDamper and
        // inventorySlot itself and inherits the other two through the RearDrive -> CarScript
        // declarations in config.cpp; if a declaration ever re-parents a class, the inherited
        // values go empty with no engine warning, so each wheel's resolved values are checked
        // against the vanilla CarScript ones (wheeled.config.cpp:293-328).
        failures += WheelCases(results, "Front", "Left", "1_1", "wheelfrontleft");
        failures += WheelCases(results, "Front", "Right", "2_1", "wheelfrontright");
        failures += WheelCases(results, "Rear", "Left", "1_2", "wheelbackleft");
        failures += WheelCases(results, "Rear", "Right", "2_2", "wheelbackright");
        return failures;
    }

    protected static int WheelCases(out array<string> results, string axle, string side, string xy, string rotation)
    {
        int failures = 0;
        string text;
        string path = CAR_PATH + " SimulationModule Axles " + axle + " Wheels " + side + " ";
        GetGame().ConfigGetText(path + "wheelHub", text);
        failures += Case(results, "car-wheel-" + xy + "-hub=" + text, text == "wheel_" + xy + "_damper_land");
        GetGame().ConfigGetText(path + "animRotation", text);
        failures += Case(results, "car-wheel-" + xy + "-rotation=" + text, text == rotation);
        GetGame().ConfigGetText(path + "animDamper", text);
        failures += Case(results, "car-wheel-" + xy + "-damper=" + text, text == "damper_" + xy);
        GetGame().ConfigGetText(path + "inventorySlot", text);
        failures += Case(results, "car-wheel-" + xy + "-slot=" + text, text == "NimbyDpvWheel_" + xy);
        return failures;
    }

    protected static int Case(out array<string> results, string name, bool passed)
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
