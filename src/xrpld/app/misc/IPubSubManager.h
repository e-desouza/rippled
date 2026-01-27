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

#ifndef XRPL_APP_MISC_IPUBSUBMANAGER_H_INCLUDED
#define XRPL_APP_MISC_IPUBSUBMANAGER_H_INCLUDED

#include <xrpl/subscription/InfoSub.h>

#include <xrpl/json/json_value.h>
#include <xrpl/ledger/ReadView.h>
#include <xrpl/protocol/STTx.h>
#include <xrpl/protocol/STValidation.h>
#include <xrpl/protocol/TER.h>

#include <memory>

namespace xrpl {

/**
 * @brief Interface for publish/subscribe management.
 *
 * This interface encapsulates the pub/sub functionality from the former
 * NetworkOPs god object. It extends InfoSub::Source to provide the
 * subscription management methods (subAccount, unsubAccount, subBook,
 * unsubBook, etc.) and adds ledger/transaction/validation publishing.
 *
 * Thread-safe: All implementations must be thread-safe.
 *
 * @see InfoSub::Source (base class with subscription management)
 * @see NetworkOPs (legacy facade)
 */
class IPubSubManager : public InfoSub::Source
{
public:
    ~IPubSubManager() override = default;

    //--------------------------------------------------------------------------
    // Publishing Methods
    //--------------------------------------------------------------------------

    /**
     * @brief Publish a validated ledger to subscribers.
     *
     * Notifies all ledger subscribers about a newly validated ledger.
     * This triggers notifications for ledger stream subscribers and
     * account-specific subscriptions.
     *
     * @param lpAccepted The validated ledger to publish.
     */
    virtual void
    pubLedger(std::shared_ptr<ReadView const> const& lpAccepted) = 0;

    /**
     * @brief Publish a proposed transaction to subscribers.
     *
     * Notifies real-time transaction subscribers about a transaction
     * that has been proposed (but not yet validated). Used for the
     * "transactions_proposed" subscription stream.
     *
     * @param ledger The ledger context for the transaction.
     * @param transaction The proposed transaction.
     * @param result The transaction engine result code.
     */
    virtual void
    pubProposedTransaction(
        std::shared_ptr<ReadView const> const& ledger,
        std::shared_ptr<STTx const> const& transaction,
        TER result) = 0;

    /**
     * @brief Publish a validation to subscribers.
     *
     * Notifies validation stream subscribers about a new validation
     * received from a validator.
     *
     * @param val The validation to publish.
     */
    virtual void
    pubValidation(std::shared_ptr<STValidation> const& val) = 0;

    //--------------------------------------------------------------------------
    // Accounting/Statistics
    //--------------------------------------------------------------------------

    /**
     * @brief Add state accounting information to a JSON object.
     *
     * Populates the provided JSON object with server state duration
     * statistics, showing how long the server has spent in each
     * operating mode.
     *
     * @param obj The JSON object to populate with state accounting data.
     */
    virtual void
    stateAccounting(Json::Value& obj) = 0;
};

}  // namespace xrpl

#endif
