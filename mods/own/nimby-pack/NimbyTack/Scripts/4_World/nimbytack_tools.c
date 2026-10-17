// -----------------------------------------------------------------------------
// NimbyTack - the pin actions on the fasteners (T2, T7).
// Purpose : Each fastener class offers its pin action when it is in hands. The
//           class lists live in NimbyTackRules (IsHammerTool, IsKnife); adding a
//           class means a modded class here and a line there (T8).
// Vanilla : Hammer.c:1, Hatchet.c:1, Wrench.c:1, PipeWrench.c:1, Gear/Tools/LugWrench.c:1,
//           Gear/Tools/Crowbar.c, Gear/Consumables/consumables.c:252 (DuctTape),
//           Gear/Tools/EpoxyPutty.c:1, KitchenKnife.c:1, SteakKnife.c:1, HuntingKnife.c:1,
//           CombatKnife.c:1, KukriKnife.c:1, Machete.c:1.
// -----------------------------------------------------------------------------
modded class Hammer
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionNimbyTackPinNail);
    }
}

modded class Hatchet
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionNimbyTackPinNail);
    }
}

modded class Wrench
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionNimbyTackPinNail);
    }
}

modded class PipeWrench
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionNimbyTackPinNail);
    }
}

modded class LugWrench
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionNimbyTackPinNail);
    }
}

modded class Crowbar
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionNimbyTackPinNail);
    }
}

modded class DuctTape
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionNimbyTackPinTape);
    }
}

modded class EpoxyPutty
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionNimbyTackPinEpoxy);
    }
}

modded class KitchenKnife
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionNimbyTackPinKnife);
    }
}

modded class SteakKnife
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionNimbyTackPinKnife);
    }
}

modded class HuntingKnife
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionNimbyTackPinKnife);
    }
}

modded class CombatKnife
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionNimbyTackPinKnife);
    }
}

modded class KukriKnife
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionNimbyTackPinKnife);
    }
}

modded class Machete
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionNimbyTackPinKnife);
    }
}
