#ifndef XRPL_OVERLAY_ILEDGERREPLAYMSGHANDLER_H_INCLUDED
#define XRPL_OVERLAY_ILEDGERREPLAYMSGHANDLER_H_INCLUDED

#include <xrpl/protocol/messages.h>

#include <memory>

namespace xrpl {

/**
 * Interface for ledger replay message handling.
 *
 * This interface abstracts the LedgerReplayMsgHandler from the app module
 * to break the dependency cycle between overlay and app modules.
 * The overlay module defines this interface, and the app module provides
 * the implementation.
 */
class ILedgerReplayMsgHandler
{
public:
    virtual ~ILedgerReplayMsgHandler() = default;

    /**
     * Process TMProofPathRequest and return TMProofPathResponse
     * @note check has_error() and error() of the response for error
     */
    virtual protocol::TMProofPathResponse
    processProofPathRequest(
        std::shared_ptr<protocol::TMProofPathRequest> const& msg) = 0;

    /**
     * Process TMProofPathResponse
     * @return false if the response message has bad format or bad data;
     *         true otherwise
     */
    virtual bool
    processProofPathResponse(
        std::shared_ptr<protocol::TMProofPathResponse> const& msg) = 0;

    /**
     * Process TMReplayDeltaRequest and return TMReplayDeltaResponse
     * @note check has_error() and error() of the response for error
     */
    virtual protocol::TMReplayDeltaResponse
    processReplayDeltaRequest(
        std::shared_ptr<protocol::TMReplayDeltaRequest> const& msg) = 0;

    /**
     * Process TMReplayDeltaResponse
     * @return false if the response message has bad format or bad data;
     *         true otherwise
     */
    virtual bool
    processReplayDeltaResponse(
        std::shared_ptr<protocol::TMReplayDeltaResponse> const& msg) = 0;
};

/**
 * Factory function type for creating ILedgerReplayMsgHandler instances.
 * Each PeerImp needs its own handler instance.
 */
using LedgerReplayMsgHandlerFactory =
    std::function<std::unique_ptr<ILedgerReplayMsgHandler>()>;

}  // namespace xrpl

#endif
