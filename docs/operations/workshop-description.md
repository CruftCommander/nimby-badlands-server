# Workshop description

The text for the public Steam Workshop item `@NIMBY`. Drafted October 25, 2026 for the Tack troubleshooting section Jeff asked for; filled at release 1 (Legs, Ears, Scrawl, Greet, Tack, Hands) and extended for release 2 (DPV, credits) on November 8, 2026. Keep it plain English: players read this, not operators. No em dashes.

## NIMBY pack

One mod for the NIMBY | HARDCORE | BADLANDS | 1pp server. Subscribe once; the server tells your game which version to run.

- Legs: double tap Left Shift to autorun (rebindable in Options > Controls > NIMBY).
- Ears: earplugs on a key.
- Scrawl: write on paper with a pen.
- Tack: pin a written note to a wall with a hammer and nail, duct tape, epoxy putty, or a knife through the sheet into wood. Others read it in place; pulling it down damages the sheet.
- Hands: hold an item and press O to offer it to the player in front of you; empty hands and O asks for theirs.
- Greet: the server welcome window (Insert reopens it) with the how-to for every mod and a "What's changed" tab.
- DPV (release 2): a desert patrol buggy found at military sites. It spawns on its wheels; bring a battery, spark plug, bulbs and fuel. Side rails carry jerry cans, ammo cans, packs and tents, the roof a wooden crate, car tent or a teddy bear, the cabin a first aid kit. Look at an item on the car to take it.

Credits: the DPV model is derived from "Desert Patrol Vehicle Low-poly" by carbuni (CGTrader), licensed for this use; Legs is a fork of SchanaModAutorun (Apache-2.0); Hands is a fork of Give&Take by affenb3rt, repacked with permission. Full notices ship in each addon's NOTICE file.

## Tack: why did my note not pin?

Check these first.

1. **No prompt with the tool in hand.** You are holding a stack of paper. Tack pins one sheet: drag a single sheet out of the stack in your inventory, then aim again.
2. **No prompt, and a box says "Too many notes nearby".** There are already 20 notes within 50 m. Take one down or pin somewhere else.
3. **The hammering ran, then "Too many notes nearby" and no note.** Same limit, counted by the server. Nothing was lost; your sheet is still in your inventory.
4. **No prompt on a plain wall.** Stand within arm's reach (1.5 m) and aim at the flat of the wall, not a window, sill, or edge. Map fences, posts, and doors work too.
5. **"There is no wall there."** What you aimed at is not a map building, wall, or fence.
5b. **"Notes pin on map buildings."** Player-built walls, fences, watchtowers, and tents do not take a note yet; that is a later update.
6. **"The wall is not where it looks."** That building draws its wall in front of where the game can measure it; the note would be buried. Try another face.
7. **"The knife will not go into that."** A knife only pins on wood. Use a hammer, tape, or epoxy.
8. **The note sits in the wall or floats off it.** Some building models draw their walls where the game cannot measure them. Tell an admin which building (a screenshot is enough); the server can be tuned per building without an update.
9. **Doors.** A note pins on a door open or closed and swings with it. After a server restart it is back on the door.
