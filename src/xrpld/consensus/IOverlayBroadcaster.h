//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012-2026 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef XRPL_CONSENSUS_IOVERLAY_BROADCASTER_H_INCLUDED
#define XRPL_CONSENSUS_IOVERLAY_BROADCASTER_H_INCLUDED

#include <xrpl/basics/base_uint.h>
#include <xrpl/protocol/PublicKey.h>
#include <xrpl/protocol/messages.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <set>

namespace ripple {
class Peer;
}  // namespace ripple

namespace xrpl {

/// Peer identifier type (matches Peer::id_t)
using PeerId = std::uint32_t;

/**
 * @brief Interface for broadcasting and relaying consensus messages to peers.
 *
 * This interface abstracts the overlay network's broadcast and relay
 * capabilities needed by the consensus system. It enables consensus
 * components to send proposals, validations, and transactions to the
 * network without directly depending on the full Overlay implementation.
 *
 * Thread Safety:
 * - All implementations must be thread-safe
 * - Methods may be called concurrently from multiple threads
 *
 * @see Overlay (concrete implementation)
 * @see ConsensusAdaptor
 */
class IOverlayBroadcaster
{
public:
    virtual ~IOverlayBroadcaster() = default;

    //--------------------------------------------------------------------------
    // Broadcast Methods
    //--------------------------------------------------------------------------

    /**
     * @brief Broadcast a proposal to all connected peers.
     *
     * Sends the proposal message to all active peers in the network.
     * Used when this node creates a new proposal for consensus.
     *
     * @param m The proposal message to broadcast
     */
    virtual void
    broadcast(protocol::TMProposeSet const& m) = 0;

    /**
     * @brief Broadcast a validation to all connected peers.
     *
     * Sends the validation message to all active peers in the network.
     * Used when this node creates a validation for a closed ledger.
     *
     * @param m The validation message to broadcast
     */
    virtual void
    broadcast(protocol::TMValidation const& m) = 0;

    //--------------------------------------------------------------------------
    // Relay Methods
    //--------------------------------------------------------------------------

    /**
     * @brief Relay a proposal to peers that haven't seen it.
     *
     * Propagates a proposal received from another peer to the rest of
     * the network, avoiding sending back to peers that have already
     * seen this proposal.
     *
     * @param m The proposal message to relay
     * @param suppression The unique identifier for suppression tracking
     * @param validator The public key of the validator that issued the proposal
     * @return Set of peer IDs that have already sent us this proposal
     */
    virtual std::set<PeerId>
    relay(
        protocol::TMProposeSet const& m,
        uint256 const& suppression,
        PublicKey const& validator) = 0;

    /**
     * @brief Relay a transaction to peers that haven't seen it.
     *
     * Propagates a transaction to peers in the network, skipping peers
     * that have already received this transaction.
     *
     * @param hash The transaction hash for identification
     * @param m The transaction message to relay
     * @param skip Set of peer IDs to skip (they already have this tx)
     */
    virtual void
    relay(
        uint256 const& hash,
        protocol::TMTransaction const& m,
        std::set<PeerId> const& skip) = 0;

    //--------------------------------------------------------------------------
    // Peer Iteration
    //--------------------------------------------------------------------------

    /**
     * @brief Iterate over all active peers.
     *
     * Calls the provided function for each connected peer. Used for
     * sending status notifications and other peer-specific operations.
     *
     * @param f Function to call for each peer
     */
    virtual void
    foreach(std::function<void(std::shared_ptr<ripple::Peer> const&)> f) = 0;
};

}  // namespace xrpl

#endif
