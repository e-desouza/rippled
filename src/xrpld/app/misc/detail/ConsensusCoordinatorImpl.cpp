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

#include <xrpld/app/main/Application.h>
#include <xrpld/app/misc/detail/ConsensusCoordinatorImpl.h>

#include <stdexcept>

namespace xrpl {

ConsensusCoordinatorImpl::ConsensusCoordinatorImpl(Application& app) : app_(app)
{
    // Constructor - actual initialization will be added
    // when wiring together the NetworkOPs split components.
    // Will need: mConsensus, mLastConsensusPhase from NetworkOPsImp
}

bool
ConsensusCoordinatorImpl::processTrustedProposal(RCLCxPeerPos peerPos)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will extract logic from NetworkOPsImp (consensus proposal handling).
    (void)peerPos;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp consensus proposal handling");
}

bool
ConsensusCoordinatorImpl::recvValidation(
    std::shared_ptr<STValidation> const& val,
    std::string const& source)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will extract logic from NetworkOPsImp (validation receiving).
    (void)val;
    (void)source;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp validation handling");
}

void
ConsensusCoordinatorImpl::mapComplete(
    std::shared_ptr<SHAMap> const& map,
    bool fromAcquire)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will extract logic from NetworkOPsImp::mapComplete().
    (void)map;
    (void)fromAcquire;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp mapComplete");
}

bool
ConsensusCoordinatorImpl::beginConsensus(
    uint256 const& netLCL,
    std::unique_ptr<std::stringstream> const& clog)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will extract logic from NetworkOPsImp::beginConsensus()
    // (NetworkOPs.cpp lines ~1868-2100).
    (void)netLCL;
    (void)clog;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 1868-2100");
}

void
ConsensusCoordinatorImpl::endConsensus(
    std::unique_ptr<std::stringstream> const& clog)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will extract logic from NetworkOPsImp::endConsensus()
    // (NetworkOPs.cpp lines ~2100-2195).
    (void)clog;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 2100-2195");
}

void
ConsensusCoordinatorImpl::setStateTimer()
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will extract logic from NetworkOPsImp::setStateTimer().
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp setStateTimer");
}

void
ConsensusCoordinatorImpl::consensusViewChange()
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will extract logic from NetworkOPsImp::consensusViewChange().
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp consensusViewChange");
}

Json::Value
ConsensusCoordinatorImpl::getConsensusInfo()
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will extract logic from NetworkOPsImp::getConsensusInfo().
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp getConsensusInfo");
}

std::uint32_t
ConsensusCoordinatorImpl::acceptLedger(
    std::optional<std::chrono::milliseconds> consensusDelay)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will extract logic from NetworkOPsImp::acceptLedger()
    // (NetworkOPs.cpp lines ~2100-2195).
    (void)consensusDelay;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 2100-2195");
}

void
ConsensusCoordinatorImpl::reportFeeChange()
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will extract logic from NetworkOPsImp::reportFeeChange().
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp reportFeeChange");
}

}  // namespace xrpl
