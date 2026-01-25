//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012-2024 Ripple Labs Inc.

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

#ifndef XRPLD_CONSENSUS_DETAIL_TXSETMANAGERIMPL_H_INCLUDED
#define XRPLD_CONSENSUS_DETAIL_TXSETMANAGERIMPL_H_INCLUDED

#include <xrpld/consensus/ITxSetManager.h>

namespace xrpl {

class Application;
class InboundTransactions;

/**
 * Implementation of ITxSetManager that delegates to InboundTransactions.
 */
class TxSetManagerImpl final : public ITxSetManager
{
public:
    explicit TxSetManagerImpl(Application& app);

    std::shared_ptr<SHAMap>
    getSet(uint256 const& setId, bool acquire) override;

    void
    giveSet(
        uint256 const& setId,
        std::shared_ptr<SHAMap> const& set,
        bool acquired) override;

    void
    newRound(std::uint32_t seq) override;

private:
    Application& app_;
    InboundTransactions& inboundTransactions_;
};

}  // namespace xrpl

#endif
