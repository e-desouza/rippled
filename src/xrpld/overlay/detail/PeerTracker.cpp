#include <xrpld/overlay/detail/PeerTracker.h>

#include <algorithm>

namespace xrpl {

PeerTracker::PeerTracker()
    : tracking_(Tracking::unknown), trackingTime_(clock_type::now())
{
}

PeerTracker::Tracking
PeerTracker::tracking() const
{
    return tracking_.load();
}

void
PeerTracker::setTracking(Tracking t)
{
    tracking_.store(t);
}

PeerTracker::clock_type::time_point
PeerTracker::trackingTime() const
{
    std::lock_guard sl(recentLock_);
    return trackingTime_;
}

void
PeerTracker::setTrackingTime(clock_type::time_point t)
{
    std::lock_guard sl(recentLock_);
    trackingTime_ = t;
}

void
PeerTracker::ledgerRange(std::uint32_t& minSeq, std::uint32_t& maxSeq) const
{
    std::lock_guard sl(recentLock_);
    minSeq = minLedger_;
    maxSeq = maxLedger_;
}

void
PeerTracker::setLedgerRange(std::uint32_t minSeq, std::uint32_t maxSeq)
{
    std::lock_guard sl(recentLock_);
    minLedger_ = minSeq;
    maxLedger_ = maxSeq;
}

bool
PeerTracker::hasRange(std::uint32_t uMin, std::uint32_t uMax) const
{
    std::lock_guard sl(recentLock_);
    return (tracking_.load() != Tracking::diverged) && (uMin >= minLedger_) &&
        (uMax <= maxLedger_);
}

std::uint32_t
PeerTracker::maxLedger() const
{
    std::lock_guard sl(recentLock_);
    return maxLedger_;
}

uint256 const&
PeerTracker::closedLedgerHash() const
{
    // Note: caller must hold recentLock_ or ensure thread safety externally
    return closedLedgerHash_;
}

uint256 const&
PeerTracker::previousLedgerHash() const
{
    // Note: caller must hold recentLock_ or ensure thread safety externally
    return previousLedgerHash_;
}

void
PeerTracker::setClosedLedgerHash(uint256 const& hash)
{
    std::lock_guard sl(recentLock_);
    closedLedgerHash_ = hash;
}

void
PeerTracker::setPreviousLedgerHash(uint256 const& hash)
{
    std::lock_guard sl(recentLock_);
    previousLedgerHash_ = hash;
}

bool
PeerTracker::hasLedger(uint256 const& hash, std::uint32_t seq) const
{
    std::lock_guard sl(recentLock_);

    if ((seq != 0) && (seq >= minLedger_) && (seq <= maxLedger_) &&
        (tracking_.load() == Tracking::converged))
    {
        return true;
    }

    if (std::find(recentLedgers_.begin(), recentLedgers_.end(), hash) !=
        recentLedgers_.end())
    {
        return true;
    }

    return false;
}

bool
PeerTracker::hasTxSet(uint256 const& hash) const
{
    std::lock_guard sl(recentLock_);
    return std::find(recentTxSets_.begin(), recentTxSets_.end(), hash) !=
        recentTxSets_.end();
}

void
PeerTracker::addLedger(uint256 const& hash)
{
    std::lock_guard sl(recentLock_);
    recentLedgers_.push_back(hash);
}

void
PeerTracker::addTxSet(uint256 const& hash)
{
    std::lock_guard sl(recentLock_);
    recentTxSets_.push_back(hash);
}

void
PeerTracker::cycleStatus()
{
    // Operations on closedLedgerHash_ and previousLedgerHash_ must be
    // guarded by recentLock_.
    std::lock_guard sl(recentLock_);
    previousLedgerHash_ = closedLedgerHash_;
    closedLedgerHash_.zero();
}

}  // namespace xrpl
