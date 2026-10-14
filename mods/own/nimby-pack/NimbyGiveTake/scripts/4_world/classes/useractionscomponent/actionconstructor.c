modded class ActionConstructor
{
    override void RegisterActions( TTypenameArray actions )
    {
        super.RegisterActions( actions );

        actions.Insert( NimbyActionOfferItem );
        actions.Insert( NimbyActionTakeItem );

        actions.Insert( NimbyActionRequestItem );
        actions.Insert( NimbyActionGiveItemInHands );

        actions.Insert( NimbyActionTogglePocketAccess );
        actions.Insert( NimbyActionPutInPockets );
    }
}