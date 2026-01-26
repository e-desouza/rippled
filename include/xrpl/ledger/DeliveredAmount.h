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

#ifndef XRPL_LEDGER_DELIVEREDAMOUNT_H_INCLUDED
#define XRPL_LEDGER_DELIVEREDAMOUNT_H_INCLUDED

#include <xrpl/protocol/STAmount.h>

#include <memory>

namespace Json {
class Value;
}

namespace xrpl {

class ReadView;
class TxMeta;
class STTx;

/**
   Add a `delivered_amount` field to the `meta` input/output parameter.
   The field is only added to successful payment and check cash transactions.
   If a delivered amount field is available in the TxMeta parameter, that value
   is used. Otherwise, the transaction's `Amount` field is used. If neither is
   available, then the delivered amount is set to "unavailable".

   This overload uses a ReadView to get ledger information and does not require
   any RPC context.

   @param meta The JSON object to add the delivered_amount field to
   @param ledger The ledger view to get header information from
   @param serializedTx The transaction
   @param transactionMeta The transaction metadata
 */
void
insertDeliveredAmount(
    Json::Value& meta,
    ReadView const& ledger,
    std::shared_ptr<STTx const> const& serializedTx,
    TxMeta const& transactionMeta);

}  // namespace xrpl

#endif

