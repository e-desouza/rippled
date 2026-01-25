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

#ifndef XRPL_APP_MISC_TRANSACTIONPROCESSORIMPL_H_INCLUDED
#define XRPL_APP_MISC_TRANSACTIONPROCESSORIMPL_H_INCLUDED

#include <xrpld/app/misc/ITransactionProcessor.h>

namespace xrpl {

class Application;

/**
 * @brief Implementation of the ITransactionProcessor interface.
 *
 * This class provides transaction processing functionality including
 * transaction submission, processing, and local transaction management.
 *
 * This is a stub implementation that will be completed when wiring
 * together the NetworkOPs split components.
 *
 * Thread-safe: All methods are thread-safe.
 *
 * @note When implementing, extract logic from NetworkOPs.cpp:
 *       - Lines 1200-1500: processTransaction(), preprocessing
 *       - Lines 1500-1650: transactionBatch(), apply()
 *       - Lines 2195-2400: Ledger acceptance transaction handling
 *       - Inner class: TransactionStatus
 *       - Variables: mMutex, mCond, mDispatchState, mTransactions, m_localTX
 */
class TransactionProcessorImpl final : public ITransactionProcessor
{
public:
    /**
     * @brief Construct a TransactionProcessorImpl.
     *
     * @param app Reference to the Application instance.
     */
    explicit TransactionProcessorImpl(Application& app);

    ~TransactionProcessorImpl() override = default;

    // Non-copyable, non-movable
    TransactionProcessorImpl(TransactionProcessorImpl const&) = delete;
    TransactionProcessorImpl&
    operator=(TransactionProcessorImpl const&) = delete;
    TransactionProcessorImpl(TransactionProcessorImpl&&) = delete;
    TransactionProcessorImpl&
    operator=(TransactionProcessorImpl&&) = delete;

    //--------------------------------------------------------------------------
    // ITransactionProcessor interface implementation
    //--------------------------------------------------------------------------

    void
    submitTransaction(std::shared_ptr<STTx const> const& tx) override;

    void
    processTransaction(
        std::shared_ptr<Transaction>& transaction,
        bool bUnlimited,
        bool bLocal,
        NetworkOPs::FailHard failType) override;

    void
    processTransactionSet(CanonicalTXSet const& set) override;

    void
    updateLocalTx(ReadView const& newValidLedger) override;

    std::size_t
    getLocalTxCount() override;

private:
    Application& app_;
};

}  // namespace xrpl

#endif
