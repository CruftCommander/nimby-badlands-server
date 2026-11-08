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

        // Rail storage (build 37, November 6, 2026; four side-rail slots since build 47): the slots exist, point at the model
        // selections the builder writes, sit in the car's attachments[], and the vanilla item
        // patch reached CanisterGasoline and AmmoBox (CGame.ConfigGetTextArray, Game.c:558).
        failures += RailCases(results, "1");
        failures += RailCases(results, "2");
        failures += RailCases(results, "3");
        failures += RailCases(results, "4");
        // build 53: the backpack twin slots (config.cpp CfgSlots Slot_NimbyDpvRailBag_N)
        failures += RailBagCases(results, "1");
        failures += RailBagCases(results, "2");
        failures += RailBagCases(results, "3");
        failures += RailBagCases(results, "4");
        failures += Case(results, "railbag-car-attachments", ConfigArrayHas(CAR_PATH + " attachments", "NimbyDpvRailBag_1") && ConfigArrayHas(CAR_PATH + " attachments", "NimbyDpvRailBag_4"));
        failures += Case(results, "railbag-proxy-registered", ConfigArrayHas("CfgNonAIVehicles ProxyDpvRail inventorySlot", "NimbyDpvRailBag_1") && ConfigArrayHas("CfgNonAIVehicles ProxyDpvRail inventorySlot", "NimbyDpvRailBag_4"));
        failures += Case(results, "railbag-ui-group", ConfigArrayHas(CAR_PATH + " GUIInventoryAttachmentsProps Rail attachmentSlots", "NimbyDpvRailBag_2"));
        failures += Case(results, "rail-bag-not-on-rail-slot", !ConfigArrayHas("CfgVehicles HuntingBag inventorySlot", "NimbyDpvRail_1") && ConfigArrayHas("CfgVehicles HuntingBag inventorySlot", "NimbyDpvRailBag_1"));
        failures += Case(results, "rail-box-not-on-bag-slot", !ConfigArrayHas("CfgVehicles AmmoBox inventorySlot", "NimbyDpvRailBag_1") && !ConfigArrayHas("CfgVehicles CanisterGasoline inventorySlot", "NimbyDpvRailBag_1"));
        failures += Case(results, "rail-car-attachments", ConfigArrayHas(CAR_PATH + " attachments", "NimbyDpvRail_1") && ConfigArrayHas(CAR_PATH + " attachments", "NimbyDpvRail_4"));
        failures += Case(results, "rail-canister-patched", ConfigArrayHas("CfgVehicles CanisterGasoline inventorySlot", "NimbyDpvRail_1") && ConfigArrayHas("CfgVehicles CanisterGasoline inventorySlot", "NimbyDpvRail_4"));
        failures += Case(results, "rail-bag-patched-keeps-back", ConfigArrayHas("CfgVehicles TaloonBag_ColorBase inventorySlot", "NimbyDpvRailBag_3") && ConfigArrayHas("CfgVehicles TaloonBag_ColorBase inventorySlot", "Back"));
        failures += Case(results, "rail-smersh-keeps-vestbackpack", ConfigArrayHas("CfgVehicles SmershBag inventorySlot", "VestBackpack") && ConfigArrayHas("CfgVehicles SmershBag inventorySlot", "NimbyDpvRailBag_1"));
        failures += Case(results, "rail-proxy-registered-1", ConfigArrayHas("CfgNonAIVehicles ProxyDpvRail inventorySlot", "NimbyDpvRail_1"));
        failures += Case(results, "rail-proxy-registered-4", ConfigArrayHas("CfgNonAIVehicles ProxyDpvRail inventorySlot", "NimbyDpvRail_4"));
        failures += Case(results, "rail-ui-group", ConfigArrayHas(CAR_PATH + " GUIInventoryAttachmentsProps Rail attachmentSlots", "NimbyDpvRail_3"));
        failures += Case(results, "rail-ammobox-patched", ConfigArrayHas("CfgVehicles AmmoBox inventorySlot", "NimbyDpvRail_1") && ConfigArrayHas("CfgVehicles AmmoBox inventorySlot", "NimbyDpvRail_4"));
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

    protected static int RailCases(out array<string> results, string n)
    {
        int failures = 0;
        string text;
        string path = "CfgSlots Slot_NimbyDpvRail_" + n;
        failures += Case(results, "rail-" + n + "-slot-exists", GetGame().ConfigIsExisting(path));
        GetGame().ConfigGetText(path + " name", text);
        failures += Case(results, "rail-" + n + "-slot-name=" + text, text == "NimbyDpvRail_" + n);
        GetGame().ConfigGetText(path + " selection", text);
        failures += Case(results, "rail-" + n + "-slot-selection=" + text, text == "rail_" + n);
        return failures;
    }

    protected static int RailBagCases(out array<string> results, string n)
    {
        int failures = 0;
        string text;
        string path = "CfgSlots Slot_NimbyDpvRailBag_" + n;
        failures += Case(results, "railbag-" + n + "-slot-exists", GetGame().ConfigIsExisting(path));
        GetGame().ConfigGetText(path + " name", text);
        failures += Case(results, "railbag-" + n + "-slot-name=" + text, text == "NimbyDpvRailBag_" + n);
        GetGame().ConfigGetText(path + " selection", text);
        failures += Case(results, "railbag-" + n + "-slot-selection=" + text, text == "railbag_" + n);
        return failures;
    }

    protected static bool ConfigArrayHas(string path, string value)
    {
        TStringArray values = new TStringArray;
        GetGame().ConfigGetTextArray(path, values);
        return values.Find(value) >= 0;
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
