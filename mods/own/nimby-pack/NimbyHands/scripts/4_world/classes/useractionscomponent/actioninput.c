class NimbyHandsOfferRequestActionInput extends ContinuousDefaultActionInput
{
    void NimbyHandsOfferRequestActionInput( PlayerBase player )
    {
        SetInput( "UANimbyHandsOfferRequest" );
        m_InputType = ActionInputType.AIT_CLICKCONTINUOUS;
        m_DetectFromItem = false;
    }
}

class NimbyHandsTogglePocketsActionInput extends StandardActionInput
{
    void NimbyHandsTogglePocketsActionInput( PlayerBase player )
    {
        SetInput( "UANimbyHandsTogglePockets" );
        m_InputType = ActionInputType.AIT_SINGLE;
        m_DetectFromItem = false;
    }
}