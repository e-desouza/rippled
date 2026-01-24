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

#ifndef XRPL_CONSENSUS_ITXSETMANAGER_H_INCLUDED
#define XRPL_CONSENSUS_ITXSETMANAGER_H_INCLUDED

#include <xrpl/basics/base_uint.h>

#include <cstdint>
#include <memory>

namespace xrpl {

// Forward declarations
class SHAMap;

/**
 * @brief Interface for managing transaction sets during consensus.
 *
 * This interface abstracts the transaction set management functionality
 * used by the consensus layer. It allows the consensus module to acquire,
 * store, and manage transaction sets without depending directly on the
 * application's internal data structures.
 *
 * Thread-safe: Implementations must be thread-safe.
 */
class ITxSetManager
{
public:
    virtual ~ITxSetManager() = default;

    /**
     * @brief Retrieve a transaction set by hash.
     *
     * @param hash The hash of the transaction set to retrieve.
     * @param acquire If true, attempt to acquire the set from the network
     *                if not available locally.
     * @return A shared pointer to the transaction set, or nullptr if not found.
     */
    virtual std::shared_ptr<SHAMap>
    getSet(uint256 const& hash, bool acquire) = 0;

    /**
     * @brief Store a transaction set.
     *
     * @param hash The hash of the transaction set.
     * @param set The transaction set to store.
     * @param acquired Whether the set was acquired from the network.
     */
    virtual void
    giveSet(
        uint256 const& hash,
        std::shared_ptr<SHAMap> const& set,
        bool acquired) = 0;

    /**
     * @brief Notify the manager that a new consensus round is starting.
     *
     * This allows the manager to perform any cleanup or preparation
     * needed for the new round.
     *
     * @param seq The sequence number of the new round.
     */
    virtual void
    newRound(std::uint32_t seq) = 0;
};

}  // namespace xrpl

#endif
