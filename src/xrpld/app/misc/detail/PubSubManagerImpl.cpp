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

#include <xrpld/app/main/Application.h>
#include <xrpld/app/misc/detail/PubSubManagerImpl.h>
#include <xrpld/app/validators/Manifest.h>

#include <stdexcept>

namespace xrpl {

PubSubManagerImpl::PubSubManagerImpl(Application& app) : app_(app)
{
    // Constructor - actual initialization will be added
    // when wiring together the NetworkOPs split components.
}

//------------------------------------------------------------------------------
// IPubSubManager interface implementation
//------------------------------------------------------------------------------

void
PubSubManagerImpl::pubLedger(std::shared_ptr<ReadView const> const& lpAccepted)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will extract logic from NetworkOPsImp::pubLedger()
    // (NetworkOPs.cpp lines ~3800-4000).
    (void)lpAccepted;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3800-4000");
}

void
PubSubManagerImpl::pubProposedTransaction(
    std::shared_ptr<ReadView const> const& ledger,
    std::shared_ptr<STTx const> const& transaction,
    TER result)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will extract logic from NetworkOPsImp::pubProposedTransaction()
    // (NetworkOPs.cpp lines ~4000-4200).
    (void)ledger;
    (void)transaction;
    (void)result;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 4000-4200");
}

void
PubSubManagerImpl::pubValidation(std::shared_ptr<STValidation> const& val)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will extract logic from NetworkOPsImp::pubValidation()
    // (NetworkOPs.cpp lines ~4200-4400).
    (void)val;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 4200-4400");
}

void
PubSubManagerImpl::stateAccounting(Json::Value& obj)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // This will extract state accounting logic from NetworkOPsImp.
    (void)obj;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp stateAccounting");
}

//------------------------------------------------------------------------------
// InfoSub::Source interface implementation - Subscription methods
//------------------------------------------------------------------------------

void
PubSubManagerImpl::subAccount(
    InfoSub::ref ispListener,
    hash_set<AccountID> const& vnaAccountIDs,
    bool realTime)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3000-3100)
    (void)ispListener;
    (void)vnaAccountIDs;
    (void)realTime;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3000-3100");
}

void
PubSubManagerImpl::unsubAccount(
    InfoSub::ref ispListener,
    hash_set<AccountID> const& vnaAccountIDs,
    bool realTime)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3100-3200)
    (void)ispListener;
    (void)vnaAccountIDs;
    (void)realTime;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3100-3200");
}

void
PubSubManagerImpl::unsubAccountInternal(
    std::uint64_t uListener,
    hash_set<AccountID> const& vnaAccountIDs,
    bool realTime)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3200-3250)
    (void)uListener;
    (void)vnaAccountIDs;
    (void)realTime;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3200-3250");
}

error_code_i
PubSubManagerImpl::subAccountHistory(
    InfoSub::ref ispListener,
    AccountID const& account)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~4400-4500)
    (void)ispListener;
    (void)account;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 4400-4500");
}

void
PubSubManagerImpl::unsubAccountHistory(
    InfoSub::ref ispListener,
    AccountID const& account,
    bool historyOnly)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~4500-4600)
    (void)ispListener;
    (void)account;
    (void)historyOnly;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 4500-4600");
}

void
PubSubManagerImpl::unsubAccountHistoryInternal(
    std::uint64_t uListener,
    AccountID const& account,
    bool historyOnly)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~4600-4650)
    (void)uListener;
    (void)account;
    (void)historyOnly;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 4600-4650");
}

bool
PubSubManagerImpl::subLedger(InfoSub::ref ispListener, Json::Value& jvResult)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3250-3300)
    (void)ispListener;
    (void)jvResult;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3250-3300");
}

bool
PubSubManagerImpl::unsubLedger(std::uint64_t uListener)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3300-3350)
    (void)uListener;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3300-3350");
}

bool
PubSubManagerImpl::subBookChanges(InfoSub::ref ispListener)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3350-3400)
    (void)ispListener;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3350-3400");
}

bool
PubSubManagerImpl::unsubBookChanges(std::uint64_t uListener)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3400-3450)
    (void)uListener;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3400-3450");
}

bool
PubSubManagerImpl::subManifests(InfoSub::ref ispListener)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3450-3500)
    (void)ispListener;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3450-3500");
}

bool
PubSubManagerImpl::unsubManifests(std::uint64_t uListener)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3500-3550)
    (void)uListener;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3500-3550");
}

void
PubSubManagerImpl::pubManifest(Manifest const& manifest)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3550-3600)
    (void)manifest;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3550-3600");
}

bool
PubSubManagerImpl::subServer(
    InfoSub::ref ispListener,
    Json::Value& jvResult,
    bool admin)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3600-3650)
    (void)ispListener;
    (void)jvResult;
    (void)admin;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3600-3650");
}

bool
PubSubManagerImpl::unsubServer(std::uint64_t uListener)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3650-3700)
    (void)uListener;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3650-3700");
}

bool
PubSubManagerImpl::subBook(InfoSub::ref ispListener, Book const& book)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3700-3750)
    (void)ispListener;
    (void)book;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3700-3750");
}

bool
PubSubManagerImpl::unsubBook(std::uint64_t uListener, Book const& book)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3750-3800)
    (void)uListener;
    (void)book;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3750-3800");
}

bool
PubSubManagerImpl::subTransactions(InfoSub::ref ispListener)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3300-3350)
    (void)ispListener;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3300-3350");
}

bool
PubSubManagerImpl::unsubTransactions(std::uint64_t uListener)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3350-3400)
    (void)uListener;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3350-3400");
}

bool
PubSubManagerImpl::subRTTransactions(InfoSub::ref ispListener)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3400-3450)
    (void)ispListener;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3400-3450");
}

bool
PubSubManagerImpl::unsubRTTransactions(std::uint64_t uListener)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3450-3500)
    (void)uListener;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3450-3500");
}

bool
PubSubManagerImpl::subValidations(InfoSub::ref ispListener)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3500-3550)
    (void)ispListener;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3500-3550");
}

bool
PubSubManagerImpl::unsubValidations(std::uint64_t uListener)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3550-3600)
    (void)uListener;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3550-3600");
}

bool
PubSubManagerImpl::subPeerStatus(InfoSub::ref ispListener)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3600-3650)
    (void)ispListener;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3600-3650");
}

bool
PubSubManagerImpl::unsubPeerStatus(std::uint64_t uListener)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3650-3700)
    (void)uListener;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3650-3700");
}

void
PubSubManagerImpl::pubPeerStatus(std::function<Json::Value(void)> const& func)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3700-3750)
    (void)func;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3700-3750");
}

bool
PubSubManagerImpl::subConsensus(InfoSub::ref ispListener)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3750-3800)
    (void)ispListener;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3750-3800");
}

bool
PubSubManagerImpl::unsubConsensus(std::uint64_t uListener)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3800-3850)
    (void)uListener;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3800-3850");
}

InfoSub::pointer
PubSubManagerImpl::findRpcSub(std::string const& strUrl)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3850-3900)
    (void)strUrl;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3850-3900");
}

InfoSub::pointer
PubSubManagerImpl::addRpcSub(std::string const& strUrl, InfoSub::ref rspEntry)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3900-3950)
    (void)strUrl;
    (void)rspEntry;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3900-3950");
}

bool
PubSubManagerImpl::tryRemoveRpcSub(std::string const& strUrl)
{
    // TODO: Implement when wiring together the NetworkOPs split.
    // (NetworkOPs.cpp lines ~3950-4000)
    (void)strUrl;
    throw std::logic_error(
        "Not yet implemented - see NetworkOPs.cpp lines 3950-4000");
}

}  // namespace xrpl
