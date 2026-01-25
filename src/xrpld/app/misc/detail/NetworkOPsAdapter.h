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

#ifndef XRPL_APP_MISC_DETAIL_NETWORKOPSADAPTER_H_INCLUDED
#define XRPL_APP_MISC_DETAIL_NETWORKOPSADAPTER_H_INCLUDED

#include <xrpld/app/misc/IConsensusCoordinator.h>
#include <xrpld/app/misc/INetworkInfo.h>
#include <xrpld/app/misc/INetworkState.h>
#include <xrpld/app/misc/IPubSubManager.h>
#include <xrpld/app/misc/ITransactionProcessor.h>
#include <xrpld/app/misc/NetworkOPs.h>

namespace xrpl {

/**
 * @brief Adapter that wraps NetworkOPs and exposes the 5 focused interfaces.
 *
 * This adapter class wraps an existing NetworkOPs instance and implements
 * all 5 decomposed interfaces by delegating to the wrapped NetworkOPs.
 * This enables gradual migration from the monolithic NetworkOPs to the
 * focused interfaces.
 *
 * Usage:
 * @code
 *     NetworkOPs& ops = app.getOPs();
 *     NetworkOPsAdapter adapter(ops);
 *     INetworkState& state = adapter;  // Use focused interface
 * @endcode
 */
class NetworkOPsAdapter : public INetworkState,
                          public ITransactionProcessor,
                          public IConsensusCoordinator,
                          public IPubSubManager,
                          public INetworkInfo
{
public:
    /**
     * @brief Construct an adapter wrapping the given NetworkOPs.
     * @param ops The NetworkOPs instance to wrap (must outlive adapter).
     */
    explicit NetworkOPsAdapter(NetworkOPs& ops);

    ~NetworkOPsAdapter() override = default;

    // Non-copyable, non-movable
    NetworkOPsAdapter(NetworkOPsAdapter const&) = delete;
    NetworkOPsAdapter&
    operator=(NetworkOPsAdapter const&) = delete;
    NetworkOPsAdapter(NetworkOPsAdapter&&) = delete;
    NetworkOPsAdapter&
    operator=(NetworkOPsAdapter&&) = delete;

    //--------------------------------------------------------------------------
    // INetworkState interface
    //--------------------------------------------------------------------------

    OperatingMode
    getOperatingMode() const override;
    std::string
    strOperatingMode(OperatingMode mode, bool admin) const override;
    std::string
    strOperatingMode(bool admin = false) const override;
    void
    setMode(OperatingMode om) override;
    bool
    isFull() override;
    bool
    isBlocked() override;
    bool
    isAmendmentBlocked() override;
    void
    setAmendmentBlocked() override;
    bool
    isAmendmentWarned() override;
    void
    setAmendmentWarned() override;
    void
    clearAmendmentWarned() override;
    bool
    isUNLBlocked() override;
    void
    setUNLBlocked() override;
    void
    clearUNLBlocked() override;
    bool
    isNeedNetworkLedger() override;
    void
    setNeedNetworkLedger() override;
    void
    clearNeedNetworkLedger() override;
    void
    setStandAlone() override;

    //--------------------------------------------------------------------------
    // ITransactionProcessor interface
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

    //--------------------------------------------------------------------------
    // IConsensusCoordinator interface
    //--------------------------------------------------------------------------

    bool
    processTrustedProposal(RCLCxPeerPos peerPos) override;
    bool
    recvValidation(
        std::shared_ptr<STValidation> const& val,
        std::string const& source) override;
    void
    mapComplete(std::shared_ptr<SHAMap> const& map, bool fromAcquire) override;
    bool
    beginConsensus(
        uint256 const& netLCL,
        std::unique_ptr<std::stringstream> const& clog) override;
    void
    endConsensus(std::unique_ptr<std::stringstream> const& clog) override;
    void
    setStateTimer() override;
    void
    consensusViewChange() override;
    Json::Value
    getConsensusInfo() override;
    std::uint32_t
    acceptLedger(
        std::optional<std::chrono::milliseconds> consensusDelay) override;
    void
    reportFeeChange() override;

    //--------------------------------------------------------------------------
    // IPubSubManager interface (extends InfoSub::Source)
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

    // InfoSub::Source methods
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
    pubManifest(Manifest const& mo) override;
    bool
    subServer(InfoSub::ref ispListener, Json::Value& jvResult, bool admin)
        override;
    bool
    unsubServer(std::uint64_t uListener) override;
    bool
    subBook(InfoSub::ref ispListener, Book const& book) override;
    bool
    unsubBook(std::uint64_t uListener, Book const& book) override;
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
    pubPeerStatus(std::function<Json::Value(void)> const& func) override;
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

    //--------------------------------------------------------------------------
    // INetworkInfo interface
    //--------------------------------------------------------------------------

    Json::Value
    getServerInfo(bool human, bool admin, bool counters) override;
    void
    clearLedgerFetch() override;
    Json::Value
    getLedgerFetchInfo() override;
    Json::Value
    getOwnerInfo(
        std::shared_ptr<ReadView const> lpLedger,
        AccountID const& account) override;
    void
    getBookPage(
        std::shared_ptr<ReadView const>& lpLedger,
        Book const& book,
        AccountID const& uTakerID,
        bool const bProof,
        unsigned int iLimit,
        Json::Value const& jvMarker,
        Json::Value& jvResult) override;

private:
    NetworkOPs& ops_;
};

}  // namespace xrpl

#endif
