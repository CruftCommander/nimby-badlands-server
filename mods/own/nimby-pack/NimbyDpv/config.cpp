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
        // DZ_Vehicles_Parts and DZ_Gear_Containers: the rail-storage patch below appends to
        // CanisterGasoline and AmmoBox, so this addon must load after them (build 37). Build 58
        // adds the patched tents and crate (DZ_Gear_Camping, CfgPatches name read from
        // DZ/gear/camping/config.cpp:3) and the Field Transceiver (DZ_Radio, DZ/gear/radio/config.cpp:3).
        requiredAddons[] = { "DZ_Data", "DZ_Scripts", "DZ_Vehicles_Wheeled", "DZ_Sounds_Effects", "DZ_Vehicles_Parts", "DZ_Gear_Containers", "DZ_Characters_Backpacks", "DZ_Gear_Camping", "DZ_Radio" };
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
    // Phase 3 rail storage (builds 37 and 38, November 6 and 7, 2026; Jeff, 23:08 and 09:35):
    // Build 47 (Jeff, November 7, 11:34): all four slots are on the side rails (1 and 2 behind the
    // seats, 3 and 4 ahead of them toward the front wheels) and each takes a jerry can, an ammo
    // can or a backpack, in any combination. The item classes get these slot names at the end of
    // CfgVehicles.
    class Slot_NimbyDpvRail_1
    {
        name = "NimbyDpvRail_1";
        displayName = "Rail";
        selection = "rail_1";
        ghostIcon = "set:dayz_inventory image:cargo";
    };
    class Slot_NimbyDpvRail_2 : Slot_NimbyDpvRail_1
    {
        name = "NimbyDpvRail_2";
        selection = "rail_2";
    };
    class Slot_NimbyDpvRail_3 : Slot_NimbyDpvRail_1
    {
        name = "NimbyDpvRail_3";
        selection = "rail_3";
    };
    class Slot_NimbyDpvRail_4 : Slot_NimbyDpvRail_3
    {
        name = "NimbyDpvRail_4";
        selection = "rail_4";
    };
    // Build 53 (Jeff, "go 1", November 7, 17:40): a second slot at each of the four rail positions,
    // for backpacks only. One slot draws every item in it in one frame (build 51 and 52 diag and
    // drives: InventorySlotsOffsets moves nothing visible on this vehicle), and a backpack needs
    // the opposite yaw, a lower frame and a 2 inch aft shift from what an ammo can needs. So the
    // NimbyDpvRail_N slots keep the can and box frame, these carry the bag frame at the same
    // spot, bags list only these, cans and boxes only those, and NimbyDpv_Car refuses and hides
    // a slot whose twin at the same position is filled (CanReceiveAttachment,
    // CanDisplayAttachmentSlot). Capacity stays four items in any mix.
    class Slot_NimbyDpvRailBag_1 : Slot_NimbyDpvRail_1
    {
        name = "NimbyDpvRailBag_1";
        displayName = "Rail (pack)";
        selection = "railbag_1";
    };
    class Slot_NimbyDpvRailBag_2 : Slot_NimbyDpvRailBag_1
    {
        name = "NimbyDpvRailBag_2";
        selection = "railbag_2";
    };
    class Slot_NimbyDpvRailBag_3 : Slot_NimbyDpvRailBag_1
    {
        name = "NimbyDpvRailBag_3";
        selection = "railbag_3";
    };
    class Slot_NimbyDpvRailBag_4 : Slot_NimbyDpvRailBag_1
    {
        name = "NimbyDpvRailBag_4";
        selection = "railbag_4";
    };
    // Build 55 (Jeff, build 54 interview): one packed-tent slot per side, centred on the shelf. It
    // takes the whole side: NimbyDpv_Car blocks it while any of that side's four slots is filled
    // and blocks those four while it is filled. Left = odd rail numbers (x negative).
    class Slot_NimbyDpvRailTent_L : Slot_NimbyDpvRail_1
    {
        name = "NimbyDpvRailTent_L";
        displayName = "Rail (tent)";
        selection = "railtent_l";
    };
    class Slot_NimbyDpvRailTent_R : Slot_NimbyDpvRailTent_L
    {
        name = "NimbyDpvRailTent_R";
        selection = "railtent_r";
    };
    // Build 58 (Jeff, November 7, 23:02): roof storage over the front seats. One position, two
    // twin slots on the rail pattern: Roof takes one piece of large storage (WoodenCrate, CarTent,
    // Attack2Bag), RoofBear a teddy bear (Bear_ColorBase) facing the bow; NimbyDpv_Car hides and
    // refuses each while the other is filled, so it is a bear OR the storage, never both.
    class Slot_NimbyDpvRoof : Slot_NimbyDpvRail_1
    {
        name = "NimbyDpvRoof";
        displayName = "Roof";
        selection = "roof";
    };
    class Slot_NimbyDpvRoofBear : Slot_NimbyDpvRoof
    {
        name = "NimbyDpvRoofBear";
        displayName = "Roof (teddy bear)";
        selection = "roofbear";
    };
    // Build 60 (Jeff, November 7, 23:46): the first aid kit inside the cabin, on the panel behind the
    // front seat backs at the third seat's feet, cross to the bow. Off the rails.
    class Slot_NimbyDpvFirstAid : Slot_NimbyDpvRail_1
    {
        name = "NimbyDpvFirstAid";
        displayName = "First aid";
        selection = "firstaid";
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
    class ProxyAttachment;    // the base of every proxy that draws the attached entity (base building, weapons)
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
    // Rail storage (build 39, November 7, 2026). A proxy is only an attachment point when its
    // model path is registered here with simulation ProxyInventory (inherited from
    // ProxyVehiclePart, wheeled.config.cpp:23901) and the slots it serves; an unregistered
    // proxy is static decoration, which is what builds 37 and 38 drew (a JerryCan.p3d at every
    // rail slot whatever was attached, and the attached items invisible). The lookup is by
    // class NAME, "Proxy" plus the p3d file name (ProxyBattery_Car, ProxyNivaWheel,
    // ProxyCrew_Cargo, this addon's ProxyDpvWheel); build 39's ProxyNimbyDpvRailCan and
    // ProxyNimbyDpvRailBox were never found and stayed static (build 40, November 7, 2026).
    // Build 40, named right, took the proxies under engine control but drew nothing with items
    // in the slots. ProxyVehiclePart is the base vanilla uses for wheels and doors, which the
    // vehicle simulation handles; the battery, plug and headlights that do render on the DPV
    // use ProxyPart (scope 2, reversed 1, "DZ\..." paths), so build 41 mirrored that exactly;
    // still nothing, with the diag showing the items at the car origin. The rule (DayZ Modding
    // Knowledge Pack, rip-vehicle-import attach-invisible.md, measured in game 2026-09-09):
    // simulation ProxyInventory (ProxyVehiclePart, ProxyPart) draws the CLASS model, right for a
    // wheel whose proxy model is the wheel; ProxyAttachment places the attached ENTITY, which is
    // what an inventory slot needs. Build 43: ProxyAttachment, as the base-building proxies
    // (DZ/gear/camping/config.cpp:11254 ProxyBaseBuilding : ProxyAttachment { scope = 2; }).
    // Build 44: one ProxyAttachment class over the addon's own minimal proxy model dpvrail.p3d
    // for all four slots (Knowledge Pack recipe: never the vanilla item model as the proxy path;
    // the engine draws the attached item's own model at the proxy). Builds 40 to 43, with the
    // item models as proxy paths, drew nothing and the diag showed the items at the car origin.
    class ProxyDpvRail : ProxyAttachment
    {
        scope = 2;
        model = "\NimbyDpv\data\dpvrail.p3d";
        inventorySlot[] = { "NimbyDpvRail_1", "NimbyDpvRail_2", "NimbyDpvRail_3", "NimbyDpvRail_4", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4", "NimbyDpvRailTent_L", "NimbyDpvRailTent_R", "NimbyDpvRoof", "NimbyDpvRoofBear", "NimbyDpvFirstAid" };
    };
};

class CfgVehicles
{
    class CarWheel;
    class Bottle_Base;
    class Container_Base;
    class TentBase;
    class Transmitter_Base;
    class Clothing;
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
            "NimbyDpvWheel_Spare_1",
            "NimbyDpvRail_1",
            "NimbyDpvRail_2",
            "NimbyDpvRail_3",
            "NimbyDpvRail_4",
            "NimbyDpvRailBag_1",
            "NimbyDpvRailBag_2",
            "NimbyDpvRailBag_3",
            "NimbyDpvRailBag_4",
            "NimbyDpvRailTent_L",
            "NimbyDpvRailTent_R",
            "NimbyDpvRoof",
            "NimbyDpvRoofBear",
            "NimbyDpvFirstAid"
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
            // build 38: the vehicle inventory panel shows only the slots listed here, which is why
            // build 37's auto-attached jerry cans had no inventory entry and no detach route
            class Rail
            {
                name = "Rail";
                description = "";
                icon = "set:dayz_inventory image:cat_vehicle_body";
                // build 53: both slot sets; a filled position hides its twin, so the group shows
                // eight icons empty and four full
                attachmentSlots[] = { "NimbyDpvRail_1", "NimbyDpvRail_2", "NimbyDpvRail_3", "NimbyDpvRail_4", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4", "NimbyDpvRailTent_L", "NimbyDpvRailTent_R" };
            };
            // build 58: the roof twins, one group so the panel shows two icons empty and one full
            class Roof
            {
                name = "Roof";
                description = "";
                icon = "set:dayz_inventory image:cat_vehicle_body";
                attachmentSlots[] = { "NimbyDpvRoof", "NimbyDpvRoofBear" };
            };
            // build 60: the cabin first aid kit
            class Cabin
            {
                name = "Cabin";
                description = "";
                icon = "set:dayz_inventory image:cat_vehicle_body";
                attachmentSlots[] = { "NimbyDpvFirstAid" };
            };
        };
        // Engine, tyre, skid, rain, damper and wind sound sets. Vanilla cars do not get these from
        // wheeled.config.cpp: the DZ_Sounds_Effects pbo binds them per vehicle class in
        // D:\DayZProjects\DZ\sounds\hpp\config.cpp. Without this block the car is silent (build 18,
        // November 6, 2026). Builds 18 to 59 carried the ADA's Offroad sets (OffroadHatchback.Sounds
        // :519352); build 60 (Jeff, November 7, 23:58) swaps in the Hatchback_02 sets (Gunter 2,
        // :519454, copied value for value): the closest vanilla voice to a small, revving flat four.
        class Sounds
        {
            thrust = 0.6;
            thrustTurbo = 1;
            thrustGentle = 0.3;
            thrustSmoothCoef = 0.02;
            camposSmoothCoef = 0.03;
            soundSetsFilter[] =
            {
                "Hatchback_02_Engine_Offload_Ext_Rpm1_SoundSet",
                "Hatchback_02_Engine_Offload_Ext_Rpm2_SoundSet",
                "Hatchback_02_Engine_Offload_Ext_Rpm3_SoundSet",
                "Hatchback_02_Engine_Offload_Ext_Rpm4_SoundSet",
                "Hatchback_02_Engine_Offload_Ext_Rpm5_SoundSet",
                "Hatchback_02_Engine_Ext_Rpm0_SoundSet",
                "Hatchback_02_Engine_Ext_Rpm1_SoundSet",
                "Hatchback_02_Engine_Ext_Rpm2_SoundSet",
                "Hatchback_02_Engine_Ext_Rpm3_SoundSet",
                "Hatchback_02_Engine_Ext_Rpm4_SoundSet",
                "Hatchback_02_Engine_Ext_Rpm5_SoundSet",
                "Hatchback_02_Engine_Ext_Broken_SoundSet",
                "Hatchback_02_Tires_ice_slow_Ext_SoundSet",
                "Hatchback_02_Tires_ice_fast_Ext_SoundSet",
                "Hatchback_02_Tires_snow_slow_Ext_SoundSet",
                "Hatchback_02_Tires_snow_fast_Ext_SoundSet",
                "Hatchback_02_Tires_rock_slow_Ext_SoundSet",
                "Hatchback_02_Tires_rock_fast_Ext_SoundSet",
                "Hatchback_02_Tires_grass_slow_Ext_SoundSet",
                "Hatchback_02_Tires_grass_fast_Ext_SoundSet",
                "Hatchback_02_Tires_gravel_slow_Ext_SoundSet",
                "Hatchback_02_Tires_gravel_fast_Ext_SoundSet",
                "Hatchback_02_Tires_gravel_dust_fast_Ext_SoundSet",
                "Hatchback_02_Tires_asphalt_slow_Ext_SoundSet",
                "Hatchback_02_Tires_asphalt_fast_Ext_SoundSet",
                "Hatchback_02_Tires_water_slow_Ext_SoundSet",
                "Hatchback_02_Tires_water_fast_Ext_SoundSet",
                "Hatchback_02_skid_dirt_SoundSet",
                "Hatchback_02_dirt_turn_SoundSet",
                "Hatchback_02_Rain_Ext_SoundSet",
                "Hatchback_02_damper_left_SoundSet",
                "Hatchback_02_damper_right_SoundSet"
            };
            soundSetsInt[] =
            {
                "Hatchback_02_Tires_Asphalt_Fast_General_Int_SoundSet",
                "Hatchback_02_Wind_SoundSet"
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

    // Rail storage item patch (builds 37 to 47): the vanilla items that may ride on the DPV's
    // rail slots. A vanilla item only accepts a slot its own inventorySlot[] names, so the slot
    // names are written here. Full lists rather than +=, because += appends to the value
    // inherited from the parent, not to the class's own original list (SmershBag would lose
    // VestBackpack). Originals: CanisterGasoline {} (DZ/vehicles/parts/config.cpp:646), AmmoBox
    // none (DZ/gear/containers/config.cpp:650), every bag {"Back"} except SmershBag {"Back",
    // "VestBackpack"} (DZ/characters/backpacks/config.cpp). Config only, no script, nothing
    // saved changes; the first NIMBY addon to modify vanilla item classes (Jeff, November 6,
    // 23:08; shared-rules registry line owed). Ghillie items and the packed tent also carry the
    // Back slot but are not backpacks and stay out.
    // Build 55: packed tents on the per-side tent slots (Jeff: all four tent classes; build 58 moved
    // the CarTent to the roof, see below, so three remain here). The colour
    // variants (MediumTent_Green, PartyTent_Blue, ...) inherit these lists. Tents have no vanilla
    // inventorySlot[] (DZ/gear/camping/config.cpp), so the list is the two rail names alone.
    class MediumTent : TentBase
    {
        inventorySlot[] = { "NimbyDpvRailTent_L", "NimbyDpvRailTent_R" };
    };
    class LargeTent : TentBase
    {
        inventorySlot[] = { "NimbyDpvRailTent_L", "NimbyDpvRailTent_R" };
    };
    // Build 58: the CarTent's long axis runs along its model forward, so on the side frame it lay
    // across the shelf (build 57 drive); it is roof-only now, with the crate and the long haul pack.
    class CarTent : TentBase
    {
        inventorySlot[] = { "NimbyDpvRoof" };
    };
    class PartyTent : TentBase
    {
        inventorySlot[] = { "NimbyDpvRailTent_L", "NimbyDpvRailTent_R" };
    };
    // Build 55 (Jeff, 18:15): small storage on the rails too. Slot set by model origin (ODOL
    // headers): FirstAidKit.p3d and Protector_Case.p3d are centre-origin like the ammo can, so
    // they take the can/box set; ScientificCase.p3d is base-origin and upright like a bag, so it
    // takes the bag set. Drysacks (DryBag, DrysackBag) were already in the bag list. Vanilla
    // gives none of the three an inventorySlot[] (DZ/gear/containers/config.cpp:454, :541, :2306).
    // Build 58: the protector case on the fore rails only (3 and 4, the ammo-can frame at z 0.625,
    // where its 0.118 centre origin floats under 1 cm; on an aft rail, z 0.685, 7 cm).
    // Build 60 (Jeff, 23:46): the first aid kit leaves the rails for the cabin slot (floated 10 cm on
    // an aft rail in build 57, "5 inches too high"; 4 cm on a fore rail).
    class FirstAidKit : Container_Base
    {
        inventorySlot[] = { "NimbyDpvFirstAid" };
    };
    class SmallProtectorCase : Container_Base
    {
        inventorySlot[] = { "NimbyDpvRail_3", "NimbyDpvRail_4" };
    };
    class ScientificBriefcase : Container_Base
    {
        inventorySlot[] = { "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    // Build 57 (Jeff, November 7, 21:30): the Field Transceiver and the Wooden Crate ride too.
    // BaseRadio is the Field Transceiver (stringtable str_cfgvehicles_baseradio0), worn on the
    // Back like a backpack (DZ/gear/radio/config.cpp:164, inventorySlot {"Back"}, 5x5, 10 kg), so it
    // takes the bag set; its CarBattery attachment and the transmitter actions are untouched.
    // WoodenCrate (DZ/gear/camping/config.cpp:10167, wooden_case.p3d, base-origin, 10x5, 10 kg)
    // already rides a vehicle in vanilla: the M3S truck's four Truck_01_WoodenCrate slots, kept
    // here in full. Build 57 put it on the per-side slots, where it lay across the shelf; since
    // build 58 it is roof-only (NimbyDpvRoof, below), one crate per car.
    class BaseRadio : Transmitter_Base
    {
        inventorySlot[] = { "Back", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    // Build 58: roof only (Jeff, build 57 drive: across the shelf on the side slots, "large storage
    // goes on the roof or in the back"); the four M3S truck slots kept in full.
    class WoodenCrate : Container_Base
    {
        inventorySlot[] = { "Truck_01_WoodenCrate1", "Truck_01_WoodenCrate2", "Truck_01_WoodenCrate3", "Truck_01_WoodenCrate4", "NimbyDpvRoof" };
    };
    // Build 58: the teddy bear (DZ/gear/containers/config.cpp:1155, teddybear.p3d, 2x3; colour
    // variants Bear_Beige, _Dark, _Pink, _White inherit). Vanilla gives it no inventorySlot[].
    class Bear_ColorBase : Container_Base
    {
        inventorySlot[] = { "NimbyDpvRoofBear" };
    };
    class CanisterGasoline : Bottle_Base
    {
        inventorySlot[] = { "NimbyDpvRail_1", "NimbyDpvRail_2", "NimbyDpvRail_3", "NimbyDpvRail_4" };
    };
    class AmmoBox : Container_Base
    {
        inventorySlot[] = { "NimbyDpvRail_1", "NimbyDpvRail_2", "NimbyDpvRail_3", "NimbyDpvRail_4" };
        // Builds 49 to 51 carried an InventorySlotsOffsets position[] here (-0.10, -0.20, -0.275):
        // on this vehicle it moved the server entity (diag rails: y 0.485 then 0.410) but never
        // the drawn item, so it only pulled the "Take to hands" hitbox away from the picture
        // (build 51, Jeff: box unmoved, prompt hard to find). Removed in build 52; the drawn
        // frame is the slot proxy alone (scripts/dpv/build_nimbydpv_model.py, rail_pos).
    };
    // Builds 50 and 51 gave every bag class an InventorySlotsOffsets block (position -0.20; yaw
    // 180, then a per-slot orientation[] calibration). Build 51's diag proved orientation[] inert
    // on this vehicle (identical axes under { 0, 0, 0 }, { 180, 0, 0 }, { 0, 180, 0 }) and position[]
    // moving only the server entity, not the drawn bag, so build 52 removes them: a bag's back
    // is on its forward face, so the slot proxies now point forward outboard, and the front pair
    // sits lower for the base-origin items (box, bag). Slot lists only below.
    class SmershBag : Clothing
    {
        inventorySlot[] = { "Back", "VestBackpack", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    class TaloonBag_ColorBase : Clothing
    {
        inventorySlot[] = { "Back", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    class TortillaBag : Clothing
    {
        inventorySlot[] = { "Back", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    class CourierBag : Clothing
    {
        inventorySlot[] = { "Back", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    class FurCourierBag : Clothing
    {
        inventorySlot[] = { "Back", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    class ImprovisedBag : Clothing
    {
        inventorySlot[] = { "Back", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    class FurImprovisedBag : Clothing
    {
        inventorySlot[] = { "Back", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    class DryBag_ColorBase : Clothing
    {
        inventorySlot[] = { "Back", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    class HuntingBag : Clothing
    {
        inventorySlot[] = { "Back", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    class MountainBag_ColorBase : Clothing
    {
        inventorySlot[] = { "Back", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    class ChildBag_ColorBase : Clothing
    {
        inventorySlot[] = { "Back", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    // LeatherSack_ColorBase: off the rails since build 58 (Jeff, build 57 drive: sat 3 inches too far
    // inboard on the shared bag frame); vanilla {"Back"} unchanged, rides in cargo.
    class AssaultBag_ColorBase : Clothing
    {
        inventorySlot[] = { "Back", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    // Build 58: the Long Haul pack is large storage and goes on the roof, standing, back to the bow
    // (Jeff, build 57 drive: slumped over the starboard side on the bag frame).
    class Attack2Bag_ColorBase : Clothing
    {
        inventorySlot[] = { "Back", "NimbyDpvRoof" };
    };
    class CoyoteBag_ColorBase : Clothing
    {
        inventorySlot[] = { "Back", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    class AliceBag_ColorBase : Clothing
    {
        inventorySlot[] = { "Back", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    class SlingBag_ColorBase : Clothing
    {
        inventorySlot[] = { "Back", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    // ArmyPouch_ColorBase: off the rails since build 58 (Jeff, build 57 drive: lay flat on the bag
    // frame, "rotate, stand up or eliminate"); vanilla {"Back"} unchanged, rides in cargo.
    class DuffelBagSmall_ColorBase : Clothing
    {
        inventorySlot[] = { "Back", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    class CanvasBag_ColorBase : Clothing
    {
        inventorySlot[] = { "Back", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
    class DrysackBag_ColorBase : Clothing
    {
        inventorySlot[] = { "Back", "NimbyDpvRailBag_1", "NimbyDpvRailBag_2", "NimbyDpvRailBag_3", "NimbyDpvRailBag_4" };
    };
};
