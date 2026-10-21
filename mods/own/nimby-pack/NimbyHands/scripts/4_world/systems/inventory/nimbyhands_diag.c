// -----------------------------------------------------------------------------
// NimbyHandsDiag - the NIMBY-T-010 diagnostic. Branch diag/hands-t010 only; never merged.
// Purpose : The logging and the server-only variant switch of the feasibility study
//           docs/design/2026-10-20-hands-t010-feasibility.md, section 7 (Jeff's rulings
//           J1 and J5, October 20, 2026). Every line is a debug line, written only with
//           $profile:NimbyHands\debug, in one format:
//             [NIMBY][Hands][Debug] diag=<name> side=<server|client> t=<ms> id=<corr|-> ...
//           DG1 diag=predispatch  server, immediately before ServerTakeToDst: the engine's
//                                 move test (cmtd), the junctures, both players' commands.
//           DG2 diag=watch        server, the giver's hands and commands for 3000 ms after
//                                 dispatch, one line per change.
//           DG3 diag=juncture     server and clients, PlayerBase.OnSyncJuncture for
//                                 SJ_INVENTORY and SJ_INVENTORY_FAILURE (PlayerBase.c:8496-8500).
//           DG4 diag=watch        the owning client, started by its own SJ_INVENTORY.
//           Also diag=clearjuncture (V1), diag=interrupt and diag=wait (V2, V3), diag=step1
//           and diag=step2 (V3).
// Variant : $profile:NimbyHands\diag-variant on the server, read at every dispatch, so a
//           block change needs no restart. Absent, unreadable, or anything else is V0.
//             V0  today's dispatch.
//             V1  the action clears its own target juncture first, vanilla's deploy order
//                 (ActionDeployBase.c:79 before :225; ActionBase.c:1207).
//             V2  the dispatch waits until neither player runs an action and the giver's
//                 action commands and hand inventory are idle (cap 3000 ms). The consenting
//                 party's offer or request loop never ends on its own (CAContinuousTime(-1)),
//                 so the server interrupts it the way vanilla does (ActionManagerServer.c:371-383).
//                 Consent is the one read when the request was made; Put In Pockets re-reads
//                 the receiver's pockets after the wait. Everything else is revalidated again.
//             V3  V2 timing, then hands to a free location in the giver's own inventory; on
//                 that location change the giver's ServerTakeToDst moves it to the receiver,
//                 which with neither end in hands is the synchronous server move
//                 (HumanInventory.c:243-251, Inventory.c:1057-1066).
//           Nothing is retried, cloned, restored, or deleted in any variant.
// Persists: Nothing.
// -----------------------------------------------------------------------------
class NimbyHandsDiagWatch
{
    PlayerBase m_Player;
    PlayerBase m_Other;
    EntityAI m_Item;
    int m_Id;
    int m_UntilMs;
    string m_Last;

    // Writes a line when the snapshot changed. Returns false when the watch is over.
    bool Step(int nowMs)
    {
        string idText = NimbyHandsDiag.IdText(m_Id);
        if (!m_Player)
        {
            NimbyHandsDiag.Line("watch", idText, "end=player gone");
            return false;
        }
        string snapshot = NimbyHandsDiag.Snapshot(m_Player, m_Item, m_Other);
        if (snapshot != m_Last)
        {
            string prefix = "";
            if (m_Last == "")
            {
                prefix = "begin player=" + NimbyHandsServer.DescribePlayer(m_Player) + " item=" + NimbyHandsServer.DescribeItem(m_Item) + " ";
            }
            m_Last = snapshot;
            NimbyHandsDiag.Line("watch", idText, prefix + snapshot);
        }
        if (nowMs >= m_UntilMs)
        {
            NimbyHandsDiag.Line("watch", idText, "end=timeout");
            return false;
        }
        return true;
    }
}

class NimbyHandsDiag
{
    static const int V0 = 0;
    static const int V1 = 1;
    static const int V2 = 2;
    static const int V3 = 3;
    static const int WATCH_MS = 3000;
    static const int WAIT_CAP_MS = 3000;
    static const int TICK_MS = 50;

    protected static ref array<ref NimbyHandsDiagWatch> s_Watches;
    protected static ref array<ref NimbyHandsMoveItemToPlayer> s_Waiting;
    protected static bool s_Ticking;

    // ---- the variant switch ---------------------------------------------------

    static string VariantName(int variant)
    {
        return "V" + variant.ToString();
    }

    // Server only. Reads the first line of the file (JsonFileLoader.c:114-123 pattern).
    static int ReadVariant()
    {
        if (!GetGame().IsServer())
        {
            return V0;
        }
        string path = "$profile:NimbyHands\\diag-variant";
        if (!FileExist(path))
        {
            return V0;
        }
        FileHandle handle = OpenFile(path, FileMode.READ);
        if (handle == 0)
        {
            return V0;
        }
        string text = "";
        FGets(handle, text);
        CloseFile(handle);
        if (text.Contains("V3") || text.Contains("v3"))
        {
            return V3;
        }
        if (text.Contains("V2") || text.Contains("v2"))
        {
            return V2;
        }
        if (text.Contains("V1") || text.Contains("v1"))
        {
            return V1;
        }
        return V0;
    }

    // Called by the three dispatching actions right before move.Execute().
    static void BeforeExecute(ActionBase action, ActionData action_data, NimbyHandsMoveItemToPlayer move)
    {
        int variant = ReadVariant();
        move.SetDiagVariant(variant);
        if (variant != V1 || !action || !action_data)
        {
            return;
        }
        int locks = 0;
        if (action_data.m_ReservedInventoryLocations)
        {
            locks = action_data.m_ReservedInventoryLocations.Count();
        }
        action.ClearActionJuncture(action_data);
        Line("clearjuncture", "-", "action=" + action.ClassName() + " player=" + NimbyHandsServer.DescribePlayer(action_data.m_Player) + " cleared=" + locks.ToString());
    }

    // ---- the line format ------------------------------------------------------

    static string Side()
    {
        if (GetGame().IsServer())
        {
            return "server";
        }
        return "client";
    }

    static string IdText(int id)
    {
        if (id <= 0)
        {
            return "-";
        }
        return id.ToString();
    }

    static void Line(string name, string id, string fields)
    {
        if (!NimbyHandsLog.IsDebug())
        {
            return;
        }
        NimbyHandsLog.Debug("diag=" + name + " side=" + Side() + " t=" + GetGame().GetTime().ToString() + " id=" + id + " " + fields);
    }

    // ---- descriptions ---------------------------------------------------------

    static string DescribeAction(PlayerBase player)
    {
        if (!player || !player.GetActionManager())
        {
            return "none";
        }
        ActionBase action = player.GetActionManager().GetRunningAction();
        if (!action)
        {
            return "none";
        }
        return action.ClassName();
    }

    // The running action (ActionManagerBase.c:91), the full-body and modifier action
    // commands (human.c:1570, :1603), and the weapons modifier that plays the hide
    // animation (human.c:1589; IsActionFinished, GetRunningAction, GetRunningActionType).
    static string DescribeCommands(PlayerBase player)
    {
        if (!player)
        {
            return "none";
        }
        string line = "act=" + DescribeAction(player);
        line = line + " cmdAct=" + NimbyHandsLog.Bool(player.GetCommand_Action() != null);
        line = line + " cmdModAct=" + NimbyHandsLog.Bool(player.GetCommandModifier_Action() != null);
        HumanCommandWeapons hcw = player.GetCommandModifier_Weapons();
        if (hcw)
        {
            line = line + " hcw=" + hcw.GetRunningAction().ToString() + "/" + hcw.GetRunningActionType().ToString();
            line = line + " hcwFin=" + NimbyHandsLog.Bool(hcw.IsActionFinished());
        }
        else
        {
            line = line + " hcw=none";
        }
        return line;
    }

    // The hand state machine: the stable state (UNKNOWN while in a transition such as the
    // animated move, HandFSM.c:36-43), idle, and the inventory's busy test (DayZPlayerInventory.c:4639).
    static string DescribeHands(PlayerBase player)
    {
        if (!player)
        {
            return "hands=none";
        }
        DayZPlayerInventory inventory = DayZPlayerInventory.Cast(player.GetInventory());
        if (!inventory)
        {
            return "hands=no-inventory";
        }
        string line = "state=" + typename.EnumToString(HandStateID, inventory.GetCurrentStateID());
        line = line + " idle=" + NimbyHandsLog.Bool(inventory.IsIdle());
        line = line + " proc=" + NimbyHandsLog.Bool(inventory.IsProcessing());
        return line;
    }

    static string DescribeItemLocation(EntityAI item)
    {
        if (!item)
        {
            return "none";
        }
        InventoryLocation loc = new InventoryLocation;
        if (!item.GetInventory().GetCurrentInventoryLocation(loc))
        {
            return "unknown";
        }
        return NimbyHandsServer.DescribeLocation(loc);
    }

    // Server only: the juncture natives (Game.c:757-759).
    static string DescribeJunctures(PlayerBase giver, PlayerBase receiver, EntityAI item, InventoryLocation dst)
    {
        if (!GetGame().IsServer())
        {
            return "j=client";
        }
        string line = "jItem=none jGiverItem=none";
        if (item)
        {
            line = "jItem=" + NimbyHandsLog.Bool(GetGame().HasInventoryJunctureItem(item));
            if (giver)
            {
                line = line + " jGiverItem=" + NimbyHandsLog.Bool(GetGame().HasInventoryJuncture(giver, item));
            }
            else
            {
                line = line + " jGiverItem=none";
            }
        }
        if (giver)
        {
            line = line + " jGiver=" + NimbyHandsLog.Bool(GetGame().HasInventoryJunctureItem(giver));
        }
        if (receiver)
        {
            line = line + " jReceiver=" + NimbyHandsLog.Bool(GetGame().HasInventoryJunctureItem(receiver));
        }
        if (giver && dst)
        {
            line = line + " jDst=" + NimbyHandsLog.Bool(GetGame().HasInventoryJunctureDestination(giver, dst));
        }
        return line;
    }

    static string Snapshot(PlayerBase player, EntityAI item, PlayerBase other)
    {
        string line = "loc=" + DescribeItemLocation(item) + " " + DescribeHands(player) + " " + DescribeCommands(player);
        if (GetGame().IsServer() && item)
        {
            line = line + " jItem=" + NimbyHandsLog.Bool(GetGame().HasInventoryJunctureItem(item));
            line = line + " jGiverItem=" + NimbyHandsLog.Bool(GetGame().HasInventoryJuncture(player, item));
        }
        if (other)
        {
            line = line + " other.act=" + DescribeAction(other);
            line = line + " other.cmdModAct=" + NimbyHandsLog.Bool(other.GetCommandModifier_Action() != null);
        }
        return line;
    }

    // ---- DG1: immediately before the dispatch ----------------------------------

    // cmtd is the engine's anti-cheat test the hand event runs at execution
    // (Hand_Events.c:298-304, Inventory.c:819), asked here for the dispatch destination and,
    // in V3, for the receiver's destination too (cmtdReceiver): false is H-C.
    static void PreDispatch(NimbyHandsWorldTransfer transfer, InventoryLocation src, InventoryLocation dispatchDst, InventoryLocation receiverDst)
    {
        if (!NimbyHandsLog.IsDebug() || !transfer)
        {
            return;
        }
        PlayerBase giver = PlayerBase.Cast(transfer.giver);
        PlayerBase receiver = transfer.receiver;
        string line = "variant=" + VariantName(transfer.diagVariant) + " stage=" + transfer.diagStage.ToString();
        line = line + " src=" + NimbyHandsServer.DescribeLocation(src) + " dst=" + NimbyHandsServer.DescribeLocation(dispatchDst);
        if (giver)
        {
            line = line + " cmtd=" + NimbyHandsLog.Bool(GameInventory.CheckMoveToDstRequest(giver, src, dispatchDst, GameInventory.c_MaxItemDistanceRadius));
            if (receiverDst && receiverDst != dispatchDst)
            {
                line = line + " receiverDst=" + NimbyHandsServer.DescribeLocation(receiverDst);
                line = line + " cmtdReceiver=" + NimbyHandsLog.Bool(GameInventory.CheckMoveToDstRequest(giver, src, receiverDst, GameInventory.c_MaxItemDistanceRadius));
            }
        }
        line = line + " dstItem=" + NimbyHandsLog.Bool(dispatchDst.GetItem() == transfer.item);
        if (giver && receiver)
        {
            line = line + " dist=" + vector.Distance(giver.GetPosition(), receiver.GetPosition()).ToString();
        }
        line = line + " " + DescribeJunctures(giver, receiver, transfer.item, dispatchDst);
        line = line + " giver{" + DescribeHands(giver) + " " + DescribeCommands(giver) + "}";
        line = line + " receiver{" + DescribeCommands(receiver) + "}";
        Line("predispatch", IdText(transfer.id), line);
    }

    // ---- DG2 and DG4: the watch -------------------------------------------------

    static void StartWatch(PlayerBase player, PlayerBase other, EntityAI item, int id)
    {
        if (!NimbyHandsLog.IsDebug() || !player)
        {
            return;
        }
        if (!s_Watches)
        {
            s_Watches = new array<ref NimbyHandsDiagWatch>;
        }
        int nowMs = GetGame().GetTime();
        // A second juncture for the same player and request extends the running watch.
        foreach (NimbyHandsDiagWatch running : s_Watches)
        {
            if (running && running.m_Player == player && running.m_Id == id && running.m_Item == item)
            {
                running.m_UntilMs = nowMs + WATCH_MS;
                return;
            }
        }
        NimbyHandsDiagWatch watch = new NimbyHandsDiagWatch();
        watch.m_Player = player;
        watch.m_Other = other;
        watch.m_Item = item;
        watch.m_Id = id;
        watch.m_UntilMs = nowMs + WATCH_MS;
        watch.m_Last = "";
        s_Watches.Insert(watch);
        watch.Step(nowMs);
        EnsureTick();
    }

    // One repeating call for every watch and wait, started once and never removed: it does
    // nothing while both lists are empty.
    protected static void EnsureTick()
    {
        if (s_Ticking)
        {
            return;
        }
        s_Ticking = true;
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyHandsDiag.Tick, TICK_MS, true);
    }

    static void Tick()
    {
        int nowMs = GetGame().GetTime();
        if (s_Waiting)
        {
            for (int i = s_Waiting.Count() - 1; i >= 0; i--)
            {
                NimbyHandsMoveItemToPlayer move = s_Waiting.Get(i);
                if (!move || move.DiagPoll(nowMs))
                {
                    s_Waiting.RemoveOrdered(i);
                }
            }
        }
        if (s_Watches)
        {
            for (int j = s_Watches.Count() - 1; j >= 0; j--)
            {
                NimbyHandsDiagWatch watch = s_Watches.Get(j);
                if (!watch || !watch.Step(nowMs))
                {
                    s_Watches.RemoveOrdered(j);
                }
            }
        }
    }

    // ---- DG3: the inventory junctures --------------------------------------------

    static void OnJuncture(PlayerBase player, int junctureId)
    {
        if (!NimbyHandsLog.IsDebug() || !player)
        {
            return;
        }
        string name = "";
        if (junctureId == DayZPlayerSyncJunctures.SJ_INVENTORY)
        {
            name = "SJ_INVENTORY";
        }
        else if (junctureId == DayZPlayerSyncJunctures.SJ_INVENTORY_FAILURE)
        {
            name = "SJ_INVENTORY_FAILURE";
        }
        else
        {
            return;
        }
        EntityAI item = player.GetItemInHands();
        int id = 0;
        if (GetGame().IsServer())
        {
            id = NimbyHandsServer.FindIdByItem(item);
        }
        string line = "player=" + NimbyHandsServer.DescribePlayer(player) + " juncture=" + name;
        line = line + " controlled=" + NimbyHandsLog.Bool(player.IsControlledPlayer());
        line = line + " hands=" + NimbyHandsServer.DescribeItem(item) + " " + DescribeHands(player);
        Line("juncture", IdText(id), line);
        if (junctureId == DayZPlayerSyncJunctures.SJ_INVENTORY && !GetGame().IsServer() && player.IsControlledPlayer())
        {
            StartWatch(player, null, item, 0);
        }
    }

    // ---- V2 and V3: the wait ------------------------------------------------------

    static void Defer(NimbyHandsMoveItemToPlayer move, NimbyHandsWorldTransfer transfer)
    {
        if (!s_Waiting)
        {
            s_Waiting = new array<ref NimbyHandsMoveItemToPlayer>;
        }
        s_Waiting.Insert(move);
        PlayerBase giver = PlayerBase.Cast(transfer.giver);
        string line = "result=start variant=" + VariantName(transfer.diagVariant);
        line = line + " giver{" + DescribeHands(giver) + " " + DescribeCommands(giver) + "}";
        line = line + " receiver{" + DescribeCommands(transfer.receiver) + "}";
        Line("wait", IdText(transfer.id), line);
        EndConsentLoop(giver, transfer.id);
        EndConsentLoop(transfer.receiver, transfer.id);
        EnsureTick();
    }

    protected static void EndConsentLoop(PlayerBase player, int id)
    {
        if (!player || !player.GetActionManager())
        {
            return;
        }
        ActionBase action = player.GetActionManager().GetRunningAction();
        if (!action)
        {
            return;
        }
        if (!NimbyHandsActionOfferItem.Cast(action) && !NimbyHandsActionRequestItem.Cast(action))
        {
            return;
        }
        player.GetActionManager().RequestInterruptAction();
        Line("interrupt", IdText(id), "player=" + NimbyHandsServer.DescribePlayer(player) + " action=" + action.ClassName());
    }

    // No running action; for the holder also no action command and an idle hand inventory.
    static bool IsFree(PlayerBase player, bool holder)
    {
        if (!player)
        {
            // Revalidation refuses a missing player.
            return true;
        }
        if (player.GetActionManager() && player.GetActionManager().GetRunningAction())
        {
            return false;
        }
        if (!holder)
        {
            return true;
        }
        if (player.GetCommand_Action() != null || player.GetCommandModifier_Action() != null)
        {
            return false;
        }
        DayZPlayerInventory inventory = DayZPlayerInventory.Cast(player.GetInventory());
        if (inventory && inventory.IsProcessing())
        {
            return false;
        }
        return true;
    }

    // ---- V3: step 2 ----------------------------------------------------------------

    // Scheduled from the item's arrival in the giver's own inventory, so the server move
    // never runs inside that location-change event.
    static void RunStep2(int id)
    {
        NimbyHandsWorldTransfer transfer = NimbyHandsServer.FindTransfer(id);
        if (!transfer || transfer.state != NimbyHandsState.REQUESTED || transfer.diagStage != 2)
        {
            Line("step2", IdText(id), "result=skipped (request no longer open)");
            return;
        }
        NimbyHandsMoveItemToPlayer.DiagStep2(transfer);
    }
}
