#ifndef XRPL_APP_TX_ITRANSACTIONHANDLER_H_INCLUDED
#define XRPL_APP_TX_ITRANSACTIONHANDLER_H_INCLUDED

#include <xrpld/app/tx/applySteps.h>
#include <xrpld/app/tx/detail/Transactor.h>

namespace xrpl {

/**
 * Interface for transaction handlers.
 *
 * This interface abstracts the transaction processing phases, allowing
 * runtime dispatch instead of compile-time macro-generated switches.
 * Each transaction type has a handler that implements this interface.
 */
class ITransactionHandler
{
public:
    virtual ~ITransactionHandler() = default;

    /**
     * Phase 1: Preflight validation (before signature check).
     * Performs early sanity checks on the transaction.
     *
     * @param ctx The preflight context containing transaction and rules
     * @return NotTEC result code (tesSUCCESS on success)
     */
    virtual NotTEC
    preflight(PreflightContext const& ctx) const = 0;

    /**
     * Phase 2: Preclaim validation (after signature verification).
     * Checks sequence, fees, permissions, and transaction-specific rules.
     *
     * @param ctx The preclaim context containing view and transaction
     * @param baseFee The calculated base fee for this transaction
     * @return TER result code
     */
    virtual TER
    preclaim(PreclaimContext const& ctx, XRPAmount baseFee) const = 0;

    /**
     * Phase 3: Apply the transaction to the ledger.
     *
     * @param ctx The apply context
     * @return ApplyResult containing result code and success flag
     */
    virtual ApplyResult
    doApply(ApplyContext& ctx) const = 0;

    /**
     * Calculate the base fee for this transaction type.
     *
     * @param view The ledger view
     * @param tx The transaction
     * @return The base fee amount
     */
    virtual XRPAmount
    calculateBaseFee(ReadView const& view, STTx const& tx) const = 0;

    /**
     * Create transaction consequences for queue management.
     *
     * @param ctx The preflight context
     * @return TxConsequences describing the transaction's effects
     */
    virtual TxConsequences
    makeConsequences(PreflightContext const& ctx) const = 0;

    /**
     * Get the consequences factory type for this transaction.
     *
     * @return The ConsequencesFactoryType (Normal, Blocker, or Custom)
     */
    virtual Transactor::ConsequencesFactoryType
    consequencesFactory() const = 0;
};

}  // namespace xrpl

#endif
