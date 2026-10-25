// -----------------------------------------------------------------------------
// NimbyTack - placement at pin time (server) and door attachment.
// Purpose : Where the note goes, measured before it is created (October 25, 2026
//           placement round, plan v0.15):
//           T20 (NIMBY-T-025): a horizontal fire scan across the hit finds a post
//             narrower than the sheet; the note moves onto the post's middle and
//             faces the fence line when both side probes agree.
//           T19 (NIMBY-T-028): a 3x3 fire-geometry grid across the sheet; the note
//             sits faceClearanceMeters in front of the most forward sample, or at
//             the fallback offset off the view hit when fewer than FIRE_MIN_HITS
//             samples land (Jeff, October 25, 2026).
//           T21 (NIMBY-T-029): a note on a door leaf. The server keeps it where the
//             closed door holds it (storage saves that), the door index, bone pivot,
//             and the note's transform in the pivot's frame are net-synced, and each
//             client parents its own copy to the pivot so it swings with the door.
//             Nothing new is saved (shared rules 6, net-synced only).
// Rays    : CollisionFlags.ALLOBJECTS (first contact per object, EnDebug.c:147), so a
//           note already on the wall cannot hide the wall. The default NEARESTCONTACT
//           (DayZPhysics.c, RaycastRVParams) is why the debug footprint's centre
//           sample missed on 49 of 54 pins on October 25: it ran after the note was
//           created, and the note was the nearest contact on the centre ray.
// Vanilla : Parenting to a bone pivot as Object.AddArrow (Object.c:1387-1415): the
//           child's transform is made local to the pivot, then AddChild(child, pivot).
//           The door from the aimed component as ActionLockDoors.c:38.
// -----------------------------------------------------------------------------
class NimbyTackPlacement
{
    vector position;
    vector normal;
    float offset;
    string method;
    float front;
    int fireHits;
    int fireSamples;
    float postWidth = -1.0;
    float shift = 0.0;
    string face = "hit";

    // The pin line's placement tokens.
    string Tag()
    {
        string post = "-";
        if (postWidth >= 0.0)
        {
            post = postWidth.ToString();
        }
        string tag = "placement=" + method + " front=" + NimbyTackRules.DepthTag(front) + " samples=" + fireHits.ToString() + "/" + fireSamples.ToString();
        tag = tag + " post=" + post + " shift=" + shift.ToString() + " face=" + face;
        return tag;
    }
}

class NimbyTackPlace
{
    // How far in front of and behind a sample point the fire ray runs. The back end is past the
    // 10 cm cap so a face 9.7 cm behind (test 43) is seen and then judged by the cap.
    static const float RAY_FRONT_METERS = 1.0;
    static const float RAY_BACK_METERS = 0.15;
    static const float SIDE_PROBE_METERS = 0.2;

    // T19 and T20 on the server, before the note exists.
    static NimbyTackPlacement Compute(PlayerBase player, NimbyTackHit hit, NimbyTackConfig config)
    {
        NimbyTackPlacement placement = new NimbyTackPlacement();
        vector normal = hit.normal;
        vector origin = hit.position;
        vector basis[4];
        Math3D.DirectionAndUpMatrix(NimbyTackRules.FaceDirection(normal), vector.Up, basis);
        vector right = basis[0];
        vector up = basis[1];
        float cap = NimbyTackRules.FIRE_SAMPLE_CAP_METERS;

        // T20: the post scan.
        array<bool> scan = new array<bool>;
        for (int index = 0; index < NimbyTackRules.POST_SCAN_COUNT; index++)
        {
            float along = index - NimbyTackRules.POST_SCAN_CENTRE;
            along = NimbyTackRules.POST_SCAN_STEP_METERS * along;
            float scanDepth = FireDepth(player, hit.wall, origin + right * along, normal);
            bool scanHit = scanDepth != NimbyTackRules.DEPTH_MISS && Math.AbsFloat(scanDepth) <= cap;
            scan.Insert(scanHit);
        }
        int seed = NimbyTackRules.RunSeed(scan, NimbyTackRules.POST_SCAN_CENTRE, NimbyTackRules.POST_SEED_REACH);
        if (seed >= 0)
        {
            int first = NimbyTackRules.RunEdge(scan, seed, -1, NimbyTackRules.POST_SCAN_MAX_GAP);
            int last = NimbyTackRules.RunEdge(scan, seed, 1, NimbyTackRules.POST_SCAN_MAX_GAP);
            bool bounded = NimbyTackRules.RunBounded(scan, first, last, NimbyTackRules.POST_SCAN_MAX_GAP);
            float width = NimbyTackRules.RunWidth(first, last, NimbyTackRules.POST_SCAN_STEP_METERS);
            if (NimbyTackRules.IsPost(bounded, width, NimbyTackRules.SHEET_WIDTH_METERS))
            {
                placement.postWidth = width;
                placement.shift = NimbyTackRules.RunShift(first, last, NimbyTackRules.POST_SCAN_CENTRE, NimbyTackRules.POST_SCAN_STEP_METERS);
                origin = origin + right * placement.shift;
                vector leftNormal = SideNormal(player, hit.wall, origin, normal, right, -SIDE_PROBE_METERS);
                vector rightNormal = SideNormal(player, hit.wall, origin, normal, right, SIDE_PROBE_METERS);
                if (NimbyTackRules.SidesAgree(leftNormal, rightNormal, NimbyTackRules.POST_SIDE_AGREE_DEGREES))
                {
                    vector sides = leftNormal + rightNormal;
                    sides.Normalize();
                    normal = sides;
                    placement.face = "sides";
                    Math3D.DirectionAndUpMatrix(NimbyTackRules.FaceDirection(normal), vector.Up, basis);
                    right = basis[0];
                    up = basis[1];
                }
            }
            if (NimbyTackLog.IsDebug())
            {
                string scanLine = "post-scan wall=" + NimbyTackWorld.ClassOf(hit.wall) + " seed=" + seed.ToString() + " first=" + first.ToString();
                scanLine = scanLine + " last=" + last.ToString() + " bounded=" + NimbyTackLog.Bool(bounded) + " width=" + width.ToString();
                scanLine = scanLine + " hits=" + ScanTag(scan);
                NimbyTackLog.Debug(scanLine);
            }
        }
        else if (NimbyTackLog.IsDebug())
        {
            NimbyTackLog.Debug("post-scan wall=" + NimbyTackWorld.ClassOf(hit.wall) + " seed=-1 hits=" + ScanTag(scan));
        }

        // T19: the fire grid across the sheet at the (possibly moved) origin.
        float halfWidth = NimbyTackRules.SHEET_WIDTH_METERS * 0.5;
        float halfHeight = NimbyTackRules.SHEET_HEIGHT_METERS * 0.5;
        array<float> depths = new array<float>;
        for (int row = -1; row <= 1; row++)
        {
            for (int column = -1; column <= 1; column++)
            {
                float across = column;
                float down = row;
                vector point = origin + right * (halfWidth * across) + up * (halfHeight * down);
                depths.Insert(FireDepth(player, hit.wall, point, normal));
            }
        }
        placement.fireSamples = depths.Count();
        placement.fireHits = NimbyTackRules.FireHits(depths, cap);
        placement.front = NimbyTackRules.FireFront(depths, cap);
        if (NimbyTackRules.UseFire(placement.fireHits))
        {
            placement.method = NimbyTackRules.PLACEMENT_FIRE;
            placement.offset = NimbyTackRules.FireOffset(placement.front, config.faceClearanceMeters);
        }
        else
        {
            placement.method = NimbyTackRules.PLACEMENT_FALLBACK;
            placement.offset = NimbyTackRules.OffsetFor(hit.surface, config);
        }
        placement.normal = normal;
        placement.position = NimbyTackRules.PinPosition(origin, normal, placement.offset);
        if (NimbyTackLog.IsDebug())
        {
            string grid = "";
            foreach (float sample : depths)
            {
                grid = grid + " " + NimbyTackRules.DepthTag(sample);
            }
            NimbyTackLog.Debug("fire-grid wall=" + NimbyTackWorld.ClassOf(hit.wall) + " depths=" + grid + " " + placement.Tag());
        }
        return placement;
    }

    // The signed depth of the wall's fire geometry under one point, along the normal from the
    // plane through the point, or DEPTH_MISS. First contact per object, so other notes and
    // objects in front of the wall are passed over.
    static float FireDepth(PlayerBase player, Object wall, vector point, vector normal)
    {
        if (!wall)
        {
            return NimbyTackRules.DEPTH_MISS;
        }
        vector start = point + normal * RAY_FRONT_METERS;
        vector end = point - normal * RAY_BACK_METERS;
        RaycastRVParams params = new RaycastRVParams(start, end, player, 0.0);
        params.sorted = true;
        params.type = ObjIntersectFire;
        params.flags = CollisionFlags.ALLOBJECTS;
        array<ref RaycastRVResult> results = new array<ref RaycastRVResult>;
        if (!DayZPhysics.RaycastRVProxy(params, results))
        {
            return NimbyTackRules.DEPTH_MISS;
        }
        foreach (RaycastRVResult result : results)
        {
            if (result.obj != wall)
            {
                continue;
            }
            return NimbyTackRules.SignedDepth(result.pos, point, normal);
        }
        return NimbyTackRules.DEPTH_MISS;
    }

    // T20: the physics normal of the wall object beside the post, facing the player; "0 0 0" when
    // the probe finds nothing of the wall there.
    static vector SideNormal(PlayerBase player, Object wall, vector origin, vector normal, vector right, float side)
    {
        vector start = origin + right * side + normal * RAY_FRONT_METERS;
        vector end = origin + right * side - normal * 0.3;
        Object hitObject;
        vector hitPosition;
        vector hitNormal;
        float hitFraction;
        if (!DayZPhysics.RayCastBullet(start, end, PhxInteractionLayers.BUILDING, player, hitObject, hitPosition, hitNormal, hitFraction))
        {
            return "0 0 0";
        }
        if (hitObject != wall || hitNormal.Length() < 0.5)
        {
            return "0 0 0";
        }
        hitNormal.Normalize();
        if (vector.Dot(hitNormal, normal) < 0.0)
        {
            hitNormal = hitNormal * -1;
        }
        return hitNormal;
    }

    protected static string ScanTag(array<bool> scan)
    {
        string tag = "";
        foreach (bool sample : scan)
        {
            if (sample)
            {
                tag = tag + "x";
            }
            else
            {
                tag = tag + ".";
            }
        }
        return tag;
    }

    // ---- T21: doors ----

    // The door index of the aimed component, or -1 when the target is no building or no door.
    static int DoorIndexOf(Object wall, int component)
    {
        Building building = Building.Cast(wall);
        if (!building || component < 0)
        {
            return -1;
        }
        return building.GetDoorIndex(component);
    }

    static bool DoorClosed(Object wall, int doorIndex)
    {
        Building building = Building.Cast(wall);
        if (!building || doorIndex < 0)
        {
            return false;
        }
        return building.IsDoorClosed(doorIndex);
    }

    // The bone pivot of a component: view geometry first (the LOD the action target's component
    // comes from, ActionTargets.c:248), then fire (Object.AddArrow), then geometry. levelName
    // receives which one answered.
    static int DoorPivot(Object wall, int component, out string levelName)
    {
        levelName = "none";
        if (!wall || component < 0)
        {
            return -1;
        }
        int pivot = wall.GetBonePivot(wall.GetViewGeometryLevel(), component);
        if (pivot >= 0)
        {
            levelName = "view";
            return pivot;
        }
        pivot = wall.GetBonePivot(wall.GetFireGeometryLevel(), component);
        if (pivot >= 0)
        {
            levelName = "fire";
            return pivot;
        }
        pivot = wall.GetBonePivot(wall.GetGeometryLevel(), component);
        if (pivot >= 0)
        {
            levelName = "geometry";
            return pivot;
        }
        return -1;
    }

    // The building whose door doorIndex stands nearest to position (within 3 m), for a client
    // that knows only the note and the synced door index. Map buildings carry no network id.
    static Building FindDoorBuilding(vector position, int doorIndex)
    {
        array<Object> objects = new array<Object>;
        array<CargoBase> proxies = new array<CargoBase>;
        GetGame().GetObjectsAtPosition3D(position, 30.0, objects, proxies);
        Building best = null;
        float bestDistance = 3.0;
        foreach (Object candidate : objects)
        {
            Building building = Building.Cast(candidate);
            if (!building)
            {
                continue;
            }
            if (doorIndex >= building.GetDoorCount())
            {
                continue;
            }
            float distance = vector.Distance(building.GetDoorSoundPos(doorIndex), position);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                best = building;
            }
        }
        return best;
    }

    // The door component under a pinned note, by a view ray through it along its facing, for the
    // server after a restart (the doors are closed then; the note does not know its door).
    static bool FindDoorUnder(EntityAI note, out Object wall, out int component)
    {
        wall = null;
        component = -1;
        vector transform[4];
        note.GetTransform(transform);
        vector facing = transform[2];
        vector position = transform[3];
        RaycastRVParams params = new RaycastRVParams(position + facing * 0.3, position - facing * 0.3, note, 0.0);
        params.sorted = true;
        params.flags = CollisionFlags.ALLOBJECTS;
        array<ref RaycastRVResult> results = new array<ref RaycastRVResult>;
        if (!DayZPhysics.RaycastRVProxy(params, results))
        {
            return false;
        }
        foreach (RaycastRVResult result : results)
        {
            if (!result.obj || result.obj == note)
            {
                continue;
            }
            if (!Building.Cast(result.obj))
            {
                continue;
            }
            wall = result.obj;
            component = result.component;
            return true;
        }
        return false;
    }
}
