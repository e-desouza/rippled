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

#ifndef XRPL_APP_MISC_ITRANSACTIONPROCESSOR_H_INCLUDED
#define XRPL_APP_MISC_ITRANSACTIONPROCESSOR_H_INCLUDED

#include <xrpld/app/misc/NetworkOPs.h>  // For FailHard enum

#include <cstddef>
#include <memory>

namespace xrpl {

// Forward declarations
class CanonicalTXSet;
class ReadView;
class STTx;
class Transaction;

/**
 * @brief Interface for transaction processing operations.
 *
 * This interface encapsulates the transaction processing portion of the
 * former NetworkOPs god object. It handles transaction submission,
 * processing, and local transaction management.
 *
 * Thread-safe: All implementations must be thread-safe. Transaction
 * processing may occur from multiple threads concurrently.
 *
 * @see NetworkOPs (legacy facade)
 * @see Transaction
 * @see CanonicalTXSet
 */
class ITransactionProcessor
{
public:
    virtual ~ITransactionProcessor() = default;

    //--------------------------------------------------------------------------
    // Transaction Submission
    //--------------------------------------------------------------------------

    /**
     * @brief Submit a signed transaction to the network.
     *
     * Queues the transaction for processing. The transaction will be
     * validated and, if valid, broadcast to peers.
     *
     * @param tx The signed transaction to submit.
     */
    virtual void
    submitTransaction(std::shared_ptr<STTx const> const& tx) = 0;

    //--------------------------------------------------------------------------
    // Transaction Processing
    //--------------------------------------------------------------------------

    /**
     * @brief Process a transaction as it arrives from the network or client.
     *
     * Local transactions are processed synchronously. Network transactions
     * may be queued for later processing.
     *
     * @param transaction The transaction object to process.
     * @param bUnlimited  Whether a privileged client connection submitted it.
     * @param bLocal      True if this is a client submission (local).
     * @param failType    Fail-hard setting from transaction submission.
     */
    virtual void
    processTransaction(
        std::shared_ptr<Transaction>& transaction,
        bool bUnlimited,
        bool bLocal,
        NetworkOPs::FailHard failType) = 0;

    /**
     * @brief Process a set of transactions synchronously as a batch.
     *
     * Ensures that all transactions in the set are processed together
     * in a single batch operation.
     *
     * @param set The canonical transaction set to process.
     */
    virtual void
    processTransactionSet(CanonicalTXSet const& set) = 0;

    //--------------------------------------------------------------------------
    // Local Transaction Management
    //--------------------------------------------------------------------------

    /**
     * @brief Update local transactions based on a new validated ledger.
     *
     * Removes transactions that have been included in the validated
     * ledger and updates the status of pending transactions.
     *
     * @param newValidLedger The newly validated ledger to check against.
     */
    virtual void
    updateLocalTx(ReadView const& newValidLedger) = 0;

    /**
     * @brief Get the count of local transactions awaiting validation.
     *
     * @return The number of local transactions pending validation.
     */
    virtual std::size_t
    getLocalTxCount() = 0;
};

}  // namespace xrpl

#endif
