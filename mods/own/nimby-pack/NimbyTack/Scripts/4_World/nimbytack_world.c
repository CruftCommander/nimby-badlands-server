// -----------------------------------------------------------------------------
// NimbyTack - world queries shared by the actions.
// Purpose : The engine-facing half of Tack: the wall probe (raycast from the head
//           to the cursor hit, normal and surface read from RaycastRVResult,
//           DayZPhysics.c:208), the inventory searches, the per-area count, the
//           pinned-note creation, and the log tags. Every decision these feed is
//           made in NimbyTackRules.
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

    // T2: the wall under the cursor. The action target gives the object and the cursor hit
    // position but no normal (ActionTargets.c:115), so one raycast from the head to just past
    // the hit point recovers the normal and the surface material. Players, items, and the
    // probing player are never walls.
    static bool ProbeWall(PlayerBase player, ActionTarget target, NimbyTackConfig config, out NimbyTackHit hit)
    {
        if (!player || !target)
        {
            return false;
        }
        Object wall = target.GetObject();
        if (!wall)
        {
            wall = target.GetParent();
        }
        if (!wall)
        {
            return false;
        }
        if (wall.IsMan() || wall.IsInherited(ItemBase))
        {
            return false;
        }
        vector head = MiscGameplayFunctions.GetPlayerHeadPosition(player);
        vector cursor = target.GetCursorHitPos();
        if (vector.Distance(head, cursor) > config.reachMeters)
        {
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
            vector normal = result.dir;
            normal.Normalize();
            string surfaceName = "";
            if (result.surface)
            {
                surfaceName = result.surface.GetName();
            }
            NimbyTackLog.Debug("probe: hit=" + result.obj.GetType() + " surface=" + surfaceName + " normal=" + normal.ToString() + " " + PositionTag(result.pos));
            if (!NimbyTackRules.IsWallNormal(normal, config.wallAngleDegrees))
            {
                return false;
            }
            hit = new NimbyTackHit();
            hit.position = result.pos;
            hit.normal = normal;
            hit.surface = surfaceName;
            hit.wall = result.obj;
            return true;
        }
        return false;
    }

    // T2: one sheet of paper anywhere in the player's inventory, hands included. A pinned
    // note is never a candidate (it is not in an inventory anyway).
    static Paper FindSingleSheet(PlayerBase player)
    {
        if (!player)
        {
            return null;
        }
        array<EntityAI> items = new array<EntityAI>;
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
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
            if (NimbyTackRules.IsSingleSheet(sheet.HasQuantity(), sheet.GetQuantity()))
            {
                return sheet;
            }
        }
        return null;
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

    // T3: create the pinned class at the hit point, 1 cm off the wall, facing the normal, with
    // the sheet's text and health. No ECE_CREATEPHYSICS; ECE_KEEPHEIGHT so the engine does not
    // drop it to the ground (CentralEconomy.c:28).
    static NimbyTack_PinnedNote CreatePinned(int fastener, NimbyTackHit hit, Paper sheet)
    {
        string className = NimbyTackFastener.PinnedClass(fastener);
        if (className == "")
        {
            return null;
        }
        vector position = NimbyTackRules.PinPosition(hit.position, hit.normal);
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
        pinned.SetPosition(position);
        pinned.SetOrientation(NimbyTackRules.OrientationFor(hit.normal));
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
