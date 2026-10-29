// -----------------------------------------------------------------------------
// NimbyDevHarness - forged requests against the real Legs and Hands handlers.
// Purpose : The only place forged-request tests live (shared rules 8). Boot cases
//           run with no client at mission start; online cases run against a
//           connected player's identity. Every case reads the world after the
//           handler ran and prints PASS or FAIL with what it saw.
// Timing  : The boot run spawns its dummies and items first and runs the cases one
//           second later: a spawned entity finishes its init on a deferred call
//           (EntityAI.c:240), and a native inventory search that touched an item
//           created in the same tick crashed the server (build-20261021-142821).
//           The last boot case runs production Hands step 2 between the dummies on a
//           forged STAGED entry (NIMBY-T-063 bot case; a dummy never runs a hand event,
//           so step 1 cannot run here); the world is deleted BOT_SETTLE_MS later.
//           Since the step 1 wait (NIMBY-T-063), Execute registers and defers; every
//           case that expects a step 1 refusal calls NimbyHandsServer.Tick() after
//           Execute and reads the verdict from the request it holds.
// Vanilla : CreateObjectEx (Global/Game.c:703), ObjectDelete (:705), GetPlayers
//           (:945), ScriptReadWriteContext (gameplay.c:134-141),
//           ScriptCallQueue.CallLater (2_GameLib/DayZ/tools.c:61),
//           HumanInventory.CreateInHands (HumanInventory.c:44),
//           GameInventory.CreateAttachment (Inventory.c:209), CreateInInventory
//           (Inventory.c:876), PlayerBase.SetRestrained (PlayerBase.c:2206),
//           ECE_PLACE_ON_SURFACE (CentralEconomy.c:38), FGets (EnSystem.c:501).
// Persists: Nothing. Every dummy and item it spawns is deleted before it reports.
// -----------------------------------------------------------------------------
class NimbyDevHarnessCase
{
    string name;
    bool pass;
    string detail;
}

class NimbyDevHarness
{
    static const string PREFIX = "[NIMBY][DevHarness]";
    static const string SELFTEST_FLAG = "$profile:NimbyDevHarness\\selftest";
    static const string FORGE_FILE = "$profile:NimbyDevHarness\\forge";
    static const int BOOT_DELAY_MS = 1500;
    static const int SETTLE_MS = 1000;
    static const int ONLINE_DELAY_MS = 10000;
    static const int FLOOD_COUNT = 50;
    // After the bot step 2, one second for SendServerMove's aftermath and the deferred toast
    // before the world is deleted and the report is written.
    static const int BOT_SETTLE_MS = 1000;
    // Dummy B stands this far from dummy A: inside UAMaxDistances.DEFAULT (2.0 m) plus the
    // tolerance with room for the surface placement, and outside their capsules.
    static const float DUMMY_SPACING_M = 1.5;

    protected static ref array<string> s_Forge;
    protected static bool s_ForgeRead;
    protected static ref array<ref NimbyDevHarnessCase> s_Results;

    // The boot run's world objects, spawned in phase 1 and used in phase 2.
    protected static PlayerBase s_DummyA;
    protected static PlayerBase s_DummyB;
    protected static EntityAI s_Wrench;
    protected static EntityAI s_Pliers;
    protected static int s_RegistryBefore;
    // The bot step 2's request: the registry evicts a terminal entry, so the harness holds it.
    protected static ref NimbyHandsWorldTransfer s_BotTransfer;

    // ---- the forge file ---------------------------------------------------------

    static void ReadForge()
    {
        s_ForgeRead = true;
        s_Forge = new array<string>;
        if (!FileExist(FORGE_FILE))
        {
            return;
        }
        FileHandle handle = OpenFile(FORGE_FILE, FileMode.READ);
        if (handle == 0)
        {
            return;
        }
        string line = "";
        while (FGets(handle, line) >= 0)
        {
            line = line.Trim();
            if (line != "")
            {
                s_Forge.Insert(line);
            }
            line = "";
        }
        CloseFile(handle);
    }

    static bool ForgeHas(string word)
    {
        if (!s_ForgeRead)
        {
            ReadForge();
        }
        foreach (string line : s_Forge)
        {
            if (line.Contains(word))
            {
                return true;
            }
        }
        return false;
    }

    // ---- entry points --------------------------------------------------------------

    // Mission start on the server (MissionBaseWorld.OnInit). Arms the cost path B switch
    // when the forge file asks for it, and schedules the boot cases when the flag exists.
    static void OnMissionStart()
    {
        if (!GetGame().IsServer())
        {
            return;
        }
        ReadForge();
        Print(PREFIX + " loaded; selftest=" + Bool(FileExist(SELFTEST_FLAG)) + " forge=" + s_Forge.Count().ToString() + " lines");
        if (ForgeHas("step2-refuse"))
        {
            NimbyHandsServer.ArmStep2Refusal(1);
            Print(PREFIX + " armed one forced Hands step-2 refusal (forge: step2-refuse)");
        }
        if (FileExist(SELFTEST_FLAG))
        {
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyDevHarness.SpawnBootWorld, BOOT_DELAY_MS, false);
        }
    }

    // A player connected (MissionServer.InvokeOnConnect). The online cases need the player
    // in the world, so they wait.
    static void OnPlayerConnected(PlayerBase player)
    {
        if (!GetGame().IsServer() || !player || !ForgeHas("online"))
        {
            return;
        }
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyDevHarness.RunOnlineCases, ONLINE_DELAY_MS, false, player);
    }

    // ---- reporting ----------------------------------------------------------------------

    protected static void Begin()
    {
        s_Results = new array<ref NimbyDevHarnessCase>;
    }

    protected static void Record(string name, bool pass, string detail)
    {
        NimbyDevHarnessCase result = new NimbyDevHarnessCase();
        result.name = name;
        result.pass = pass;
        result.detail = detail;
        s_Results.Insert(result);
    }

    // tag is "SelfTest" for the boot cases (the boot check's contract) or "Forge" for the
    // online cases, and word is "selftest" or "forge" for the completion line.
    protected static void Report(string tag, string word)
    {
        int failures = 0;
        foreach (NimbyDevHarnessCase result : s_Results)
        {
            string verdict = "PASS";
            if (!result.pass)
            {
                verdict = "FAIL";
                failures = failures + 1;
            }
            Print(PREFIX + "[" + tag + "] " + verdict + " " + result.name + " detail=" + result.detail);
        }
        Print(PREFIX + " " + word + " complete; cases=" + s_Results.Count().ToString() + " failures=" + failures.ToString());
    }

    static string Bool(bool value)
    {
        if (value)
        {
            return "true";
        }
        return "false";
    }

    // ---- world helpers ------------------------------------------------------------------

    // Dry land, on the ground: the first of the mission's own player spawn points
    // (cfgplayerspawnpoints.xml, dayzOffline.nasdara) that is neither sea nor pond, at its
    // road-surface height (SurfaceRoadY, Game.c:1177; SurfaceIsSea :1196, SurfaceIsPond :1197).
    // "1024 0 1024" was sea on Nasdara: the dummies swam, and the bot transfer was refused
    // "giver swimming" (build-20261029-114223). The last candidate is used when every check
    // reads wet, so the run still reports instead of stopping.
    static vector BootPosition()
    {
        vector first = "14069.8 0 13083.1";
        vector second = "13571.8 0 12821.1";
        vector third = "13282.3 0 12401.0";
        vector fourth = "11440.9 0 12169.1";
        array<vector> candidates = new array<vector>;
        candidates.Insert(first);
        candidates.Insert(second);
        candidates.Insert(third);
        candidates.Insert(fourth);
        vector chosen = candidates.Get(candidates.Count() - 1);
        foreach (vector candidate : candidates)
        {
            if (!GetGame().SurfaceIsSea(candidate[0], candidate[2]) && !GetGame().SurfaceIsPond(candidate[0], candidate[2]))
            {
                chosen = candidate;
                break;
            }
        }
        chosen[1] = GetGame().SurfaceRoadY(chosen[0], chosen[2]);
        return chosen;
    }

    // A survivor with a shirt, trousers, and a backpack, so it carries cargo like a real
    // character and has room for the wrench the bot transfer stages and delivers.
    static PlayerBase SpawnDummy(vector position)
    {
        Object spawned = GetGame().CreateObjectEx("SurvivorM_Mirek", position, ECE_PLACE_ON_SURFACE);
        PlayerBase dummy = PlayerBase.Cast(spawned);
        if (!dummy)
        {
            return null;
        }
        dummy.GetInventory().CreateAttachment("TShirt_Black");
        dummy.GetInventory().CreateAttachment("Jeans_Black");
        dummy.GetInventory().CreateAttachment("TaloonBag_Blue");
        return dummy;
    }

    static void Delete(Object object)
    {
        if (object)
        {
            GetGame().ObjectDelete(object);
        }
    }

    static string Where(EntityAI item)
    {
        return NimbyHandsServer.DescribeItemLocation(item);
    }

    static bool ItemInHandsOf(EntityAI item, PlayerBase player)
    {
        if (!item || !player)
        {
            return false;
        }
        return player.GetItemInHands() == item;
    }

    static bool ItemWith(EntityAI item, PlayerBase player)
    {
        if (!item || !player)
        {
            return false;
        }
        return item.GetHierarchyRootPlayer() == player;
    }

    // "state=<STATE> detail=<text>" for a request a case holds, or "entry=none".
    static string DescribeEntry(NimbyHandsWorldTransfer transfer)
    {
        if (!transfer)
        {
            return "entry=none";
        }
        return "state=" + NimbyHandsState.Name(transfer.state) + " detail=" + transfer.detail;
    }

    // A request Execute registered and the wait tick refused with the expected detail.
    static bool RefusedWith(NimbyHandsWorldTransfer transfer, string expected)
    {
        if (!transfer)
        {
            return false;
        }
        return transfer.state == NimbyHandsState.REFUSED && NimbyHandsRules.StartsWith(transfer.detail, expected);
    }

    // A forged registry entry in the given state, as the registry would hold it after a real
    // request; the world layer never sees a dispatch for it.
    static NimbyHandsWorldTransfer ForgeEntry(PlayerBase giver, PlayerBase receiver, EntityAI item, string action, int state)
    {
        NimbyHandsWorldTransfer forged = new NimbyHandsWorldTransfer();
        forged.giver = giver;
        forged.receiver = receiver;
        forged.requester = receiver;
        forged.item = item;
        forged.reach = UAMaxDistances.DEFAULT;
        forged.destinationFlags = NimbyHandsMoveItemToPlayer.DestinationFlags();
        forged.consent = true;
        forged.consentDetail = "forged by dev harness";
        forged.hasExpected = false;
        NimbyHandsServer.Describe(forged);
        NimbyHandsTransfers registry = NimbyHandsServer.Registry();
        if (!registry.Begin(forged, NimbyHandsServer.ItemKey(item), action, GetGame().GetTime()))
        {
            return null;
        }
        if (state == NimbyHandsState.PENDING)
        {
            return forged;
        }
        registry.Move(forged, NimbyHandsState.REQUESTED, "forged by dev harness");
        if (state == NimbyHandsState.STAGED)
        {
            InventoryLocation loc = new InventoryLocation;
            if (item && item.GetInventory().GetCurrentInventoryLocation(loc))
            {
                NimbyHandsServer.RecordStaged(forged, loc);
            }
            registry.Move(forged, NimbyHandsState.STAGED, "forged by dev harness");
        }
        return forged;
    }

    // Evicts a forged entry that is still open, so the next case starts clean.
    static void Cleanup(NimbyHandsWorldTransfer forged)
    {
        if (forged && !NimbyHandsState.IsTerminal(forged.state))
        {
            NimbyHandsServer.Advance(forged, NimbyHandsState.REFUSED, "dev harness cleanup");
        }
    }

    // A Legs toggle request as a client would send it (Param3<bool, int, int>), written field
    // by field, delivered straight to the target's OnRPC with the given sender.
    static void SendLegsRequest(PlayerBase target, PlayerIdentity sender, bool running, int movementIdx, int angle)
    {
        ScriptReadWriteContext ctx = new ScriptReadWriteContext;
        ctx.GetWriteContext().Write(running);
        ctx.GetWriteContext().Write(movementIdx);
        ctx.GetWriteContext().Write(angle);
        target.OnRPC(sender, NimbyAutorunRpc.TOGGLE, ctx.GetReadContext());
    }

    static void SendLegsJunk(PlayerBase target, PlayerIdentity sender)
    {
        ScriptReadWriteContext ctx = new ScriptReadWriteContext;
        ctx.GetWriteContext().Write("junk");
        target.OnRPC(sender, NimbyAutorunRpc.TOGGLE, ctx.GetReadContext());
    }

    // ---- boot cases ---------------------------------------------------------------------

    // Phase 1: spawn two dressed dummies, a wrench in A's hands, and pliers in A's cargo,
    // then let them settle for a second before any handler touches them.
    static void SpawnBootWorld()
    {
        Begin();
        s_RegistryBefore = NimbyHandsServer.Registry().Count();
        vector position = BootPosition();
        s_DummyA = SpawnDummy(position);
        s_DummyB = SpawnDummy(position + Vector(DUMMY_SPACING_M, 0, 0));
        if (!s_DummyA || !s_DummyB)
        {
            Record("harness-spawn-dummies", false, "CreateObjectEx returned no PlayerBase for SurvivorM_Mirek");
            FinishBoot();
            return;
        }
        Record("harness-spawn-dummies", true, "two dressed survivor dummies at " + position.ToString() + " sea=" + Bool(GetGame().SurfaceIsSea(position[0], position[2])));
        s_Wrench = s_DummyA.GetHumanInventory().CreateInHands("Wrench");
        s_Pliers = s_DummyA.GetInventory().CreateInInventory("Pliers");
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyDevHarness.RunBootCases, SETTLE_MS, false);
    }

    // Phase 2: the cases.
    static void RunBootCases()
    {
        NimbyHandsTransfers registry = NimbyHandsServer.Registry();
        int before = s_RegistryBefore;
        PlayerBase dummyA = s_DummyA;
        PlayerBase dummyB = s_DummyB;
        EntityAI wrench = s_Wrench;
        EntityAI pliers = s_Pliers;
        if (!dummyA || !dummyB)
        {
            Record("harness-dummies-settled", false, "a dummy vanished during the settle second");
            FinishBoot();
            return;
        }

        // Legs: a request aimed at another player's PlayerBase from a sender that does not own
        // it (no client is connected, so the sender is null; the branch is the same).
        SendLegsRequest(dummyA, null, true, DayZPlayerConstants.MOVEMENTIDX_RUN, 1);
        Record("legs-request-aimed-at-another-player", !dummyA.NimbyIsAutorunning(), "target autorunning=" + Bool(dummyA.NimbyIsAutorunning()) + " after a forged toggle from a sender that does not own it");

        bool probe = ProbeMixedExpression(dummyA, 4);
        Record("harness-expression-probe", !probe, "present != null && 4 == 2 reads " + Bool(probe) + " (C reads false)");
        NimbyHandsWorldTransfer probeEntry = new NimbyHandsWorldTransfer();
        probeEntry.state = NimbyHandsState.UNCONFIRMED;
        bool memberProbe = ProbeMemberExpression(probeEntry, wrench, dummyB);
        Record("harness-expression-probe-member", !memberProbe, "t != null && t.state == CONFIRMED && item != null && b != null && ItemWith(item, b) && !ItemInHandsOf(item, b) reads " + Bool(memberProbe) + " with state=UNCONFIRMED and the wrench in A's hands (C reads false)");

        if (!wrench)
        {
            Record("harness-item-in-hands", false, "CreateInHands(Wrench) returned null");
        }
        else
        {
            Record("harness-item-in-hands", ItemInHandsOf(wrench, dummyA), "wrench at " + Where(wrench));

            // Execute registers and waits (NIMBY-T-063); the refusal lands on the wait tick.
            NimbyHandsMoveItemToPlayer noConsent = new NimbyHandsMoveItemToPlayer(dummyB, dummyB, wrench, "NimbyHandsActionTakeItem", UAMaxDistances.DEFAULT);
            noConsent.SetConsent(false, "giver is no longer offering");
            bool submitted = noConsent.Execute();
            NimbyHandsServer.Tick();
            NimbyHandsWorldTransfer noConsentEntry = noConsent.Transfer();
            bool stayed = ItemInHandsOf(wrench, dummyA) && registry.Count() == before;
            Record("hands-take-without-consent", submitted && RefusedWith(noConsentEntry, "no consent") && stayed, "registered=" + Bool(submitted) + " " + DescribeEntry(noConsentEntry) + " wrench at " + Where(wrench) + " registry=" + registry.Count().ToString());

            NimbyHandsWorldTransfer inFlight = ForgeEntry(dummyA, dummyB, wrench, "NimbyHandsActionTakeItem", NimbyHandsState.REQUESTED);
            NimbyHandsMoveItemToPlayer second = new NimbyHandsMoveItemToPlayer(dummyB, dummyB, wrench, "NimbyHandsActionTakeItem", UAMaxDistances.DEFAULT);
            second.SetConsent(true, "giver is no longer offering");
            bool secondSubmitted = second.Execute();
            bool oneEntry = inFlight != null && registry.Count() == before + 1 && registry.Find(NimbyHandsServer.ItemKey(wrench)) == inFlight;
            Record("hands-two-takers-on-one-offer", !secondSubmitted && oneEntry && ItemInHandsOf(wrench, dummyA), "second submitted=" + Bool(secondSubmitted) + " registry=" + registry.Count().ToString() + " wrench at " + Where(wrench));
            Cleanup(inFlight);
            Record("hands-forged-entry-evicts", registry.Count() == before, "registry=" + registry.Count().ToString());

            dummyB.NimbyHandsSetPocketsOpen(true);
            dummyB.SetRestrained(true);
            bool closed = !dummyB.NimbyHandsGetPocketsOpen();
            // The wait tick reads the pockets again for Put In Pockets and refuses them closed.
            NimbyHandsMoveItemToPlayer deposit = new NimbyHandsMoveItemToPlayer(dummyA, dummyB, wrench, "NimbyHandsActionPutInPockets", UAMaxDistances.SMALL);
            deposit.SetConsent(true, "receiver's pockets are closed");
            bool depositSubmitted = deposit.Execute();
            NimbyHandsServer.Tick();
            NimbyHandsWorldTransfer depositEntry = deposit.Transfer();
            bool depositRefused = RefusedWith(depositEntry, "no consent: receiver's pockets are closed");
            Record("hands-pockets-close-on-restraint-and-deposit-refused", closed && depositSubmitted && depositRefused && ItemInHandsOf(wrench, dummyA) && registry.Count() == before, "pockets open=" + Bool(dummyB.NimbyHandsGetPocketsOpen()) + " registered=" + Bool(depositSubmitted) + " " + DescribeEntry(depositEntry) + " wrench at " + Where(wrench));
            dummyB.SetRestrained(false);

            NimbyHandsServer.OnStep2(987654);
            Record("hands-forged-step2-unknown-id", registry.Count() == before && ItemInHandsOf(wrench, dummyA), "registry=" + registry.Count().ToString() + " wrench at " + Where(wrench));

            NimbyHandsWorldTransfer requested = ForgeEntry(dummyA, dummyB, wrench, "NimbyHandsActionGiveItem", NimbyHandsState.REQUESTED);
            if (requested)
            {
                NimbyHandsServer.OnStep2(requested.id);
                Record("hands-forged-step2-on-requested", requested.state == NimbyHandsState.REQUESTED && ItemInHandsOf(wrench, dummyA), "state=" + NimbyHandsState.Name(requested.state) + " wrench at " + Where(wrench));
                Cleanup(requested);
            }
            else
            {
                Record("hands-forged-step2-on-requested", false, "could not forge the entry");
            }
        }

        // Hands step 2 guard: an item staged in A's cargo.
        if (!pliers)
        {
            Record("harness-item-in-cargo", false, "CreateInInventory(Pliers) returned null");
        }
        else
        {
            Record("harness-item-in-cargo", ItemWith(pliers, dummyA), "pliers at " + Where(pliers));

            NimbyHandsWorldTransfer staged = ForgeEntry(dummyA, dummyB, pliers, "NimbyHandsActionTakeItem", NimbyHandsState.STAGED);
            RunStagedGuardCase("hands-forged-step2-staged-guard", staged, pliers, dummyA, "giver disconnected");

            NimbyHandsServer.ArmStep2Refusal(1);
            NimbyHandsWorldTransfer forced = ForgeEntry(dummyA, dummyB, pliers, "NimbyHandsActionTakeItem", NimbyHandsState.STAGED);
            RunStagedGuardCase("hands-step2-forced-refusal-switch", forced, pliers, dummyA, NimbyHandsRules.DETAIL_FORCED);
            Record("hands-step2-forced-refusal-consumed", NimbyHandsServer.ForcedStep2RefusalsArmed() == 0, "armed=" + NimbyHandsServer.ForcedStep2RefusalsArmed().ToString());

            NimbyHandsWorldTransfer again = ForgeEntry(dummyA, dummyB, pliers, "NimbyHandsActionTakeItem", NimbyHandsState.STAGED);
            RunStagedGuardCase("hands-step2-switch-fires-once", again, pliers, dummyA, "giver disconnected");
        }

        NimbyHandsMoveItemToPlayer noItem = new NimbyHandsMoveItemToPlayer(dummyB, dummyB, null, "NimbyHandsActionTakeItem", UAMaxDistances.DEFAULT);
        noItem.SetConsent(true, "giver is no longer offering");
        bool noItemSubmitted = noItem.Execute();
        Record("hands-no-item-unregistered-refusal", !noItemSubmitted && registry.Count() == before, "submitted=" + Bool(noItemSubmitted) + " registry=" + registry.Count().ToString());

        // NIMBY-T-063 bot case, last: production step 2 between the dummies, cargo to cargo. A
        // dummy's hand state machine never runs a hand event (build-20261029-115044: the Take
        // was dispatched, REQUESTED, and ended UNCONFIRMED with the wrench still in hands), so
        // step 1 cannot run here. Step 2 can: the pliers sit in A's cargo, a forged STAGED entry
        // stands for step 1's landing exactly as the guard cases forge it, and OnStep2 runs the
        // production CollectStep2Facts, IsAtStagedLocation and MoveStep2 against two real
        // players with the dummy party switch on. The October 28 crashes were all in that code.
        // Readings: CONFIRMED with the pliers in B's cargo means the server move between two
        // players works on this server build with no client and no hand event before it; a
        // server exit here reproduces the crash without either; REFUSED names the fact.
        if (pliers && ItemWith(pliers, dummyA))
        {
            NimbyHandsServer.AllowDummyParties(true);
            NimbyHandsWorldTransfer bot = ForgeEntry(dummyA, dummyB, pliers, "NimbyHandsActionTakeItem", NimbyHandsState.STAGED);
            s_BotTransfer = bot;
            if (bot)
            {
                NimbyHandsServer.OnStep2(bot.id);
                Record("hands-bot-step2-cargo-to-cargo", BotDelivered(bot, pliers, dummyB), DescribeEntry(bot) + " pliers at " + Where(pliers) + "; " + ReadBotOutcome(bot, pliers, dummyA));
                Cleanup(bot);
            }
            else
            {
                Record("hands-bot-step2-cargo-to-cargo", false, "could not forge the staged entry");
            }
            NimbyHandsServer.AllowDummyParties(false);
            // The report waits one more second: SendServerMove and the deferred toast run after
            // the call, and a crash there must land before the world is deleted.
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyDevHarness.FinishBotTransfer, BOT_SETTLE_MS, false);
            return;
        }
        Record("hands-bot-step2-cargo-to-cargo", false, "pliers are not with A; at " + Where(pliers));

        FinishBoot();
    }

    // The forged request ended CONFIRMED and the item is in B's cargo. Plain statements, not one
    // expression: build-20261029-115044 reported PASS for a verdict written as
    // "a != null && a.state == X && f(b)" that was false by its own detail text; see
    // harness-expression-probe.
    protected static bool BotDelivered(NimbyHandsWorldTransfer transfer, EntityAI item, PlayerBase receiver)
    {
        if (!transfer)
        {
            return false;
        }
        if (transfer.state != NimbyHandsState.CONFIRMED)
        {
            return false;
        }
        if (!ItemWith(item, receiver))
        {
            return false;
        }
        if (ItemInHandsOf(item, receiver))
        {
            return false;
        }
        return true;
    }

    // Phase 3: the settle second after the bot step 2, then the report.
    static void FinishBotTransfer()
    {
        s_BotTransfer = null;
        FinishBoot();
    }

    // What the bot step 2 verdict means for the NIMBY-T-063 question, in the detail text.
    protected static string ReadBotOutcome(NimbyHandsWorldTransfer transfer, EntityAI item, PlayerBase giver)
    {
        if (!transfer)
        {
            return "reading: no request to read";
        }
        if (transfer.state == NimbyHandsState.CONFIRMED)
        {
            return "reading: the server move between two players works on this server build with no client and no hand event before it";
        }
        if (transfer.state == NimbyHandsState.STAGED)
        {
            return "reading: step 2 returned without a verdict; left to the reconcile";
        }
        if (transfer.state == NimbyHandsState.REFUSED)
        {
            return "reading: step 2 refused; the detail names the fact";
        }
        return "reading: unexpected state";
    }

    // One mixed expression with known values, so the log says how Enforce reads
    // "obj != null && int == int" inside a bool. Expected false (4 is not 2).
    // build-20261029-115841: reads false, as in C.
    static bool ProbeMixedExpression(PlayerBase present, int four)
    {
        bool result = present != null && four == 2;
        return result;
    }

    // The exact shape of the verdict that read PASS while false in build-20261029-115044
    // (hands-bot-transfer-confirmed): a member read under &&, two function terms, one negated.
    // Expected false for an UNCONFIRMED entry and an item that is not with b.
    static bool ProbeMemberExpression(NimbyHandsWorldTransfer t, EntityAI item, PlayerBase b)
    {
        bool confirmed = t != null && t.state == NimbyHandsState.CONFIRMED;
        bool delivered = item != null && b != null && ItemWith(item, b) && !ItemInHandsOf(item, b);
        return confirmed && delivered;
    }

    // Deletes everything phase 1 spawned and prints the report.
    protected static void FinishBoot()
    {
        NimbyHandsServer.AllowDummyParties(false);
        Delete(s_Wrench);
        Delete(s_Pliers);
        Delete(s_DummyA);
        Delete(s_DummyB);
        s_Wrench = null;
        s_Pliers = null;
        s_DummyA = null;
        s_DummyB = null;
        int count = NimbyHandsServer.Registry().Count();
        Record("harness-cleanup", count == s_RegistryBefore, "registry=" + count.ToString() + " dummies and items deleted");
        Report("SelfTest", "selftest");
    }

    // Runs step 2 on a forged staged entry and expects a refusal whose detail starts with the
    // given text, with the item still with the giver and the entry evicted.
    protected static void RunStagedGuardCase(string name, NimbyHandsWorldTransfer staged, EntityAI item, PlayerBase giver, string expected)
    {
        if (!staged)
        {
            Record(name, false, "could not forge the staged entry");
            return;
        }
        NimbyHandsServer.OnStep2(staged.id);
        bool refused = staged.state == NimbyHandsState.REFUSED && NimbyHandsRules.StartsWith(staged.detail, expected);
        bool stayed = ItemWith(item, giver) && !NimbyHandsServer.Registry().IsInFlight(staged.itemKey);
        Record(name, refused && stayed, "state=" + NimbyHandsState.Name(staged.state) + " detail=" + staged.detail);
        Cleanup(staged);
    }

    // ---- online cases ----------------------------------------------------------------------

    static void RunOnlineCases(PlayerBase player)
    {
        if (!player || !player.GetIdentity())
        {
            return;
        }
        Begin();
        PlayerIdentity identity = player.GetIdentity();
        string who = NimbyHandsLog.DescribeIdentity(identity);

        SendLegsJunk(player, identity);
        Record("legs-unreadable-payload", !player.NimbyIsAutorunning(), "player=" + who + " autorunning=" + Bool(player.NimbyIsAutorunning()) + " after a one-string payload");

        SendLegsRequest(player, identity, true, 9, 0);
        Record("legs-idx-9-angle-0", !player.NimbyIsAutorunning(), "player=" + who + " autorunning=" + Bool(player.NimbyIsAutorunning()) + " after idx=9 angle=0");

        for (int i = 0; i < FLOOD_COUNT; i++)
        {
            SendLegsRequest(player, identity, true, DayZPlayerConstants.MOVEMENTIDX_RUN, 1);
        }
        Record("legs-flood-50-in-one-second", !player.NimbyIsAutorunning(), "player=" + who + " autorunning=" + Bool(player.NimbyIsAutorunning()) + " after " + FLOOD_COUNT.ToString() + " requests; the first throttled request clears the override");

        PlayerBase dummy = SpawnDummy(player.GetPosition() + Vector(3, 0, 0));
        if (dummy)
        {
            SendLegsRequest(dummy, identity, true, DayZPlayerConstants.MOVEMENTIDX_RUN, 1);
            Record("legs-request-aimed-at-another-player-from-identity", !dummy.NimbyIsAutorunning(), "sender=" + who + " target autorunning=" + Bool(dummy.NimbyIsAutorunning()));
        }
        else
        {
            Record("legs-request-aimed-at-another-player-from-identity", false, "could not spawn the target dummy");
        }

        PlayerBase other = OtherConnectedPlayer(player);
        PlayerBase receiver = other;
        string receiverNote = "receiver is a second connected player";
        if (!receiver)
        {
            receiver = dummy;
            receiverNote = "no second player connected; receiver is a dummy, so the party check refuses first";
        }
        EntityAI held = player.GetItemInHands();
        if (held && receiver)
        {
            NimbyHandsMoveItemToPlayer stale = new NimbyHandsMoveItemToPlayer(player, receiver, held, "NimbyHandsActionGiveItem", UAMaxDistances.DEFAULT);
            stale.SetConsent(true, "receiver is no longer requesting");
            stale.SetExpected(null, NimbyHandsActionData.IdentityId(player), NimbyHandsActionData.IdentityId(receiver));
            bool staleSubmitted = stale.Execute();
            Record("hands-stale-item-replaced", !staleSubmitted && ItemInHandsOf(held, player), "submitted=" + Bool(staleSubmitted) + " item at " + Where(held) + "; " + receiverNote);

            NimbyHandsMoveItemToPlayer swapped = new NimbyHandsMoveItemToPlayer(player, receiver, held, "NimbyHandsActionGiveItem", UAMaxDistances.DEFAULT);
            swapped.SetConsent(true, "receiver is no longer requesting");
            swapped.SetExpected(held, NimbyHandsActionData.IdentityId(player), "forged-receiver-id");
            bool swappedSubmitted = swapped.Execute();
            Record("hands-stale-receiver-replaced", !swappedSubmitted && ItemInHandsOf(held, player), "submitted=" + Bool(swappedSubmitted) + " item at " + Where(held) + "; " + receiverNote);
        }
        else
        {
            Record("hands-stale-item-replaced", false, "hold an item in your hands when the forge runs (no item in hands)");
            Record("hands-stale-receiver-replaced", false, "hold an item in your hands when the forge runs (no item in hands)");
        }
        Delete(dummy);
        Report("Forge", "forge");
    }

    static PlayerBase OtherConnectedPlayer(PlayerBase player)
    {
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        foreach (Man man : players)
        {
            PlayerBase candidate = PlayerBase.Cast(man);
            if (candidate && candidate != player && candidate.GetIdentity())
            {
                return candidate;
            }
        }
        return null;
    }
}
