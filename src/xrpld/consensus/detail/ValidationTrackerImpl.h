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

#ifndef XRPLD_CONSENSUS_DETAIL_VALIDATIONTRACKERIMPL_H_INCLUDED
#define XRPLD_CONSENSUS_DETAIL_VALIDATIONTRACKERIMPL_H_INCLUDED

#include <xrpld/consensus/IValidationTracker.h>

namespace xrpl {

class Application;

/**
 * Implementation of IValidationTracker that delegates to RCLValidations.
 */
class ValidationTrackerImpl final : public IValidationTracker
{
public:
    explicit ValidationTrackerImpl(Application& app);

    std::size_t
    numTrustedForLedger(LedgerHash const& hash) const override;

    std::size_t
    getNodesAfter(RCLValidatedLedger const& ledger, LedgerHash const& hash)
        const override;

    uint256
    getPreferred(RCLValidatedLedger const& ledger, LedgerIndex minValidSeq)
        const override;

    bool
    canValidateSeq(LedgerIndex seq) const override;

    std::size_t
    laggards(LedgerIndex seq, hash_set<PublicKey>& trustedKeys) const override;

    Json::Value
    getJsonTrie() const override;

private:
    Application& app_;
    RCLValidations& validations_;
};

}  // namespace xrpl

#endif
