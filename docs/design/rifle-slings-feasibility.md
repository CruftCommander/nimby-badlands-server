# Rifle slings feasibility

Draft v0.1, October 21, 2026. Owner: Jeff Fabish. Status: feasibility only, ordered by Jeff on October 21, 2026 for the rifle-slings idea recorded October 17, 2026 in `docs/research/candidate-features.md`. Working name Slings, code prefix `NimbySlings`, log prefix `[NIMBY][Slings]`, RPC block `65513xx` (reserved, nothing below needs an RPC); Jeff names it. Every `file:line` is relative to `vendor/dayz-1.30-scripts/scripts/` (build 1.30.164078.38).

Target: NIMBY | HARDCORE | BADLANDS | 1pp, HostHavoc, DayZ 1.30 / Nasdara.

## Verdict

Feasible, and the gate is one override: the player decides what it accepts on its back through `CanReceiveAttachment(attachment, slotId)` (`EntityAI.c:1472`), and the two back slots are `InventorySlots.SHOULDER` and `InventorySlots.MELEE` (`EntityAI.c:4674`). A modded `PlayerBase.CanReceiveAttachment` that refuses a long gun on either slot unless it carries a sling attachment is the whole rule. The sling is a new item with a new attachment slot added to every long-gun base class by config; vanilla keeps `CanLoadAttachment` returning true (`EntityAI.c:1484-1487`), so rifles already on backs when the mod ships stay there until they are next moved, which is the migration story for free.

The cost is in breadth, not depth: long guns do not share one base. `Rifle_Base` covers eleven families, but bolt-actions (`BoltActionRifle_Base`, `BoltRifle_Base`), the crossbow (`Archery_Base`), the M249, and the Magnums each extend `Weapon_Base` directly, so the slot is added to five or six base classes and the rule tests "is a long gun" by a list, not by one cast. The visual is the honest limit: no vanilla weapon model has a sling proxy, so the sling shows in the inventory and never on the gun; adding a visible strap to twenty weapon models is the kind of model work this fleet does not do.

Because the sling is a persistent item class, Slings is a content addon under shared rules 6.1 and takes the two extra boots at its gate.

## What this is not

- Not a model mod. The sling is invisible on the weapon; it is an inventory attachment, like a vanilla suppressor on a gun with no suppressor proxy would be.
- Not a melee rule. Axes, bats, and other `Melee` slot items are untouched; a sling on an axe is not a thing, and the second back slot stays usable for them.
- Not a pistol rule. Pistols never go on the back.
- Not an inventory-size change. A rifle without a sling goes in hands or in any cargo it already fits (large backpacks), exactly as vanilla sizes allow; Slings does not resize anything.

## Feasibility by requirement

| # | Requirement | Verdict | How | Source |
| --- | --- | --- | --- | --- |
| R1 | A sling item | Yes | `NimbySlings_Sling : Inventory_Base`, 1x2, with health (a ruined sling fails R4). Model: a vanilla strap-like model as placeholder (the `Rope` model `Rope : Inventory_Base`, `crafting.c:58`, or a belt), replaced by a Claude-made model under the smoking study's D2 rule if Jeff wants one | `4_World/DayZ/Entities/ItemBase/Gear/Crafting/crafting.c:58` |
| R2 | A sling slot on all or most rifles | Yes, by config on each long-gun base | Modded `config.cpp` adds `attachments[] += {"NimbySling"}` and a `CfgSlots` entry `Slot_NimbySling` to: `Rifle_Base` (covers `OTS14`, `SKS_Base`, `ChamberFirst_InnerMagazine_SemiAutomatic_Base`, `DoubleBarrel_Base`, `OpenBolt_Base`, `Repeater_Base`, `Trumpet`, `RifleBoltFree_Base`, `RifleBoltLock_Base`, `RifleSingleShot_Base`, `Mp133Shotgun_Base`), `BoltActionRifle_Base` (its `_ExternalMagazine_Base`, `_InnerMagazine_Base`), `BoltRifle_Base`, `Archery_Base` (`Crossbow_Base`), `M249`, and `Magnum_Base` only if Jeff wants revolvers excluded (recommended: excluded, they are handguns). Shotguns are `Rifle_Base` subclasses and come along | `4_World/DayZ/Entities/Firearms/Rifle_Base.c:10`; `BoltActionRifle_Base.c:5`; `Archery/Crossbow.c:53`; class list from `grep` of `4_World` on October 21, 2026 |
| R3 | On the back only with a sling | Yes, one override | `modded class PlayerBase { override bool CanReceiveAttachment(EntityAI attachment, int slotId) }`: when `slotId` is `InventorySlots.SHOULDER` or `InventorySlots.MELEE` and `attachment` is a long gun (a `Weapon_Base` that is not a `Pistol_Base`), return `attachment.FindAttachmentBySlotName("NimbySling") != null` and then `super`. Vanilla's own slot-swap exceptions for hands, shoulder, melee, and left hand (`EntityAI.c:4674-4675,4702-4703`) keep working because the rule only adds a refusal. The vanilla selection-texture switch for rifle-versus-melee on each slot (`PlayerBase.c:1595-1609`) is untouched | `3_Game/DayZ/Entities/EntityAI.c:1472-1475,4674-4703`; `4_World/DayZ/Entities/ManBase/PlayerBase.c:1595-1609` |
| R4 | Sling removed or ruined while the rifle is on the back | Yes, with a rule to pick | `Weapon_Base.EEItemDetached` (`:1115`) fires when the sling leaves; `OnAttachmentRuined` fires when it ruins. Two choices: (a) the rifle stays on the back until the player next moves it, then cannot return without a sling (passive, no surprise drops); (b) the server drops the rifle to the ground at the player's feet the moment the sling goes (strict). Recommend (a): a sling cut in a fight is a strong enough penalty when the rifle comes off the back for the next reload and cannot go back, and (b) risks a rifle vanishing into geometry | `4_World/DayZ/Entities/Firearms/Weapon_Base.c:1115`; `3_Game/DayZ/Entities/EntityAI.c` (`OnAttachmentRuined`) |
| R5 | Existing rifles on backs when the mod ships | Yes, free | `CanLoadAttachment` stays vanilla (returns true), so storage loads rifles onto backs unchanged; R3 applies at the next move. No wipe, no cleanup pass | `3_Game/DayZ/Entities/EntityAI.c:1484-1487` |
| R6 | Loot and crafting | Yes | `types.xml` row for `NimbySlings_Sling` (nominal to be tuned; it should be common, since the rule otherwise punishes every new spawn); a script recipe `RecipeBase` subclass "craft sling" from `Rope` plus a knife, or from two `Rope`, so a player is never locked out by loot | `4_World/DayZ/Classes/Recipes/RecipeBase.c:19`; `crafting.c:58`; `server/live/db/types.xml` |
| R7 | Scope for shotguns, crossbows, long melee | Shotguns and the crossbow in (they are long guns on the same slots); long melee out (see What this is not) | R2 list | - |

## Save footprint

The sling is a persistent item (it has health and sits in an attachment slot that is saved with the weapon), so shared rules 6.1 applies: the content-addon boots (save and reload with the addon, then boot that storage without it) are part of the gate. Removing Slings later leaves rifles on backs (R5 in reverse) and drops the sling items as unknown classes, which is the expected content-addon removal shape.

## Risks

| Risk | Mitigation |
| --- | --- |
| A long-gun family extends `Weapon_Base` directly and is missed by the list | The self-test enumerates every `Weapon_Base` subclass in the loaded config at mission start and prints the ones with no `NimbySling` slot that are not `Pistol_Base`; the boot check fails on any |
| Players read the rule as a bug ("my rifle will not go on my back") | Greet tip at release; the refusal is also a vanilla inventory "cannot attach" cue, so a one-line HUD notice ("needs a sling") is Phase 1 work |
| Sling nominal too low makes early game miserable | Start common (nominal on the order of hats), tune on live data |
| Invisible sling looks like a missing feature | Accepted; recorded in the Workshop description |

## Decisions for Jeff (when the plan is written)

| # | Decision | Recommendation |
| --- | --- | --- |
| RS1 | Display name | Jeff names it; working name Slings |
| RS2 | Order on the board | After Bikes; it changes core gun handling, so it ships after the first themed drop has settled |
| RS3 | Revolvers | Excluded (handguns) |
| RS4 | Removal rule (R4) | (a) stays until next moved |
| RS5 | Recipe | Two `Rope` with a knife in hands |
| RS6 | HUD notice | Yes, one line on refusal |

## Jeff's requirements, October 27, 2026

Stated by Jeff on October 27, 2026, 20:37 Central, after this study. They take precedence over the recommendations above where they differ; nothing is built yet and the plan is not written.

| # | Requirement (Jeff's words, lightly edited) | Effect on this study |
| --- | --- | --- |
| SR1 | A rifle goes on your back only with a sling attached, and you can craft your own sling. "Initially at least, you should be able to make a sling out of 1 rope." | Replaces the RS5 recommendation (two `Rope` with a knife). A vanilla `RecipeBase` combines two items, so a one-rope sling is most likely a single-item action on the rope ("Make sling") rather than a recipe; settle it in the plan. "Initially" leaves room to change the cost later |
| SR2 | The first phase "must support attaching a long rifle to your backpack", but only rifles that are not very long. Examples Jeff gave: Mosins, MP5s, sawn-offs | New scope, not in R1 to R7: a rifle carried on a backpack (an attachment slot on backpacks), limited by a length rule. Claude's reading, to be confirmed by Jeff: very long rifles such as the Mosin are excluded, short long guns such as the MP5 and sawn-off shotguns are allowed. Feasibility to study: which backpacks get the slot, how "very long" is decided (a list, or the weapon's inventory size), and whether the backpack slot also needs a sling |
| SR3 | A slung rifle in the primary slot goes across your chest | New scope. Assumed mapping: "primary" is the vanilla shoulder slot (`InventorySlots.SHOULDER`). Vanilla draws both back slots on the back, so a chest position needs a different attachment point on the character, which may be model or proxy work; this study's verdict said that kind of work is out of scope, so SR3 needs its own feasibility check before the plan |
| SR4 | A slung rifle in the secondary slot goes across your back | Assumed mapping: "secondary" is the vanilla second back slot (`InventorySlots.MELEE`), which vanilla already draws on the back; likely no visual change beyond the sling rule |

Open questions for Jeff: SR2's reading of the Mosin (excluded or allowed); whether the backpack carry in SR2 also requires a sling; whether the one-rope sling needs a knife; and whether "primary" and "secondary" mean the vanilla shoulder and second back slots.
