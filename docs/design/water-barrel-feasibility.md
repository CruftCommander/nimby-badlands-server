# Water barrel feasibility

Draft v0.1, October 21, 2026. Owner: Jeff Fabish. Status: feasibility only, ordered by Jeff on October 21, 2026 for the water-barrel idea recorded October 19, 2026 in `docs/research/candidate-features.md`. Every `file:line` is relative to `vendor/dayz-1.30-scripts/scripts/` (build 1.30.164078.38).

Target: NIMBY | HARDCORE | BADLANDS | 1pp, HostHavoc, DayZ 1.30 / Nasdara.

## Verdict

Vanilla 1.30 already does almost all of it. The barrel (`Barrel_ColorBase`) is a liquid container that persists its open state and its liquid, collects rain while its lid is open through a dedicated `RainProcurementComponentBarrel`, and is handled by name in the shared liquid-transfer rules, so a bottle in hands pours into an open barrel (`ActionPourLiquid`) and fills from one (`ActionDrainLiquid`). The idea as written ("filled from jerry cans, canteens, bottles; those containers refill from it; an open barrel collects rain") is therefore a replica check, not a build. Nothing new is a content addon, and no persistence change is needed because the barrel already saves its liquid through vanilla.

What may still be missing, each a config or one-line change rather than a feature: whether the vanilla jerry can (`CanisterGasoline`) accepts water at all (its container mask decides, and that is config, not script); whether the barrel's capacity reads as a "long reserve" (its `varQuantityMax` is config; the dye recipes treat 10,000 ml as a working minimum); and whether players know the lid must be open for every transfer, which is a Greet tip, not code. If Jeff wants a visibly different water barrel (a dedicated class or texture), that becomes a small content addon; the study recommends against it because the vanilla barrel already carries the behavior.

## What this is not

- Not a new container. The vanilla barrel is the water barrel.
- Not a Tweaker item unless the replica check finds a gap; then the gap is a config override on `Barrel_ColorBase` and lives in Tweaker.
- Not a rain-system change. Rain collection rate and liquid type are vanilla's.

## Feasibility by requirement

| # | Requirement | Verdict | How | Source |
| --- | --- | --- | --- | --- |
| W1 | A barrel holds water as a long reserve | Vanilla | `Barrel_ColorBase : DeployableContainer_Base` is a liquid container (`GetLiquidType`, `GetQuantity`, items inside get soaked above a threshold); liquid and quantity persist through vanilla item storage; `OnStoreSave`/`OnStoreLoad` already save the lid state | `4_World/DayZ/Entities/ItemBase/Barrel_ColorBase.c:1,29-48,52-66,183-199` |
| W2 | Filled from jerry cans, canteens, bottles | Vanilla for bottles and canteens; config check for jerry cans | `Liquid.CanTransfer(source, target)` refuses a closed barrel on either side and otherwise applies the container mask (`CanFillContainer`); `ActionPourLiquid` (item in hands into the target) is registered on every `Bottle_Base`. Whether `CanisterGasoline` can hold water is its `liquidContainerType` mask in config; if it is gasoline-only the jerry can never carries water, which is a vanilla design choice rather than a barrel limit | `4_World/DayZ/Static/Liquid.c:121-158,235-250`; `4_World/DayZ/Entities/ItemBase/Edible_Base/Bottle_Base.c:369-370`; `4_World/DayZ/Entities/ItemBase.c:825-830` |
| W3 | Those containers refill from the barrel | Vanilla | `ActionDrainLiquid` is the reverse transfer: `Liquid.CanTransfer(target_item, item)` with the barrel as source, registered on `Bottle_Base` | `ActionDrainLiquid.c:33-42`; `Bottle_Base.c:369` |
| W4 | An open barrel collects rain | Vanilla | `Open()` calls `m_RainProcComponent.StartRainProcurement()`, `Close()` stops it; the component reads `GetRain().GetActual()` plus snowfall each tick and skips processing when the coefficient is zero. Opening on attach (`OnWasAttached`) and the lid animation phases are handled | `Barrel_ColorBase.c:87-118,130-156`; `4_World/DayZ/Classes/RainProcurementHandler.c:50-51,162` |
| W5 | Capacity as a long reserve | Config | The barrel's `varQuantityMax` is in its `config.cpp`, not script; the recipes in `Barrel_ColorBase.c:183-199` assume at least 10,000 ml is a meaningful fill, so capacity is already in the tens of litres. A larger cap is a one-line config override if Jeff wants it | `Barrel_ColorBase.c:183-199`; `3_Game/DayZ/constants.c:613` (`LIQUID_THROUGHPUT_BARREL = 100`) |
| W6 | Content-addon question (shared rules 6.1) | No | No new persistent class; the barrel already saves everything it needs | Shared rules 6.1 |

## Replica check (before any decision)

1. Place a barrel on the replica, open it, pour from a canteen, a bottle, a cooking pot, and a jerry can; record which transfers the game offers and the resulting quantity.
2. Fill each container back from the barrel; record the same.
3. Leave the open barrel in rain on the replica (Central Scrutinizer Phase 3 can force rain once it exists; until then, wait for weather) and read the quantity before and after.
4. Close the lid and confirm every transfer disappears.
5. Note the capacity shown in the inventory tooltip.

Exit: a one-paragraph record in `docs/testing/` saying which of W1-W5 vanilla already covers on 1.30. Anything missing becomes a Tweaker row (config override on `Barrel_ColorBase`), nothing more.

## Decisions for Jeff

| # | Decision | Recommendation |
| --- | --- | --- |
| WB1 | Build anything before the replica check | No. Run the check in the next replica session; it costs ten minutes |
| WB2 | If the jerry can is gasoline-only | Leave it. Water in a fuel can is a game-balance choice Bohemia made; canteens, bottles, and pots cover the use case |
| WB3 | If capacity is small | Tweaker config override, not a new class |
| WB4 | Player education | One Greet tip: "Open the lid; an open barrel fills with rain and pours into any container" |
