// NimbyDpv rail items: vanilla jerry can, ammo box and backpacks gain the attach-by-looking
// action so a player holding one can look at an empty rail pad on the DPV and attach it there.
// Vanilla only gives ActionAttachOnSelection to wheels, doors, batteries, camo nets and the
// like; everything else attaches through the inventory screen only. ActionAttachOnSelection
// is item-side: it resolves the car's View Geometry component under the cursor (the rail pad,
// named NimbyDpvRail_N) to a slot through InventorySlots.GetSlotIdFromString and only offers
// itself when the item carries that slot, so these overrides change nothing on any other
// target. Build 50 (2026-11-07).
//
// ActionDetach ("Take to hands" while looking at an attached item) is likewise item-side and
// vanilla gives it only to Edible_Base (hence the jerry can), CarWheel, VehicleBattery and a few
// others; Container_Base and Clothing never had it, which is why the ammo box and the bag only
// offered "Push car". Found 2026-11-07 13:30 reading ActionDetach.c and Edible_Base.c:702 -
// the earlier note that InventoryItem.c:267 gave every item ActionDetach was wrong (that line is
// inside CarWheel). Added for the box and the bags; the can already inherits it. Untested as of
// build 50 - first proof is build 51.

modded class CanisterGasoline
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionAttachOnSelection);
	}
}

modded class AmmoBox
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionAttachOnSelection);
		AddAction(ActionDetach);
	}
}

modded class Backpack_Base
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionAttachOnSelection);
		AddAction(ActionDetach);
	}
}

// Build 55: packed tents on the per-side tent slots. TentBase.SetActions (TentBase.c:852) has the
// pack, unpack and deploy actions and neither of these; same reasoning as the box and the bags.
modded class TentBase
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionAttachOnSelection);
		AddAction(ActionDetach);
	}
}

// Build 55: small storage (Jeff, 18:15). The three are Container_Base children with no vanilla
// attach or detach action (containers.c:20, :74; ScientificBriefcase.c:1); modded one by one rather
// than at Container_Base so barrels, crates and chests stay as vanilla left them.
modded class FirstAidKit
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionAttachOnSelection);
		AddAction(ActionDetach);
	}
}

modded class SmallProtectorCase
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionAttachOnSelection);
		AddAction(ActionDetach);
	}
}

modded class ScientificBriefcase
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionAttachOnSelection);
		AddAction(ActionDetach);
	}
}
