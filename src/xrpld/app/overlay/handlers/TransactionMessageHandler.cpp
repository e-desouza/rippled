#include <xrpld/app/ledger/LedgerMaster.h>
#include <xrpld/app/ledger/TransactionMaster.h>
#include <xrpld/app/main/Application.h>
#include <xrpld/app/misc/NetworkOPs.h>
#include <xrpld/app/misc/Transaction.h>
#include <xrpld/app/tx/apply.h>
#include <xrpld/app/txqueue/HashRouter.h>
#include <xrpld/overlay/Message.h>
#include <xrpld/overlay/detail/OverlayImpl.h>
#include <xrpld/overlay/detail/PeerImp.h>
#include <xrpld/overlay/detail/TrafficCount.h>
#include <xrpld/overlay/detail/handlers/TransactionMessageHandler.h>

#include <xrpl/protocol/STTx.h>
#include <xrpl/protocol/TxFlags.h>

using namespace std::chrono_literals;

namespace xrpl {

void
TransactionMessageHandler::onMessage(
    std::shared_ptr<protocol::TMTransaction> const& m,
    PeerImp& peer)
{
    handleTransaction(peer, m, true, false);
}

void
TransactionMessageHandler::onMessage(
    std::shared_ptr<protocol::TMTransactions> const& m,
    PeerImp& peer)
{
    if (!peer.txReduceRelayEnabled())
    {
        JLOG(peer.p_journal_.error())
            << "TMTransactions: tx reduce-relay is disabled";
        peer.fee_.update(Resource::feeMalformedRequest, "disabled");
        return;
    }

    JLOG(peer.p_journal_.trace())
        << "received TMTransactions " << m->transactions_size();

    peer.overlay_.addTxMetrics(m->transactions_size());

    for (std::uint32_t i = 0; i < m->transactions_size(); ++i)
        handleTransaction(
            peer,
            std::shared_ptr<protocol::TMTransaction>(
                m->mutable_transactions(i), [](protocol::TMTransaction*) {}),
            false,
            true);
}

void
TransactionMessageHandler::handleTransaction(
    PeerImp& peer,
    std::shared_ptr<protocol::TMTransaction> const& m,
    bool eraseTxQueue,
    bool batch)
{
    auto& app = peer.app_;
    auto& overlay = peer.overlay_;
    auto const& journal = peer.p_journal_;

    XRPL_ASSERT(
        eraseTxQueue != batch,
        ("xrpl::TransactionMessageHandler::handleTransaction : valid inputs"));
    if (peer.tracking_.load() == PeerImp::Tracking::diverged)
        return;

    if (app.getOPs().isNeedNetworkLedger())
    {
        // If we've never been in synch, there's nothing we can do
        // with a transaction
        JLOG(journal.debug())
            << "Ignoring incoming transaction: Need network ledger";
        return;
    }

    SerialIter sit(makeSlice(m->rawtransaction()));

    try
    {
        auto stx = std::make_shared<STTx const>(sit);
        uint256 txID = stx->getTransactionID();

        // Charge strongly for attempting to relay a txn with tfInnerBatchTxn
        // LCOV_EXCL_START
        if (stx->isFlag(tfInnerBatchTxn))
        {
            JLOG(journal.warn()) << "Ignoring Network relayed Tx containing "
                                    "tfInnerBatchTxn (handleTransaction).";
            peer.fee_.update(Resource::feeModerateBurdenPeer, "inner batch txn");
            return;
        }
        // LCOV_EXCL_STOP

        HashRouterFlags flags;
        constexpr std::chrono::seconds tx_interval = 10s;

        if (!app.getHashRouter().shouldProcess(txID, peer.id_, flags, tx_interval))
        {
            // we have seen this transaction recently
            if (any(flags & HashRouterFlags::BAD))
            {
                peer.fee_.update(Resource::feeUselessData, "known bad");
                JLOG(journal.debug()) << "Ignoring known bad tx " << txID;
            }

            // Erase only if the server has seen this tx.
            else if (eraseTxQueue && peer.txReduceRelayEnabled())
                peer.removeTxQueue(txID);

            overlay.reportInboundTraffic(
                TrafficCount::category::transaction_duplicate,
                Message::messageSize(*m));

            return;
        }

        JLOG(journal.debug()) << "Got tx " << txID;

        bool checkSignature = true;
        if (peer.cluster())
        {
            if (!m->has_deferred() || !m->deferred())
            {
                // Skip local checks if a server we trust
                // put the transaction in its open ledger
                flags |= HashRouterFlags::TRUSTED;
            }

            // for non-validator nodes only
            if (!app.getValidationPublicKey())
            {
                checkSignature = false;
            }
        }

        if (app.getLedgerMaster().getValidatedLedgerAge() > 4min)
        {
            JLOG(journal.trace())
                << "No new transactions until synchronized";
        }
        else if (
            app.getJobQueue().getJobCount(jtTRANSACTION) >
            app.config().MAX_TRANSACTIONS)
        {
            overlay.incJqTransOverflow();
            JLOG(journal.info()) << "Transaction queue is full";
        }
        else
        {
            app.getJobQueue().addJob(
                jtTRANSACTION,
                "RcvCheckTx",
                [weak = std::weak_ptr<PeerImp>(peer.shared_from_this()),
                 flags,
                 checkSignature,
                 batch,
                 stx]() {
                    if (auto p = weak.lock())
                        TransactionMessageHandler::checkTransaction(
                            *p, flags, checkSignature, stx, batch);
                });
        }
    }
    catch (std::exception const& ex)
    {
        JLOG(journal.warn())
            << "Transaction invalid: " << strHex(m->rawtransaction())
            << ". Exception: " << ex.what();
    }
}

void
TransactionMessageHandler::checkTransaction(
    PeerImp& peer,
    HashRouterFlags flags,
    bool checkSignature,
    std::shared_ptr<STTx const> const& stx,
    bool batch)
{
    auto& app = peer.app_;
    auto const& journal = peer.p_journal_;

    // VFALCO TODO Rewrite to not use exceptions
    try
    {
        // charge strongly for relaying batch txns
        // LCOV_EXCL_START
        if (stx->isFlag(tfInnerBatchTxn))
        {
            JLOG(journal.warn()) << "Ignoring Network relayed Tx containing "
                                    "tfInnerBatchTxn (checkSignature).";
            peer.charge(Resource::feeModerateBurdenPeer, "inner batch txn");
            return;
        }
        // LCOV_EXCL_STOP

        // Expired?
        if (stx->isFieldPresent(sfLastLedgerSequence) &&
            (stx->getFieldU32(sfLastLedgerSequence) <
             app.getLedgerMaster().getValidLedgerIndex()))
        {
            JLOG(journal.info())
                << "Marking transaction " << stx->getTransactionID()
                << "as BAD because it's expired";
            app.getHashRouter().setFlags(
                stx->getTransactionID(), HashRouterFlags::BAD);
            peer.charge(Resource::feeUselessData, "expired tx");
            return;
        }

        if (isPseudoTx(*stx))
        {
            // Don't do anything with pseudo transactions except put them in the
            // TransactionMaster cache
            std::string reason;
            auto tx = std::make_shared<Transaction>(stx, reason, app);
            XRPL_ASSERT(
                tx->getStatus() == NEW,
                "xrpl::TransactionMessageHandler::checkTransaction Transaction "
                "created correctly");
            if (tx->getStatus() == NEW)
            {
                JLOG(journal.debug())
                    << "Processing " << (batch ? "batch" : "unsolicited")
                    << " pseudo-transaction tx " << tx->getID();

                app.getMasterTransaction().canonicalize(&tx);
                // Tell the overlay about it, but don't relay it.
                auto const toSkip =
                    app.getHashRouter().shouldRelay(tx->getID());
                if (toSkip)
                {
                    JLOG(journal.debug())
                        << "Passing skipped pseudo pseudo-transaction tx "
                        << tx->getID();
                    app.overlay().relay(tx->getID(), {}, *toSkip);
                }
                if (!batch)
                {
                    JLOG(journal.debug())
                        << "Charging for pseudo-transaction tx " << tx->getID();
                    peer.charge(Resource::feeUselessData, "pseudo tx");
                }

                return;
            }
        }

        if (checkSignature)
        {
            // Check the signature before handing off to the job queue.
            if (auto [valid, validReason] = checkValidity(
                    app.getHashRouter(),
                    *stx,
                    app.getLedgerMaster().getValidatedRules(),
                    app.config());
                valid != Validity::Valid)
            {
                if (!validReason.empty())
                {
                    JLOG(journal.debug())
                        << "Exception checking transaction: " << validReason;
                }

                // Probably not necessary to set HashRouterFlags::BAD, but
                // doesn't hurt.
                app.getHashRouter().setFlags(
                    stx->getTransactionID(), HashRouterFlags::BAD);
                peer.charge(
                    Resource::feeInvalidSignature,
                    "check transaction signature failure");
                return;
            }
        }
        else
        {
            forceValidity(
                app.getHashRouter(), stx->getTransactionID(), Validity::Valid);
        }

        std::string reason;
        auto tx = std::make_shared<Transaction>(stx, reason, app);

        if (tx->getStatus() == INVALID)
        {
            if (!reason.empty())
            {
                JLOG(journal.debug())
                    << "Exception checking transaction: " << reason;
            }
            app.getHashRouter().setFlags(
                stx->getTransactionID(), HashRouterFlags::BAD);
            peer.charge(Resource::feeInvalidSignature, "tx (impossible)");
            return;
        }

        bool const trusted = any(flags & HashRouterFlags::TRUSTED);
        app.getOPs().processTransaction(
            tx, trusted, false, NetworkOPs::FailHard::no);
    }
    catch (std::exception const& ex)
    {
        JLOG(journal.warn())
            << "Exception in " << __func__ << ": " << ex.what();
        app.getHashRouter().setFlags(
            stx->getTransactionID(), HashRouterFlags::BAD);
        using namespace std::string_literals;
        peer.charge(Resource::feeInvalidData, "tx "s + ex.what());
    }
}

}  // namespace xrpl

