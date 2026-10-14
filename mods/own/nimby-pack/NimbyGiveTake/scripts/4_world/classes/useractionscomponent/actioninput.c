class ContinuousOfferOrRequestItemActionInput extends ContinuousDefaultActionInput
{
    void ContinuousOfferOrRequestItemActionInput( PlayerBase player )
    {
        SetInput( "UAOfferRequestItem" );
        m_InputType = ActionInputType.AIT_CLICKCONTINUOUS;
        m_DetectFromItem = false;
    }
}

class ToggleAllowPutInPocketsActionInput extends StandardActionInput
{
    void ToggleAllowPutInPocketsActionInput( PlayerBase player )
    {
        SetInput( "UAToggelAllowPutInPockets" );
        m_InputType = ActionInputType.AIT_SINGLE;
        m_DetectFromItem = false;
    }
}