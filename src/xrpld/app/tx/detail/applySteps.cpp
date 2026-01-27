#include <xrpld/app/tx/applySteps.h>
#include <xrpld/app/tx/detail/TransactionRegistry.h>

#include <xrpl/protocol/TxFormats.h>

#include <stdexcept>

namespace xrpl {

namespace {

/**
 * RAII helper to set up global number/rules state for transaction processing.
 *
 * This state management was previously embedded in with_txn_type() and needs
 * to be preserved for correct transaction processing behavior.
 */
struct TransactionRulesScope
{
    std::optional<NumberSO> stNumberSO;
    std::optional<CurrentTransactionRulesGuard> rulesGuard;
    std::optional<NumberMantissaScaleGuard> mantissaScaleGuard;

    explicit TransactionRulesScope(Rules const& rules)
    {
        // These global updates really should have been for every Transaction
        // step: preflight, preclaim, calculateBaseFee, and doApply.
        // Unfortunately, they were only included in doApply (via
        // Transactor::operator()). That may have been sufficient when the
        // changes were only related to operations that mutated data, but some
        // features will now change how they read data, so these need to be
        // more global.
        //
        // To prevent unintentional side effects on existing checks, they will
        // be set for every operation only once SingleAssetVault (or later
        // LendingProtocol) are enabled.
        //
        // See also Transactor::operator().
        if (rules.enabled(featureSingleAssetVault) ||
            rules.enabled(featureLendingProtocol))
        {
            // raii classes for the current ledger rules.
            // fixUniversalNumber predates the rulesGuard and should be
            // replaced.
            stNumberSO.emplace(rules.enabled(fixUniversalNumber));
            rulesGuard.emplace(rules);
        }
        else
        {
            // Without those features enabled, always use the old number rules.
            mantissaScaleGuard.emplace(MantissaRange::small);
        }
    }
};

}  // namespace

static std::pair<NotTEC, TxConsequences>
invoke_preflight(PreflightContext const& ctx)
{
    TransactionRulesScope scope(ctx.rules);

    auto const* handler =
        TransactionRegistry::instance().getHandler(ctx.tx.getTxnType());
    if (!handler)
    {
        // Should never happen - all transaction types should be registered
        // LCOV_EXCL_START
        JLOG(ctx.j.fatal())
            << "Unknown transaction type in preflight: " << ctx.tx.getTxnType();
        UNREACHABLE("xrpl::invoke_preflight : unknown transaction type");
        return {temUNKNOWN, TxConsequences{temUNKNOWN}};
        // LCOV_EXCL_STOP
    }

    auto const tec = handler->preflight(ctx);
    return std::make_pair(
        tec,
        isTesSuccess(tec) ? handler->makeConsequences(ctx)
                          : TxConsequences{tec});
}

static TER
invoke_preclaim(PreclaimContext const& ctx)
{
    TransactionRulesScope scope(ctx.view.rules());

    auto const* handler =
        TransactionRegistry::instance().getHandler(ctx.tx.getTxnType());
    if (!handler)
    {
        // Should never happen
        // LCOV_EXCL_START
        JLOG(ctx.j.fatal())
            << "Unknown transaction type in preclaim: " << ctx.tx.getTxnType();
        UNREACHABLE("xrpl::invoke_preclaim : unknown transaction type");
        return temUNKNOWN;
        // LCOV_EXCL_STOP
    }

    return handler->preclaim(ctx, calculateBaseFee(ctx.view, ctx.tx));
}

/**
 * @brief Calculates the base fee for a given transaction.
 *
 * This function determines the base fee required for the specified transaction
 * by invoking the appropriate fee calculation logic based on the transaction
 * type. It uses the transaction registry for runtime dispatch.
 *
 * @param view The ledger view to use for fee calculation.
 * @param tx The transaction for which the base fee is to be calculated.
 * @return The calculated base fee as an XRPAmount.
 */
static XRPAmount
invoke_calculateBaseFee(ReadView const& view, STTx const& tx)
{
    auto const* handler =
        TransactionRegistry::instance().getHandler(tx.getTxnType());
    if (!handler)
    {
        // LCOV_EXCL_START
        UNREACHABLE("xrpl::invoke_calculateBaseFee : unknown transaction type");
        return XRPAmount{0};
        // LCOV_EXCL_STOP
    }

    return handler->calculateBaseFee(view, tx);
}

static ApplyResult
invoke_apply(ApplyContext& ctx)
{
    TransactionRulesScope scope(ctx.view().rules());

    auto const* handler =
        TransactionRegistry::instance().getHandler(ctx.tx.getTxnType());
    if (!handler)
    {
        // Should never happen
        // LCOV_EXCL_START
        JLOG(ctx.journal.fatal())
            << "Unknown transaction type in apply: " << ctx.tx.getTxnType();
        UNREACHABLE("xrpl::invoke_apply : unknown transaction type");
        return {temUNKNOWN, false};
        // LCOV_EXCL_STOP
    }

    return handler->doApply(ctx);
}

PreflightResult
preflight(
    Application& app,
    Rules const& rules,
    STTx const& tx,
    ApplyFlags flags,
    beast::Journal j)
{
    PreflightContext const pfCtx(app, tx, rules, flags, j);
    try
    {
        return {pfCtx, invoke_preflight(pfCtx)};
    }
    catch (std::exception const& e)
    {
        JLOG(j.fatal()) << "apply (preflight): " << e.what();
        return {pfCtx, {tefEXCEPTION, TxConsequences{tx}}};
    }
}

PreflightResult
preflight(
    Application& app,
    Rules const& rules,
    uint256 const& parentBatchId,
    STTx const& tx,
    ApplyFlags flags,
    beast::Journal j)
{
    PreflightContext const pfCtx(app, tx, parentBatchId, rules, flags, j);
    try
    {
        return {pfCtx, invoke_preflight(pfCtx)};
    }
    catch (std::exception const& e)
    {
        JLOG(j.fatal()) << "apply (preflight): " << e.what();
        return {pfCtx, {tefEXCEPTION, TxConsequences{tx}}};
    }
}

PreclaimResult
preclaim(
    PreflightResult const& preflightResult,
    Application& app,
    OpenView const& view)
{
    std::optional<PreclaimContext const> ctx;
    if (preflightResult.rules != view.rules())
    {
        auto secondFlight = [&]() {
            if (preflightResult.parentBatchId)
                return preflight(
                    app,
                    view.rules(),
                    preflightResult.parentBatchId.value(),
                    preflightResult.tx,
                    preflightResult.flags,
                    preflightResult.j);

            return preflight(
                app,
                view.rules(),
                preflightResult.tx,
                preflightResult.flags,
                preflightResult.j);
        }();

        ctx.emplace(
            app,
            view,
            secondFlight.ter,
            secondFlight.tx,
            secondFlight.flags,
            secondFlight.parentBatchId,
            secondFlight.j);
    }
    else
    {
        ctx.emplace(
            app,
            view,
            preflightResult.ter,
            preflightResult.tx,
            preflightResult.flags,
            preflightResult.parentBatchId,
            preflightResult.j);
    }

    try
    {
        if (ctx->preflightResult != tesSUCCESS)
            return {*ctx, ctx->preflightResult};
        return {*ctx, invoke_preclaim(*ctx)};
    }
    catch (std::exception const& e)
    {
        JLOG(ctx->j.fatal()) << "apply (preclaim): " << e.what();
        return {*ctx, tefEXCEPTION};
    }
}

XRPAmount
calculateBaseFee(ReadView const& view, STTx const& tx)
{
    return invoke_calculateBaseFee(view, tx);
}

XRPAmount
calculateDefaultBaseFee(ReadView const& view, STTx const& tx)
{
    return Transactor::calculateBaseFee(view, tx);
}

ApplyResult
doApply(PreclaimResult const& preclaimResult, Application& app, OpenView& view)
{
    if (preclaimResult.view.seq() != view.seq())
    {
        // Logic error from the caller. Don't have enough
        // info to recover.
        return {tefEXCEPTION, false};
    }
    try
    {
        if (!preclaimResult.likelyToClaimFee)
            return {preclaimResult.ter, false};
        ApplyContext ctx(
            app,
            view,
            preclaimResult.parentBatchId,
            preclaimResult.tx,
            preclaimResult.ter,
            calculateBaseFee(view, preclaimResult.tx),
            preclaimResult.flags,
            preclaimResult.j);
        return invoke_apply(ctx);
    }
    catch (std::exception const& e)
    {
        JLOG(preclaimResult.j.fatal()) << "apply: " << e.what();
        return {tefEXCEPTION, false};
    }
}

}  // namespace xrpl
