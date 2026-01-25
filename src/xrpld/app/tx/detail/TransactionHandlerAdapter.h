#ifndef XRPL_APP_TX_TRANSACTIONHANDLERADAPTER_H_INCLUDED
#define XRPL_APP_TX_TRANSACTIONHANDLERADAPTER_H_INCLUDED

#include <xrpld/app/tx/detail/ITransactionHandler.h>
#include <xrpld/app/tx/detail/Transactor.h>

namespace xrpl {

/**
 * CRTP adapter that wraps a Transactor-derived class into the
 * ITransactionHandler interface.
 *
 * This adapter bridges the static polymorphism of Transactor classes
 * (using name hiding for compile-time dispatch) to the virtual
 * polymorphism of ITransactionHandler (for runtime dispatch).
 *
 * @tparam T The Transactor-derived class (e.g., Payment, OfferCreate)
 */
template <typename T>
class TransactionHandlerAdapter : public ITransactionHandler
{
public:
    NotTEC
    preflight(PreflightContext const& ctx) const override
    {
        return Transactor::invokePreflight<T>(ctx);
    }

    TER
    preclaim(PreclaimContext const& ctx, XRPAmount baseFee) const override
    {
        // Replicate the invoke_preclaim logic from applySteps.cpp
        // This handles the pre-signature and post-signature checks

        auto const id = ctx.tx.getAccountID(sfAccount);

        if (id != beast::zero)
        {
            // Pre-signature checks (must return NotTEC)
            if (NotTEC const result = T::checkSeqProxy(ctx.view, ctx.tx, ctx.j))
                return result;

            if (NotTEC const result = T::checkPriorTxAndLastLedger(ctx))
                return result;

            if (NotTEC const result = T::checkPermission(ctx.view, ctx.tx))
                return result;

            if (NotTEC const result = T::checkSign(ctx))
                return result;

            // Post-signature check (can return TER)
            if (TER const result = T::checkFee(ctx, baseFee))
                return result;
        }

        return T::preclaim(ctx);
    }

    ApplyResult
    doApply(ApplyContext& ctx) const override
    {
        T transactor(ctx);
        return transactor();
    }

    XRPAmount
    calculateBaseFee(ReadView const& view, STTx const& tx) const override
    {
        return T::calculateBaseFee(view, tx);
    }

    TxConsequences
    makeConsequences(PreflightContext const& ctx) const override
    {
        if constexpr (T::ConsequencesFactory == Transactor::Normal)
        {
            return TxConsequences(ctx.tx);
        }
        else if constexpr (T::ConsequencesFactory == Transactor::Blocker)
        {
            return TxConsequences(ctx.tx, TxConsequences::blocker);
        }
        else
        {
            // Custom consequences factory
            return T::makeTxConsequences(ctx);
        }
    }

    Transactor::ConsequencesFactoryType
    consequencesFactory() const override
    {
        return T::ConsequencesFactory;
    }
};

}  // namespace xrpl

#endif
