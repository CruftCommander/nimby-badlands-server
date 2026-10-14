modded class ActionConstructor
{
    override void RegisterActions( TTypenameArray actions )
    {
        super.RegisterActions( actions );

        actions.Insert( ActionOfferItemToPlayer );
        actions.Insert( ActionTakeItemFromPlayer );
        actions.Insert( ActionTakeItemFromPlayerToHands );

        actions.Insert( ActionRequestItemFromPlayer );
        actions.Insert( ActionGiveItemInHandsToPlayer );

        actions.Insert( ActionToggleAllowPutInPockets );
        actions.Insert( ActionPutItemInHandsInPlayersPockets );
    }
}