#ifndef XRPL_OVERLAY_HANDLERS_PROPOSALMESSAGEHANDLER_H_INCLUDED
#define XRPL_OVERLAY_HANDLERS_PROPOSALMESSAGEHANDLER_H_INCLUDED

#include <xrpl/protocol/messages.h>

#include <memory>

namespace xrpl {

class PeerImp;
class RCLCxPeerPos;

/**
 * @brief Handles proposal-related protocol messages.
 *
 * This handler processes the following message types:
 * - TMProposeSet: Consensus proposals from validators
 *
 * The handler is a friend of PeerImp and has access to its internals.
 * All handler methods are static and stateless.
 *
 * Thread Safety:
 * - All handler methods are called on the PeerImp strand
 * - Handler does not maintain any state between calls
 */
class ProposalMessageHandler
{
public:
    /**
     * @brief Process a TMProposeSet message.
     *
     * Validates incoming proposals, checks for duplicates,
     * and schedules signature verification on the job queue.
     *
     * @param m The proposal message
     * @param peer The peer that received this message
     */
    static void
    onMessage(std::shared_ptr<protocol::TMProposeSet> const& m, PeerImp& peer);

    /**
     * @brief Check and process a proposal after initial validation.
     *
     * Called from the job queue after initial processing. Verifies the
     * proposal signature and relays to other peers if valid.
     *
     * @param peer The peer that received this message
     * @param isTrusted Whether the proposal is from a trusted validator
     * @param packet The original protocol message for relaying
     * @param peerPos The deserialized proposal position
     */
    static void
    checkPropose(
        PeerImp& peer,
        bool isTrusted,
        std::shared_ptr<protocol::TMProposeSet> const& packet,
        RCLCxPeerPos peerPos);

    // No state - all methods are static
    ProposalMessageHandler() = delete;
};

}  // namespace xrpl

#endif

