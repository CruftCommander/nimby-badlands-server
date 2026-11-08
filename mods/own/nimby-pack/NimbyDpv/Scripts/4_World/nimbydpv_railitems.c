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
// Build 62 (November 8, 2026, 00:26): a packed tent on a rail offered no "Take to hands" by looking
// (MediumTent and CarTent, builds 57 to 61). MiscGameplayFunctions.IsObjectObstructedEx
// (:1630-1640) treats a target as obstructed when the aim ray hits the target's OWN proxy
// geometry and CanProxyObstructSelf() is true; TentBase alone returns true (TentBase.c:78-81), so
// a pitched tent cannot be taken through its camo-net and light proxies. Packed, those proxies
// still answer the ray, so the detach was refused. A packed tent has nothing deployed to obstruct
// itself with; pitched behaviour is unchanged. GetState() and PACKED: TentBase.c:484-494.
modded class TentBase
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionAttachOnSelection);
		AddAction(ActionDetach);
	}

	override bool CanProxyObstructSelf()
	{
		if (GetState() == PACKED)
			return false;
		return super.CanProxyObstructSelf();
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

// Build 57: the Field Transceiver (BaseRadio) and the Wooden Crate. TransmitterBase.SetActions
// (TransmitterBase.c:159) has only the on, off and tune actions, so the radio gets both.
// WoodenCrate.SetActions (Gear/Camping/camping.c:29) already adds ActionAttachOnSelection for
// the M3S truck's crate slots and never had ActionDetach, so the crate gets that one alone.
modded class BaseRadio
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionAttachOnSelection);
		AddAction(ActionDetach);
	}
}

modded class WoodenCrate
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionDetach);
	}
}

// Build 58: the teddy bear on the roof (Bear_ColorBase, Container_Base/EN5C_Bear_ColorBase.c:1; the
// colour variants inherit). No vanilla attach or detach action, same as the other containers.
modded class Bear_ColorBase
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionAttachOnSelection);
		AddAction(ActionDetach);
	}
}
