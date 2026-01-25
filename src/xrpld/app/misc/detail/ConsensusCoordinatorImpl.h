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

#ifndef XRPL_APP_MISC_CONSENSUSCOORDINATORIMPL_H_INCLUDED
#define XRPL_APP_MISC_CONSENSUSCOORDINATORIMPL_H_INCLUDED

#include <xrpld/app/misc/IConsensusCoordinator.h>

namespace xrpl {

class Application;

/**
 * @brief Implementation of the IConsensusCoordinator interface.
 *
 * This class provides consensus coordination functionality, including
 * managing consensus proposals, validations, the consensus state machine,
 * and ledger acceptance.
 *
 * This is a stub implementation that will be completed when wiring
 * together the NetworkOPs split components.
 *
 * Thread-safe: All methods are thread-safe.
 */
class ConsensusCoordinatorImpl final : public IConsensusCoordinator
{
public:
    /**
     * @brief Construct a ConsensusCoordinatorImpl.
     *
     * @param app Reference to the Application instance.
     */
    explicit ConsensusCoordinatorImpl(Application& app);

    ~ConsensusCoordinatorImpl() override = default;

    // Non-copyable, non-movable
    ConsensusCoordinatorImpl(ConsensusCoordinatorImpl const&) = delete;
    ConsensusCoordinatorImpl&
    operator=(ConsensusCoordinatorImpl const&) = delete;
    ConsensusCoordinatorImpl(ConsensusCoordinatorImpl&&) = delete;
    ConsensusCoordinatorImpl&
    operator=(ConsensusCoordinatorImpl&&) = delete;

    //--------------------------------------------------------------------------
    // IConsensusCoordinator interface implementation
    //--------------------------------------------------------------------------

    // Consensus Proposals and Validations
    bool
    processTrustedProposal(RCLCxPeerPos peerPos) override;

    bool
    recvValidation(
        std::shared_ptr<STValidation> const& val,
        std::string const& source) override;

    void
    mapComplete(std::shared_ptr<SHAMap> const& map, bool fromAcquire) override;

    // Consensus State Machine
    bool
    beginConsensus(
        uint256 const& netLCL,
        std::unique_ptr<std::stringstream> const& clog) override;

    void
    endConsensus(std::unique_ptr<std::stringstream> const& clog) override;

    void
    setStateTimer() override;

    void
    consensusViewChange() override;

    // Consensus Information
    Json::Value
    getConsensusInfo() override;

    // Ledger Acceptance
    std::uint32_t
    acceptLedger(
        std::optional<std::chrono::milliseconds> consensusDelay =
            std::nullopt) override;

    // Fee Reporting
    void
    reportFeeChange() override;

private:
    Application& app_;
};

}  // namespace xrpl

#endif
