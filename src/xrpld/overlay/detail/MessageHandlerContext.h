#ifndef XRPL_OVERLAY_MESSAGEHANDLERCONTEXT_H_INCLUDED
#define XRPL_OVERLAY_MESSAGEHANDLERCONTEXT_H_INCLUDED

#include <xrpl/beast/utility/Journal.h>
#include <xrpl/resource/Fees.h>

#include <functional>
#include <string>

namespace xrpl {

class Application;
class OverlayImpl;
class PeerImp;

/**
 * @brief Context passed to message handlers for processing protocol messages.
 *
 * This struct provides handlers with access to all dependencies needed
 * to process incoming protocol messages. It uses references to avoid
 * copying and maintains the lifetime guarantees of the parent PeerImp.
 *
 * Thread Safety:
 * - All handlers are called on the PeerImp strand
 * - Context members are only accessed during message processing
 */
struct MessageHandlerContext
{
    /// The application instance
    Application& app;

    /// The overlay implementation for relay and peer operations
    OverlayImpl& overlay;

    /// The peer that received this message (provides access to peer operations)
    PeerImp& peer;

    /// Journal for protocol-level logging
    beast::Journal const journal;

    /// Callback to charge the peer for resource usage
    std::function<void(Resource::Charge const&, std::string const&)> charge;

    MessageHandlerContext(
        Application& app_,
        OverlayImpl& overlay_,
        PeerImp& peer_,
        beast::Journal const& journal_,
        std::function<void(Resource::Charge const&, std::string const&)>
            charge_)
        : app(app_)
        , overlay(overlay_)
        , peer(peer_)
        , journal(journal_)
        , charge(std::move(charge_))
    {
    }

    // Disable copy/move - context is always passed by reference
    MessageHandlerContext(MessageHandlerContext const&) = delete;
    MessageHandlerContext&
    operator=(MessageHandlerContext const&) = delete;
    MessageHandlerContext(MessageHandlerContext&&) = delete;
    MessageHandlerContext&
    operator=(MessageHandlerContext&&) = delete;
};

}  // namespace xrpl

#endif
