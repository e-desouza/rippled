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

#ifndef XRPL_CONSENSUS_ICONSENSUSTIMESOURCE_H_INCLUDED
#define XRPL_CONSENSUS_ICONSENSUSTIMESOURCE_H_INCLUDED

#include <xrpl/basics/chrono.h>

#include <chrono>
#include <cstdint>

namespace xrpl {

/**
 * @brief Interface for time-related operations during consensus.
 *
 * This interface abstracts the time source functionality used by the
 * consensus layer. It allows the consensus module to query and adjust
 * time without depending directly on the application's clock
 * implementation.
 *
 * Thread-safe: Implementations must be thread-safe.
 */
class IConsensusTimeSource
{
public:
    virtual ~IConsensusTimeSource() = default;

    /**
     * @brief Get the current network time.
     *
     * @return The current time according to the network clock.
     */
    virtual NetClock::time_point
    now() const = 0;

    /**
     * @brief Get the expected close time for the current ledger.
     *
     * @return The expected close time.
     */
    virtual NetClock::time_point
    closeTime() const = 0;

    /**
     * @brief Adjust the close time offset.
     *
     * This is used to synchronize the local node's close time
     * with the network consensus.
     *
     * @param offset The adjustment to apply to the close time.
     */
    virtual void
    adjustCloseTime(std::chrono::duration<std::int32_t> offset) = 0;
};

}  // namespace xrpl

#endif
