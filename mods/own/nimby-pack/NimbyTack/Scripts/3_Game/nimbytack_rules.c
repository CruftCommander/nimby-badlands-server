// -----------------------------------------------------------------------------
// NimbyTack - engine-free rules.
// Purpose : The predicates and arithmetic of Tack as pure functions, so the
//           self-test covers them and the 4_World layer only feeds them facts it
//           reads from the world: the wall-angle test (T2), the fastener lists
//           (T2, T7), the wood test for the knife (T2), the written-first sheet
//           choice (T2), the sheet's facing and offset (T3), take-down damage (T4),
//           the per-area cap (T6), the config clamps and upgrade (T6), the
//           pure parts of the Part B diagnostics (pin-check gate, signed depth,
//           sheet kinds, footprint standoff; T-014 study, October 20, 2026), and
//           the physics-depth guard (NIMBY-T-015, T17, October 21, 2026).
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
    // The three offsets (T3) have one home each: NimbyTackConfig.wallOffsetMeters,
    // unknownSurfaceOffsetMeters, and roughSurfaceOffsetMeters. Callers read them from the loaded
    // config, or from a fresh NimbyTackConfig for the client-side default, through OffsetFor.
    // Part B (B5): a footprint sample that did not hit the wall.
    static const float DEPTH_MISS = -999.0;
    // Part B (B1): what one sheet in inventory counts as, for the no-single-sheet line.
    static const int SHEET_SINGLE = 0;
    static const int SHEET_STACK = 1;
    static const int SHEET_RUINED = 2;

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

    // T3: the direction the sheet's model +Z takes when it is pinned: the outward wall normal,
    // toward the player, so the printed side reads correctly from where the pin was made (page
    // top, model +Y, takes world up; CreatePinned feeds both to Math3D.DirectionAndUpMatrix).
    // History: the October 17, 2026 builds passed RaycastRVResult.dir as the normal, which is the
    // ray into the wall, so "negated normal" was what faced the player; with a real outward
    // normal from the physics ray (October 18, 2026) the same facing is +normal.
    static vector FaceDirection(vector normal)
    {
        return normal;
    }

    static vector PinPosition(vector hitPosition, vector normal, float offsetMeters)
    {
        return hitPosition + normal * offsetMeters;
    }

    // T3, NIMBY-T-008 and NIMBY-T-013: the offset for one pin. A wall with no surface name after
    // both lookups (the Afghan tent's cloth, October 19, 2026) gets the unknown-surface offset,
    // because its physics shell can sit centimeters behind what is drawn; a rough material
    // (Takistan mud brick, October 20, 2026) gets the rough-surface offset, because its stones
    // stand proud of the point the sheet is placed from; every other named surface keeps the wall
    // offset.
    static float OffsetFor(string surfaceName, NimbyTackConfig config)
    {
        if (surfaceName == "")
        {
            return config.unknownSurfaceOffsetMeters;
        }
        if (IsRoughSurface(surfaceName))
        {
            return config.roughSurfaceOffsetMeters;
        }
        return config.wallOffsetMeters;
    }

    // T3, NIMBY-T-013: the rough materials, by the full .bisurf path the fire geometry reports
    // (October 20, 2026: dz\data_takistan\data\penetration\mud_brick.bisurf on every Takistan
    // mud-brick wall tried). A code list, like the tool lists (T8), so the client and the server
    // agree; Jeff ruled the list starts with mud brick alone.
    static bool IsRoughSurface(string surfaceName)
    {
        string key = NormalizeSurface(surfaceName);
        if (key == "dz\\data_takistan\\data\\penetration\\mud_brick.bisurf")
        {
            return true;
        }
        return false;
    }

    // Lowercase with backslashes, because one data tree is reported as both "penetration" and
    // "Penetration" (fabricP.bisurf, October 19, 2026).
    static string NormalizeSurface(string surfaceName)
    {
        string key = surfaceName;
        key.ToLower();
        key.Replace("/", "\\");
        return key;
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

    // T2, NIMBY-T-009: which single sheet goes up. A written sheet wins over a blank one; among
    // sheets of the same kind the first one found stays. True means the candidate replaces the
    // sheet chosen so far (the caller takes the first candidate unconditionally).
    static bool PreferWritten(bool candidateWritten, bool haveWritten)
    {
        return candidateWritten && !haveWritten;
    }

    // The pin log's text= token. Kept here so the log never calls bool.ToString().
    static string TextTag(bool written)
    {
        if (written)
        {
            return "written";
        }
        return "blank";
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
        before = config.unknownSurfaceOffsetMeters;
        config.unknownSurfaceOffsetMeters = Math.Clamp(before, 0.005, 0.2);
        changed = changed + Changed(before, config.unknownSurfaceOffsetMeters);
        before = config.roughSurfaceOffsetMeters;
        config.roughSurfaceOffsetMeters = Math.Clamp(before, 0.005, 0.2);
        changed = changed + Changed(before, config.roughSurfaceOffsetMeters);
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
        // NIMBY-T-023 (Jeff, October 24, 2026): a negative cap is a typo, so it takes the default
        // instead of 0, which disables the cap; 0 written on purpose still disables it.
        if (config.areaCapCount < 0)
        {
            NimbyTackConfig defaults = new NimbyTackConfig();
            config.areaCapCount = defaults.areaCapCount;
            changed = changed + 1;
        }
        return changed;
    }

    // T6: bring a file written by an older build to the current version; true when it changed
    // something, so the loader writes the file back once. Version 1 files (October 17-20, 2026)
    // can lack the surface offsets added since. Whether the JSON reader leaves a missing key at
    // its initializer or at zero is native and unstated, so a surface offset that reads zero or
    // below takes the default here and a value the operator wrote by hand is kept. The rough
    // default's move from 0.06 to 0.08 (TK-P10, October 21, 2026) did not bump the version: a
    // version 2 file keeps what it holds, because an upgrade could not tell a hand-set 0.06 from
    // the old default. A file newer than the build is left alone (version 3 returns false here).
    static bool UpgradeConfig(NimbyTackConfig config, NimbyTackConfig defaults)
    {
        if (config.version >= defaults.version)
        {
            return false;
        }
        if (config.unknownSurfaceOffsetMeters <= 0.0)
        {
            config.unknownSurfaceOffsetMeters = defaults.unknownSurfaceOffsetMeters;
        }
        if (config.roughSurfaceOffsetMeters <= 0.0)
        {
            config.roughSurfaceOffsetMeters = defaults.roughSurfaceOffsetMeters;
        }
        config.version = defaults.version;
        return true;
    }

    // Part B (B1): a pin-check line is written when its (reason, target) key differs from the last
    // line's key and at least minSeconds have passed since that line, so a player holding a tool
    // at one wall writes one line, not thirty a second. A change inside the floor waits for the
    // next evaluation after it.
    static bool ShouldLogCheck(string lastKey, float lastSeconds, string key, float nowSeconds, float minSeconds)
    {
        if (key == lastKey)
        {
            return false;
        }
        return nowSeconds - lastSeconds >= minSeconds;
    }

    // Part B (B4): how far a point stands in front of the view hit along the outward normal, in
    // meters; positive is toward the player, negative is behind the visible face.
    static float SignedDepth(vector point, vector viewHit, vector normal)
    {
        return vector.Dot(point - viewHit, normal);
    }

    // NIMBY-T-015 (T17, Jeff, October 21, 2026): the physics hit may stand at most this far in
    // front of the placement point (the view hit). On Land_House_2M1 it stood 0.6 to 0.98 m in
    // front and the note was buried inside the wall (TK-P5, two faces); the visible pins of
    // October 21 reached 10.3 cm (TK-P4, mud brick) and the T-016 shed face reads 15.6 cm, which
    // this also refuses. A code constant, not a config key: a key would need config version 3, an
    // UpgradeConfig branch, a Sanitize clamp, and the write-back path.
    static const float PHYSICS_DEPTH_LIMIT_METERS = 0.15;
    // The probe reason the guard reports (the production refuse line's detail= and the pin-check
    // line's reason=).
    static const string REASON_PHYSICS_AHEAD = "physics-ahead";

    // NIMBY-T-015: true when the physics hit is accepted. depth is SignedDepth(physicsHit,
    // viewHit, normal) with the normal already flipped toward the player, so a positive depth is in
    // front of the placement point. The ruling says "more than 15 cm", so exactly the limit is
    // accepted, and every negative depth is accepted, which keeps the tent whose physics shell sits
    // 3 to 5 cm behind the cloth (NIMBY-T-008).
    static bool PhysicsHitAccepted(float depth, float limitMeters)
    {
        return depth <= limitMeters;
    }

    // NIMBY-T-022 and T-024 (T18, Jeff, October 24, 2026): the full check runs when an action
    // starts; during the hold only the same target is confirmed, with its frozen cursor point
    // within this reach of the head or the feet. Vanilla's start limit is UAMaxDistances.DEFAULT
    // (2.0 m); the full-body animation moves the camera and the feet past it at the edge of
    // reach. A code constant, not a config key, like PHYSICS_DEPTH_LIMIT_METERS.
    static const float HOLD_REACH_METERS = 2.5;

    // T18: true while the hold may continue. Written as two positive comparisons so a NaN
    // distance refuses.
    static bool WithinHoldReach(float headDistance, float feetDistance, float limitMeters)
    {
        if (headDistance <= limitMeters)
        {
            return true;
        }
        return feetDistance <= limitMeters;
    }

    // NIMBY-T-025 study (debug only): the angle in degrees between two unit normals, 0 for the
    // same direction. The dot product is clamped first, so rounding cannot make Acos return NaN.
    static float NormalAngleDegrees(vector a, vector b)
    {
        float dot = Math.Clamp(vector.Dot(a, b), -1.0, 1.0);
        return Math.Acos(dot) * Math.RAD2DEG;
    }

    // Part B (B1): what one Paper in inventory counts as on the no-single-sheet line, in the order
    // FindSingleSheet skips them (ruined first, then anything but one sheet).
    static int SheetKind(bool ruined, bool hasQuantity, float quantity)
    {
        if (ruined)
        {
            return SHEET_RUINED;
        }
        if (IsSingleSheet(hasQuantity, quantity))
        {
            return SHEET_SINGLE;
        }
        return SHEET_STACK;
    }

    // Part B (B1): the player-state bits that refused the action at ActionBase.Can's first test
    // (ActionBase.c:931): the bits set now that the action does not allow.
    static int RefusedMaskBits(int conditionMask, int allowedMask)
    {
        return conditionMask - (conditionMask & allowedMask);
    }

    // Part B (B5): option (b) of the T-013 study, measured and not applied. The largest depth in
    // front of the view hit among the footprint samples, ignoring misses and any sample farther
    // than cap either way (a window, a door, the next piece); 0 when no sample qualifies.
    static float FootprintStandoff(array<float> depths, float cap)
    {
        float standoff = 0.0;
        foreach (float depth : depths)
        {
            if (depth == DEPTH_MISS)
            {
                continue;
            }
            if (Math.AbsFloat(depth) > cap)
            {
                continue;
            }
            if (depth > standoff)
            {
                standoff = depth;
            }
        }
        return standoff;
    }

    static string DepthTag(float depth)
    {
        if (depth == DEPTH_MISS)
        {
            return "miss";
        }
        return depth.ToString();
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
        Math3D.DirectionAndUpMatrix(FaceDirection(wallNormal), vector.Up, basis);
        vector face = basis[2];
        vector pageTop = basis[1];
        failures += Case(results, "pinned face points along the outward normal", vector.Distance(face, wallNormal) < 0.001);
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

        // NIMBY-T-008: an empty surface name takes the unknown-surface offset.
        float unknownPicked = OffsetFor("", defaults);
        failures += Case(results, "empty surface takes the unknown-surface offset", Math.AbsFloat(unknownPicked - defaults.unknownSurfaceOffsetMeters) < 0.0001);
        float concretePicked = OffsetFor("dz\\data\\data\\penetration\\concrete.bisurf", defaults);
        failures += Case(results, "concrete surface keeps the wall offset", Math.AbsFloat(concretePicked - defaults.wallOffsetMeters) < 0.0001);
        float woodPicked = OffsetFor("dz\\data\\data\\penetration\\wood.bisurf", defaults);
        failures += Case(results, "wood surface keeps the wall offset", Math.AbsFloat(woodPicked - defaults.wallOffsetMeters) < 0.0001);
        // NIMBY-T-013: Takistan mud brick takes the rough-surface offset; nothing else does.
        string mudBrick = "dz\\data_takistan\\data\\penetration\\mud_brick.bisurf";
        float roughPicked = OffsetFor(mudBrick, defaults);
        failures += Case(results, "mud brick takes the rough-surface offset", Math.AbsFloat(roughPicked - defaults.roughSurfaceOffsetMeters) < 0.0001);
        failures += Case(results, "rough-surface default is 8 cm", Math.AbsFloat(defaults.roughSurfaceOffsetMeters - 0.08) < 0.0001);
        failures += Case(results, "mud brick match ignores case and slash direction", IsRoughSurface("DZ/Data_Takistan/Data/Penetration/Mud_Brick.bisurf"));
        failures += Case(results, "mud brick in another folder is not on the list", !IsRoughSurface("dz\\data\\data\\penetration\\mud_brick.bisurf"));
        failures += Case(results, "concrete is not a rough surface", !IsRoughSurface("dz\\data\\data\\penetration\\concrete.bisurf"));
        failures += Case(results, "empty surface is not a rough surface", !IsRoughSurface(""));
        NimbyTackConfig tunedRough = new NimbyTackConfig();
        tunedRough.roughSurfaceOffsetMeters = 0.09;
        float tunedPicked = OffsetFor(mudBrick, tunedRough);
        failures += Case(results, "mud brick uses the configured rough-surface offset", Math.AbsFloat(tunedPicked - 0.09) < 0.0001);
        float tunedConcrete = OffsetFor("dz\\data\\data\\penetration\\concrete.bisurf", tunedRough);
        failures += Case(results, "rough-surface offset leaves concrete at the wall offset", Math.AbsFloat(tunedConcrete - tunedRough.wallOffsetMeters) < 0.0001);
        NimbyTackConfig lowRough = new NimbyTackConfig();
        lowRough.roughSurfaceOffsetMeters = 0.0;
        Sanitize(lowRough);
        failures += Case(results, "rough-surface offset below 5 mm clamps to 5 mm", Math.AbsFloat(lowRough.roughSurfaceOffsetMeters - 0.005) < 0.0001);
        NimbyTackConfig highRough = new NimbyTackConfig();
        highRough.roughSurfaceOffsetMeters = 1.0;
        Sanitize(highRough);
        failures += Case(results, "rough-surface offset above 20 cm clamps to 20 cm", Math.AbsFloat(highRough.roughSurfaceOffsetMeters - 0.2) < 0.0001);
        NimbyTackConfig lowUnknown = new NimbyTackConfig();
        lowUnknown.unknownSurfaceOffsetMeters = 0.0;
        Sanitize(lowUnknown);
        failures += Case(results, "unknown-surface offset below 5 mm clamps to 5 mm", Math.AbsFloat(lowUnknown.unknownSurfaceOffsetMeters - 0.005) < 0.0001);
        NimbyTackConfig highUnknown = new NimbyTackConfig();
        highUnknown.unknownSurfaceOffsetMeters = 1.0;
        Sanitize(highUnknown);
        failures += Case(results, "unknown-surface offset above 20 cm clamps to 20 cm", Math.AbsFloat(highUnknown.unknownSurfaceOffsetMeters - 0.2) < 0.0001);

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
        // NIMBY-T-009: written first, blank as the fallback.
        failures += Case(results, "written sheet replaces a blank choice", PreferWritten(true, false));
        failures += Case(results, "blank sheet does not replace a written choice", !PreferWritten(false, true));
        failures += Case(results, "second written sheet does not replace the first", !PreferWritten(true, true));
        failures += Case(results, "text tag names a blank sheet", TextTag(false) == "blank");
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
        failures += Case(results, "sanitize resets a negative cap to the default", bad.areaCapCount == defaults.areaCapCount);
        NimbyTackConfig capOff = new NimbyTackConfig();
        capOff.areaCapCount = 0;
        failures += Case(results, "sanitize keeps cap 0 (disabled on purpose)", Sanitize(capOff) == 0 && capOff.areaCapCount == 0);

        // T18: the hold reach.
        failures += Case(results, "hold continues with the head in reach", WithinHoldReach(2.4, 2.9, HOLD_REACH_METERS));
        failures += Case(results, "hold continues with the feet in reach", WithinHoldReach(2.9, 2.4, HOLD_REACH_METERS));
        failures += Case(results, "hold continues at exactly the reach", WithinHoldReach(2.5, 9.0, HOLD_REACH_METERS));
        failures += Case(results, "hold stops with both past the reach", !WithinHoldReach(2.6, 2.7, HOLD_REACH_METERS));
        failures += Case(results, "hold stops on NaN distances", !WithinHoldReach(Math.Sqrt(-1.0), Math.Sqrt(-1.0), HOLD_REACH_METERS));
        failures += Case(results, "hold reach is looser than the vanilla 2 m start", HOLD_REACH_METERS > 2.0);

        // NIMBY-T-025 study: the angle helper.
        failures += Case(results, "normal angle of a vector with itself is 0", NormalAngleDegrees("1 0 0", "1 0 0") < 0.01);
        failures += Case(results, "normal angle of perpendicular normals is 90", Math.AbsFloat(NormalAngleDegrees("1 0 0", "0 0 1") - 90.0) < 0.01);
        failures += Case(results, "sanitize leaves defaults alone", Sanitize(defaults) == 0);

        // T6: the version 1 to 2 upgrade adds roughSurfaceOffsetMeters once.
        NimbyTackConfig oldFile = new NimbyTackConfig();
        oldFile.version = 1;
        oldFile.roughSurfaceOffsetMeters = 0.0;
        bool upgraded = UpgradeConfig(oldFile, defaults);
        failures += Case(results, "version 1 file is upgraded", upgraded && oldFile.version == defaults.version);
        failures += Case(results, "upgrade fills a missing rough-surface offset with the default", Math.AbsFloat(oldFile.roughSurfaceOffsetMeters - defaults.roughSurfaceOffsetMeters) < 0.0001);
        // 0.11 is no default of any version, so this case fails if the upgrade ever overwrites a
        // hand-set value (0.08 became the default on October 21, 2026).
        NimbyTackConfig handSet = new NimbyTackConfig();
        handSet.version = 1;
        handSet.roughSurfaceOffsetMeters = 0.11;
        UpgradeConfig(handSet, defaults);
        failures += Case(results, "upgrade keeps a hand-set rough-surface offset", Math.AbsFloat(handSet.roughSurfaceOffsetMeters - 0.11) < 0.0001);
        NimbyTackConfig oldUnknown = new NimbyTackConfig();
        oldUnknown.version = 1;
        oldUnknown.unknownSurfaceOffsetMeters = 0.0;
        UpgradeConfig(oldUnknown, defaults);
        failures += Case(results, "upgrade fills a missing unknown-surface offset with the default", Math.AbsFloat(oldUnknown.unknownSurfaceOffsetMeters - defaults.unknownSurfaceOffsetMeters) < 0.0001);
        NimbyTackConfig current = new NimbyTackConfig();
        failures += Case(results, "current file is not upgraded", !UpgradeConfig(current, defaults));
        NimbyTackConfig future = new NimbyTackConfig();
        future.version = 3;
        future.roughSurfaceOffsetMeters = 0.11;
        bool futureTouched = UpgradeConfig(future, defaults);
        failures += Case(results, "newer file is left alone", !futureTouched && future.version == 3 && Math.AbsFloat(future.roughSurfaceOffsetMeters - 0.11) < 0.0001);

        // NIMBY-T-015 (T17): the physics-depth guard against the depths measured on October 21, 2026.
        float depthLimit = PHYSICS_DEPTH_LIMIT_METERS;
        failures += Case(results, "physics depth limit is 15 cm", Math.AbsFloat(depthLimit - 0.15) < 0.0001);
        failures += Case(results, "physics 0.979 m ahead refuses (TK-P5 first face)", !PhysicsHitAccepted(0.979, depthLimit));
        failures += Case(results, "physics 0.635 m ahead refuses (TK-P5 second face)", !PhysicsHitAccepted(0.635, depthLimit));
        failures += Case(results, "physics 15.6 cm ahead refuses (TK-P3 shed face, T-016)", !PhysicsHitAccepted(0.156, depthLimit));
        failures += Case(results, "physics 15.01 cm ahead refuses", !PhysicsHitAccepted(0.1501, depthLimit));
        failures += Case(results, "physics exactly 15 cm ahead is accepted", PhysicsHitAccepted(0.15, depthLimit));
        failures += Case(results, "physics 10.3 cm ahead is accepted (TK-P4 right face)", PhysicsHitAccepted(0.103, depthLimit));
        failures += Case(results, "physics 7 cm ahead is accepted (18:15 shed face)", PhysicsHitAccepted(0.07, depthLimit));
        failures += Case(results, "physics 4 cm ahead is accepted (chain-link fence)", PhysicsHitAccepted(0.040, depthLimit));
        failures += Case(results, "physics 5 cm behind is accepted (tent)", PhysicsHitAccepted(-0.05, depthLimit));
        string ahead = REASON_PHYSICS_AHEAD;
        bool distinct = ahead != "" && ahead != "no-physics-normal" && ahead != "no-hit" && ahead != "no-building-hit";
        distinct = distinct && ahead != "not-wall-angle" && ahead != "out-of-reach" && ahead != "target-not-building";
        distinct = distinct && ahead != "no-target" && ahead != "no-object" && ahead != "not-wood";
        failures += Case(results, "physics-ahead reason is its own token", distinct);

        // Part B: the pin-check gate, the signed depth, sheet kinds, the mask, the standoff.
        failures += Case(results, "pin-check logs a new key after the floor", ShouldLogCheck("ok|a", 10.0, "no-hit|a", 10.3, 0.25));
        failures += Case(results, "pin-check skips an unchanged key", !ShouldLogCheck("ok|a", 10.0, "ok|a", 20.0, 0.25));
        failures += Case(results, "pin-check holds a new key inside 250 ms", !ShouldLogCheck("ok|a", 10.0, "no-hit|a", 10.1, 0.25));
        failures += Case(results, "depth is positive in front of the face", Math.AbsFloat(SignedDepth("10 2 10.04", "10 2 10", "0 0 1") - 0.04) < 0.0001);
        failures += Case(results, "depth is negative behind the face", SignedDepth("10 2 9.97", "10 2 10", "0 0 1") < -0.029);
        failures += Case(results, "ruined sheet counts as ruined", SheetKind(true, true, 1.0) == SHEET_RUINED);
        failures += Case(results, "stack of 2 counts as a stack", SheetKind(false, true, 2.0) == SHEET_STACK);
        failures += Case(results, "vanilla sheet counts as single", SheetKind(false, false, 0.0) == SHEET_SINGLE);
        failures += Case(results, "raised hands are the refused mask bit", RefusedMaskBits(16 + 512, 512) == 16);
        failures += Case(results, "allowed mask bits refuse nothing", RefusedMaskBits(512, 512) == 0);
        array<float> depths = new array<float>;
        depths.Insert(0.01);
        depths.Insert(DEPTH_MISS);
        depths.Insert(0.045);
        depths.Insert(0.3);
        depths.Insert(-0.02);
        failures += Case(results, "standoff takes the most proud sample inside the cap", Math.AbsFloat(FootprintStandoff(depths, 0.1) - 0.045) < 0.0001);
        array<float> behind = new array<float>;
        behind.Insert(-0.02);
        behind.Insert(DEPTH_MISS);
        failures += Case(results, "standoff is 0 when no sample stands proud", FootprintStandoff(behind, 0.1) == 0.0);
        failures += Case(results, "a missed sample prints as miss", DepthTag(DEPTH_MISS) == "miss");

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
