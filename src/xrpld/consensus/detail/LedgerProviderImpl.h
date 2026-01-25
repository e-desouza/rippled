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

#ifndef XRPLD_CONSENSUS_DETAIL_LEDGERPROVIDERIMPL_H_INCLUDED
#define XRPLD_CONSENSUS_DETAIL_LEDGERPROVIDERIMPL_H_INCLUDED

#include <xrpld/consensus/ILedgerProvider.h>

namespace xrpl {

class Application;
class LedgerMaster;

/**
 * Implementation of ILedgerProvider that delegates to LedgerMaster.
 */
class LedgerProviderImpl final : public ILedgerProvider
{
public:
    explicit LedgerProviderImpl(Application& app);

    std::shared_ptr<Ledger const>
    getLedgerByHash(LedgerHash const& hash) override;

    bool
    getFullValidatedRange(std::uint32_t& min, std::uint32_t& max) override;

    LedgerIndex
    getEarliestFetch() override;

    LedgerIndex
    getValidLedgerIndex() override;

    std::shared_ptr<Ledger const>
    getValidatedLedger() override;

    bool
    haveValidated() const override;

    bool
    isCompatible(
        ReadView const& view,
        beast::Journal::Stream s,
        char const* reason) override;

    void
    applyHeldTransactions() override;

    void
    setBuildingLedger(LedgerIndex index) override;

    bool
    storeLedger(std::shared_ptr<Ledger const> ledger) override;

    void
    switchLCL(std::shared_ptr<Ledger const> ledger) override;

    void
    consensusBuilt(
        std::shared_ptr<Ledger const> ledger,
        uint256 const& consensusHash,
        Json::Value consensusJson) override;

    std::unique_ptr<LedgerReplay>
    releaseReplay() override;

private:
    Application& app_;
    LedgerMaster& ledgerMaster_;
};

}  // namespace xrpl

#endif
