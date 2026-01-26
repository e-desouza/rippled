//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2024 Ripple Labs Inc.

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

#ifndef XRPLD_OVERLAY_ILEDGERMASTEROPS_H_INCLUDED
#define XRPLD_OVERLAY_ILEDGERMASTEROPS_H_INCLUDED

#include <xrpl/basics/Blob.h>
#include <xrpl/basics/UptimeClock.h>
#include <xrpl/basics/chrono.h>
#include <xrpl/protocol/Protocol.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

// Forward declarations to minimize includes
namespace protocol {
class TMGetObjectByHash;
}  // namespace protocol

namespace xrpl {

class Ledger;
class Peer;

// Pull protocol namespace into xrpl
using ::protocol::TMGetObjectByHash;

/** Interface for LedgerMaster operations used by the overlay module.
    This interface allows overlay components (e.g., PeerImp) to access
    ledger operations without directly depending on LedgerMaster.h.
*/
class ILedgerMasterOps
{
public:
    virtual ~ILedgerMasterOps() = default;

    /** Get the age of the validated ledger.
        @return Age in seconds since the last validated ledger.
    */
    virtual std::chrono::seconds
    getValidatedLedgerAge() = 0;

    /** Get the sequence number of the validated ledger.
        @return Sequence number of the most recent validated ledger.
    */
    virtual LedgerIndex
    getValidLedgerIndex() = 0;

    /** Check if we have a ledger with the given sequence number.
        @param seq The ledger sequence to check.
        @return true if we have the ledger, false otherwise.
    */
    virtual bool
    haveLedger(std::uint32_t seq) = 0;

    /** Add data to the fetch pack.
        @param hash Hash of the data.
        @param data The data to add.
    */
    virtual void
    addFetchPack(uint256 const& hash, std::shared_ptr<Blob> data) = 0;

    /** Notify that we got a fetch pack.
        @param progress Whether progress was made.
        @param seq The ledger sequence.
    */
    virtual void
    gotFetchPack(bool progress, std::uint32_t seq) = 0;

    /** Make a fetch pack for a peer.
        @param wPeer Weak pointer to the peer.
        @param request The request message.
        @param haveLedgerHash Hash of the ledger we have.
        @param uptime Time of the request.
    */
    virtual void
    makeFetchPack(
        std::weak_ptr<Peer> const& wPeer,
        std::shared_ptr<TMGetObjectByHash> const& request,
        uint256 haveLedgerHash,
        UptimeClock::time_point uptime) = 0;

    /** Get a ledger by its hash.
        @param hash The hash of the ledger.
        @return Shared pointer to the ledger, or nullptr if not found.
    */
    virtual std::shared_ptr<Ledger const>
    getLedgerByHash(uint256 const& hash) = 0;

    /** Get the earliest ledger sequence we can fetch.
        @return The earliest fetchable ledger sequence.
    */
    virtual std::uint32_t
    getEarliestFetch() = 0;

    /** Get a ledger by its sequence number.
        @param index The sequence number.
        @return Shared pointer to the ledger, or nullptr if not found.
    */
    virtual std::shared_ptr<Ledger const>
    getLedgerBySeq(std::uint32_t index) = 0;

    /** Get the closed ledger.
        @return Shared pointer to the closed ledger.
    */
    virtual std::shared_ptr<Ledger const>
    getClosedLedger() = 0;
};

}  // namespace xrpl

#endif
