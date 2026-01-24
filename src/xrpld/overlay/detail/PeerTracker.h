#ifndef XRPL_OVERLAY_PEERTRACKER_H_INCLUDED
#define XRPL_OVERLAY_PEERTRACKER_H_INCLUDED

#include <xrpl/basics/base_uint.h>
#include <xrpl/protocol/Protocol.h>

#include <boost/circular_buffer.hpp>

#include <atomic>
#include <chrono>
#include <mutex>

namespace xrpl {

/**
 * @brief Tracks a peer's ledger state and recent items.
 *
 * This class encapsulates the tracking state for a peer connection,
 * including ledger range information, closed/previous ledger hashes,
 * and circular buffers for recently seen ledgers and transaction sets.
 *
 * Thread-safe: All public methods are protected by internal locking.
 */
class PeerTracker
{
public:
    /** Whether the peer's view of the ledger converges or diverges from ours */
    enum class Tracking { diverged, unknown, converged };

    using clock_type = std::chrono::steady_clock;

    PeerTracker();
    ~PeerTracker() = default;

    // Non-copyable, non-movable
    PeerTracker(PeerTracker const&) = delete;
    PeerTracker&
    operator=(PeerTracker const&) = delete;
    PeerTracker(PeerTracker&&) = delete;
    PeerTracker&
    operator=(PeerTracker&&) = delete;

    // Tracking state
    Tracking
    tracking() const;

    void
    setTracking(Tracking t);

    clock_type::time_point
    trackingTime() const;

    void
    setTrackingTime(clock_type::time_point t);

    // Ledger range
    void
    ledgerRange(std::uint32_t& minSeq, std::uint32_t& maxSeq) const;

    void
    setLedgerRange(std::uint32_t minSeq, std::uint32_t maxSeq);

    bool
    hasRange(std::uint32_t uMin, std::uint32_t uMax) const;

    std::uint32_t
    maxLedger() const;

    // Closed/previous ledger
    uint256 const&
    closedLedgerHash() const;

    uint256 const&
    previousLedgerHash() const;

    void
    setClosedLedgerHash(uint256 const& hash);

    void
    setPreviousLedgerHash(uint256 const& hash);

    // Recent ledgers/txsets
    bool
    hasLedger(uint256 const& hash, std::uint32_t seq) const;

    bool
    hasTxSet(uint256 const& hash) const;

    void
    addLedger(uint256 const& hash);

    void
    addTxSet(uint256 const& hash);

    void
    cycleStatus();

private:
    mutable std::mutex recentLock_;

    std::atomic<Tracking> tracking_{Tracking::unknown};
    clock_type::time_point trackingTime_{clock_type::now()};

    LedgerIndex minLedger_{0};
    LedgerIndex maxLedger_{0};
    uint256 closedLedgerHash_;
    uint256 previousLedgerHash_;

    boost::circular_buffer<uint256> recentLedgers_{128};
    boost::circular_buffer<uint256> recentTxSets_{128};
};

}  // namespace xrpl

#endif
