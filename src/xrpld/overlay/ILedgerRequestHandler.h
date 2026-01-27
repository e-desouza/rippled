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

#ifndef XRPLD_OVERLAY_ILEDGERREQUESTHANDLER_H_INCLUDED
#define XRPLD_OVERLAY_ILEDGERREQUESTHANDLER_H_INCLUDED

#include <xrpl/protocol/messages.h>

#include <memory>
#include <optional>

namespace xrpl {

class Message;

/** Interface for handling ledger data requests.
    This interface allows overlay components to request ledger data
    without directly depending on Ledger.h.

    Note: This handler only processes ledger-based requests (liBASE,
    liTX_NODE, liAS_NODE). Transaction set requests (liTS_CANDIDATE)
    are handled separately by PeerImp since they involve peer relay logic.
*/
class ILedgerRequestHandler
{
public:
    virtual ~ILedgerRequestHandler() = default;

    /** Process a ledger data request and return the response message.
        @param m The ledger request message (must not be liTS_CANDIDATE).
        @param isHighLatency Whether the peer is high latency.
        @return The response message to send, or nullptr if no response.
    */
    virtual std::shared_ptr<Message>
    processLedgerDataRequest(
        std::shared_ptr<protocol::TMGetLedger> const& m,
        bool isHighLatency) = 0;
};

}  // namespace xrpl

#endif
