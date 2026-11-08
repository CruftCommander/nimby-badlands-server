// -----------------------------------------------------------------------------
// NimbyDpv - Phase 1 test diagnostics shared by NimbyDpv_Car and NimbyDpv_TestCar (the
// vanilla ADA baseline). Logs every 5 s while $profile:NimbyDpv\spawn-testcar exists.
// -----------------------------------------------------------------------------
class NimbyDpvDiag
{
    static const string DIAG_FLAG = "$profile:NimbyDpv\\spawn-testcar";

    static bool Enabled()
    {
        return FileExist(DIAG_FLAG);
    }

    static void Log(CarScript car, string tag)
    {
        vector pos = car.GetPosition();
        float surface = g_Game.SurfaceY(pos[0], pos[2]);
        float above = pos[1] - surface;
        float fuel = car.GetFluidFraction(CarFluid.FUEL);
        float coolant = car.GetFluidFraction(CarFluid.COOLANT);
        float global = car.GetHealth();
        float globalMax = car.GetMaxHealth();
        float engine = car.GetHealth("Engine", "Health");
        int engineLevel = car.GetHealthLevel("Engine");
        float fuelTank = car.GetHealth("FuelTank", "Health");
        bool engineOn = car.EngineIsOn();
        int reqState = car.CheckOperationalRequirements();
        string line = "[NIMBY][Dpv][Diag] " + tag + " pos=" + pos.ToString() + " originAboveSurface=" + above.ToString();
        line = line + " fuel=" + fuel.ToString() + " coolant=" + coolant.ToString();
        line = line + " global=" + global.ToString() + "/" + globalMax.ToString();
        line = line + " engine=" + engine.ToString() + " lvl=" + engineLevel.ToString() + " fueltank=" + fuelTank.ToString();
        line = line + " engineOn=" + engineOn.ToString() + " reqState=" + reqState.ToString();
        ItemBase battery = car.GetBattery();
        if (battery)
        {
            float energy = battery.GetCompEM().GetEnergy();
            bool batteryRuined = battery.IsRuined();
            line = line + " battery=" + energy.ToString() + " ruined=" + batteryRuined.ToString();
        }
        else
        {
            line = line + " battery=none";
        }
        EntityAI plug = car.FindAttachmentBySlotName("SparkPlug");
        if (plug)
        {
            float plugHealth = plug.GetHealth();
            line = line + " plug=" + plugHealth.ToString();
        }
        else
        {
            line = line + " plug=none";
        }
        Print(line);

        // Phase 2 (build 26, November 6, 2026): drive telemetry for the engine and gearbox pass.
        // speed is km/h from the speedometer (Car.c:102), rpm and gear from the engine
        // (Car.c:227, :248), throttle 0..1 (Car.c:188), pitch is the
        // car's nose-up angle in degrees (Object.c:305 GetOrientation()[1], the same reading vanilla
        // CarScript.c:1148 uses as its hill factor) so a hill-climb reading carries
        // the grade it was taken on. Sign of speed and pitch: forward and nose-up positive.
        // Build 33 (November 6, 2026, telemetry only): yaw and roll added from the same vector
        // (Object.c:305, x = yaw, y = pitch, z = roll, degrees). T-097: the car pulled right on
        // build 31 and left on build 32, and parked DPVs sit with the right dampers 10 to 40 mm
        // lower in most samples. Roll at rest on flat tarmac shows whether the car is right-heavy;
        // yaw drift over a straight run at steady throttle measures the pull in degrees per 5 s.
        if (car.EngineIsOn() || car.CrewSize() > 0)
        {
            float speed = car.GetSpeedometer();
            float rpm = car.EngineGetRPM();
            int gear = car.GetGear();
            float throttle = car.GetThrottle();
            vector ori = car.GetOrientation();
            float pitch = ori[1];
            float yaw = ori[0];
            float roll = ori[2];
            // Build 34 (November 7, 2026): traction-control window counters from NimbyDpv_Car
            // (tc= cuts, tcair= of which airborne, tcslip= max wheel-minus-ground m/s, tcin= OnInput
            // calls, tcapplyin=/tcapplypost= SetThrottle calls per hook) and the raw rear wheel
            // angular velocities w2=/w3= (Car.c:279) so the rad/s assumption can be checked against
            // speed: expected rad/s = speed / 3.6 / 0.40 when rolling without slip.
            string tc = "";
            NimbyDpv_Car dpv = NimbyDpv_Car.Cast(car);
            if (dpv)
                tc = dpv.NimbyDpvTcTakeWindow();
            string rear = "";
            if (car.WheelCount() >= 4)
                rear = " w2=" + car.WheelGetAngularVelocity(2).ToString() + " w3=" + car.WheelGetAngularVelocity(3).ToString();
            Print("[NIMBY][Dpv][Diag] " + tag + " drive speed=" + speed.ToString() + " rpm=" + rpm.ToString() + " gear=" + gear.ToString() + " throttle=" + throttle.ToString() + " pitch=" + pitch.ToString() + " yaw=" + yaw.ToString() + " roll=" + roll.ToString() + tc + rear);
        }

        for (int i = 0; i < car.CrewSize(); i++)
        {
            PlayerBase crew = PlayerBase.Cast(car.CrewMember(i));
            if (!crew)
                continue;
            vector head = car.WorldToModel(crew.GetBonePositionWS(crew.GetBoneIndexByName("Head")));
            vector pelvis = car.WorldToModel(crew.GetBonePositionWS(crew.GetBoneIndexByName("Pelvis")));
            vector root = car.WorldToModel(crew.GetPosition());
            Print("[NIMBY][Dpv][Diag] " + tag + " seat=" + i.ToString() + " root=" + root.ToString() + " pelvis=" + pelvis.ToString() + " head=" + head.ToString() + " (car model space: x right, y up, z forward)");
        }
        string wheels = "";
        int wheelCount = car.WheelCount();
        int wheelPresent = car.WheelCountPresent();
        for (int w = 0; w < wheelCount; w++)
        {
            bool contact = car.WheelHasContact(w);
            EntityAI wheelEntity = car.WheelGetEntity(w);
            vector wheelPos = car.WheelGetPositionLS(w);
            string wheelType = "none";
            if (wheelEntity)
                wheelType = wheelEntity.GetType();
            wheels = wheels + " w" + w.ToString() + "=" + contact.ToString() + "/" + wheelType + "@" + wheelPos.ToString();
        }
        Print("[NIMBY][Dpv][Diag] " + tag + " wheels=" + wheelCount.ToString() + " present=" + wheelPresent.ToString() + " contact:" + wheels);
        // Build 42 (November 7, 2026, rail storage): what sits in each rail slot and where the
        // engine places it (world position and orientation of the attached entity), so an
        // attachment the client does not draw can still be located. InventorySlots.GetSlotIdFromString
        // (3_Game/DayZ/Inventory/InventorySlots.c), GameInventory.FindAttachment.
        if (car.IsInherited(NimbyDpv_Car))
        {
            string rails = "";
            // build 53: both slot sets, eight names; build 55: plus the two tent slots
            for (int r = 1; r <= 10; r++)
            {
                string slotName = "NimbyDpvRail_" + r.ToString();
                if (r == 9)
                    slotName = "NimbyDpvRailTent_L";
                else if (r == 10)
                    slotName = "NimbyDpvRailTent_R";
                else if (r > 4)
                    slotName = "NimbyDpvRailBag_" + (r - 4).ToString();
                int slotId = InventorySlots.GetSlotIdFromString(slotName);
                EntityAI att = car.GetInventory().FindAttachment(slotId);
                if (att)
                {
                    // build 48: the ActionDetach ("Take to hands") conditions, ActionDetach.c:38-60,
                    // minus the player-side ones (hands free, not seated), to see why the ammo box
                    // offers no prompt while the jerry can does
                    ItemBase attItem = ItemBase.Cast(att);
                    string cond = "";
                    if (attItem)
                        cond = " takeable=" + attItem.IsTakeable().ToString() + " placing=" + attItem.IsBeingPlaced().ToString();
                    cond = cond + " canDetach=" + att.CanDetachAttachment(car).ToString() + " carReleases=" + car.CanReleaseAttachment(att).ToString();
                    // build 51 (orientation calibration): the item's forward and up axes in car model
                    // space (Object.GetDirection/GetDirectionUp, 3_Game/DayZ/Entities/Object.c:314,324;
                    // EnEntity.VectorToLocal, 1_Core/DayZ/proto/EnEntity.c:486) so a slot offset
                    // orientation[] that applies shows as a turned axis here, not only on screen
                    string axes = " fwd=" + car.VectorToLocal(att.GetDirection()).ToString() + " up=" + car.VectorToLocal(att.GetDirectionUp()).ToString();
                    rails = rails + " " + slotName + "=" + att.GetType() + "@" + car.WorldToModel(att.GetPosition()).ToString() + axes + cond;
                }
                else
                    rails = rails + " " + slotName + "=empty(id=" + slotId.ToString() + ")";
            }
            Print("[NIMBY][Dpv][Diag] " + tag + " rails:" + rails);
        }
    }

}
