modded class ActionConstructor
{
    override void RegisterActions( TTypenameArray actions )
    {
        super.RegisterActions( actions );

        actions.Insert( NimbyHandsActionOfferItem );
        actions.Insert( NimbyHandsActionTakeItem );

        actions.Insert( NimbyHandsActionRequestItem );
        actions.Insert( NimbyHandsActionGiveItem );

        actions.Insert( NimbyHandsActionTogglePockets );
        actions.Insert( NimbyHandsActionPutInPockets );
    }
}