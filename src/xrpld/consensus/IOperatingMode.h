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

#ifndef XRPL_CONSENSUS_IOPERATINGMODE_H_INCLUDED
#define XRPL_CONSENSUS_IOPERATINGMODE_H_INCLUDED

#include <xrpld/app/misc/NetworkOPs.h>  // For OperatingMode enum

#include <xrpl/protocol/STValidation.h>

#include <memory>
#include <sstream>

namespace xrpl {

/**
 * @brief Interface for operating mode and consensus event management.
 *
 * This interface encapsulates the operating mode queries and consensus
 * lifecycle events that were previously part of the NetworkOPs god object.
 * It provides methods for querying and setting operating mode, as well as
 * callbacks for consensus events like view changes and validation publishing.
 *
 * Thread-safe: All implementations must be thread-safe.
 *
 * @see NetworkOPs
 * @see OperatingMode
 */
class IOperatingMode
{
public:
    virtual ~IOperatingMode() = default;

    //--------------------------------------------------------------------------
    // Operating Mode Queries
    //--------------------------------------------------------------------------

    /**
     * @brief Get the current operating mode of the server.
     *
     * @return The current OperatingMode value.
     */
    virtual OperatingMode
    getOperatingMode() const = 0;

    /**
     * @brief Check if the server is operating in FULL mode.
     *
     * FULL mode indicates the server is fully synchronized with the network
     * and participating in consensus.
     *
     * @return true if in FULL mode, false otherwise.
     */
    virtual bool
    isFull() const = 0;

    /**
     * @brief Check if the server is blocked.
     *
     * A server may be blocked due to unsupported amendments or UNL issues.
     *
     * @return true if the server is blocked, false otherwise.
     */
    virtual bool
    isBlocked() const = 0;

    /**
     * @brief Set the operating mode.
     *
     * @param mode The new operating mode to set.
     */
    virtual void
    setMode(OperatingMode mode) = 0;

    //--------------------------------------------------------------------------
    // Consensus Event Callbacks
    //--------------------------------------------------------------------------

    /**
     * @brief Notify that a consensus view change has occurred.
     *
     * Called when the consensus round transitions to a new view, typically
     * due to timeout or other consensus state changes.
     */
    virtual void
    consensusViewChange() = 0;

    /**
     * @brief Notify that consensus has ended.
     *
     * Called when a consensus round completes. The provided stream may
     * contain debug or logging information about the round.
     *
     * @param consensusLog Optional stream containing consensus debug info.
     */
    virtual void
    endConsensus(std::unique_ptr<std::stringstream> const& consensusLog) = 0;

    /**
     * @brief Report that transaction fees have changed.
     *
     * Called when the local fee escalation state changes, triggering
     * potential notifications to subscribers.
     */
    virtual void
    reportFeeChange() = 0;

    /**
     * @brief Publish a validation to the network.
     *
     * Called when this node creates a validation for a ledger that
     * should be broadcast to peers and subscribers.
     *
     * @param validation The validation to publish.
     */
    virtual void
    pubValidation(std::shared_ptr<STValidation> const& validation) = 0;
};

}  // namespace xrpl

#endif
