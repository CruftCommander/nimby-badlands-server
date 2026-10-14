modded class ItemBase
{
    bool CanBeOffered()
    {
        return !IsHeavyBehaviour();
    }

    bool CanBeGiven()
    {
        return !IsHeavyBehaviour();
    }
}