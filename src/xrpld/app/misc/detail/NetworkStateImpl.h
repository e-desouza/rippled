//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

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

#ifndef XRPL_APP_MISC_DETAIL_NETWORKSTATEIMPL_H_INCLUDED
#define XRPL_APP_MISC_DETAIL_NETWORKSTATEIMPL_H_INCLUDED

#include <xrpld/app/misc/INetworkState.h>

#include <xrpl/beast/utility/Journal.h>
#include <xrpl/json/json_value.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>

namespace xrpl {

/**
 * @brief Implementation of INetworkState interface.
 *
 * This class manages the operating mode state and blocking states
 * for a rippled node. Extracted from NetworkOPsImp as part of the
 * NetworkOPs decomposition effort.
 *
 * Thread-safe: All state changes use atomic operations.
 */
class NetworkStateImpl final : public INetworkState
{
public:
    /**
     * Callback type for notifying of mode changes.
     */
    using ModeChangeCallback = std::function<void()>;

    /**
     * Callback type for getting validated ledger age.
     */
    using GetValidatedLedgerAgeCallback = std::function<std::chrono::seconds()>;

    /**
     * Construct a NetworkStateImpl.
     *
     * @param startMode Initial operating mode
     * @param journal Logger for state transitions
     * @param onModeChange Callback invoked after mode changes
     * @param getValidatedLedgerAge Callback to get validated ledger age
     */
    NetworkStateImpl(
        OperatingMode startMode,
        beast::Journal journal,
        ModeChangeCallback onModeChange,
        GetValidatedLedgerAgeCallback getValidatedLedgerAge);

    ~NetworkStateImpl() override;

    // Non-copyable, non-movable
    NetworkStateImpl(NetworkStateImpl const&) = delete;
    NetworkStateImpl&
    operator=(NetworkStateImpl const&) = delete;
    NetworkStateImpl(NetworkStateImpl&&) = delete;
    NetworkStateImpl&
    operator=(NetworkStateImpl&&) = delete;

    // --- INetworkState interface ---

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

    // --- Additional methods ---

    /** Output state accounting data to JSON. */
    void
    stateAccounting(Json::Value& obj) const;

private:
    class StateAccounting;
    std::unique_ptr<StateAccounting> accounting_;

    beast::Journal journal_;
    ModeChangeCallback onModeChange_;
    GetValidatedLedgerAgeCallback getValidatedLedgerAge_;

    std::atomic<OperatingMode> mode_;
    std::atomic<bool> needNetworkLedger_{false};
    std::atomic<bool> amendmentBlocked_{false};
    std::atomic<bool> amendmentWarned_{false};
    std::atomic<bool> unlBlocked_{false};
};

}  // namespace xrpl

#endif
