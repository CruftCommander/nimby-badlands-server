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
//           the physics-depth guard (NIMBY-T-015, T17, October 21, 2026; 20 cm since
//           NIMBY-T-030), and the placement round of October 25, 2026: the fire front
//           (NIMBY-T-028, T19), the post run (NIMBY-T-025, T20), and the door rule
//           (NIMBY-T-029, T21).
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
    // The placement numbers (T3, T19) have one home each: NimbyTackConfig.faceClearanceMeters, and
    // the two fallbacks wallOffsetMeters and unknownSurfaceOffsetMeters. Callers read them from the
    // loaded config, or from a fresh NimbyTackConfig for the client-side default.
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

    // T3, NIMBY-T-008, T19: the fallback offset off the view hit, used only when the fire
    // measurement has fewer than FIRE_MIN_HITS samples. A wall with no surface name after both
    // lookups (the Afghan tent's cloth, October 19, 2026) gets the unknown-surface offset, because
    // its physics shell can sit centimeters behind what is drawn; every other surface keeps the
    // wall offset. The rough-surface branch (Takistan mud brick, NIMBY-T-013) is retired by
    // NIMBY-T-028 (Jeff, October 25, 2026): the fire measurement covers the stones.
    static float OffsetFor(string surfaceName, NimbyTackConfig config)
    {
        if (surfaceName == "")
        {
            return config.unknownSurfaceOffsetMeters;
        }
        return config.wallOffsetMeters;
    }

    // ---- T19, NIMBY-T-028 (Jeff, October 25, 2026): fire-relative placement ----
    // The server samples the fire geometry across the sheet before it creates the note; the note
    // sits faceClearanceMeters in front of the most forward sample. A sample further than
    // FIRE_SAMPLE_CAP_METERS either way from the view hit is ignored (a window, a sill, the next
    // piece), and fewer than FIRE_MIN_HITS samples fall back to OffsetFor (Jeff, October 25, 2026).
    // The sheet's size, from the pinned note's collision box as the footprint read it on
    // October 25, 2026 (size=0.223137x0.31625 size-source=box on every pin). Placement runs before
    // the note exists, so it cannot read the box itself; the debug footprint still does.
    static const float SHEET_WIDTH_METERS = 0.223;
    static const float SHEET_HEIGHT_METERS = 0.316;
    static const int FIRE_MIN_HITS = 3;
    static const float FIRE_SAMPLE_CAP_METERS = 0.1;
    static const string PLACEMENT_FIRE = "fire";
    static const string PLACEMENT_FALLBACK = "fallback";

    // How many samples hit within the cap.
    static int FireHits(array<float> depths, float cap)
    {
        int hits = 0;
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
            hits = hits + 1;
        }
        return hits;
    }

    // The most forward sample within the cap, signed (negative is behind the view hit), or
    // DEPTH_MISS when none qualifies. Unlike FootprintStandoff this is not floored at 0, so a
    // face that stands behind the view hit pulls the note in.
    static float FireFront(array<float> depths, float cap)
    {
        float front = DEPTH_MISS;
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
            if (front == DEPTH_MISS || depth > front)
            {
                front = depth;
            }
        }
        return front;
    }

    static bool UseFire(int hits)
    {
        return hits >= FIRE_MIN_HITS;
    }

    // The offset off the view-hit plane, along the outward normal, for a fire placement.
    static float FireOffset(float front, float clearance)
    {
        return front + clearance;
    }

    // ---- T23, NIMBY-T-034 (Jeff, October 25, 2026): door leaves measure the geometry layer ----
    // A door leaf is placed from the geometry layer, not the fire layer. On the Land_House_1C1
    // front door, pinned from outside, the fire ray answered nothing within the wall ray and the
    // geometry layer stood 13.2 cm behind the view hit (October 25, 2026, 18:37:01); from inside
    // the same leaf the fire layer sat 1.5 cm behind the view hit (17:54:40). The door ray reaches
    // DOOR_RAY_BACK_METERS behind the view hit, where the wall ray reaches WALL_RAY_BACK_METERS,
    // and a door sample counts within DOOR_SAMPLE_CAP_METERS, where a wall sample counts within
    // FIRE_SAMPLE_CAP_METERS. Fewer than FIRE_MIN_HITS geometry samples fall back to
    // faceClearanceMeters off the view hit. The door allowance (T22) applies either way.
    static const float WALL_RAY_BACK_METERS = 0.15;
    static const float DOOR_RAY_BACK_METERS = 0.3;
    static const float DOOR_SAMPLE_CAP_METERS = 0.25;
    static const string PLACEMENT_DOOR = "door-geom";

    // How far behind the sample point the measuring ray runs.
    static float RayBack(bool onDoor)
    {
        if (onDoor)
        {
            return DOOR_RAY_BACK_METERS;
        }
        return WALL_RAY_BACK_METERS;
    }

    // How far from the view hit a sample still counts.
    static float SampleCap(bool onDoor)
    {
        if (onDoor)
        {
            return DOOR_SAMPLE_CAP_METERS;
        }
        return FIRE_SAMPLE_CAP_METERS;
    }

    // The placement method of a door leaf from its geometry hits.
    static string DoorMethod(int geometryHits)
    {
        if (UseFire(geometryHits))
        {
            return PLACEMENT_DOOR;
        }
        return PLACEMENT_FALLBACK;
    }

    // ---- T20, NIMBY-T-025 (Jeff, October 25, 2026): notes on posts ----
    // A horizontal fire scan across the hit, POST_SCAN_HALF_METERS either side in
    // POST_SCAN_STEP_METERS steps (31 samples, the hit at index 15). The run of hits around the
    // hit, bridging gaps of POST_SCAN_MAX_GAP samples, is a post when misses end it on both sides
    // inside the scan and it is narrower than the sheet; the note then moves onto the middle of the
    // run and faces along the fence line when the two side probes agree within
    // POST_SIDE_AGREE_DEGREES.
    static const float POST_SCAN_HALF_METERS = 0.3;
    static const float POST_SCAN_STEP_METERS = 0.02;
    static const int POST_SCAN_MAX_GAP = 1;
    static const int POST_SEED_REACH = 2;
    static const float POST_SIDE_AGREE_DEGREES = 10.0;

    // 0.3 m either side in 2 cm steps: 31 samples, the hit at index 15. Kept as integers so no
    // float is rounded into an index; the self-test checks they agree with the meters above.
    static const int POST_SCAN_COUNT = 31;
    static const int POST_SCAN_CENTRE = 15;

    // The hit index nearest centre within reach, or -1. The centre itself is tried first.
    static int RunSeed(array<bool> hits, int centre, int reach)
    {
        for (int distance = 0; distance <= reach; distance++)
        {
            int left = centre - distance;
            if (left >= 0 && left < hits.Count() && hits[left])
            {
                return left;
            }
            int right = centre + distance;
            if (right >= 0 && right < hits.Count() && hits[right])
            {
                return right;
            }
        }
        return -1;
    }

    // The first (step -1) or last (step +1) hit index of the run through seed: the walk stops at
    // maxGap + 1 misses in a row or at the end of the scan.
    static int RunEdge(array<bool> hits, int seed, int step, int maxGap)
    {
        int edge = seed;
        int misses = 0;
        int index = seed + step;
        while (index >= 0 && index < hits.Count())
        {
            if (hits[index])
            {
                edge = index;
                misses = 0;
            }
            else
            {
                misses = misses + 1;
                if (misses > maxGap)
                {
                    break;
                }
            }
            index = index + step;
        }
        return edge;
    }

    // True when misses, not the end of the scan, end the run on both sides.
    static bool RunBounded(array<bool> hits, int first, int last, int maxGap)
    {
        if (first - (maxGap + 1) < 0)
        {
            return false;
        }
        return last + maxGap + 1 <= hits.Count() - 1;
    }

    // The run's width, counting each sample as one step wide.
    static float RunWidth(int first, int last, float step)
    {
        // Float first: Enforce types a binary expression by its left operand.
        float count = last - first + 1;
        return step * count;
    }

    // How far the middle of the run stands from the hit, along the sheet's width.
    static float RunShift(int first, int last, int centre, float step)
    {
        float middle = first + last;
        middle = middle * 0.5;
        float centreIndex = centre;
        return step * (middle - centreIndex);
    }

    static bool IsPost(bool bounded, float width, float sheetWidth)
    {
        return bounded && width < sheetWidth;
    }

    static bool SidesAgree(vector left, vector right, float maxDegrees)
    {
        if (left.Length() < 0.5 || right.Length() < 0.5)
        {
            return false;
        }
        return NormalAngleDegrees(left, right) <= maxDegrees;
    }

    // ---- T21, NIMBY-T-029 (Jeff, October 25, 2026): notes on doors ----
    // A note on a door leaf attaches to the door (each client parents its copy to the door's
    // pivot) and the server moves it with the door (T21b). doorIndex is -1 off a door. T24 (Jeff,
    // October 25, 2026, late evening): a door leaf pins open or closed; the "closed pins" rule of
    // T21 is retired because T21b records the note in the door's live frame, so an open-leaf pin
    // rides the leaf and snaps onto the closed door after a restart like any other. The
    // door-not-closed refusal and its finish re-check are gone (83 refusals in one evening).

    // T22, NIMBY-T-032: the last allowance listed for the model on one list, or 0.
    static float ListAllowance(array<ref NimbyTackAllowance> list, string model)
    {
        float meters = 0.0;
        if (!list)
        {
            return meters;
        }
        foreach (NimbyTackAllowance allowance : list)
        {
            if (allowance && allowance.model == model)
            {
                meters = allowance.meters;
            }
        }
        return meters;
    }

    static float ModelAllowance(NimbyTackConfig config, string model)
    {
        if (!config)
        {
            return 0.0;
        }
        return ListAllowance(config.modelAllowances, model);
    }

    // T25, NIMBY-T-048: the door-leaf list is separate from the wall list, because one building
    // can want both signs (Land_House_1C1: walls +3 cm, its front door already floats).
    static float DoorModelAllowance(NimbyTackConfig config, string model)
    {
        if (!config)
        {
            return 0.0;
        }
        return ListAllowance(config.doorModelAllowances, model);
    }

    // The extra offset for one pin: on a door leaf the door allowance plus the door-leaf model
    // allowance; on a wall the wall model allowance. The wall list never reaches a door leaf.
    static float AllowanceFor(NimbyTackConfig config, string model, bool onDoor)
    {
        if (onDoor)
        {
            return config.doorAllowanceMeters + DoorModelAllowance(config, model);
        }
        return ModelAllowance(config, model);
    }

    // T22: the store data of a door note is sane: no door, or a door index 0 to 63 with a pivot.
    static bool DoorDataValid(int doorIndex, int pivot)
    {
        if (doorIndex == -1)
        {
            return true;
        }
        return doorIndex >= 0 && doorIndex <= 63 && pivot >= 0;
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
        before = config.faceClearanceMeters;
        config.faceClearanceMeters = Math.Clamp(before, 0.005, 0.05);
        changed = changed + Changed(before, config.faceClearanceMeters);
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
        before = config.doorAllowanceMeters;
        config.doorAllowanceMeters = Math.Clamp(before, 0.0, 0.15);
        changed = changed + Changed(before, config.doorAllowanceMeters);
        changed = changed + SanitizeList(config.modelAllowances);
        changed = changed + SanitizeList(config.doorModelAllowances);
        // T27: the notices switch is 0 or 1; anything else means on.
        if (config.refusalNotices != 0 && config.refusalNotices != 1)
        {
            config.refusalNotices = 1;
            changed = changed + 1;
        }
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

    // T22, T25: every allowance on a list clamps to 0 and 15 cm; the count of entries changed.
    static int SanitizeList(array<ref NimbyTackAllowance> list)
    {
        int changed = 0;
        if (!list)
        {
            return changed;
        }
        foreach (NimbyTackAllowance allowance : list)
        {
            if (!allowance)
            {
                continue;
            }
            float before = allowance.meters;
            allowance.meters = Math.Clamp(before, 0.0, 0.15);
            changed = changed + Changed(before, allowance.meters);
        }
        return changed;
    }

    // T6: bring a file written by an older build to the current version; true when it changed
    // something, so the loader writes the file back once. Version 1 files (October 17-20, 2026)
    // can lack the surface offsets added since, and version 1 and 2 files lack faceClearanceMeters
    // (version 3, NIMBY-T-028, October 25, 2026). Whether the JSON reader leaves a missing key at
    // its initializer or at zero is native and unstated, so a value that reads zero or below takes
    // the default here and a value the operator wrote by hand is kept. A version 2 file's
    // roughSurfaceOffsetMeters is no longer a field; the write-back drops it. A file newer than
    // the build is left alone (version 5 returns false here). Version 5 (NIMBY-T-048): a file
    // without the door-leaf list takes the built-in one; a version 4 doorAllowanceMeters is kept
    // as written (the profile that measured it at 0 holds 0).
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
        if (config.faceClearanceMeters <= 0.0)
        {
            config.faceClearanceMeters = defaults.faceClearanceMeters;
        }
        // Version 4 (NIMBY-T-032): a file without the allowance list takes the built-in one.
        if (!config.modelAllowances || config.modelAllowances.Count() == 0)
        {
            config.modelAllowances = defaults.modelAllowances;
        }
        if (!config.doorModelAllowances || config.doorModelAllowances.Count() == 0)
        {
            config.doorModelAllowances = defaults.doorModelAllowances;
        }
        // Version 6 (T27): the key did not exist before, so a file older than 6 takes the default
        // (a missing int may read as 0, which would silence the boxes by accident).
        if (config.version < 6)
        {
            config.refusalNotices = defaults.refusalNotices;
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
    // October 21 reached 10.3 cm (TK-P4, mud brick). NIMBY-T-030 (Jeff, October 25, 2026): 0.15
    // refused most of the Land_House_2W04_Blue front face at 16 to 17 cm and the T-016 shed face at
    // 15.6 cm, so the limit is 0.20. A code constant, not a config key.
    static const float PHYSICS_DEPTH_LIMIT_METERS = 0.2;
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
        // NIMBY-T-028 (T19): the rough-surface offset is retired; mud brick falls back like any
        // named surface, and the fire clearance is 1.5 cm, clamped 5 mm to 5 cm.
        string mudBrick = "dz\\data_takistan\\data\\penetration\\mud_brick.bisurf";
        float mudPicked = OffsetFor(mudBrick, defaults);
        failures += Case(results, "mud brick falls back to the wall offset (rough offset retired)", Math.AbsFloat(mudPicked - defaults.wallOffsetMeters) < 0.0001);
        failures += Case(results, "face clearance default is 1.5 cm", Math.AbsFloat(defaults.faceClearanceMeters - 0.015) < 0.0001);
        NimbyTackConfig lowClearance = new NimbyTackConfig();
        lowClearance.faceClearanceMeters = 0.0;
        Sanitize(lowClearance);
        failures += Case(results, "face clearance below 5 mm clamps to 5 mm", Math.AbsFloat(lowClearance.faceClearanceMeters - 0.005) < 0.0001);
        NimbyTackConfig highClearance = new NimbyTackConfig();
        highClearance.faceClearanceMeters = 1.0;
        Sanitize(highClearance);
        failures += Case(results, "face clearance above 5 cm clamps to 5 cm", Math.AbsFloat(highClearance.faceClearanceMeters - 0.05) < 0.0001);
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

        // T6: the version 2 to 3 upgrade adds faceClearanceMeters once (NIMBY-T-028).
        failures += Case(results, "config version is 6", defaults.version == 6);
        // T27: the refusal boxes switch.
        failures += Case(results, "refusal notices default on", defaults.refusalNotices == 1);
        NimbyTackConfig v5File = new NimbyTackConfig();
        v5File.version = 5;
        v5File.refusalNotices = 0;
        bool upgradedV5 = UpgradeConfig(v5File, defaults);
        failures += Case(results, "version 5 file gains notices on", upgradedV5 && v5File.version == 6 && v5File.refusalNotices == 1);
        NimbyTackConfig oddNotices = new NimbyTackConfig();
        oddNotices.refusalNotices = 2;
        failures += Case(results, "sanitize folds an odd notices value to 1 and keeps 0", Sanitize(oddNotices) == 1 && oddNotices.refusalNotices == 1);
        // T22, NIMBY-T-032: allowances.
        failures += Case(results, "1C1 allowance default is 3 cm", Math.AbsFloat(ModelAllowance(defaults, "Land_House_1C1") - 0.03) < 0.0001);
        failures += Case(results, "2C1 has no allowance (known clipping, T-032)", ModelAllowance(defaults, "Land_HouseBlock_2C1") == 0.0);
        failures += Case(results, "a door leaf ignores the wall model allowance", Math.AbsFloat(AllowanceFor(defaults, "Land_House_1C1", true) - defaults.doorAllowanceMeters) < 0.0001);
        // T23, T25: the door allowance measured at 0 on October 25, 2026; Workshop2's door at 3 cm.
        failures += Case(results, "door allowance default is 0 (measured)", defaults.doorAllowanceMeters == 0.0);
        failures += Case(results, "Workshop2 door allowance default is 3 cm", Math.AbsFloat(DoorModelAllowance(defaults, "Land_Workshop2") - 0.03) < 0.0001);
        failures += Case(results, "a door leaf adds its door model allowance", Math.AbsFloat(AllowanceFor(defaults, "Land_Workshop2", true) - 0.03) < 0.0001);
        failures += Case(results, "a Workshop2 wall ignores the door model allowance", AllowanceFor(defaults, "Land_Workshop2", false) == 0.0);
        failures += Case(results, "door data with no door is valid", DoorDataValid(-1, -1));
        failures += Case(results, "door data with a door and a pivot is valid", DoorDataValid(2, 83886083));
        failures += Case(results, "a junk door index is invalid", !DoorDataValid(1065353216, 0));
        failures += Case(results, "a door without a pivot is invalid", !DoorDataValid(0, -1));
        failures += Case(results, "an unlisted model has no allowance", ModelAllowance(defaults, "Land_House_2B01") == 0.0);
        failures += Case(results, "a door leaf adds the door allowance", Math.AbsFloat(AllowanceFor(defaults, "Land_House_2B01", true) - defaults.doorAllowanceMeters) < 0.0001);
        NimbyTackConfig tuned = new NimbyTackConfig();
        tuned.modelAllowances.Insert(new NimbyTackAllowance("Land_House_1C1", 0.08));
        failures += Case(results, "the last allowance listed for a model wins", Math.AbsFloat(ModelAllowance(tuned, "Land_House_1C1") - 0.08) < 0.0001);
        NimbyTackConfig wild = new NimbyTackConfig();
        wild.doorAllowanceMeters = 1.0;
        wild.modelAllowances.Insert(new NimbyTackAllowance("Land_House_1C1", -0.5));
        wild.doorModelAllowances.Insert(new NimbyTackAllowance("Land_Workshop2", 0.9));
        Sanitize(wild);
        failures += Case(results, "allowances clamp to 0 and 15 cm", Math.AbsFloat(wild.doorAllowanceMeters - 0.15) < 0.0001 && ModelAllowance(wild, "Land_House_1C1") == 0.0);
        failures += Case(results, "door model allowances clamp to 15 cm", Math.AbsFloat(DoorModelAllowance(wild, "Land_Workshop2") - 0.15) < 0.0001);
        NimbyTackConfig v3File = new NimbyTackConfig();
        v3File.version = 3;
        v3File.modelAllowances = new array<ref NimbyTackAllowance>;
        bool upgradedV3 = UpgradeConfig(v3File, defaults);
        failures += Case(results, "version 3 file gains the built-in allowance list", upgradedV3 && v3File.version == defaults.version && Math.AbsFloat(ModelAllowance(v3File, "Land_House_1C1") - 0.03) < 0.0001);
        NimbyTackConfig v4File = new NimbyTackConfig();
        v4File.version = 4;
        v4File.doorAllowanceMeters = 0.0;
        v4File.doorModelAllowances = new array<ref NimbyTackAllowance>;
        bool upgradedV4 = UpgradeConfig(v4File, defaults);
        failures += Case(results, "version 4 file gains the door model list and keeps its door allowance", upgradedV4 && v4File.version == defaults.version && Math.AbsFloat(DoorModelAllowance(v4File, "Land_Workshop2") - 0.03) < 0.0001 && v4File.doorAllowanceMeters == 0.0);
        NimbyTackConfig oldFile = new NimbyTackConfig();
        oldFile.version = 2;
        oldFile.faceClearanceMeters = 0.0;
        bool upgraded = UpgradeConfig(oldFile, defaults);
        failures += Case(results, "version 2 file is upgraded", upgraded && oldFile.version == defaults.version);
        failures += Case(results, "upgrade fills a missing face clearance with the default", Math.AbsFloat(oldFile.faceClearanceMeters - defaults.faceClearanceMeters) < 0.0001);
        // 0.022 is no default of any version, so this case fails if the upgrade ever overwrites a
        // hand-set value.
        NimbyTackConfig handSet = new NimbyTackConfig();
        handSet.version = 2;
        handSet.faceClearanceMeters = 0.022;
        UpgradeConfig(handSet, defaults);
        failures += Case(results, "upgrade keeps a hand-set face clearance", Math.AbsFloat(handSet.faceClearanceMeters - 0.022) < 0.0001);
        NimbyTackConfig oldestFile = new NimbyTackConfig();
        oldestFile.version = 1;
        bool upgradedOldest = UpgradeConfig(oldestFile, defaults);
        failures += Case(results, "version 1 file is upgraded to the current version", upgradedOldest && oldestFile.version == defaults.version);
        NimbyTackConfig oldUnknown = new NimbyTackConfig();
        oldUnknown.version = 1;
        oldUnknown.unknownSurfaceOffsetMeters = 0.0;
        UpgradeConfig(oldUnknown, defaults);
        failures += Case(results, "upgrade fills a missing unknown-surface offset with the default", Math.AbsFloat(oldUnknown.unknownSurfaceOffsetMeters - defaults.unknownSurfaceOffsetMeters) < 0.0001);
        NimbyTackConfig current = new NimbyTackConfig();
        failures += Case(results, "current file is not upgraded", !UpgradeConfig(current, defaults));
        NimbyTackConfig future = new NimbyTackConfig();
        future.version = defaults.version + 1;
        future.faceClearanceMeters = 0.022;
        bool futureTouched = UpgradeConfig(future, defaults);
        failures += Case(results, "newer file is left alone", !futureTouched && future.version == defaults.version + 1 && Math.AbsFloat(future.faceClearanceMeters - 0.022) < 0.0001);

        // NIMBY-T-015 (T17) and NIMBY-T-030: the physics-depth guard at 20 cm.
        float depthLimit = PHYSICS_DEPTH_LIMIT_METERS;
        failures += Case(results, "physics depth limit is 20 cm", Math.AbsFloat(depthLimit - 0.2) < 0.0001);
        failures += Case(results, "physics 0.979 m ahead refuses (TK-P5 first face)", !PhysicsHitAccepted(0.979, depthLimit));
        failures += Case(results, "physics 0.635 m ahead refuses (TK-P5 second face)", !PhysicsHitAccepted(0.635, depthLimit));
        failures += Case(results, "physics 20.01 cm ahead refuses", !PhysicsHitAccepted(0.2001, depthLimit));
        failures += Case(results, "physics exactly 20 cm ahead is accepted", PhysicsHitAccepted(0.2, depthLimit));
        failures += Case(results, "physics 17 cm ahead is accepted (2W04_Blue front, T-030)", PhysicsHitAccepted(0.17, depthLimit));
        failures += Case(results, "physics 16 cm ahead is accepted (2W04_Blue front, T-030)", PhysicsHitAccepted(0.16, depthLimit));
        failures += Case(results, "physics 15.6 cm ahead is accepted (TK-P3 shed face, T-016)", PhysicsHitAccepted(0.156, depthLimit));
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

        // T19, NIMBY-T-028: the fire front, against footprint depths logged on October 25, 2026.
        array<float> mixed = new array<float>;
        mixed.Insert(0.01);
        mixed.Insert(DEPTH_MISS);
        mixed.Insert(0.045);
        mixed.Insert(0.3);
        mixed.Insert(-0.02);
        failures += Case(results, "fire hits skip misses and samples past the cap", FireHits(mixed, FIRE_SAMPLE_CAP_METERS) == 3);
        failures += Case(results, "fire front is the most forward sample inside the cap", Math.AbsFloat(FireFront(mixed, FIRE_SAMPLE_CAP_METERS) - 0.045) < 0.0001);
        array<float> clipper = new array<float>;
        clipper.Insert(0.0574);
        clipper.Insert(0.0621);
        clipper.Insert(0.0588);
        clipper.Insert(0.0634);
        float clipFront = FireFront(clipper, FIRE_SAMPLE_CAP_METERS);
        failures += Case(results, "clipping face pushes the note out (test 46: 7.8 cm)", Math.AbsFloat(FireOffset(clipFront, defaults.faceClearanceMeters) - 0.0784) < 0.0001);
        array<float> floater = new array<float>;
        floater.Insert(-0.0753);
        floater.Insert(-0.0829);
        floater.Insert(-0.0938);
        floater.Insert(-0.0973);
        float floatFront = FireFront(floater, FIRE_SAMPLE_CAP_METERS);
        failures += Case(results, "face behind the view hit pulls the note in (test 43: -6.0 cm)", Math.AbsFloat(FireOffset(floatFront, defaults.faceClearanceMeters) + 0.0603) < 0.0001);
        array<float> twoHits = new array<float>;
        twoHits.Insert(DEPTH_MISS);
        twoHits.Insert(0.0);
        twoHits.Insert(DEPTH_MISS);
        twoHits.Insert(0.0);
        failures += Case(results, "two fire hits fall back", !UseFire(FireHits(twoHits, FIRE_SAMPLE_CAP_METERS)));
        failures += Case(results, "three fire hits place by fire", UseFire(3));
        array<float> noFront = new array<float>;
        noFront.Insert(DEPTH_MISS);
        noFront.Insert(0.5);
        failures += Case(results, "no qualifying sample has no front", FireFront(noFront, FIRE_SAMPLE_CAP_METERS) == DEPTH_MISS);

        // T23, NIMBY-T-034: door leaves from the geometry layer (Land_House_1C1 front door, 18:37:01).
        failures += Case(results, "a door leaf is measured deeper than a wall", RayBack(true) > RayBack(false) && SampleCap(true) > SampleCap(false));
        failures += Case(results, "the door ray and cap reach the 1C1 leaf 13.2 cm back", RayBack(true) >= 0.132 && SampleCap(true) >= 0.132);
        array<float> doorLeaf = new array<float>;
        for (int leafSample = 0; leafSample < 9; leafSample++)
        {
            doorLeaf.Insert(-0.131907);
        }
        failures += Case(results, "the 1C1 leaf is a miss under the wall cap", FireHits(doorLeaf, SampleCap(false)) == 0);
        failures += Case(results, "the 1C1 leaf counts 9 of 9 under the door cap", FireHits(doorLeaf, SampleCap(true)) == 9);
        float leafFront = FireFront(doorLeaf, SampleCap(true));
        failures += Case(results, "door geometry pulls the note in to the leaf (-11.7 cm)", Math.AbsFloat(FireOffset(leafFront, defaults.faceClearanceMeters) + 0.116907) < 0.0001);
        failures += Case(results, "three door samples place by geometry, two fall back", DoorMethod(3) == PLACEMENT_DOOR && DoorMethod(2) == PLACEMENT_FALLBACK);

        // T20, NIMBY-T-025: the post run.
        float scanSpan = POST_SCAN_COUNT - 1;
        scanSpan = POST_SCAN_STEP_METERS * scanSpan;
        failures += Case(results, "post scan constants agree", Math.AbsFloat(scanSpan - 2.0 * POST_SCAN_HALF_METERS) < 0.0001 && POST_SCAN_CENTRE * 2 == POST_SCAN_COUNT - 1);
        float sheetWidth = SHEET_WIDTH_METERS;
        array<bool> post = ScanPattern(16, 20, -1);
        int postSeed = RunSeed(post, POST_SCAN_CENTRE, POST_SEED_REACH);
        int postFirst = RunEdge(post, postSeed, -1, POST_SCAN_MAX_GAP);
        int postLast = RunEdge(post, postSeed, 1, POST_SCAN_MAX_GAP);
        failures += Case(results, "post seed is the nearest hit", postSeed == 16);
        failures += Case(results, "post run spans the post", postFirst == 16 && postLast == 20);
        failures += Case(results, "post narrower than the sheet is a post", IsPost(RunBounded(post, postFirst, postLast, POST_SCAN_MAX_GAP), RunWidth(postFirst, postLast, POST_SCAN_STEP_METERS), sheetWidth));
        failures += Case(results, "note moves onto the post's middle (6 cm)", Math.AbsFloat(RunShift(postFirst, postLast, POST_SCAN_CENTRE, POST_SCAN_STEP_METERS) - 0.06) < 0.0001);
        array<bool> gapped = ScanPattern(12, 18, 15);
        int gappedSeed = RunSeed(gapped, POST_SCAN_CENTRE, POST_SEED_REACH);
        failures += Case(results, "one missed sample does not split a post", RunEdge(gapped, gappedSeed, -1, POST_SCAN_MAX_GAP) == 12 && RunEdge(gapped, gappedSeed, 1, POST_SCAN_MAX_GAP) == 18);
        array<bool> wall = ScanPattern(0, POST_SCAN_COUNT - 1, -1);
        failures += Case(results, "a run that reaches the scan's end is not a post", !RunBounded(wall, 0, POST_SCAN_COUNT - 1, POST_SCAN_MAX_GAP));
        array<bool> pillar = ScanPattern(3, 27, -1);
        failures += Case(results, "a pillar wider than the sheet is not a post", !IsPost(RunBounded(pillar, 3, 27, POST_SCAN_MAX_GAP), RunWidth(3, 27, POST_SCAN_STEP_METERS), sheetWidth));
        array<bool> aside = ScanPattern(24, 28, -1);
        failures += Case(results, "a run beyond the seed reach has no seed", RunSeed(aside, POST_SCAN_CENTRE, POST_SEED_REACH) == -1);
        failures += Case(results, "side normals of one fence agree", SidesAgree("0.1045 0 -0.9945", "0.1044 0 -0.9945", POST_SIDE_AGREE_DEGREES));
        failures += Case(results, "side normals 90 degrees apart disagree", !SidesAgree("1 0 0", "0 0 1", POST_SIDE_AGREE_DEGREES));
        failures += Case(results, "a missing side normal disagrees", !SidesAgree("0 0 0", "0 0 1", POST_SIDE_AGREE_DEGREES));

        // T24: door leaves pin open or closed; the area cap is the only count-based refusal.
        failures += Case(results, "the area cap refuses at the count", !WithinAreaCap(20, 20) && WithinAreaCap(19, 20));

        failures += Case(results, "pinned class per fastener", NimbyTackFastener.PinnedClass(NimbyTackFastener.KNIFE) == "NimbyTack_PinnedNote_Knife");
        failures += Case(results, "unknown fastener has no class", NimbyTackFastener.PinnedClass(9) == "");
        return failures;
    }

    // Self-test helper: a post scan with hits from first to last, and one miss at gap (-1: none).
    protected static array<bool> ScanPattern(int first, int last, int gap)
    {
        array<bool> pattern = new array<bool>;
        for (int index = 0; index < POST_SCAN_COUNT; index++)
        {
            bool hit = index >= first && index <= last && index != gap;
            pattern.Insert(hit);
        }
        return pattern;
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
