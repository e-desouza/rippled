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

#include <xrpld/app/ledger/LedgerMaster.h>
#include <xrpld/app/main/Application.h>
#include <xrpld/consensus/detail/LedgerProviderImpl.h>

namespace xrpl {

LedgerProviderImpl::LedgerProviderImpl(Application& app)
    : app_(app), ledgerMaster_(app.getLedgerMaster())
{
}

std::shared_ptr<Ledger const>
LedgerProviderImpl::getLedgerByHash(LedgerHash const& hash)
{
    return ledgerMaster_.getLedgerByHash(hash);
}

bool
LedgerProviderImpl::getFullValidatedRange(
    std::uint32_t& min,
    std::uint32_t& max)
{
    return ledgerMaster_.getFullValidatedRange(min, max);
}

LedgerIndex
LedgerProviderImpl::getEarliestFetch()
{
    return ledgerMaster_.getEarliestFetch();
}

LedgerIndex
LedgerProviderImpl::getValidLedgerIndex()
{
    return ledgerMaster_.getValidLedgerIndex();
}

std::shared_ptr<Ledger const>
LedgerProviderImpl::getValidatedLedger()
{
    return ledgerMaster_.getValidatedLedger();
}

bool
LedgerProviderImpl::haveValidated() const
{
    return ledgerMaster_.haveValidated();
}

bool
LedgerProviderImpl::isCompatible(
    ReadView const& view,
    beast::Journal::Stream s,
    char const* reason)
{
    return ledgerMaster_.isCompatible(view, s, reason);
}

void
LedgerProviderImpl::applyHeldTransactions()
{
    ledgerMaster_.applyHeldTransactions();
}

void
LedgerProviderImpl::setBuildingLedger(LedgerIndex index)
{
    ledgerMaster_.setBuildingLedger(index);
}

bool
LedgerProviderImpl::storeLedger(std::shared_ptr<Ledger const> ledger)
{
    return ledgerMaster_.storeLedger(ledger);
}

void
LedgerProviderImpl::switchLCL(std::shared_ptr<Ledger const> ledger)
{
    ledgerMaster_.switchLCL(ledger);
}

void
LedgerProviderImpl::consensusBuilt(
    std::shared_ptr<Ledger const> ledger,
    uint256 const& consensusHash,
    Json::Value consensusJson)
{
    ledgerMaster_.consensusBuilt(
        ledger, consensusHash, std::move(consensusJson));
}

std::unique_ptr<LedgerReplay>
LedgerProviderImpl::releaseReplay()
{
    return ledgerMaster_.releaseReplay();
}

}  // namespace xrpl
