modded class ItemBase
{
    bool NimbyCanBeOffered()
    {
        return !IsHeavyBehaviour();
    }

    bool NimbyCanBeGiven()
    {
        return !IsHeavyBehaviour();
    }
}