// -----------------------------------------------------------------------------
// NimbyTack - world queries shared by the actions.
// Purpose : The engine-facing half of Tack: the wall probe (view-geometry raycast
//           from the head to the cursor hit for the object, position, and surface,
//           DayZPhysics.c:98; physics raycast for the normal, DayZPhysics.c:211),
//           the inventory searches, the per-area count, the pinned-note creation,
//           and the log tags. Every decision these feed is made in NimbyTackRules.
// Normal  : RaycastRVResult.dir is NOT the surface normal for a line-object hit.
//           The October 18, 2026 round logged it varying with the player's pitch on
//           one flat shed wall (y from +0.18 to -0.20), so it is the ray itself;
//           "normal * offset" had been pushing the sheet INTO the wall, which is why
//           1 cm sat inside the mud brick and 6 cm vanished in both walls. The normal
//           now comes from DayZPhysics.RayCastBullet and points toward the player.
// Guard   : NIMBY-T-015 (T17, October 21, 2026): WallNormal refuses the pin when the
//           physics hit stands more than NimbyTackRules.PHYSICS_DEPTH_LIMIT_METERS in
//           front of the view hit (reason physics-ahead), so a note is never placed
//           inside a wall whose physics shell sits far in front of its drawn face.
// Part B  : The T-013/T-014 study (October 20, 2026) adds debug-only diagnostics:
//           a detail for every probe refusal (B1), the vanilla cursor ray rebuilt
//           (B2), the client's probe lines held for the pin-check gate (B3), the
//           fire line's ray= and engine-computed depth= (B4), and the footprint
//           samples at pin time (B5). None changes where or whether a note pins.
// Place   : Where the note goes is decided by NimbyTackPlace (nimbytack_placement.c,
//           October 25, 2026 placement round); this file probes the wall, refuses a
//           door leaf that is not closed (T21), and creates the note.
// Cost    : The probe's detail strings and probe: lines are built only when they
//           can be read: with the debug flag, or (the detail alone) on the server,
//           whose production refuse line prints it once per request. The client
//           without the flag builds no string per frame (review row 1).
// -----------------------------------------------------------------------------
class NimbyTackHit
{
    vector position;
    vector normal;
    string surface;
    Object wall;
    // T21: the aimed component and its door index (-1 off a door).
    int component = -1;
    int doorIndex = -1;
}

class NimbyTackWorld
{
    // Both identities on every player line (shared rules 4.2).
    static string PlayerTag(PlayerBase player)
    {
        if (!player)
        {
            return "player=none";
        }
        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
        {
            return "player=local/local";
        }
        return "player=" + identity.GetId() + "/" + identity.GetPlainId();
    }

    static string PositionTag(vector position)
    {
        return "pos=" + position[0].ToString() + " " + position[1].ToString() + " " + position[2].ToString();
    }

    // Why the last ProbeWall call refused, as one token for the server's refusal line. The
    // actions run one probe at a time on each side, so a static is enough and avoids a second
    // out parameter (Enforce out parameters on members are unreliable, shared rules).
    protected static string s_LastProbeReason = "";

    static string LastProbeReason()
    {
        return s_LastProbeReason;
    }

    // Part B (B1): what the last refusal measured, for the pin-check line ("-" when nothing).
    protected static string s_LastProbeDetail = "-";

    static string LastProbeDetail()
    {
        return s_LastProbeDetail;
    }

    // True when a refusal detail is worth building: the debug flag is on (the pin-check line reads
    // it on either side), or this is the server, whose production refuse pin line prints it once
    // per request. The client's per-frame condition without the flag skips the ToString work.
    protected static bool WantDetail()
    {
        if (NimbyTackLog.IsDebug())
        {
            return true;
        }
        return GetGame().IsServer();
    }

    // Part B (B3): on the client the pin condition runs every frame; its probe lines are held
    // here and written only when the pin-check gate writes its line, then dropped. The server
    // never holds (its probes run once per request and at pin time).
    protected static bool s_HoldTrace = false;
    protected static ref array<string> s_HeldTrace;

    static void HoldTrace()
    {
        s_HoldTrace = true;
        if (!s_HeldTrace)
        {
            s_HeldTrace = new array<string>;
        }
        s_HeldTrace.Clear();
    }

    static void ReleaseTrace(bool write)
    {
        s_HoldTrace = false;
        if (!s_HeldTrace)
        {
            return;
        }
        if (write)
        {
            foreach (string held : s_HeldTrace)
            {
                NimbyTackLog.Debug(held);
            }
        }
        s_HeldTrace.Clear();
    }

    static void Trace(string line)
    {
        if (!NimbyTackLog.IsDebug())
        {
            return;
        }
        if (s_HoldTrace && s_HeldTrace)
        {
            s_HeldTrace.Insert(line);
            return;
        }
        NimbyTackLog.Debug(line);
    }

    // T2: the wall under the cursor. The action target gives the object and the cursor hit
    // position but no normal (ActionTargets.c:115), so one raycast from the head to just past
    // the hit point recovers the normal and, when asked, the surface material. Players, items,
    // and the probing player are never walls.
    // wantSurface: the surface name costs a second raycast against fire geometry, so only the
    // knife's wood test, the server's pin log, and the debug flag ask for it; the other
    // fasteners' per-frame client condition does not.
    static bool ProbeWall(PlayerBase player, ActionTarget target, NimbyTackConfig config, bool wantSurface, out NimbyTackHit hit)
    {
        s_LastProbeReason = "";
        s_LastProbeDetail = "-";
        if (!player || !target)
        {
            s_LastProbeReason = "no-target";
            return false;
        }
        Object wall = target.GetObject();
        if (!wall)
        {
            wall = target.GetParent();
        }
        if (!wall)
        {
            s_LastProbeReason = "no-object";
            return false;
        }
        if (wall.IsMan() || wall.IsInherited(ItemBase))
        {
            s_LastProbeReason = "target-not-building";
            if (WantDetail())
            {
                s_LastProbeDetail = ClassOf(wall);
            }
            return false;
        }
        vector head = MiscGameplayFunctions.GetPlayerHeadPosition(player);
        vector cursor = target.GetCursorHitPos();
        float headDistance = vector.Distance(head, cursor);
        // Written as "not within reach" so a NaN distance (a cursor with a NaN component) refuses
        // instead of reaching the raycast (review row 3).
        if (!(headDistance <= config.reachMeters))
        {
            s_LastProbeReason = "out-of-reach";
            if (WantDetail())
            {
                s_LastProbeDetail = "head=" + headDistance.ToString() + " limit=" + config.reachMeters.ToString();
            }
            return false;
        }
        vector direction = vector.Direction(head, cursor);
        direction.Normalize();
        vector end = cursor + direction * 0.1;
        RaycastRVParams params = new RaycastRVParams(head, end, player, 0.0);
        params.sorted = true;
        array<ref RaycastRVResult> results = new array<ref RaycastRVResult>;
        if (!DayZPhysics.RaycastRVProxy(params, results))
        {
            s_LastProbeReason = "no-hit";
            Trace("probe: no hit from head to cursor");
            return false;
        }
        // Part B (B1): the first object the ray skipped, for the no-building-hit detail.
        string skipped = "-";
        foreach (RaycastRVResult result : results)
        {
            if (!result.obj)
            {
                continue;
            }
            if (result.obj == player || result.obj.IsMan() || result.obj.IsInherited(ItemBase))
            {
                if (skipped == "-")
                {
                    skipped = ClassOf(result.obj);
                }
                continue;
            }
            vector normal;
            if (!WallNormal(head, end, direction, player, result.pos, normal))
            {
                // WallNormal names its own refusal (no-physics-normal or physics-ahead); this is
                // the fallback for a path that did not.
                if (s_LastProbeReason == "")
                {
                    s_LastProbeReason = "no-physics-normal";
                }
                return false;
            }
            // The angle test comes before the surface lookup: a floor or a ceiling never pays
            // for the second ray.
            if (!NimbyTackRules.IsWallNormal(normal, config.wallAngleDegrees))
            {
                s_LastProbeReason = "not-wall-angle";
                if (WantDetail())
                {
                    s_LastProbeDetail = "ny=" + normal[1].ToString();
                }
                if (NimbyTackLog.IsDebug())
                {
                    Trace("probe: refused hit=" + result.obj.GetType() + " normal=" + normal.ToString() + " " + PositionTag(result.pos));
                }
                return false;
            }
            string surfaceName = "";
            if (wantSurface)
            {
                surfaceName = SurfaceNameOf(result);
                if (surfaceName == "")
                {
                    // The view-geometry hit carried no surface on every Nasdara building tried
                    // on October 17, 2026. Fire geometry is what bullets read materials from,
                    // so ask it.
                    surfaceName = FireSurfaceName(head, end, player, result.obj, result.pos, normal);
                }
            }
            if (NimbyTackLog.IsDebug())
            {
                Trace("probe: hit=" + result.obj.GetType() + " surface=" + surfaceName + " normal=" + normal.ToString() + " " + PositionTag(result.pos));
            }
            // T21 (NIMBY-T-029): the action target's component names the door, as vanilla
            // ActionLockDoors.c:38; only when the probe hit the target object itself.
            int component = -1;
            int doorIndex = -1;
            if (result.obj == wall)
            {
                component = target.GetComponentIndex();
                doorIndex = NimbyTackPlace.DoorIndexOf(wall, component);
            }
            // T24 (Jeff, October 25, 2026): a door leaf pins open or closed; T21b follows the leaf.
            hit = new NimbyTackHit();
            hit.position = result.pos;
            hit.normal = normal;
            hit.surface = surfaceName;
            hit.wall = result.obj;
            hit.component = component;
            hit.doorIndex = doorIndex;
            return true;
        }
        s_LastProbeReason = "no-building-hit";
        if (WantDetail())
        {
            s_LastProbeDetail = "skipped=" + skipped;
        }
        return false;
    }

    // T2: one sheet of paper anywhere in the player's inventory, hands included. A pinned
    // note is never a candidate (it is not in an inventory anyway).
    // NIMBY-T-009 (Jeff, October 19, 2026): a written sheet goes up before a blank one. One walk
    // of the enumeration does what two passes would (first written single sheet, else the first
    // single sheet): the first candidate is kept until NimbyTackRules.PreferWritten replaces it,
    // and the walk stops at the first written one. A blank pin stays legal by plan T2: a sheet
    // left on a wall for someone else to write on.
    // The text lives on the server (WrittenNoteData.c:6,73; a client holds it only after a write
    // or read RPC, :22,:50), so the choice is the server's. The client's per-frame condition only
    // asks whether any sheet exists, which the order does not change.
    static Paper FindSingleSheet(PlayerBase player)
    {
        if (!player)
        {
            return null;
        }
        array<EntityAI> items = new array<EntityAI>;
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
        Paper chosen = null;
        bool chosenWritten = false;
        foreach (EntityAI entity : items)
        {
            Paper sheet = Paper.Cast(entity);
            if (!sheet)
            {
                continue;
            }
            if (sheet.IsInherited(NimbyTack_PinnedNote))
            {
                continue;
            }
            if (sheet.IsDamageDestroyed())
            {
                continue;
            }
            if (!NimbyTackRules.IsSingleSheet(sheet.HasQuantity(), sheet.GetQuantity()))
            {
                continue;
            }
            bool written = IsWritten(sheet);
            if (!chosen || NimbyTackRules.PreferWritten(written, chosenWritten))
            {
                chosen = sheet;
                chosenWritten = written;
            }
            if (chosenWritten)
            {
                break;
            }
        }
        return chosen;
    }

    // A sheet carries text when its WrittenNoteData (Paper.c:30) holds a non-empty string
    // (WrittenNoteData.c:73).
    static bool IsWritten(Paper sheet)
    {
        if (!sheet)
        {
            return false;
        }
        WrittenNoteData data = sheet.GetWrittenNoteData();
        if (!data)
        {
            return false;
        }
        return data.GetNoteText() != "";
    }

    // T2: a Nail item with at least one nail in it. Nails stack to 999 in one item.
    static ItemBase FindNail(PlayerBase player)
    {
        if (!player)
        {
            return null;
        }
        array<EntityAI> items = new array<EntityAI>;
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
        foreach (EntityAI entity : items)
        {
            if (entity.GetType() != "Nail")
            {
                continue;
            }
            ItemBase nail = ItemBase.Cast(entity);
            if (!nail)
            {
                continue;
            }
            if (nail.GetQuantity() >= 1.0)
            {
                return nail;
            }
        }
        return null;
    }

    // T26, NIMBY-T-047: the wall and door of a refusal, for the production refuse pin lines.
    static string WallTag(NimbyTackHit hit)
    {
        if (!hit)
        {
            return "wall=- door=-1";
        }
        return "wall=" + ClassOf(hit.wall) + " door=" + hit.doorIndex.ToString();
    }

    // T6: pinned notes already inside the radius of a position. On the client (T26) the notes
    // inside the radius are the networked copies it has been sent, so the count can run short of
    // the server's near the edge of the player's network bubble; the server's count decides.
    static int CountPinnedNear(vector position, float radius)
    {
        array<Object> objects = new array<Object>;
        array<CargoBase> proxies = new array<CargoBase>;
        GetGame().GetObjectsAtPosition3D(position, radius, objects, proxies);
        int count = 0;
        foreach (Object candidate : objects)
        {
            if (candidate.IsInherited(NimbyTack_PinnedNote))
            {
                count = count + 1;
            }
        }
        return count;
    }

    // The outward surface normal at the wall under the ray, from the physics geometry
    // (PhxInteractionLayers.BUILDING). RayCastBullet reports the face normal; when the physics
    // geometry hands back the far face, the normal is flipped so it always points toward the
    // player, which is the side the sheet hangs on and faces.
    // Part B (B4): the line carries gap= (straight distance to the view hit) and depth= (signed,
    // along the normal, positive in front of the view hit), computed here at full precision.
    // NIMBY-T-015 (T17, Jeff, October 21, 2026): a physics hit more than
    // NimbyTackRules.PHYSICS_DEPTH_LIMIT_METERS in front of the view hit refuses the pin with
    // reason physics-ahead; on Land_House_2M1 it stood 0.6 to 0.98 m in front and the note was
    // buried (TK-P5). This is the only place that holds both hits, and the normal is already
    // flipped toward the player when the depth is taken. Every refusal here sets the reason; the
    // physics line is traced before the guard so the depth stays visible with the flag on.
    static bool WallNormal(vector head, vector end, vector direction, PlayerBase player, vector viewHit, out vector normal)
    {
        Object hitObject;
        vector hitPosition;
        vector hitNormal;
        float hitFraction;
        if (!DayZPhysics.RayCastBullet(head, end, PhxInteractionLayers.BUILDING, player, hitObject, hitPosition, hitNormal, hitFraction))
        {
            s_LastProbeReason = "no-physics-normal";
            s_LastProbeDetail = "none";
            Trace("probe: physics ray had no building hit");
            return false;
        }
        if (hitNormal.Length() < 0.5)
        {
            s_LastProbeReason = "no-physics-normal";
            s_LastProbeDetail = "zero";
            Trace("probe: physics ray returned a zero normal");
            return false;
        }
        hitNormal.Normalize();
        if (vector.Dot(hitNormal, direction) > 0.0)
        {
            hitNormal = hitNormal * -1;
        }
        string hitName = "none";
        if (hitObject)
        {
            hitName = ClassOf(hitObject);
        }
        float depth = NimbyTackRules.SignedDepth(hitPosition, viewHit, hitNormal);
        if (NimbyTackLog.IsDebug())
        {
            float gap = vector.Distance(hitPosition, viewHit);
            string line = "probe: physics hit=" + hitName + " normal=" + hitNormal.ToString() + " fraction=" + hitFraction.ToString() + " " + PositionTag(hitPosition);
            line = line + " gap=" + gap.ToString() + " depth=" + depth.ToString();
            Trace(line);
        }
        float depthLimit = NimbyTackRules.PHYSICS_DEPTH_LIMIT_METERS;
        if (!NimbyTackRules.PhysicsHitAccepted(depth, depthLimit))
        {
            s_LastProbeReason = NimbyTackRules.REASON_PHYSICS_AHEAD;
            if (WantDetail())
            {
                s_LastProbeDetail = "depth=" + depth.ToString() + " limit=" + depthLimit.ToString() + " physics=" + hitName;
            }
            return false;
        }
        normal = hitNormal;
        return true;
    }

    // Surface name from one raycast hit: the surface file name, else its type. Empty when the
    // geometry carries no surface info.
    static string SurfaceNameOf(RaycastRVResult result)
    {
        if (!result)
        {
            return "";
        }
        if (!result.surface)
        {
            return "";
        }
        string name = result.surface.GetName();
        if (name != "")
        {
            return name;
        }
        return result.surface.GetSurfaceType();
    }

    // Same ray against fire geometry (ObjIntersectFire), surface of the first hit on the wall
    // object. Debug-logs what it found, with the fire hit's position next to the view-geometry
    // hit (viewHit), the gap between them, and the signed depth along the physics normal
    // (Part B, B4): the pin sits on the view hit.
    // RaycastRVResult.dir is the ray for a line-object hit, not a normal (DayZPhysics.c:104;
    // T-013 study section 4.1), so the line names it ray=.
    static string FireSurfaceName(vector head, vector end, PlayerBase player, Object wall, vector viewHit, vector normal)
    {
        RaycastRVParams params = new RaycastRVParams(head, end, player, 0.0);
        params.sorted = true;
        params.type = ObjIntersectFire;
        array<ref RaycastRVResult> results = new array<ref RaycastRVResult>;
        if (!DayZPhysics.RaycastRVProxy(params, results))
        {
            Trace("probe: fire-geometry ray had no hit");
            return "";
        }
        foreach (RaycastRVResult result : results)
        {
            if (!result.obj)
            {
                continue;
            }
            if (result.obj != wall)
            {
                continue;
            }
            string name = SurfaceNameOf(result);
            vector ray = result.dir;
            ray.Normalize();
            float gap = vector.Distance(result.pos, viewHit);
            float depth = NimbyTackRules.SignedDepth(result.pos, viewHit, normal);
            string line = "probe: fire-geometry surface=" + name + " component=" + result.component.ToString() + " ray=" + ray.ToString() + " " + PositionTag(result.pos);
            line = line + " view-" + PositionTag(viewHit) + " gap=" + gap.ToString() + " depth=" + depth.ToString();
            Trace(line);
            return name;
        }
        Trace("probe: fire-geometry ray did not hit the wall object");
        return "";
    }

    // ---- Part B diagnostics (debug only; T-013/T-014 study, October 20, 2026) ----

    // A class name for a log token: "-" for no object or an object with no config class (the
    // class-less mud-brick piece at 4026, 14111).
    static string ClassOf(Object obj)
    {
        if (!obj)
        {
            return "-";
        }
        string name = obj.GetType();
        if (name == "")
        {
            return "-";
        }
        return name;
    }

    // The model file (Object.c:503); "-" when there is none.
    static string ShapeOf(Object obj)
    {
        if (!obj)
        {
            return "-";
        }
        string shape = obj.GetShapeName();
        if (shape == "")
        {
            return "-";
        }
        return shape;
    }

    // The entity id (EnEntity.c:518). Vanilla targeting skips a proxy whose id is below 1
    // (ActionTargets.c:271-275); this is the number that says whether that happened.
    static string IdOf(Object obj)
    {
        if (!obj)
        {
            return "-";
        }
        return obj.GetID().ToString();
    }

    // B1: the action target as log tokens: object, model, id, proxy flag (ActionTargets.c:152),
    // parent, and component.
    static string TargetTag(ActionTarget target)
    {
        if (!target)
        {
            return "target=none";
        }
        Object obj = target.GetObject();
        Object parent = target.GetParent();
        string tag = "target=" + ClassOf(obj) + " shape=" + ShapeOf(obj) + " id=" + IdOf(obj) + " proxy=" + NimbyTackLog.Bool(target.IsProxy());
        tag = tag + " parent=" + ClassOf(parent) + " pshape=" + ShapeOf(parent) + " component=" + target.GetComponentIndex().ToString();
        return tag;
    }

    // B1: the gate key's target half. The id tells two class-less pieces apart.
    static string TargetKey(ActionTarget target)
    {
        if (!target)
        {
            return "none";
        }
        return ClassOf(target.GetObject()) + "|" + IdOf(target.GetObject()) + "|" + ClassOf(target.GetParent());
    }

    // B1: where the cursor is and how far it is from the head and the feet, the two points the
    // vanilla 2.0 m test measures from (CCTCursor.c:27-35).
    static string ReachTag(PlayerBase player, ActionTarget target)
    {
        if (!player || !target)
        {
            return "cursor=none";
        }
        vector cursor = target.GetCursorHitPos();
        float head = vector.Distance(MiscGameplayFunctions.GetPlayerHeadPosition(player), cursor);
        float feet = vector.Distance(player.GetPosition(), cursor);
        string tag = "cursor=" + cursor[0].ToString() + " " + cursor[1].ToString() + " " + cursor[2].ToString();
        tag = tag + " head=" + head.ToString() + " feet=" + feet.ToString();
        return tag;
    }

    // B1, no-single-sheet: what the Paper in inventory is, by NimbyTackRules.SheetKind.
    static string SheetSummary(PlayerBase player)
    {
        if (!player)
        {
            return "singles=0 stacks=0 ruined=0";
        }
        array<EntityAI> items = new array<EntityAI>;
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
        int singles = 0;
        int stacks = 0;
        int ruined = 0;
        foreach (EntityAI entity : items)
        {
            Paper sheet = Paper.Cast(entity);
            if (!sheet)
            {
                continue;
            }
            if (sheet.IsInherited(NimbyTack_PinnedNote))
            {
                continue;
            }
            int kind = NimbyTackRules.SheetKind(sheet.IsDamageDestroyed(), sheet.HasQuantity(), sheet.GetQuantity());
            if (kind == NimbyTackRules.SHEET_SINGLE)
            {
                singles = singles + 1;
            }
            if (kind == NimbyTackRules.SHEET_STACK)
            {
                stacks = stacks + 1;
            }
            if (kind == NimbyTackRules.SHEET_RUINED)
            {
                ruined = ruined + 1;
            }
        }
        return "singles=" + singles.ToString() + " stacks=" + stacks.ToString() + " ruined=" + ruined.ToString();
    }

    // B2: the vanilla cursor ray rebuilt (ActionTargets.c:245-255: camera, 5 m, all objects) and
    // its first four hits, each with whether vanilla would keep it as a target (:267, :271-275,
    // :411-417). Client only, when the pin-check reason points at targeting.
    static void LogCursorRay(PlayerBase player)
    {
        vector start = GetGame().GetCurrentCameraPosition();
        vector end = start + GetGame().GetCurrentCameraDirection() * 5.0;
        RaycastRVParams params = new RaycastRVParams(start, end, player, 0.0);
        params.flags = CollisionFlags.ALLOBJECTS;
        params.sorted = true;
        array<ref RaycastRVResult> results = new array<ref RaycastRVResult>;
        if (!DayZPhysics.RaycastRVProxy(params, results))
        {
            NimbyTackLog.Debug("cursor-ray hits=0");
            return;
        }
        int index = 0;
        foreach (RaycastRVResult result : results)
        {
            if (index >= 4)
            {
                break;
            }
            index = index + 1;
            bool actionTarget = true;
            if (result.obj && !result.obj.CanBeActionTarget())
            {
                actionTarget = false;
            }
            bool kept = actionTarget;
            if (kept && result.hierLevel > 0)
            {
                if (result.parent && result.parent.IsMan())
                {
                    kept = false;
                }
                if (result.obj && result.obj.GetID() < 1 && !TentBase.Cast(result.parent))
                {
                    kept = false;
                }
            }
            float distance = vector.Distance(start, result.pos);
            string line = "cursor-ray hit=" + index.ToString() + " class=" + ClassOf(result.obj) + " shape=" + ShapeOf(result.obj) + " hier=" + result.hierLevel.ToString() + " id=" + IdOf(result.obj);
            line = line + " parent=" + ClassOf(result.parent) + " dist=" + distance.ToString() + " action-target=" + NimbyTackLog.Bool(actionTarget) + " kept=" + NimbyTackLog.Bool(kept);
            line = line + " component=" + result.component.ToString() + " " + PositionTag(result.pos);
            NimbyTackLog.Debug(line);
        }
    }

    // B5: the sheet's size, read once per start from the pinned note's collision box
    // (Object.c:370); model X is the width and model Y the height (FaceDirection, T3).
    protected static bool s_SheetSizeRead = false;
    protected static float s_SheetHalfWidth = 0.075;
    protected static float s_SheetHalfHeight = 0.105;
    protected static string s_SheetSizeSource = "default";

    protected static void ReadSheetSize(Object sheet)
    {
        if (s_SheetSizeRead || !sheet)
        {
            return;
        }
        s_SheetSizeRead = true;
        vector box[2];
        if (!sheet.GetCollisionBox(box))
        {
            return;
        }
        float width = box[1][0] - box[0][0];
        float height = box[1][1] - box[0][1];
        if (width < 0.02 || height < 0.02)
        {
            return;
        }
        s_SheetHalfWidth = width * 0.5;
        s_SheetHalfHeight = height * 0.5;
        s_SheetSizeSource = "box";
    }

    // B5: option (b) of the T-013 study measured at pin time and not applied. Five points over the
    // sheet's footprint (center and four corners, in the plane of the view hit), each cast back
    // along the normal from 1.0 m out to 10 cm behind, against the view, fire, and physics layers
    // of the wall object; depth is signed along the normal from the view hit. standoff= is what
    // option (b) would add to the wall offset (NimbyTackRules.FootprintStandoff, 10 cm cap). The
    // rays started 30 cm out until October 21, 2026, which is why TK-P5 read 0.2999 on faces whose
    // layers stand 0.6 to 0.98 m out; 1.0 m lets B5 see the T-015 geometry. A pin on such a face
    // is refused since T17, so this now matters only for faces between 15 cm and 1.0 m.
    static void LogFootprint(PlayerBase player, NimbyTackHit hit, Object pinned, float usedOffset, float wallOffset)
    {
        if (!NimbyTackLog.IsDebug() || !hit || !hit.wall)
        {
            return;
        }
        ReadSheetSize(pinned);
        vector basis[4];
        Math3D.DirectionAndUpMatrix(NimbyTackRules.FaceDirection(hit.normal), vector.Up, basis);
        vector right = basis[0];
        vector up = basis[1];
        array<vector> points = new array<vector>;
        points.Insert(hit.position);
        points.Insert(hit.position - right * s_SheetHalfWidth + up * s_SheetHalfHeight);
        points.Insert(hit.position + right * s_SheetHalfWidth + up * s_SheetHalfHeight);
        points.Insert(hit.position - right * s_SheetHalfWidth - up * s_SheetHalfHeight);
        points.Insert(hit.position + right * s_SheetHalfWidth - up * s_SheetHalfHeight);
        float width = s_SheetHalfWidth * 2.0;
        float height = s_SheetHalfHeight * 2.0;
        string header = "footprint wall=" + ClassOf(hit.wall) + " surface=" + hit.surface + " offset=" + usedOffset.ToString() + " size=" + width.ToString() + "x" + height.ToString();
        header = header + " size-source=" + s_SheetSizeSource + " " + PlayerTag(player) + " " + PositionTag(hit.position);
        NimbyTackLog.Debug(header);
        LogFootprintLayer("view", ObjIntersectView, player, hit, points, wallOffset);
        LogFootprintLayer("fire", ObjIntersectFire, player, hit, points, wallOffset);
        LogFootprintLayer("physics", -1, player, hit, points, wallOffset);
        LogPostNormals(player, hit, right);
    }

    // NIMBY-T-025 study (debug only, Jeff, October 25, 2026): a note on a thin post can hang
    // turned against the fence, because its normal comes from the post's own collision shape.
    // Two more building-layer rays, this far either side of the hit along the sheet's width,
    // record the normal beside the post, its angle to the hit normal, and its depth, so the
    // record can tell whether a broad-face normal would hang the note flush. Moves nothing.
    static const float POST_PROBE_SIDE_METERS = 0.2;

    protected static void LogPostNormals(PlayerBase player, NimbyTackHit hit, vector right)
    {
        string line = "post-normals wall=" + ClassOf(hit.wall) + " hit=" + hit.normal.ToString();
        line = line + " left=" + SideNormalTag(player, hit, right, -POST_PROBE_SIDE_METERS);
        line = line + " right=" + SideNormalTag(player, hit, right, POST_PROBE_SIDE_METERS);
        line = line + " " + PositionTag(hit.position);
        NimbyTackLog.Debug(line);
    }

    // normal/angle=<deg>/depth=<m>/obj=<class>, or none when the ray finds no building.
    protected static string SideNormalTag(PlayerBase player, NimbyTackHit hit, vector right, float side)
    {
        vector start = hit.position + right * side + hit.normal * 1.0;
        vector end = hit.position + right * side - hit.normal * 0.3;
        Object hitObject;
        vector hitPosition;
        vector hitNormal;
        float hitFraction;
        if (!DayZPhysics.RayCastBullet(start, end, PhxInteractionLayers.BUILDING, player, hitObject, hitPosition, hitNormal, hitFraction))
        {
            return "none";
        }
        if (hitNormal.Length() < 0.5)
        {
            return "zero";
        }
        hitNormal.Normalize();
        // Facing the player, as WallNormal flips it (the ray runs along -hit.normal).
        if (vector.Dot(hitNormal, hit.normal) < 0.0)
        {
            hitNormal = hitNormal * -1;
        }
        float angle = NimbyTackRules.NormalAngleDegrees(hitNormal, hit.normal);
        float depth = NimbyTackRules.SignedDepth(hitPosition, hit.position, hit.normal);
        string tag = hitNormal.ToString() + "/angle=" + angle.ToString();
        tag = tag + "/depth=" + depth.ToString() + "/obj=" + ClassOf(hitObject);
        return tag;
    }

    // One B5 line: intersectType is ObjIntersectView or ObjIntersectFire, or -1 for the physics
    // shell (RayCastBullet on the building layer, as WallNormal).
    protected static void LogFootprintLayer(string layer, int intersectType, PlayerBase player, NimbyTackHit hit, array<vector> points, float wallOffset)
    {
        array<float> depths = new array<float>;
        foreach (vector point : points)
        {
            vector start = point + hit.normal * 1.0;
            vector end = point - hit.normal * 0.1;
            depths.Insert(SampleDepth(intersectType, start, end, player, hit));
        }
        float standoff = NimbyTackRules.FootprintStandoff(depths, 0.1);
        float wouldOffset = wallOffset + standoff;
        string line = "footprint layer=" + layer + " center=" + NimbyTackRules.DepthTag(depths[0]) + " tl=" + NimbyTackRules.DepthTag(depths[1]) + " tr=" + NimbyTackRules.DepthTag(depths[2]);
        line = line + " bl=" + NimbyTackRules.DepthTag(depths[3]) + " br=" + NimbyTackRules.DepthTag(depths[4]) + " standoff=" + standoff.ToString() + " would-offset=" + wouldOffset.ToString();
        NimbyTackLog.Debug(line);
    }

    // The depth of the first hit on the wall object along one footprint ray, or DEPTH_MISS.
    protected static float SampleDepth(int intersectType, vector start, vector end, PlayerBase player, NimbyTackHit hit)
    {
        if (intersectType < 0)
        {
            Object hitObject;
            vector hitPosition;
            vector hitNormal;
            float hitFraction;
            if (!DayZPhysics.RayCastBullet(start, end, PhxInteractionLayers.BUILDING, player, hitObject, hitPosition, hitNormal, hitFraction))
            {
                return NimbyTackRules.DEPTH_MISS;
            }
            if (hitObject != hit.wall)
            {
                return NimbyTackRules.DEPTH_MISS;
            }
            return NimbyTackRules.SignedDepth(hitPosition, hit.position, hit.normal);
        }
        RaycastRVParams params = new RaycastRVParams(start, end, player, 0.0);
        params.sorted = true;
        params.type = intersectType;
        // First contact per object: the note just created stands on the centre ray, and with the
        // default NEARESTCONTACT it hid the wall (centre sample missed on 49 of 54 pins, October
        // 25, 2026).
        params.flags = CollisionFlags.ALLOBJECTS;
        array<ref RaycastRVResult> results = new array<ref RaycastRVResult>;
        if (!DayZPhysics.RaycastRVProxy(params, results))
        {
            return NimbyTackRules.DEPTH_MISS;
        }
        foreach (RaycastRVResult result : results)
        {
            if (result.obj != hit.wall)
            {
                continue;
            }
            return NimbyTackRules.SignedDepth(result.pos, hit.position, hit.normal);
        }
        return NimbyTackRules.DEPTH_MISS;
    }

    // T3: create the pinned class where NimbyTackPlace.Compute put it (T19, T20), facing out along
    // its normal, with the sheet's text and health. No ECE_CREATEPHYSICS; ECE_KEEPHEIGHT so the
    // engine does not drop it to the ground (CentralEconomy.c:28).
    static NimbyTack_PinnedNote CreatePinned(int fastener, NimbyTackPlacement placement, Paper sheet)
    {
        string className = NimbyTackFastener.PinnedClass(fastener);
        if (className == "")
        {
            return null;
        }
        vector position = placement.position;
        Object created = GetGame().CreateObjectEx(className, position, ECE_KEEPHEIGHT | ECE_SETUP);
        NimbyTack_PinnedNote pinned = NimbyTack_PinnedNote.Cast(created);
        if (!pinned)
        {
            if (created)
            {
                GetGame().ObjectDelete(created);
            }
            return null;
        }
        // Orientation as a basis, not Euler angles: model +Z takes NimbyTackRules.FaceDirection
        // (the outward normal, toward the player) and model +Y takes world up. The reasoning
        // and the failed forms are recorded on FaceDirection.
        vector transform[4];
        Math3D.DirectionAndUpMatrix(NimbyTackRules.FaceDirection(placement.normal), vector.Up, transform);
        transform[3] = position;
        pinned.SetTransform(transform);
        pinned.NimbyTackFreeze();
        pinned.SetHealth(sheet.GetHealth());
        WrittenNoteData source = sheet.GetWrittenNoteData();
        WrittenNoteData destination = pinned.GetWrittenNoteData();
        if (source && destination)
        {
            destination.SetNoteText(source.GetNoteText());
        }
        return pinned;
    }
}
