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

#ifndef XRPLD_APP_OVERLAY_HANDLERS_LEDGERREQUESTHANDLER_H_INCLUDED
#define XRPLD_APP_OVERLAY_HANDLERS_LEDGERREQUESTHANDLER_H_INCLUDED

#include <xrpld/app/ledger/LedgerMaster.h>
#include <xrpld/overlay/ILedgerRequestHandler.h>

#include <xrpl/beast/utility/Journal.h>

namespace xrpl {

/** Handler for processing ledger data requests.
    This handler encapsulates all ledger access needed to respond to
    TMGetLedger requests, allowing the overlay module to avoid direct
    dependency on Ledger.h.
*/
class LedgerRequestHandler : public ILedgerRequestHandler
{
private:
    LedgerMaster& ledgerMaster_;
    beast::Journal journal_;

public:
    LedgerRequestHandler(LedgerMaster& ledgerMaster, beast::Journal journal)
        : ledgerMaster_(ledgerMaster), journal_(journal)
    {
    }

    std::shared_ptr<Message>
    processLedgerDataRequest(
        std::shared_ptr<protocol::TMGetLedger> const& m,
        bool isHighLatency) override;

private:
    /** Get a ledger based on the request parameters.
        @param m The request message.
        @return The ledger, or nullptr if not found.
    */
    std::shared_ptr<Ledger const>
    getLedger(std::shared_ptr<protocol::TMGetLedger> const& m);

    /** Send base ledger data (header and root nodes).
        @param ledger The ledger to send.
        @param ledgerData The response message to populate.
    */
    void
    sendLedgerBase(
        std::shared_ptr<Ledger const> const& ledger,
        protocol::TMLedgerData& ledgerData);
};

}  // namespace xrpl

#endif
