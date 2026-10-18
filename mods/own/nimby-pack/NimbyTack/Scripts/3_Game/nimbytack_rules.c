// -----------------------------------------------------------------------------
// NimbyTack - engine-free rules.
// Purpose : The predicates and arithmetic of Tack as pure functions, so the
//           self-test covers them and the 4_World layer only feeds them facts it
//           reads from the world: the wall-angle test (T2), the fastener lists
//           (T2, T7), the wood test for the knife (T2), the sheet's facing (T3),
//           take-down damage (T4), the per-area cap (T6), and the config clamps (T6).
// -----------------------------------------------------------------------------
class NimbyTackFastener
{
    static const int NAIL = 0;
    static const int TAPE = 1;
    static const int EPOXY = 2;
    static const int KNIFE = 3;

    static string Name(int fastener)
    {
        if (fastener == NAIL)
        {
            return "nail";
        }
        if (fastener == TAPE)
        {
            return "tape";
        }
        if (fastener == EPOXY)
        {
            return "epoxy";
        }
        if (fastener == KNIFE)
        {
            return "knife";
        }
        return "unknown";
    }

    // T3: one persistent class per fastener; the class is the record of how it was pinned.
    static string PinnedClass(int fastener)
    {
        if (fastener == NAIL)
        {
            return "NimbyTack_PinnedNote_Nail";
        }
        if (fastener == TAPE)
        {
            return "NimbyTack_PinnedNote_Tape";
        }
        if (fastener == EPOXY)
        {
            return "NimbyTack_PinnedNote_Epoxy";
        }
        if (fastener == KNIFE)
        {
            return "NimbyTack_PinnedNote_Knife";
        }
        return "";
    }
}

class NimbyTackRules
{
    // Paper has 15 hitpoints (DZ/gear/consumables/config.cpp:40).
    static const float PAPER_HITPOINTS = 15.0;
    // The wall offset (T3) has one home: NimbyTackConfig.wallOffsetMeters. Callers read it from
    // the loaded config, or from a fresh NimbyTackConfig for the client-side default.

    // T7: every class that drives a nail. Vanilla 1.30 classes: DZ/gear/tools/config.cpp:1660
    // (Hammer), :1806, :1884, :1497; DZ/weapons/melee/blade/config.cpp:171; DZ/weapons/melee/blunt/config.cpp:329.
    static bool IsHammerTool(string typeName)
    {
        if (typeName == "Hammer")
        {
            return true;
        }
        if (typeName == "Hatchet")
        {
            return true;
        }
        if (typeName == "Wrench")
        {
            return true;
        }
        if (typeName == "PipeWrench")
        {
            return true;
        }
        if (typeName == "LugWrench")
        {
            return true;
        }
        if (typeName == "Crowbar")
        {
            return true;
        }
        return false;
    }

    // T2: the knives that go through a sheet into wood.
    static bool IsKnife(string typeName)
    {
        if (typeName == "KitchenKnife")
        {
            return true;
        }
        if (typeName == "SteakKnife")
        {
            return true;
        }
        if (typeName == "HuntingKnife")
        {
            return true;
        }
        if (typeName == "CombatKnife")
        {
            return true;
        }
        if (typeName == "KukriKnife")
        {
            return true;
        }
        if (typeName == "Machete")
        {
            return true;
        }
        return false;
    }

    // T2: a wall is a surface whose normal is within maxDegrees of horizontal, so a floor,
    // a ceiling, or a steep roof refuses the pin.
    static bool IsWallNormal(vector normal, float maxDegrees)
    {
        if (normal.Length() < 0.5)
        {
            return false;
        }
        float limit = Math.Sin(maxDegrees * Math.DEG2RAD);
        return Math.AbsFloat(normal[1]) <= limit;
    }

    // T2: the knife pins only into wood. The raycast reports the surface material name;
    // Phase 0 item 6 records the names Nasdara buildings return, and this test widens if a
    // wooden wall reports something without "wood" in it.
    static bool IsWoodSurface(string surfaceName)
    {
        string lowered = surfaceName;
        lowered.ToLower();
        return lowered.Contains("wood");
    }

    // T3: the direction the sheet's model +Z takes when it is pinned. The Paper model is a sheet
    // in its own XY plane with its printed front on -Z, so the face takes the negated wall normal
    // and the page top (model +Y) takes world up; CreatePinned feeds both to
    // Math3D.DirectionAndUpMatrix. Two Euler forms left the sheet flat like a shelf, and +normal
    // put the red margin on the right (October 17, 2026 builds 173256 and 182834).
    static vector FaceDirection(vector normal)
    {
        return normal * -1;
    }

    static vector PinPosition(vector hitPosition, vector normal, float offsetMeters)
    {
        return hitPosition + normal * offsetMeters;
    }

    // T4: health taken from the sheet by one take-down.
    static float TakeDownDamage(int fastener, NimbyTackConfig config)
    {
        if (fastener == NimbyTackFastener.NAIL)
        {
            return config.takeDownDamageNail;
        }
        if (fastener == NimbyTackFastener.TAPE)
        {
            return config.takeDownDamageTape;
        }
        if (fastener == NimbyTackFastener.EPOXY)
        {
            return config.takeDownDamageEpoxy;
        }
        if (fastener == NimbyTackFastener.KNIFE)
        {
            return config.takeDownDamageKnife;
        }
        return 0.0;
    }

    static float HealthAfterPull(float health, float damage)
    {
        float remaining = health - damage;
        if (remaining < 0.0)
        {
            return 0.0;
        }
        return remaining;
    }

    // How many pulls a sheet at startHealth survives before it is ruined (health 0).
    static int PullsUntilRuined(float startHealth, float damage)
    {
        if (damage <= 0.0)
        {
            return -1;
        }
        int pulls = 0;
        float health = startHealth;
        while (health > 0.0)
        {
            health = HealthAfterPull(health, damage);
            pulls = pulls + 1;
        }
        return pulls;
    }

    // T2: the sheet to pin is exactly one sheet. With Scrawl loaded Paper has a 1-5 quantity and
    // a stack of 2 refuses; without Scrawl (vanilla Paper, no quantity) any sheet is one sheet.
    static bool IsSingleSheet(bool hasQuantity, float quantity)
    {
        if (!hasQuantity)
        {
            return true;
        }
        return quantity == 1.0;
    }

    // T2: a roll or tube pins only when it still holds a full portion.
    static bool HasPortion(float quantity, float portion)
    {
        return quantity >= portion;
    }

    // T6: the cap counts pinned notes already inside the radius; 0 disables the cap.
    static bool WithinAreaCap(int nearbyPinned, int cap)
    {
        if (cap <= 0)
        {
            return true;
        }
        return nearbyPinned < cap;
    }

    // T6: clamp every tunable to a sane bound; returns how many fields were changed.
    static int Sanitize(NimbyTackConfig config)
    {
        int changed = 0;
        float before;
        before = config.reachMeters;
        config.reachMeters = Math.Clamp(before, 0.5, 3.0);
        changed = changed + Changed(before, config.reachMeters);
        before = config.wallAngleDegrees;
        config.wallAngleDegrees = Math.Clamp(before, 0.0, 60.0);
        changed = changed + Changed(before, config.wallAngleDegrees);
        before = config.wallOffsetMeters;
        config.wallOffsetMeters = Math.Clamp(before, 0.005, 0.2);
        changed = changed + Changed(before, config.wallOffsetMeters);
        before = config.takeDownDamageNail;
        config.takeDownDamageNail = Math.Clamp(before, 0.0, PAPER_HITPOINTS);
        changed = changed + Changed(before, config.takeDownDamageNail);
        before = config.takeDownDamageTape;
        config.takeDownDamageTape = Math.Clamp(before, 0.0, PAPER_HITPOINTS);
        changed = changed + Changed(before, config.takeDownDamageTape);
        before = config.takeDownDamageEpoxy;
        config.takeDownDamageEpoxy = Math.Clamp(before, 0.0, PAPER_HITPOINTS);
        changed = changed + Changed(before, config.takeDownDamageEpoxy);
        before = config.takeDownDamageKnife;
        config.takeDownDamageKnife = Math.Clamp(before, 0.0, PAPER_HITPOINTS);
        changed = changed + Changed(before, config.takeDownDamageKnife);
        before = config.tapeQuantityPerPin;
        config.tapeQuantityPerPin = Math.Clamp(before, 0.0, 100.0);
        changed = changed + Changed(before, config.tapeQuantityPerPin);
        before = config.epoxyQuantityPerPin;
        config.epoxyQuantityPerPin = Math.Clamp(before, 0.0, 100.0);
        changed = changed + Changed(before, config.epoxyQuantityPerPin);
        before = config.areaCapRadiusMeters;
        config.areaCapRadiusMeters = Math.Clamp(before, 1.0, 500.0);
        changed = changed + Changed(before, config.areaCapRadiusMeters);
        if (config.areaCapCount < 0)
        {
            config.areaCapCount = 0;
            changed = changed + 1;
        }
        return changed;
    }

    protected static int Changed(float before, float after)
    {
        if (before == after)
        {
            return 0;
        }
        return 1;
    }

    // Engine-free validators for the $profile:NimbyTack\selftest flag (shared rules 8).
    static int SelfTest(out array<string> results)
    {
        int failures = 0;
        NimbyTackConfig defaults = new NimbyTackConfig();

        failures += Case(results, "wall normal passes", IsWallNormal("0 0 1", defaults.wallAngleDegrees));
        failures += Case(results, "wall normal 20 degrees off passes", IsWallNormal(Vector(0, Math.Sin(20 * Math.DEG2RAD), Math.Cos(20 * Math.DEG2RAD)), defaults.wallAngleDegrees));
        failures += Case(results, "wall normal 30 degrees off refuses", !IsWallNormal(Vector(0, Math.Sin(30 * Math.DEG2RAD), Math.Cos(30 * Math.DEG2RAD)), defaults.wallAngleDegrees));
        failures += Case(results, "floor normal refuses", !IsWallNormal("0 1 0", defaults.wallAngleDegrees));
        failures += Case(results, "ceiling normal refuses", !IsWallNormal("0 -1 0", defaults.wallAngleDegrees));
        failures += Case(results, "zero normal refuses", !IsWallNormal("0 0 0", defaults.wallAngleDegrees));

        failures += Case(results, "hammer drives a nail", IsHammerTool("Hammer"));
        failures += Case(results, "wrench drives a nail", IsHammerTool("Wrench"));
        failures += Case(results, "crowbar drives a nail", IsHammerTool("Crowbar"));
        failures += Case(results, "screwdriver does not drive a nail", !IsHammerTool("Screwdriver"));
        failures += Case(results, "hunting knife is a knife", IsKnife("HuntingKnife"));
        failures += Case(results, "machete is a knife", IsKnife("Machete"));
        failures += Case(results, "hatchet is not a knife", !IsKnife("Hatchet"));

        failures += Case(results, "wood surface accepts the knife", IsWoodSurface("cp_wood"));
        failures += Case(results, "Wood surface is case-insensitive", IsWoodSurface("WoodPlanks_int"));
        failures += Case(results, "concrete surface refuses the knife", !IsWoodSurface("cp_concrete1"));

        vector basis[4];
        vector wallNormal = "0 0 1";
        vector awayFromWall = "0 0 -1";
        Math3D.DirectionAndUpMatrix(FaceDirection(wallNormal), vector.Up, basis);
        vector face = basis[2];
        vector pageTop = basis[1];
        failures += Case(results, "pinned face points away from the wall", vector.Distance(face, awayFromWall) < 0.001);
        failures += Case(results, "pinned page top stays up", vector.Distance(pageTop, vector.Up) < 0.001);
        vector leaning = Vector(0.99, -0.137, 0.04);
        leaning.Normalize();
        Math3D.DirectionAndUpMatrix(FaceDirection(leaning), vector.Up, basis);
        vector leaningUp = basis[1];
        failures += Case(results, "leaning wall keeps the page top near up", leaningUp[1] > 0.98);
        vector pinned = PinPosition("10 2 10", "0 0 1", 0.01);
        failures += Case(results, "pin sits 1 cm off the wall when asked", Math.AbsFloat(pinned[2] - 10.01) < 0.0001);
        vector pinnedDefault = PinPosition("10 2 10", "0 0 1", defaults.wallOffsetMeters);
        failures += Case(results, "pin sits at the config default offset", Math.AbsFloat(pinnedDefault[2] - (10.0 + defaults.wallOffsetMeters)) < 0.0001);
        failures += Case(results, "config default offset is inside the clamp", defaults.wallOffsetMeters >= 0.005 && defaults.wallOffsetMeters <= 0.2);
        NimbyTackConfig lowOffset = new NimbyTackConfig();
        lowOffset.wallOffsetMeters = 0.0;
        Sanitize(lowOffset);
        failures += Case(results, "offset below 5 mm clamps to 5 mm", Math.AbsFloat(lowOffset.wallOffsetMeters - 0.005) < 0.0001);
        NimbyTackConfig highOffset = new NimbyTackConfig();
        highOffset.wallOffsetMeters = 1.0;
        Sanitize(highOffset);
        failures += Case(results, "offset above 20 cm clamps to 20 cm", Math.AbsFloat(highOffset.wallOffsetMeters - 0.2) < 0.0001);

        failures += Case(results, "nail damage default 5", TakeDownDamage(NimbyTackFastener.NAIL, defaults) == 5.0);
        failures += Case(results, "tape damage default 3", TakeDownDamage(NimbyTackFastener.TAPE, defaults) == 3.0);
        failures += Case(results, "epoxy damage default 7", TakeDownDamage(NimbyTackFastener.EPOXY, defaults) == 7.0);
        failures += Case(results, "knife damage default 5", TakeDownDamage(NimbyTackFastener.KNIFE, defaults) == 5.0);
        failures += Case(results, "nailed sheet ruins on the third pull", PullsUntilRuined(PAPER_HITPOINTS, defaults.takeDownDamageNail) == 3);
        failures += Case(results, "taped sheet ruins on the fifth pull", PullsUntilRuined(PAPER_HITPOINTS, defaults.takeDownDamageTape) == 5);
        failures += Case(results, "glued sheet ruins on the third pull", PullsUntilRuined(PAPER_HITPOINTS, defaults.takeDownDamageEpoxy) == 3);
        failures += Case(results, "health never goes below zero", HealthAfterPull(2.0, 5.0) == 0.0);

        failures += Case(results, "vanilla sheet without quantity is one sheet", IsSingleSheet(false, 0.0));
        failures += Case(results, "Scrawl sheet at quantity 1 is one sheet", IsSingleSheet(true, 1.0));
        failures += Case(results, "Scrawl stack of 2 is not one sheet", !IsSingleSheet(true, 2.0));
        failures += Case(results, "full roll has a portion", HasPortion(100.0, defaults.tapeQuantityPerPin));
        failures += Case(results, "nine of tape has no portion", !HasPortion(9.0, defaults.tapeQuantityPerPin));
        failures += Case(results, "exact portion pins", HasPortion(10.0, defaults.tapeQuantityPerPin));

        failures += Case(results, "cap allows under the limit", WithinAreaCap(19, 20));
        failures += Case(results, "cap refuses at the limit", !WithinAreaCap(20, 20));
        failures += Case(results, "cap 0 disables the cap", WithinAreaCap(500, 0));

        NimbyTackConfig bad = new NimbyTackConfig();
        bad.reachMeters = 50.0;
        bad.wallAngleDegrees = -5.0;
        bad.takeDownDamageNail = 99.0;
        bad.areaCapCount = -1;
        int clamped = Sanitize(bad);
        failures += Case(results, "sanitize clamps four bad fields", clamped == 4);
        failures += Case(results, "sanitize caps reach at 3 m", bad.reachMeters == 3.0);
        failures += Case(results, "sanitize floors angle at 0", bad.wallAngleDegrees == 0.0);
        failures += Case(results, "sanitize caps damage at paper hitpoints", bad.takeDownDamageNail == PAPER_HITPOINTS);
        failures += Case(results, "sanitize leaves defaults alone", Sanitize(defaults) == 0);

        failures += Case(results, "pinned class per fastener", NimbyTackFastener.PinnedClass(NimbyTackFastener.KNIFE) == "NimbyTack_PinnedNote_Knife");
        failures += Case(results, "unknown fastener has no class", NimbyTackFastener.PinnedClass(9) == "");
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
