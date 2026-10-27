// Constants for NimbyHands. Replaces the original's modded GameConstants member so the
// fork adds nothing to a vanilla class.
class NimbyHandsConstants
{
    // Seconds the pocket open or closed notification stays on screen.
    static const float POCKET_NOTIFICATION_TIME = 1.5;

    // G5: milliseconds a transfer may stay unresolved before the server reconciles where the
    // item is and evicts the entry (plan v0.5, Phase 1). Not a completion signal: a request
    // that reaches this point is logged CONFIRMED only when the item is with the receiver.
    static const int TRANSFER_TIMEOUT_MS = 5000;

    // Meters added to an action's own reach when the server revalidates proximity at dispatch,
    // so a receiver who drifts during the animation is not refused for a few centimeters.
    static const float PROXIMITY_TOLERANCE = 0.5;

    // Seconds the transfer outcome toast stays on screen (NIMBY-T-012, J4), and its title.
    static const float TOAST_SECONDS = 4.0;
    static const string TOAST_TITLE = "Hands";

    // Server-side minimum interval between two pocket toggles of one player (review item 3).
    // A press inside it is refused silently: a refused repeat is not a consent change.
    static const int POCKET_TOGGLE_MIN_MS = 500;
}
