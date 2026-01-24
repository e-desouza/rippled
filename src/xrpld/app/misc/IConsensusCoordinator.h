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

#ifndef XRPL_APP_MISC_ICONSENSUSCOORDINATOR_H_INCLUDED
#define XRPL_APP_MISC_ICONSENSUSCOORDINATOR_H_INCLUDED

#include <xrpld/app/consensus/RCLCxPeerPos.h>

#include <xrpl/basics/base_uint.h>
#include <xrpl/json/json_value.h>
#include <xrpl/protocol/STValidation.h>
#include <xrpl/shamap/SHAMap.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

namespace xrpl {

/**
 * @brief Interface for consensus coordination.
 *
 * This interface encapsulates the consensus-related operations of the former
 * NetworkOPs god object. It manages consensus proposals, validations, and
 * the consensus state machine.
 *
 * Thread-safe: All implementations must be thread-safe.
 *
 * @see NetworkOPs (legacy facade)
 * @see RCLConsensus
 */
class IConsensusCoordinator
{
public:
    virtual ~IConsensusCoordinator() = default;

    //--------------------------------------------------------------------------
    // Consensus Proposals and Validations
    //--------------------------------------------------------------------------

    /**
     * @brief Process a trusted consensus proposal from a peer.
     * @param peerPos The peer's proposed position.
     * @return true if the proposal was processed successfully.
     */
    virtual bool
    processTrustedProposal(RCLCxPeerPos peerPos) = 0;

    /**
     * @brief Receive and process a validation from the network.
     * @param val The validation message.
     * @param source Identifier of the validation source.
     * @return true if the validation was processed successfully.
     */
    virtual bool
    recvValidation(
        std::shared_ptr<STValidation> const& val,
        std::string const& source) = 0;

    /**
     * @brief Called when a transaction set map is complete.
     * @param map The completed SHAMap.
     * @param fromAcquire true if the map was acquired from the network.
     */
    virtual void
    mapComplete(std::shared_ptr<SHAMap> const& map, bool fromAcquire) = 0;

    //--------------------------------------------------------------------------
    // Consensus State Machine
    //--------------------------------------------------------------------------

    /**
     * @brief Begin a new consensus round.
     * @param netLCL The network's last closed ledger hash.
     * @param clog Optional consensus logging stream.
     * @return true if consensus was started successfully.
     */
    virtual bool
    beginConsensus(
        uint256 const& netLCL,
        std::unique_ptr<std::stringstream> const& clog) = 0;

    /**
     * @brief End the current consensus round.
     * @param clog Optional consensus logging stream.
     */
    virtual void
    endConsensus(std::unique_ptr<std::stringstream> const& clog) = 0;

    /**
     * @brief Set the state timer for consensus timeouts.
     */
    virtual void
    setStateTimer() = 0;

    /**
     * @brief Handle a consensus view change event.
     */
    virtual void
    consensusViewChange() = 0;

    //--------------------------------------------------------------------------
    // Consensus Information
    //--------------------------------------------------------------------------

    /**
     * @brief Get current consensus status information.
     * @return JSON object containing consensus state details.
     */
    virtual Json::Value
    getConsensusInfo() = 0;

    //--------------------------------------------------------------------------
    // Ledger Acceptance
    //--------------------------------------------------------------------------

    /**
     * @brief Accept the current transaction tree and close the ledger.
     *
     * This API is primarily used via RPC with the server in STANDALONE mode
     * and performs a virtual consensus round, with all proposed transactions
     * being accepted.
     *
     * @param consensusDelay Optional delay to simulate consensus timing.
     * @return The new ledger's sequence number.
     */
    virtual std::uint32_t
    acceptLedger(
        std::optional<std::chrono::milliseconds> consensusDelay =
            std::nullopt) = 0;

    //--------------------------------------------------------------------------
    // Fee Reporting
    //--------------------------------------------------------------------------

    /**
     * @brief Report a change in transaction fees to connected clients.
     */
    virtual void
    reportFeeChange() = 0;
};

}  // namespace xrpl

#endif
