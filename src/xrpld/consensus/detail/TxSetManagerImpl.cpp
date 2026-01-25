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

#include <xrpld/app/ledger/InboundTransactions.h>
#include <xrpld/app/main/Application.h>
#include <xrpld/consensus/detail/TxSetManagerImpl.h>

namespace xrpl {

TxSetManagerImpl::TxSetManagerImpl(Application& app)
    : app_(app), inboundTransactions_(app.getInboundTransactions())
{
}

std::shared_ptr<SHAMap>
TxSetManagerImpl::getSet(uint256 const& setId, bool acquire)
{
    return inboundTransactions_.getSet(setId, acquire);
}

void
TxSetManagerImpl::giveSet(
    uint256 const& setId,
    std::shared_ptr<SHAMap> const& set,
    bool acquired)
{
    inboundTransactions_.giveSet(setId, set, acquired);
}

void
TxSetManagerImpl::newRound(std::uint32_t seq)
{
    inboundTransactions_.newRound(seq);
}

}  // namespace xrpl
