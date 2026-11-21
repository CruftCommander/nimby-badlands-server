// -----------------------------------------------------------------------------
// NimbyTapes - the deck's own actions (Plan 13, Architecture, Actions; T-3, T-6, T-11).
// Skip    : Next track and Previous track, each in an in-hands form (single use, no target)
//           and an on-ground form (interact, the deck is the target). They show only while
//           the deck plays. The server repeats every check at execution inside
//           NimbyTapes_Deck.NimbyTapesServerNext and NimbyTapesServerPrevious.
// Test kit: Ruin and Delete, on-ground form only: put the deck down and look at it. A client
//           shows them while its own $profile:NimbyTapes\debug exists; the server runs them
//           only while its $profile:NimbyTapes\spawn-testkit exists (T-11). Neither file is on
//           the live server, where the two actions never show and never run. Each use writes
//           one line with the player's GetId() and Steam64 id (shared rules 4.2).
// Register: modded ActionConstructor, as Tack does
//           (NimbyTack/Scripts/4_World/nimbytack_actions.c:1164-1176). The deck's SetActions
//           attaches them.
// Shape   : The in-hands classes follow the vanilla ActionTurnOffWhileInHands
//           (4_World/DayZ/Classes/UserActionsComponent/Actions/SingleUse/
//           ActionTurnOffWhileInHands.c:1-22) and the on-ground classes the vanilla
//           ActionTurnOffWhileOnGround (.../Actions/Interact/ActionTurnOffWhileOnGround.c:1-18),
//           the ground test included.
// API     : ActionConstructor.RegisterActions (.../UserActionsComponent/ActionConstructor.c:27),
//           Object.SetHealth (3_Game/DayZ/Entities/Object.c:1015),
//           EntityAI.DeleteSafe (3_Game/DayZ/Entities/EntityAI.c:791),
//           Man.GetIdentity (3_Game/DayZ/Entities/Man.c:21),
//           PlayerIdentity.GetId and GetPlainId (3_Game/DayZ/gameplay.c:368, :370).
//           Lines are relative to vendor/dayz-1.30-scripts/scripts/ (build 1.30.164289).
// Style   : one statement per line; no ternary; no variable name is declared twice in one
//           function.
// -----------------------------------------------------------------------------
class NimbyTapesActionHelp
{
    // The deck an on-ground action aims at, or null: a living player, a deck as the target,
    // and the deck lying on the ground (the vanilla test, ActionTurnOffWhileOnGround.c:12-17).
    static NimbyTapes_Deck GroundDeck(PlayerBase player, ActionTarget target)
    {
        if (!player || !target)
        {
            return null;
        }
        if (!player.IsAlive())
        {
            return null;
        }
        NimbyTapes_Deck deck = NimbyTapes_Deck.Cast(target.GetObject());
        if (!deck)
        {
            return null;
        }
        InventoryLocation location = new InventoryLocation;
        deck.GetInventory().GetCurrentInventoryLocation(location);
        if (location.GetType() != InventoryLocationType.GROUND)
        {
            return null;
        }
        return deck;
    }

    // T-11: the server decides by its test-kit flag; a client shows the prompt by its own
    // debug flag, because it cannot read the server's profile.
    static bool TestKitAllowed()
    {
        if (!GetGame())
        {
            return false;
        }
        if (GetGame().IsServer())
        {
            return NimbyTapesLog.IsTestKit();
        }
        return NimbyTapesLog.IsDebug();
    }

    // "player=<GetId()>/<Steam64 id>" for a log line (shared rules 4.2).
    static string PlayerField(PlayerBase player)
    {
        if (!player)
        {
            return "player=none";
        }
        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
        {
            return "player=none";
        }
        return "player=" + identity.GetId() + "/" + identity.GetPlainId();
    }
}

// ---- Next and Previous, in hands -------------------------------------------------------------

class ActionNimbyTapesNextInHands : ActionSingleUseBase
{
    void ActionNimbyTapesNextInHands()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_ITEM_ON;
        m_CommandUIDProne = DayZPlayerConstants.CMD_ACTIONFB_ITEM_ON;
        m_Text = "Next track";
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINonRuined();
        m_ConditionTarget = new CCTNone();
    }

    override bool HasTarget()
    {
        return false;
    }

    override bool HasProneException()
    {
        return true;
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        NimbyTapes_Deck deck = NimbyTapes_Deck.Cast(item);
        if (!deck)
        {
            return false;
        }
        return deck.NimbyTapesIsPlaying();
    }

    override void OnExecuteServer(ActionData action_data)
    {
        NimbyTapes_Deck deck = NimbyTapes_Deck.Cast(action_data.m_MainItem);
        if (deck)
        {
            deck.NimbyTapesServerNext();
        }
    }
}

class ActionNimbyTapesPreviousInHands : ActionSingleUseBase
{
    void ActionNimbyTapesPreviousInHands()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_ITEM_ON;
        m_CommandUIDProne = DayZPlayerConstants.CMD_ACTIONFB_ITEM_ON;
        m_Text = "Previous track";
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINonRuined();
        m_ConditionTarget = new CCTNone();
    }

    override bool HasTarget()
    {
        return false;
    }

    override bool HasProneException()
    {
        return true;
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        NimbyTapes_Deck deck = NimbyTapes_Deck.Cast(item);
        if (!deck)
        {
            return false;
        }
        return deck.NimbyTapesIsPlaying();
    }

    override void OnExecuteServer(ActionData action_data)
    {
        NimbyTapes_Deck deck = NimbyTapes_Deck.Cast(action_data.m_MainItem);
        if (deck)
        {
            deck.NimbyTapesServerPrevious();
        }
    }
}

// ---- Next and Previous, on the ground --------------------------------------------------------

class ActionNimbyTapesNextOnGround : ActionInteractBase
{
    void ActionNimbyTapesNextOnGround()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH | DayZPlayerConstants.STANCEMASK_ERECT;
        m_Text = "Next track";
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        NimbyTapes_Deck deck = NimbyTapesActionHelp.GroundDeck(player, target);
        if (!deck)
        {
            return false;
        }
        return deck.NimbyTapesIsPlaying();
    }

    override void OnExecuteServer(ActionData action_data)
    {
        NimbyTapes_Deck deck = NimbyTapesActionHelp.GroundDeck(action_data.m_Player, action_data.m_Target);
        if (deck)
        {
            deck.NimbyTapesServerNext();
        }
    }
}

class ActionNimbyTapesPreviousOnGround : ActionInteractBase
{
    void ActionNimbyTapesPreviousOnGround()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH | DayZPlayerConstants.STANCEMASK_ERECT;
        m_Text = "Previous track";
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        NimbyTapes_Deck deck = NimbyTapesActionHelp.GroundDeck(player, target);
        if (!deck)
        {
            return false;
        }
        return deck.NimbyTapesIsPlaying();
    }

    override void OnExecuteServer(ActionData action_data)
    {
        NimbyTapes_Deck deck = NimbyTapesActionHelp.GroundDeck(action_data.m_Player, action_data.m_Target);
        if (deck)
        {
            deck.NimbyTapesServerPrevious();
        }
    }
}

// ---- Test kit: Ruin and Delete (T-11) --------------------------------------------------------

class ActionNimbyTapesDebugRuin : ActionInteractBase
{
    void ActionNimbyTapesDebugRuin()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH | DayZPlayerConstants.STANCEMASK_ERECT;
        m_Text = "[test] Ruin";
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (!NimbyTapesActionHelp.TestKitAllowed())
        {
            return false;
        }
        NimbyTapes_Deck deck = NimbyTapesActionHelp.GroundDeck(player, target);
        if (!deck)
        {
            return false;
        }
        return !deck.IsRuined();
    }

    override void OnExecuteServer(ActionData action_data)
    {
        if (!NimbyTapesLog.IsTestKit())
        {
            return;
        }
        NimbyTapes_Deck deck = NimbyTapesActionHelp.GroundDeck(action_data.m_Player, action_data.m_Target);
        if (!deck)
        {
            return;
        }
        NimbyTapesLog.Info("test kit ruin; deck=" + deck.NimbyTapesLogId() + " " + NimbyTapesActionHelp.PlayerField(action_data.m_Player));
        deck.SetHealth("", "", 0);
    }
}

class ActionNimbyTapesDebugDelete : ActionInteractBase
{
    void ActionNimbyTapesDebugDelete()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH | DayZPlayerConstants.STANCEMASK_ERECT;
        m_Text = "[test] Delete";
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (!NimbyTapesActionHelp.TestKitAllowed())
        {
            return false;
        }
        NimbyTapes_Deck deck = NimbyTapesActionHelp.GroundDeck(player, target);
        if (!deck)
        {
            return false;
        }
        return true;
    }

    override void OnExecuteServer(ActionData action_data)
    {
        if (!NimbyTapesLog.IsTestKit())
        {
            return;
        }
        NimbyTapes_Deck deck = NimbyTapesActionHelp.GroundDeck(action_data.m_Player, action_data.m_Target);
        if (!deck)
        {
            return;
        }
        NimbyTapesLog.Info("test kit delete; deck=" + deck.NimbyTapesLogId() + " " + NimbyTapesActionHelp.PlayerField(action_data.m_Player));
        deck.DeleteSafe();
    }
}

modded class ActionConstructor
{
    override void RegisterActions(TTypenameArray actions)
    {
        super.RegisterActions(actions);
        actions.Insert(ActionNimbyTapesNextInHands);
        actions.Insert(ActionNimbyTapesPreviousInHands);
        actions.Insert(ActionNimbyTapesNextOnGround);
        actions.Insert(ActionNimbyTapesPreviousOnGround);
        actions.Insert(ActionNimbyTapesDebugRuin);
        actions.Insert(ActionNimbyTapesDebugDelete);
    }
}
