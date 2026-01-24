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

#ifndef XRPL_OVERLAY_PEERMETRICS_H_INCLUDED
#define XRPL_OVERLAY_PEERMETRICS_H_INCLUDED

#include <boost/circular_buffer.hpp>

#include <chrono>
#include <cstdint>
#include <numeric>
#include <shared_mutex>

namespace xrpl {

/**
 * @brief Tracks message throughput metrics for a peer connection.
 *
 * This class maintains a rolling average of message bytes over time,
 * using a 30-second window with 1-second intervals. It is thread-safe
 * and can be accessed from multiple threads.
 */
class PeerMetrics
{
public:
    using clock_type = std::chrono::steady_clock;

    PeerMetrics() = default;
    ~PeerMetrics() = default;

    // Non-copyable, non-movable
    PeerMetrics(PeerMetrics const&) = delete;
    PeerMetrics&
    operator=(PeerMetrics const&) = delete;
    PeerMetrics(PeerMetrics&&) = delete;
    PeerMetrics&
    operator=(PeerMetrics&&) = delete;

    /** Add a message's bytes to the metrics */
    void
    add_message(std::uint64_t bytes)
    {
        using namespace std::chrono_literals;
        std::unique_lock lock{mutex_};

        totalBytes_ += bytes;
        accumBytes_ += bytes;
        auto const timeElapsed = clock_type::now() - intervalStart_;
        auto const timeElapsedInSecs =
            std::chrono::duration_cast<std::chrono::seconds>(timeElapsed);

        if (timeElapsedInSecs >= 1s)
        {
            auto const avgBytes = accumBytes_ / timeElapsedInSecs.count();
            rollingAvg_.push_back(avgBytes);

            auto const totalBytes =
                std::accumulate(rollingAvg_.begin(), rollingAvg_.end(), 0ull);
            rollingAvgBytes_ = totalBytes / rollingAvg_.size();

            intervalStart_ = clock_type::now();
            accumBytes_ = 0;
        }
    }

    /** Get the rolling average bytes per interval */
    std::uint64_t
    average_bytes() const
    {
        std::shared_lock lock{mutex_};
        return rollingAvgBytes_;
    }

    /** Get the total bytes recorded */
    std::uint64_t
    total_bytes() const
    {
        std::shared_lock lock{mutex_};
        return totalBytes_;
    }

private:
    mutable std::shared_mutex mutex_;
    boost::circular_buffer<std::uint64_t> rollingAvg_{30, 0ull};
    clock_type::time_point intervalStart_{clock_type::now()};
    std::uint64_t totalBytes_{0};
    std::uint64_t accumBytes_{0};
    std::uint64_t rollingAvgBytes_{0};
};

/**
 * @brief Container for both send and receive metrics.
 */
struct PeerMetricsBundle
{
    PeerMetrics sent;
    PeerMetrics recv;
};

}  // namespace xrpl

#endif
