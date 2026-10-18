// -----------------------------------------------------------------------------
// NimbyNotes - pens.
// Purpose : Makes the 1.30 write action reachable from a pen (N5). Pen_ColorBase has
//           no SetActions in 1.30 (Pen_ColorBase.c:1); ActionWritePaper exists, is
//           marked deprecated, and is registered nowhere.
// -----------------------------------------------------------------------------
modded class Pen_ColorBase
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionWritePaper);
    }
}
