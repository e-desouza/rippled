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

#include <xrpld/app/misc/OperatingModeManager.h>
#include <xrpl/json/json_value.h>
#include <xrpl/protocol/jss.h>

#include <array>
#include <chrono>
#include <mutex>

namespace xrpl {

namespace {
static std::array<char const*, 5> const stateNames = {
    {"disconnected", "connected", "syncing", "tracking", "full"}};
}  // namespace

/**
 * State accounting records two attributes for each possible server state:
 * 1) Amount of time spent in each state (in microseconds). This value is
 *    updated upon each state transition.
 * 2) Number of transitions to each state.
 */
class OperatingModeManager::StateAccounting
{
    struct Counters
    {
        explicit Counters() = default;

        std::uint64_t transitions = 0;
        std::chrono::microseconds dur = std::chrono::microseconds(0);
    };

    OperatingMode mode_ = OperatingMode::DISCONNECTED;
    std::array<Counters, 5> counters_;
    mutable std::mutex mutex_;
    std::chrono::steady_clock::time_point start_ =
        std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point const processStart_ = start_;
    std::uint64_t initialSyncUs_{0};
    static std::array<Json::StaticString const, 5> const states_;

public:
    explicit StateAccounting()
    {
        counters_[static_cast<std::size_t>(OperatingMode::DISCONNECTED)]
            .transitions = 1;
    }

    void
    mode(OperatingMode om)
    {
        auto now = std::chrono::steady_clock::now();

        std::lock_guard lock(mutex_);
        ++counters_[static_cast<std::size_t>(om)].transitions;
        if (om == OperatingMode::FULL &&
            counters_[static_cast<std::size_t>(om)].transitions == 1)
        {
            initialSyncUs_ =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    now - processStart_)
                    .count();
        }
        counters_[static_cast<std::size_t>(mode_)].dur +=
            std::chrono::duration_cast<std::chrono::microseconds>(now - start_);

        mode_ = om;
        start_ = now;
    }

    void
    json(Json::Value& obj) const
    {
        auto [counters, mode, start, initialSync] = getCounterData();
        auto const current =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - start);
        counters[static_cast<std::size_t>(mode)].dur += current;

        obj[jss::state_accounting] = Json::objectValue;
        for (std::size_t i =
                 static_cast<std::size_t>(OperatingMode::DISCONNECTED);
             i <= static_cast<std::size_t>(OperatingMode::FULL);
             ++i)
        {
            obj[jss::state_accounting][states_[i]] = Json::objectValue;
            auto& state = obj[jss::state_accounting][states_[i]];
            state[jss::transitions] = std::to_string(counters[i].transitions);
            state[jss::duration_us] = std::to_string(counters[i].dur.count());
        }
        obj[jss::server_state_duration_us] = std::to_string(current.count());
        if (initialSync)
            obj[jss::initial_sync_duration_us] = std::to_string(initialSync);
    }

private:
    struct CounterData
    {
        decltype(counters_) counters;
        decltype(mode_) mode;
        decltype(start_) start;
        decltype(initialSyncUs_) initialSyncUs;
    };

    CounterData
    getCounterData() const
    {
        std::lock_guard lock(mutex_);
        return {counters_, mode_, start_, initialSyncUs_};
    }
};

std::array<Json::StaticString const, 5> const
    OperatingModeManager::StateAccounting::states_ = {
        {Json::StaticString(stateNames[0]),
         Json::StaticString(stateNames[1]),
         Json::StaticString(stateNames[2]),
         Json::StaticString(stateNames[3]),
         Json::StaticString(stateNames[4])}};

//------------------------------------------------------------------------------

OperatingModeManager::OperatingModeManager(
    OperatingMode startMode,
    beast::Journal journal,
    ModeChangeCallback onModeChange,
    GetValidatedLedgerAgeCallback getValidatedLedgerAge)
    : accounting_(std::make_unique<StateAccounting>())
    , journal_(journal)
    , onModeChange_(std::move(onModeChange))
    , getValidatedLedgerAge_(std::move(getValidatedLedgerAge))
    , mode_(startMode)
{
}

OperatingMode
OperatingModeManager::getOperatingMode() const
{
    return mode_;
}

std::string
OperatingModeManager::strOperatingMode(OperatingMode mode, bool admin) const
{
    if (isAmendmentBlocked())
    {
        if (admin)
            return "amendment blocked";
        return "syncing";
    }
    if (isUNLBlocked())
    {
        if (admin)
            return "unlBlocked";
        return "syncing";
    }
    return stateNames[static_cast<std::size_t>(mode)];
}

std::string
OperatingModeManager::strOperatingMode(bool admin) const
{
    return strOperatingMode(mode_, admin);
}

bool
OperatingModeManager::isFull() const
{
    return !isNeedNetworkLedger() && (mode_ == OperatingMode::FULL);
}

void
OperatingModeManager::setMode(OperatingMode om)
{
    using namespace std::chrono_literals;
    if (om == OperatingMode::CONNECTED)
    {
        if (getValidatedLedgerAge_() < 1min)
            om = OperatingMode::SYNCING;
    }
    else if (om == OperatingMode::SYNCING)
    {
        if (getValidatedLedgerAge_() >= 1min)
            om = OperatingMode::CONNECTED;
    }

    if ((om > OperatingMode::CONNECTED) && isBlocked())
        om = OperatingMode::CONNECTED;

    if (mode_ == om)
        return;

    mode_ = om;
    accounting_->mode(om);

    JLOG(journal_.info()) << "STATE->" << strOperatingMode();
    onModeChange_();
}

void
OperatingModeManager::setModeImpl(OperatingMode om)
{
    mode_ = om;
}

void
OperatingModeManager::setStandAlone()
{
    setMode(OperatingMode::FULL);
}

bool
OperatingModeManager::isBlocked() const
{
    return amendmentBlocked_ || unlBlocked_;
}

bool
OperatingModeManager::isAmendmentBlocked() const
{
    return amendmentBlocked_;
}

void
OperatingModeManager::setAmendmentBlocked()
{
    amendmentBlocked_ = true;
}

bool
OperatingModeManager::isAmendmentWarned() const
{
    return amendmentWarned_;
}

void
OperatingModeManager::setAmendmentWarned()
{
    amendmentWarned_ = true;
}

void
OperatingModeManager::clearAmendmentWarned()
{
    amendmentWarned_ = false;
}

bool
OperatingModeManager::isUNLBlocked() const
{
    return unlBlocked_;
}

void
OperatingModeManager::setUNLBlocked()
{
    unlBlocked_ = true;
}

void
OperatingModeManager::clearUNLBlocked()
{
    unlBlocked_ = false;
}

bool
OperatingModeManager::isNeedNetworkLedger() const
{
    return needNetworkLedger_;
}

void
OperatingModeManager::setNeedNetworkLedger()
{
    needNetworkLedger_ = true;
}

void
OperatingModeManager::clearNeedNetworkLedger()
{
    needNetworkLedger_ = false;
}

void
OperatingModeManager::stateAccounting(Json::Value& obj) const
{
    accounting_->json(obj);
}

}  // namespace xrpl

