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

#ifndef XRPL_CONSENSUS_ILEDGERPROVIDER_H_INCLUDED
#define XRPL_CONSENSUS_ILEDGERPROVIDER_H_INCLUDED

#include <xrpl/beast/utility/Journal.h>
#include <xrpl/json/json_value.h>
#include <xrpl/protocol/Protocol.h>
#include <xrpl/protocol/RippleLedgerHash.h>

#include <cstdint>
#include <memory>

namespace xrpl {

class Ledger;
class LedgerReplay;
class ReadView;

/**
 * @brief Interface for ledger access operations needed by consensus.
 *
 * This interface abstracts the ledger retrieval and management operations
 * that the consensus layer requires. It enables decoupling the consensus
 * subsystem from direct dependencies on LedgerMaster and other concrete
 * ledger management implementations.
 *
 * The interface provides:
 * - Ledger retrieval by hash and sequence
 * - Validated ledger range queries
 * - Ledger compatibility checks
 * - Mutating operations for consensus (storing, switching LCL)
 * - Ledger replay support
 *
 * Thread-safe: All implementations must be thread-safe.
 *
 * @see LedgerMaster (concrete implementation)
 */
class ILedgerProvider
{
public:
    virtual ~ILedgerProvider() = default;

    //--------------------------------------------------------------------------
    // Ledger Retrieval
    //--------------------------------------------------------------------------

    /** Retrieve a ledger by its hash.
     *  @param hash The hash of the ledger to retrieve.
     *  @return The ledger if found, nullptr otherwise.
     */
    virtual std::shared_ptr<Ledger const>
    getLedgerByHash(LedgerHash const& hash) = 0;

    //--------------------------------------------------------------------------
    // Validated Range Queries
    //--------------------------------------------------------------------------

    /** Get the full range of validated ledgers.
     *  @param min Output parameter for the minimum validated ledger index.
     *  @param max Output parameter for the maximum validated ledger index.
     *  @return true if a validated range exists, false otherwise.
     */
    virtual bool
    getFullValidatedRange(std::uint32_t& min, std::uint32_t& max) = 0;

    /** Get the earliest ledger index that can be fetched.
     *  @return The earliest fetchable ledger index.
     */
    virtual LedgerIndex
    getEarliestFetch() = 0;

    /** Get the index of the current valid ledger.
     *  @return The valid ledger index.
     */
    virtual LedgerIndex
    getValidLedgerIndex() = 0;

    /** Get the current validated ledger.
     *  @return The validated ledger, or nullptr if none.
     */
    virtual std::shared_ptr<Ledger const>
    getValidatedLedger() = 0;

    /** Check if we have a validated ledger.
     *  @return true if a validated ledger exists.
     */
    virtual bool
    haveValidated() const = 0;

    //--------------------------------------------------------------------------
    // Compatibility Check
    //--------------------------------------------------------------------------

    /** Check if a ledger is compatible with the current consensus state.
     *  @param view The ledger view to check.
     *  @param s Journal stream for logging.
     *  @param reason Description of why compatibility is being checked.
     *  @return true if compatible, false otherwise.
     */
    virtual bool
    isCompatible(
        ReadView const& view,
        beast::Journal::Stream s,
        char const* reason) = 0;

    //--------------------------------------------------------------------------
    // Mutating Operations
    //--------------------------------------------------------------------------

    /** Apply any held transactions to the current open ledger. */
    virtual void
    applyHeldTransactions() = 0;

    /** Set the ledger index currently being built.
     *  @param index The ledger index being built.
     */
    virtual void
    setBuildingLedger(LedgerIndex index) = 0;

    /** Store a ledger in the ledger history.
     *  @param ledger The ledger to store.
     *  @return true if the ledger was already present.
     */
    virtual bool
    storeLedger(std::shared_ptr<Ledger const> ledger) = 0;

    /** Switch the last closed ledger.
     *  @param ledger The new last closed ledger.
     */
    virtual void
    switchLCL(std::shared_ptr<Ledger const> ledger) = 0;

    /** Notify that consensus has built a new ledger.
     *  @param ledger The newly built ledger.
     *  @param consensusHash The hash of the consensus transaction set.
     *  @param consensusJson JSON representation of the consensus result.
     */
    virtual void
    consensusBuilt(
        std::shared_ptr<Ledger const> ledger,
        uint256 const& consensusHash,
        Json::Value consensusJson) = 0;

    /** Release and return the current ledger replay data.
     *  @return The ledger replay data, or nullptr if none.
     */
    virtual std::unique_ptr<LedgerReplay>
    releaseReplay() = 0;
};

}  // namespace xrpl

#endif
