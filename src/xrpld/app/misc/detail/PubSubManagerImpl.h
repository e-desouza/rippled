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

#ifndef XRPL_APP_MISC_PUBSUBMANAGERIMPL_H_INCLUDED
#define XRPL_APP_MISC_PUBSUBMANAGERIMPL_H_INCLUDED

#include <xrpld/app/misc/IPubSubManager.h>

namespace xrpl {

class Application;

/**
 * @brief Implementation of the IPubSubManager interface.
 *
 * This class provides publish/subscribe management functionality,
 * including subscription management (subAccount, subLedger, etc.)
 * and publishing notifications (pubLedger, pubValidation, etc.).
 *
 * This is a stub implementation that will be completed when wiring
 * together the NetworkOPs split components. The actual implementation
 * will be extracted from NetworkOPs.cpp lines ~3000-4894.
 *
 * Thread-safe: All methods are thread-safe.
 */
class PubSubManagerImpl final : public IPubSubManager
{
public:
    /**
     * @brief Construct a PubSubManagerImpl.
     *
     * @param app Reference to the Application instance.
     */
    explicit PubSubManagerImpl(Application& app);

    ~PubSubManagerImpl() override = default;

    // Non-copyable, non-movable
    PubSubManagerImpl(PubSubManagerImpl const&) = delete;
    PubSubManagerImpl&
    operator=(PubSubManagerImpl const&) = delete;
    PubSubManagerImpl(PubSubManagerImpl&&) = delete;
    PubSubManagerImpl&
    operator=(PubSubManagerImpl&&) = delete;

    //--------------------------------------------------------------------------
    // IPubSubManager interface implementation
    //--------------------------------------------------------------------------

    void
    pubLedger(std::shared_ptr<ReadView const> const& lpAccepted) override;

    void
    pubProposedTransaction(
        std::shared_ptr<ReadView const> const& ledger,
        std::shared_ptr<STTx const> const& transaction,
        TER result) override;

    void
    pubValidation(std::shared_ptr<STValidation> const& val) override;

    void
    stateAccounting(Json::Value& obj) override;

    //--------------------------------------------------------------------------
    // InfoSub::Source interface implementation
    //--------------------------------------------------------------------------

    void
    subAccount(
        InfoSub::ref ispListener,
        hash_set<AccountID> const& vnaAccountIDs,
        bool realTime) override;

    void
    unsubAccount(
        InfoSub::ref ispListener,
        hash_set<AccountID> const& vnaAccountIDs,
        bool realTime) override;

    void
    unsubAccountInternal(
        std::uint64_t uListener,
        hash_set<AccountID> const& vnaAccountIDs,
        bool realTime) override;

    error_code_i
    subAccountHistory(InfoSub::ref ispListener, AccountID const& account)
        override;

    void
    unsubAccountHistory(
        InfoSub::ref ispListener,
        AccountID const& account,
        bool historyOnly) override;

    void
    unsubAccountHistoryInternal(
        std::uint64_t uListener,
        AccountID const& account,
        bool historyOnly) override;

    bool
    subLedger(InfoSub::ref ispListener, Json::Value& jvResult) override;
    bool
    unsubLedger(std::uint64_t uListener) override;

    bool
    subBookChanges(InfoSub::ref ispListener) override;
    bool
    unsubBookChanges(std::uint64_t uListener) override;

    bool
    subManifests(InfoSub::ref ispListener) override;
    bool
    unsubManifests(std::uint64_t uListener) override;
    void
    pubManifest(Manifest const&) override;

    bool
    subServer(InfoSub::ref ispListener, Json::Value& jvResult, bool admin)
        override;
    bool
    unsubServer(std::uint64_t uListener) override;

    bool
    subBook(InfoSub::ref ispListener, Book const&) override;
    bool
    unsubBook(std::uint64_t uListener, Book const&) override;

    bool
    subTransactions(InfoSub::ref ispListener) override;
    bool
    unsubTransactions(std::uint64_t uListener) override;

    bool
    subRTTransactions(InfoSub::ref ispListener) override;
    bool
    unsubRTTransactions(std::uint64_t uListener) override;

    bool
    subValidations(InfoSub::ref ispListener) override;
    bool
    unsubValidations(std::uint64_t uListener) override;

    bool
    subPeerStatus(InfoSub::ref ispListener) override;
    bool
    unsubPeerStatus(std::uint64_t uListener) override;
    void
    pubPeerStatus(std::function<Json::Value(void)> const&) override;

    bool
    subConsensus(InfoSub::ref ispListener) override;
    bool
    unsubConsensus(std::uint64_t uListener) override;

    InfoSub::pointer
    findRpcSub(std::string const& strUrl) override;
    InfoSub::pointer
    addRpcSub(std::string const& strUrl, InfoSub::ref rspEntry) override;
    bool
    tryRemoveRpcSub(std::string const& strUrl) override;

private:
    Application& app_;
};

}  // namespace xrpl

#endif
