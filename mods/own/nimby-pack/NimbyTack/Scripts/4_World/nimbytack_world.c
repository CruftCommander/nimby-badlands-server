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
// -----------------------------------------------------------------------------
class NimbyTackHit
{
    vector position;
    vector normal;
    string surface;
    Object wall;
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
            return false;
        }
        vector head = MiscGameplayFunctions.GetPlayerHeadPosition(player);
        vector cursor = target.GetCursorHitPos();
        if (vector.Distance(head, cursor) > config.reachMeters)
        {
            s_LastProbeReason = "out-of-reach";
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
            NimbyTackLog.Debug("probe: no hit from head to cursor");
            return false;
        }
        foreach (RaycastRVResult result : results)
        {
            if (!result.obj)
            {
                continue;
            }
            if (result.obj == player || result.obj.IsMan() || result.obj.IsInherited(ItemBase))
            {
                continue;
            }
            vector normal;
            if (!WallNormal(head, end, direction, player, normal))
            {
                s_LastProbeReason = "no-physics-normal";
                return false;
            }
            // The angle test comes before the surface lookup: a floor or a ceiling never pays
            // for the second ray.
            if (!NimbyTackRules.IsWallNormal(normal, config.wallAngleDegrees))
            {
                s_LastProbeReason = "not-wall-angle";
                NimbyTackLog.Debug("probe: refused hit=" + result.obj.GetType() + " normal=" + normal.ToString() + " " + PositionTag(result.pos));
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
                    surfaceName = FireSurfaceName(head, end, player, result.obj, result.pos);
                }
            }
            NimbyTackLog.Debug("probe: hit=" + result.obj.GetType() + " surface=" + surfaceName + " normal=" + normal.ToString() + " " + PositionTag(result.pos));
            hit = new NimbyTackHit();
            hit.position = result.pos;
            hit.normal = normal;
            hit.surface = surfaceName;
            hit.wall = result.obj;
            return true;
        }
        s_LastProbeReason = "no-building-hit";
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

    // T6: pinned notes already inside the radius of a position.
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
    static bool WallNormal(vector head, vector end, vector direction, PlayerBase player, out vector normal)
    {
        Object hitObject;
        vector hitPosition;
        vector hitNormal;
        float hitFraction;
        if (!DayZPhysics.RayCastBullet(head, end, PhxInteractionLayers.BUILDING, player, hitObject, hitPosition, hitNormal, hitFraction))
        {
            NimbyTackLog.Debug("probe: physics ray had no building hit");
            return false;
        }
        if (hitNormal.Length() < 0.5)
        {
            NimbyTackLog.Debug("probe: physics ray returned a zero normal");
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
            hitName = hitObject.GetType();
        }
        NimbyTackLog.Debug("probe: physics hit=" + hitName + " normal=" + hitNormal.ToString() + " fraction=" + hitFraction.ToString() + " " + PositionTag(hitPosition));
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
    // object. Debug-logs what it found, with the fire hit's position and normal next to the
    // view-geometry hit (viewHit) and the gap between them: the pin sits on the view hit, and
    // on Land_House_1M3_Dam2 that point is inside the rendered mud-brick relief at 3 cm
    // (October 17, 2026). The two planes decide whether the fire hit should place the pin.
    static string FireSurfaceName(vector head, vector end, PlayerBase player, Object wall, vector viewHit)
    {
        RaycastRVParams params = new RaycastRVParams(head, end, player, 0.0);
        params.sorted = true;
        params.type = ObjIntersectFire;
        array<ref RaycastRVResult> results = new array<ref RaycastRVResult>;
        if (!DayZPhysics.RaycastRVProxy(params, results))
        {
            NimbyTackLog.Debug("probe: fire-geometry ray had no hit");
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
            vector fireNormal = result.dir;
            fireNormal.Normalize();
            float gap = vector.Distance(result.pos, viewHit);
            NimbyTackLog.Debug("probe: fire-geometry surface=" + name + " component=" + result.component.ToString() + " normal=" + fireNormal.ToString() + " " + PositionTag(result.pos) + " view-" + PositionTag(viewHit) + " gap=" + gap.ToString());
            return name;
        }
        NimbyTackLog.Debug("probe: fire-geometry ray did not hit the wall object");
        return "";
    }

    // T3: create the pinned class at the hit point, offsetMeters off the wall along the normal,
    // facing out, with the sheet's text and health. The caller picks the offset with
    // NimbyTackRules.OffsetFor (wall or unknown surface) and logs the same value. No
    // ECE_CREATEPHYSICS; ECE_KEEPHEIGHT so the engine does not drop it to the ground
    // (CentralEconomy.c:28).
    static NimbyTack_PinnedNote CreatePinned(int fastener, NimbyTackHit hit, Paper sheet, float offsetMeters)
    {
        string className = NimbyTackFastener.PinnedClass(fastener);
        if (className == "")
        {
            return null;
        }
        vector position = NimbyTackRules.PinPosition(hit.position, hit.normal, offsetMeters);
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
        Math3D.DirectionAndUpMatrix(NimbyTackRules.FaceDirection(hit.normal), vector.Up, transform);
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
