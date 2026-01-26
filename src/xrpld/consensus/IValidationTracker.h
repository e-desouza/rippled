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

#ifndef XRPL_CONSENSUS_IVALIDATIONTRACKER_H_INCLUDED
#define XRPL_CONSENSUS_IVALIDATIONTRACKER_H_INCLUDED

#include <xrpld/consensus/RCLValidationsFwd.h>

#include <xrpl/basics/UnorderedContainers.h>
#include <xrpl/basics/base_uint.h>
#include <xrpl/json/json_value.h>
#include <xrpl/protocol/Protocol.h>
#include <xrpl/protocol/PublicKey.h>

#include <cstddef>

namespace xrpl {

/**
 * @brief Interface for validation tracking operations.
 *
 * This interface encapsulates the validation tracking portion of the
 * consensus adaptor. It provides methods for querying validation counts,
 * determining preferred ledgers based on validation trie, and checking
 * validation eligibility.
 *
 * Thread-safe: All implementations must be thread-safe.
 *
 * @see RCLValidations
 * @see Validations
 */
class IValidationTracker
{
public:
    virtual ~IValidationTracker() = default;

    /**
     * @brief Get the number of trusted validations for a given ledger hash.
     *
     * @param ledgerHash The hash of the ledger to query.
     * @return The number of trusted validations for the specified ledger.
     */
    virtual std::size_t
    numTrustedForLedger(uint256 const& ledgerHash) const = 0;

    /**
     * @brief Get the count of nodes validating ledgers after the given one.
     *
     * @param ledger The reference ledger to check against.
     * @param ledgerHash The hash of the ledger.
     * @return The number of validating nodes with ledgers after the given one.
     */
    virtual std::size_t
    getNodesAfter(RCLValidatedLedger const& ledger, uint256 const& ledgerHash)
        const = 0;

    /**
     * @brief Get the preferred ledger hash based on validation trie.
     *
     * Determines the preferred working ledger based on the current
     * validation trie state and the given constraints.
     *
     * @param currentLedger The current validated ledger for context.
     * @param minValidSeq The minimum valid sequence number to consider.
     * @return The hash of the preferred ledger.
     */
    virtual uint256
    getPreferred(
        RCLValidatedLedger const& currentLedger,
        LedgerIndex minValidSeq) const = 0;

    /**
     * @brief Check if this node can validate the given ledger sequence.
     *
     * Determines whether this node is eligible to issue a validation
     * for the specified ledger sequence number based on prior validations.
     *
     * @param seq The ledger sequence number to check.
     * @return true if this node can validate the sequence, false otherwise.
     */
    virtual bool
    canValidateSeq(LedgerIndex seq) const = 0;

    /**
     * @brief Get validators that are lagging behind.
     *
     * Identifies trusted validators that have not yet validated up to
     * the specified sequence number.
     *
     * @param seq The target sequence number.
     * @param laggardKeys Output set to receive the public keys of laggards.
     * @return The number of lagging validators.
     */
    virtual std::size_t
    laggards(LedgerIndex seq, hash_set<PublicKey>& laggardKeys) const = 0;

    /**
     * @brief Get JSON representation of the validation trie.
     *
     * Returns diagnostic information about the current state of the
     * validation trie for debugging and monitoring purposes.
     *
     * @return JSON object containing trie state information.
     */
    virtual Json::Value
    getJsonTrie() const = 0;
};

}  // namespace xrpl

#endif
