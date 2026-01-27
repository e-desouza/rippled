#ifndef XRPL_TX_APPLYSTEPS_H_INCLUDED
#define XRPL_TX_APPLYSTEPS_H_INCLUDED

#include <xrpl/beast/utility/Journal.h>
#include <xrpl/ledger/ApplyViewImpl.h>
#include <xrpl/protocol/TxConsequences.h>

namespace xrpl {

class Application;
class STTx;
class TxQ;

struct ApplyResult
{
    TER ter;
    bool applied;
    std::optional<TxMeta> metadata;

    ApplyResult(TER t, bool a, std::optional<TxMeta> m = std::nullopt)
        : ter(t), applied(a), metadata(std::move(m))
    {
    }
};

/** Return true if the transaction can claim a fee (tec),
    and the `ApplyFlags` do not allow soft failures.
 */
inline bool
isTecClaimHardFail(TER ter, ApplyFlags flags)
{
    return isTecClaim(ter) && !(flags & tapRETRY);
}

/** Describes the results of the `preflight` check

    @note All members are const to make it more difficult
        to "fake" a result without calling `preflight`.
    @see preflight, preclaim, doApply, apply
*/
struct PreflightResult
{
public:
    /// From the input - the transaction
    STTx const& tx;
    /// From the input - the batch identifier, if part of a batch
    std::optional<uint256 const> const parentBatchId;
    /// From the input - the rules
    Rules const rules;
    /// Consequences of the transaction
    TxConsequences const consequences;
    /// From the input - the flags
    ApplyFlags const flags;
    /// From the input - the journal
    beast::Journal const j;

    /// Intermediate transaction result
    NotTEC const ter;

    /// Constructor
    template <class Context>
    PreflightResult(
        Context const& ctx_,
        std::pair<NotTEC, TxConsequences> const& result)
        : tx(ctx_.tx)
        , parentBatchId(ctx_.parentBatchId)
        , rules(ctx_.rules)
        , consequences(result.second)
        , flags(ctx_.flags)
        , j(ctx_.j)
        , ter(result.first)
    {
    }

    PreflightResult(PreflightResult const&) = default;
    /// Deleted copy assignment operator
    PreflightResult&
    operator=(PreflightResult const&) = delete;
};

/** Describes the results of the `preclaim` check

    @note All members are const to make it more difficult
        to "fake" a result without calling `preclaim`.
    @see preflight, preclaim, doApply, apply
*/
struct PreclaimResult
{
public:
    /// From the input - the ledger view
    ReadView const& view;
    /// From the input - the transaction
    STTx const& tx;
    /// From the input - the batch identifier, if part of a batch
    std::optional<uint256 const> const parentBatchId;
    /// From the input - the flags
    ApplyFlags const flags;
    /// From the input - the journal
    beast::Journal const j;

    /// Intermediate transaction result
    TER const ter;

    /// Success flag - whether the transaction is likely to
    /// claim a fee
    bool const likelyToClaimFee;

    /// Constructor
    template <class Context>
    PreclaimResult(Context const& ctx_, TER ter_)
        : view(ctx_.view)
        , tx(ctx_.tx)
        , parentBatchId(ctx_.parentBatchId)
        , flags(ctx_.flags)
        , j(ctx_.j)
        , ter(ter_)
        , likelyToClaimFee(ter == tesSUCCESS || isTecClaimHardFail(ter, flags))
    {
    }

    PreclaimResult(PreclaimResult const&) = default;
    /// Deleted copy assignment operator
    PreclaimResult&
    operator=(PreclaimResult const&) = delete;
};

/** Gate a transaction based on static information.

    The transaction is checked against all possible
    validity constraints that do not require a ledger.

    @param app The current running `Application`.
    @param rules The `Rules` in effect at the time of the check.
    @param tx The transaction to be checked.
    @param flags `ApplyFlags` describing processing options.
    @param j A journal.

    @see PreflightResult, preclaim, doApply, apply

    @return A `PreflightResult` object containing, among
    other things, the `TER` code.
*/
/** @{ */
PreflightResult
preflight(
    Application& app,
    Rules const& rules,
    STTx const& tx,
    ApplyFlags flags,
    beast::Journal j);

PreflightResult
preflight(
    Application& app,
    Rules const& rules,
    uint256 const& parentBatchId,
    STTx const& tx,
    ApplyFlags flags,
    beast::Journal j);
/** @} */

/** Gate a transaction based on static ledger information.

    The transaction is checked against all possible
    validity constraints that DO require a ledger.

    If preclaim succeeds, then the transaction is very
    likely to claim a fee. This will determine if the
    transaction is safe to relay without being applied
    to the open ledger.

    "Succeeds" in this case is defined as returning a
    `tes` or `tec`, since both lead to claiming a fee.

    @pre The transaction has been checked
    and validated using `preflight`

    @param preflightResult The result of a previous
        call to `preflight` for the transaction.
    @param app The current running `Application`.
    @param view The open ledger that the transaction
        will attempt to be applied to.

    @see PreclaimResult, preflight, doApply, apply

    @return A `PreclaimResult` object containing, among
    other things the `TER` code and the base fee value for
    this transaction.
*/
PreclaimResult
preclaim(
    PreflightResult const& preflightResult,
    Application& app,
    OpenView const& view);

/** Compute only the expected base fee for a transaction.

    Base fees are transaction specific, so any calculation
    needing them must get the base fee for each transaction.

    No validation is done or implied by this function.

    Caller is responsible for handling any exceptions.
    Since none should be thrown, that will usually
    mean terminating.

    @param view The current open ledger.
    @param tx The transaction to be checked.

    @return The base fee.
*/
XRPAmount
calculateBaseFee(ReadView const& view, STTx const& tx);

/** Return the minimum fee that an "ordinary" transaction would pay.

    When computing the FeeLevel for a transaction the TxQ sometimes needs
    the know what an "ordinary" or reference transaction would be required
    to pay.

    @param view The current open ledger.
    @param tx The transaction so the correct multisigner count is used.

    @return The base fee in XRPAmount.
*/
XRPAmount
calculateDefaultBaseFee(ReadView const& view, STTx const& tx);

/** Apply a prechecked transaction to an OpenView.

    @pre The transaction has been checked
    and validated using `preflight` and `preclaim`

    @param preclaimResult The result of a previous
    call to `preclaim` for the transaction.
    @param app The current running `Application`.
    @param view The open ledger that the transaction
    will attempt to be applied to.

    @see preflight, preclaim, apply

    @return A pair with the `TER` and a `bool` indicating
    whether or not the transaction was applied.
*/
ApplyResult
doApply(PreclaimResult const& preclaimResult, Application& app, OpenView& view);

}  // namespace xrpl

#endif
