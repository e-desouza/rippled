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

#include <xrpld/app/misc/detail/NetworkOPsAdapter.h>

namespace xrpl {

NetworkOPsAdapter::NetworkOPsAdapter(NetworkOPs& ops) : ops_(ops)
{
}

//------------------------------------------------------------------------------
// INetworkState interface
//------------------------------------------------------------------------------

OperatingMode
NetworkOPsAdapter::getOperatingMode() const
{
    return ops_.getOperatingMode();
}

std::string
NetworkOPsAdapter::strOperatingMode(OperatingMode mode, bool admin) const
{
    return ops_.strOperatingMode(mode, admin);
}

std::string
NetworkOPsAdapter::strOperatingMode(bool admin) const
{
    return ops_.strOperatingMode(admin);
}

void
NetworkOPsAdapter::setMode(OperatingMode om)
{
    ops_.setMode(om);
}

bool
NetworkOPsAdapter::isFull()
{
    return ops_.isFull();
}

bool
NetworkOPsAdapter::isBlocked()
{
    return ops_.isBlocked();
}

bool
NetworkOPsAdapter::isAmendmentBlocked()
{
    return ops_.isAmendmentBlocked();
}

void
NetworkOPsAdapter::setAmendmentBlocked()
{
    ops_.setAmendmentBlocked();
}

bool
NetworkOPsAdapter::isAmendmentWarned()
{
    return ops_.isAmendmentWarned();
}

void
NetworkOPsAdapter::setAmendmentWarned()
{
    ops_.setAmendmentWarned();
}

void
NetworkOPsAdapter::clearAmendmentWarned()
{
    ops_.clearAmendmentWarned();
}

bool
NetworkOPsAdapter::isUNLBlocked()
{
    return ops_.isUNLBlocked();
}

void
NetworkOPsAdapter::setUNLBlocked()
{
    ops_.setUNLBlocked();
}

void
NetworkOPsAdapter::clearUNLBlocked()
{
    ops_.clearUNLBlocked();
}

bool
NetworkOPsAdapter::isNeedNetworkLedger()
{
    return ops_.isNeedNetworkLedger();
}

void
NetworkOPsAdapter::setNeedNetworkLedger()
{
    ops_.setNeedNetworkLedger();
}

void
NetworkOPsAdapter::clearNeedNetworkLedger()
{
    ops_.clearNeedNetworkLedger();
}

void
NetworkOPsAdapter::setStandAlone()
{
    ops_.setStandAlone();
}

//------------------------------------------------------------------------------
// ITransactionProcessor interface
//------------------------------------------------------------------------------

void
NetworkOPsAdapter::submitTransaction(std::shared_ptr<STTx const> const& tx)
{
    ops_.submitTransaction(tx);
}

void
NetworkOPsAdapter::processTransaction(
    std::shared_ptr<Transaction>& transaction,
    bool bUnlimited,
    bool bLocal,
    NetworkOPs::FailHard failType)
{
    ops_.processTransaction(transaction, bUnlimited, bLocal, failType);
}

void
NetworkOPsAdapter::processTransactionSet(CanonicalTXSet const& set)
{
    ops_.processTransactionSet(set);
}

void
NetworkOPsAdapter::updateLocalTx(ReadView const& newValidLedger)
{
    ops_.updateLocalTx(newValidLedger);
}

std::size_t
NetworkOPsAdapter::getLocalTxCount()
{
    return ops_.getLocalTxCount();
}

//------------------------------------------------------------------------------
// IConsensusCoordinator interface
//------------------------------------------------------------------------------

bool
NetworkOPsAdapter::processTrustedProposal(RCLCxPeerPos peerPos)
{
    return ops_.processTrustedProposal(std::move(peerPos));
}

bool
NetworkOPsAdapter::recvValidation(
    std::shared_ptr<STValidation> const& val,
    std::string const& source)
{
    return ops_.recvValidation(val, source);
}

void
NetworkOPsAdapter::mapComplete(
    std::shared_ptr<SHAMap> const& map,
    bool fromAcquire)
{
    ops_.mapComplete(map, fromAcquire);
}

bool
NetworkOPsAdapter::beginConsensus(
    uint256 const& netLCL,
    std::unique_ptr<std::stringstream> const& clog)
{
    return ops_.beginConsensus(netLCL, clog);
}

void
NetworkOPsAdapter::endConsensus(std::unique_ptr<std::stringstream> const& clog)
{
    ops_.endConsensus(clog);
}

void
NetworkOPsAdapter::setStateTimer()
{
    ops_.setStateTimer();
}

void
NetworkOPsAdapter::consensusViewChange()
{
    ops_.consensusViewChange();
}

Json::Value
NetworkOPsAdapter::getConsensusInfo()
{
    return ops_.getConsensusInfo();
}

std::uint32_t
NetworkOPsAdapter::acceptLedger(
    std::optional<std::chrono::milliseconds> consensusDelay)
{
    return ops_.acceptLedger(consensusDelay);
}

void
NetworkOPsAdapter::reportFeeChange()
{
    ops_.reportFeeChange();
}

//------------------------------------------------------------------------------
// IPubSubManager interface (extends InfoSub::Source)
//------------------------------------------------------------------------------

void
NetworkOPsAdapter::pubLedger(std::shared_ptr<ReadView const> const& lpAccepted)
{
    ops_.pubLedger(lpAccepted);
}

void
NetworkOPsAdapter::pubProposedTransaction(
    std::shared_ptr<ReadView const> const& ledger,
    std::shared_ptr<STTx const> const& transaction,
    TER result)
{
    ops_.pubProposedTransaction(ledger, transaction, result);
}

void
NetworkOPsAdapter::pubValidation(std::shared_ptr<STValidation> const& val)
{
    ops_.pubValidation(val);
}

void
NetworkOPsAdapter::stateAccounting(Json::Value& obj)
{
    ops_.stateAccounting(obj);
}

// InfoSub::Source methods

void
NetworkOPsAdapter::subAccount(
    InfoSub::ref ispListener,
    hash_set<AccountID> const& vnaAccountIDs,
    bool realTime)
{
    ops_.subAccount(ispListener, vnaAccountIDs, realTime);
}

void
NetworkOPsAdapter::unsubAccount(
    InfoSub::ref ispListener,
    hash_set<AccountID> const& vnaAccountIDs,
    bool realTime)
{
    ops_.unsubAccount(ispListener, vnaAccountIDs, realTime);
}

void
NetworkOPsAdapter::unsubAccountInternal(
    std::uint64_t uListener,
    hash_set<AccountID> const& vnaAccountIDs,
    bool realTime)
{
    ops_.unsubAccountInternal(uListener, vnaAccountIDs, realTime);
}

error_code_i
NetworkOPsAdapter::subAccountHistory(
    InfoSub::ref ispListener,
    AccountID const& account)
{
    return ops_.subAccountHistory(ispListener, account);
}

void
NetworkOPsAdapter::unsubAccountHistory(
    InfoSub::ref ispListener,
    AccountID const& account,
    bool historyOnly)
{
    ops_.unsubAccountHistory(ispListener, account, historyOnly);
}

void
NetworkOPsAdapter::unsubAccountHistoryInternal(
    std::uint64_t uListener,
    AccountID const& account,
    bool historyOnly)
{
    ops_.unsubAccountHistoryInternal(uListener, account, historyOnly);
}

bool
NetworkOPsAdapter::subLedger(InfoSub::ref ispListener, Json::Value& jvResult)
{
    return ops_.subLedger(ispListener, jvResult);
}

bool
NetworkOPsAdapter::unsubLedger(std::uint64_t uListener)
{
    return ops_.unsubLedger(uListener);
}

bool
NetworkOPsAdapter::subBookChanges(InfoSub::ref ispListener)
{
    return ops_.subBookChanges(ispListener);
}

bool
NetworkOPsAdapter::unsubBookChanges(std::uint64_t uListener)
{
    return ops_.unsubBookChanges(uListener);
}

bool
NetworkOPsAdapter::subManifests(InfoSub::ref ispListener)
{
    return ops_.subManifests(ispListener);
}

bool
NetworkOPsAdapter::unsubManifests(std::uint64_t uListener)
{
    return ops_.unsubManifests(uListener);
}

void
NetworkOPsAdapter::pubManifest(Manifest const& mo)
{
    ops_.pubManifest(mo);
}

bool
NetworkOPsAdapter::subServer(
    InfoSub::ref ispListener,
    Json::Value& jvResult,
    bool admin)
{
    return ops_.subServer(ispListener, jvResult, admin);
}

bool
NetworkOPsAdapter::unsubServer(std::uint64_t uListener)
{
    return ops_.unsubServer(uListener);
}

bool
NetworkOPsAdapter::subBook(InfoSub::ref ispListener, Book const& book)
{
    return ops_.subBook(ispListener, book);
}

bool
NetworkOPsAdapter::unsubBook(std::uint64_t uListener, Book const& book)
{
    return ops_.unsubBook(uListener, book);
}

bool
NetworkOPsAdapter::subTransactions(InfoSub::ref ispListener)
{
    return ops_.subTransactions(ispListener);
}

bool
NetworkOPsAdapter::unsubTransactions(std::uint64_t uListener)
{
    return ops_.unsubTransactions(uListener);
}

bool
NetworkOPsAdapter::subRTTransactions(InfoSub::ref ispListener)
{
    return ops_.subRTTransactions(ispListener);
}

bool
NetworkOPsAdapter::unsubRTTransactions(std::uint64_t uListener)
{
    return ops_.unsubRTTransactions(uListener);
}

bool
NetworkOPsAdapter::subValidations(InfoSub::ref ispListener)
{
    return ops_.subValidations(ispListener);
}

bool
NetworkOPsAdapter::unsubValidations(std::uint64_t uListener)
{
    return ops_.unsubValidations(uListener);
}

bool
NetworkOPsAdapter::subPeerStatus(InfoSub::ref ispListener)
{
    return ops_.subPeerStatus(ispListener);
}

bool
NetworkOPsAdapter::unsubPeerStatus(std::uint64_t uListener)
{
    return ops_.unsubPeerStatus(uListener);
}

void
NetworkOPsAdapter::pubPeerStatus(std::function<Json::Value(void)> const& func)
{
    ops_.pubPeerStatus(func);
}

bool
NetworkOPsAdapter::subConsensus(InfoSub::ref ispListener)
{
    return ops_.subConsensus(ispListener);
}

bool
NetworkOPsAdapter::unsubConsensus(std::uint64_t uListener)
{
    return ops_.unsubConsensus(uListener);
}

InfoSub::pointer
NetworkOPsAdapter::findRpcSub(std::string const& strUrl)
{
    return ops_.findRpcSub(strUrl);
}

InfoSub::pointer
NetworkOPsAdapter::addRpcSub(std::string const& strUrl, InfoSub::ref rspEntry)
{
    return ops_.addRpcSub(strUrl, rspEntry);
}

bool
NetworkOPsAdapter::tryRemoveRpcSub(std::string const& strUrl)
{
    return ops_.tryRemoveRpcSub(strUrl);
}

//------------------------------------------------------------------------------
// INetworkInfo interface
//------------------------------------------------------------------------------

Json::Value
NetworkOPsAdapter::getServerInfo(bool human, bool admin, bool counters)
{
    return ops_.getServerInfo(human, admin, counters);
}

void
NetworkOPsAdapter::clearLedgerFetch()
{
    ops_.clearLedgerFetch();
}

Json::Value
NetworkOPsAdapter::getLedgerFetchInfo()
{
    return ops_.getLedgerFetchInfo();
}

Json::Value
NetworkOPsAdapter::getOwnerInfo(
    std::shared_ptr<ReadView const> lpLedger,
    AccountID const& account)
{
    return ops_.getOwnerInfo(std::move(lpLedger), account);
}

void
NetworkOPsAdapter::getBookPage(
    std::shared_ptr<ReadView const>& lpLedger,
    Book const& book,
    AccountID const& uTakerID,
    bool const bProof,
    unsigned int iLimit,
    Json::Value const& jvMarker,
    Json::Value& jvResult)
{
    ops_.getBookPage(
        lpLedger, book, uTakerID, bProof, iLimit, jvMarker, jvResult);
}

}  // namespace xrpl
