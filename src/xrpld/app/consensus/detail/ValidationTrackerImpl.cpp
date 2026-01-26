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

#include <xrpld/app/consensus/RCLValidations.h>
#include <xrpld/app/main/Application.h>
#include <xrpld/consensus/detail/ValidationTrackerImpl.h>

namespace xrpl {

ValidationTrackerImpl::ValidationTrackerImpl(Application& app)
    : app_(app), validations_(app.getValidations())
{
}

std::size_t
ValidationTrackerImpl::numTrustedForLedger(LedgerHash const& hash) const
{
    return validations_.numTrustedForLedger(hash);
}

std::size_t
ValidationTrackerImpl::getNodesAfter(
    RCLValidatedLedger const& ledger,
    LedgerHash const& hash) const
{
    return validations_.getNodesAfter(ledger, hash);
}

uint256
ValidationTrackerImpl::getPreferred(
    RCLValidatedLedger const& ledger,
    LedgerIndex minValidSeq) const
{
    return validations_.getPreferred(ledger, minValidSeq);
}

bool
ValidationTrackerImpl::canValidateSeq(LedgerIndex seq) const
{
    return validations_.canValidateSeq(seq);
}

std::size_t
ValidationTrackerImpl::laggards(
    LedgerIndex seq,
    hash_set<PublicKey>& trustedKeys) const
{
    return validations_.laggards(seq, trustedKeys);
}

Json::Value
ValidationTrackerImpl::getJsonTrie() const
{
    return validations_.getJsonTrie();
}

}  // namespace xrpl
