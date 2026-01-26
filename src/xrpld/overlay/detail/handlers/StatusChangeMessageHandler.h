#ifndef XRPL_OVERLAY_HANDLERS_STATUSCHANGEMESSAGEHANDLER_H_INCLUDED
#define XRPL_OVERLAY_HANDLERS_STATUSCHANGEMESSAGEHANDLER_H_INCLUDED

#include <xrpl/basics/base_uint.h>

#include <memory>

namespace protocol {
class TMStatusChange;
}

namespace xrpl {

class PeerImp;

/** Handler for publishing peer status changes.

    This class provides a static method to publish status changes to
    subscribers via NetworkOPs. The implementation is in the app module
    to avoid dependency cycles between overlay and app.
*/
class StatusChangeMessageHandler
{
public:
    /** Publish peer status change to subscribers.

        This is called after processing a TMStatusChange message to
        notify interested parties about the peer's state.

        @param m The status change message
        @param peer The peer that sent the message
        @param closedLedgerHash The peer's current closed ledger hash
    */
    static void
    publishPeerStatus(
        std::shared_ptr<protocol::TMStatusChange> const& m,
        PeerImp& peer,
        uint256 const& closedLedgerHash);

    StatusChangeMessageHandler() = delete;
};

}  // namespace xrpl

#endif

