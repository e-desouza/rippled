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
#include <xrpld/app/misc/detail/NetworkInfoImpl.h>

#include <stdexcept>

namespace xrpl {

NetworkInfoImpl::NetworkInfoImpl(Application& app) : app_(app)
{
    // Constructor - actual initialization will be added
    // when wiring together the NetworkOPs split components.
}

Json::Value
NetworkInfoImpl::getServerInfo(bool human, bool admin, bool counters)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will extract logic from NetworkOPsImp::getServerInfo()
    // (NetworkOPs.cpp lines ~2400-3000).
    throw std::logic_error(
        "NetworkInfoImpl::getServerInfo not yet implemented");
}

void
NetworkInfoImpl::clearLedgerFetch()
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will delegate to InboundLedgers::clearFailures() or similar.
    throw std::logic_error(
        "NetworkInfoImpl::clearLedgerFetch not yet implemented");
}

Json::Value
NetworkInfoImpl::getLedgerFetchInfo()
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will delegate to InboundLedgers to get fetch info.
    throw std::logic_error(
        "NetworkInfoImpl::getLedgerFetchInfo not yet implemented");
}

Json::Value
NetworkInfoImpl::getOwnerInfo(
    std::shared_ptr<ReadView const> lpLedger,
    AccountID const& account)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will extract logic from NetworkOPsImp::getOwnerInfo()
    // (NetworkOPs.cpp lines ~1700-1810).

    // Suppress unused parameter warnings
    (void)lpLedger;
    (void)account;

    throw std::logic_error("NetworkInfoImpl::getOwnerInfo not yet implemented");
}

void
NetworkInfoImpl::getBookPage(
    std::shared_ptr<ReadView const>& lpLedger,
    Book const& book,
    AccountID const& uTakerID,
    bool const bProof,
    unsigned int iLimit,
    Json::Value const& jvMarker,
    Json::Value& jvResult)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will extract logic from NetworkOPsImp::getBookPage()
    // (NetworkOPs.cpp lines ~4647-4894).

    // Suppress unused parameter warnings
    (void)lpLedger;
    (void)book;
    (void)uTakerID;
    (void)bProof;
    (void)iLimit;
    (void)jvMarker;
    (void)jvResult;

    throw std::logic_error("NetworkInfoImpl::getBookPage not yet implemented");
}

}  // namespace xrpl
