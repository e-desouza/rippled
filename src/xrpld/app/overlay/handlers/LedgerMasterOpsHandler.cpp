//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2024 Ripple Labs Inc.

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
#include <xrpld/overlay/detail/handlers/LedgerMasterOpsHandler.h>

namespace xrpl {

class LedgerMasterOpsHandler : public ILedgerMasterOps
{
private:
    Application& app_;

public:
    explicit LedgerMasterOpsHandler(Application& app) : app_(app)
    {
    }

    std::chrono::seconds
    getValidatedLedgerAge() override
    {
        return app_.getLedgerMaster().getValidatedLedgerAge();
    }

    LedgerIndex
    getValidLedgerIndex() override
    {
        return app_.getLedgerMaster().getValidLedgerIndex();
    }

    bool
    haveLedger(std::uint32_t seq) override
    {
        return app_.getLedgerMaster().haveLedger(seq);
    }

    void
    addFetchPack(uint256 const& hash, std::shared_ptr<Blob> data) override
    {
        app_.getLedgerMaster().addFetchPack(hash, std::move(data));
    }

    void
    gotFetchPack(bool progress, std::uint32_t seq) override
    {
        app_.getLedgerMaster().gotFetchPack(progress, seq);
    }

    void
    makeFetchPack(
        std::weak_ptr<Peer> const& wPeer,
        std::shared_ptr<TMGetObjectByHash> const& request,
        uint256 haveLedgerHash,
        UptimeClock::time_point uptime) override
    {
        app_.getLedgerMaster().makeFetchPack(
            wPeer, request, haveLedgerHash, uptime);
    }

    std::shared_ptr<Ledger const>
    getLedgerByHash(uint256 const& hash) override
    {
        return app_.getLedgerMaster().getLedgerByHash(hash);
    }

    std::uint32_t
    getEarliestFetch() override
    {
        return app_.getLedgerMaster().getEarliestFetch();
    }

    std::shared_ptr<Ledger const>
    getLedgerBySeq(std::uint32_t index) override
    {
        return app_.getLedgerMaster().getLedgerBySeq(index);
    }

    std::shared_ptr<Ledger const>
    getClosedLedger() override
    {
        return app_.getLedgerMaster().getClosedLedger();
    }
};

std::unique_ptr<ILedgerMasterOps>
make_LedgerMasterOpsHandler(Application& app)
{
    return std::make_unique<LedgerMasterOpsHandler>(app);
}

}  // namespace xrpl
