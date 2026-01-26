#ifndef XRPL_OVERLAY_HANDLERS_TRANSACTIONMESSAGEHANDLER_H_INCLUDED
#define XRPL_OVERLAY_HANDLERS_TRANSACTIONMESSAGEHANDLER_H_INCLUDED

#include <xrpl/basics/HashRouterFlags.h>
#include <xrpl/protocol/messages.h>

#include <memory>

namespace xrpl {

class PeerImp;
class STTx;

/**
 * @brief Handles transaction-related protocol messages.
 *
 * This handler processes the following message types:
 * - TMTransaction: Individual transaction messages
 * - TMTransactions: Batch transaction responses
 *
 * The handler is a friend of PeerImp and has access to its internals.
 * All handler methods are static and stateless.
 *
 * Thread Safety:
 * - onMessage methods are called on the PeerImp strand
 * - checkTransaction is called from the job queue
 * - Handler does not maintain any state between calls
 */
class TransactionMessageHandler
{
public:
    /**
     * @brief Process a TMTransaction message.
     *
     * Handles incoming transaction messages, performs initial validation,
     * and schedules signature verification on the job queue.
     *
     * @param m The transaction message
     * @param peer The peer that received this message
     */
    static void
    onMessage(
        std::shared_ptr<protocol::TMTransaction> const& m,
        PeerImp& peer);

    /**
     * @brief Process a TMTransactions message (batch response).
     *
     * Handles batch transaction responses from peers when tx reduce-relay
     * is enabled.
     *
     * @param m The transactions message
     * @param peer The peer that received this message
     */
    static void
    onMessage(
        std::shared_ptr<protocol::TMTransactions> const& m,
        PeerImp& peer);

    /**
     * @brief Handle incoming transaction (common logic).
     *
     * Called from onMessage(TMTransaction(s)).
     *
     * @param peer The peer that received the message
     * @param m Transaction protocol message
     * @param eraseTxQueue true when called from onMessage(TMTransaction)
     * @param batch true when called from onMessage(TMTransactions)
     */
    static void
    handleTransaction(
        PeerImp& peer,
        std::shared_ptr<protocol::TMTransaction> const& m,
        bool eraseTxQueue,
        bool batch);

    /**
     * @brief Check transaction validity (called from job queue).
     *
     * Performs signature verification and submits valid transactions
     * to the transaction engine.
     *
     * @param peer The peer that received the message
     * @param flags Hash router flags
     * @param checkSignature Whether to verify the signature
     * @param stx The signed transaction
     * @param batch Whether this is part of a batch
     */
    static void
    checkTransaction(
        PeerImp& peer,
        HashRouterFlags flags,
        bool checkSignature,
        std::shared_ptr<STTx const> const& stx,
        bool batch);

private:
    // No state - all methods are static
    TransactionMessageHandler() = delete;
};

}  // namespace xrpl

#endif

