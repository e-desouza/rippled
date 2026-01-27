#ifndef XRPL_PROTOCOL_TXDETAILS_H_INCLUDED
#define XRPL_PROTOCOL_TXDETAILS_H_INCLUDED

#include <xrpl/protocol/AccountID.h>
#include <xrpl/protocol/Protocol.h>
#include <xrpl/protocol/STTx.h>
#include <xrpl/protocol/SeqProxy.h>
#include <xrpl/protocol/TER.h>
#include <xrpl/protocol/TxConsequences.h>
#include <xrpl/protocol/Units.h>

#include <memory>
#include <optional>

namespace xrpl {

/**
    Structure that describes a transaction in the queue
    waiting to be applied to the current open ledger.
    A collection of these is returned by @ref TxQ::getTxs.
*/
struct TxDetails
{
    /// Full initialization
    TxDetails(
        FeeLevel64 feeLevel_,
        std::optional<LedgerIndex> const& lastValid_,
        TxConsequences const& consequences_,
        AccountID const& account_,
        SeqProxy seqProxy_,
        std::shared_ptr<STTx const> const& txn_,
        int retriesRemaining_,
        TER preflightResult_,
        std::optional<TER> lastResult_)
        : feeLevel(feeLevel_)
        , lastValid(lastValid_)
        , consequences(consequences_)
        , account(account_)
        , seqProxy(seqProxy_)
        , txn(txn_)
        , retriesRemaining(retriesRemaining_)
        , preflightResult(preflightResult_)
        , lastResult(lastResult_)
    {
    }

    /// Fee level of the queued transaction
    FeeLevel64 feeLevel;
    /// LastValidLedger field of the queued transaction, if any
    std::optional<LedgerIndex> lastValid;
    /** Potential @ref TxConsequences of applying the queued transaction
        to the open ledger.
    */
    TxConsequences consequences;
    /// The account the transaction is queued for
    AccountID account;
    /// SeqProxy of the transaction
    SeqProxy seqProxy;
    /// The full transaction
    std::shared_ptr<STTx const> txn;
    /** Number of times the transactor can return a retry / `ter` result
        when attempting to apply this transaction to the open ledger
        from the queue. If the transactor returns `ter` and no retries are
        left, this transaction will be dropped.
    */
    int retriesRemaining;
    /** The *intermediate* result returned by @ref preflight before
        this transaction was queued, or after it is queued, but before
        a failed attempt to `apply` it to the open ledger. This will
        usually be `tesSUCCESS`, but there are some edge cases where
        it has another value. Those edge cases are interesting enough
        that this value is made available here. Specifically, if the
        `rules` change between attempts, `preflight` will be run again
        in `TxQ::MaybeTx::apply`.
    */
    TER preflightResult;
    /** If the transactor attempted to apply the transaction to the open
        ledger from the queue and *failed*, then this is the transactor
        result from the last attempt. Should never be a `tec`, `tef`,
        `tem`, or `tesSUCCESS`, because those results cause the
        transaction to be removed from the queue.
    */
    std::optional<TER> lastResult;
};

}  // namespace xrpl

#endif

