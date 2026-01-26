// This header provides both ledger-level and RPC-level DeliveredAmount functions.
// For just the ledger-level function, include <xrpl/ledger/DeliveredAmount.h>
#ifndef XRPL_RPC_DELIVEREDAMOUNT_H_INCLUDED
#define XRPL_RPC_DELIVEREDAMOUNT_H_INCLUDED

#include <xrpl/ledger/DeliveredAmount.h>
#include <xrpl/protocol/Protocol.h>
#include <xrpl/protocol/STAmount.h>

#include <functional>
#include <memory>

namespace Json {
class Value;
}

namespace xrpl {

class Transaction;
class TxMeta;
class STTx;

namespace RPC {

struct JsonContext;
struct Context;

// Re-export the protocol-level function in RPC namespace for backward compatibility
using xrpl::insertDeliveredAmount;

/**
   Add a `delivered_amount` field to the `meta` input/output parameter.
   These overloads use RPC context to look up ledger information.

   @{
 */
void
insertDeliveredAmount(
    Json::Value& meta,
    RPC::JsonContext const&,
    std::shared_ptr<Transaction> const&,
    TxMeta const&);

void
insertDeliveredAmount(
    Json::Value& meta,
    RPC::JsonContext const&,
    std::shared_ptr<STTx const> const&,
    TxMeta const&);

std::optional<STAmount>
getDeliveredAmount(
    RPC::Context const& context,
    std::shared_ptr<STTx const> const& serializedTx,
    TxMeta const& transactionMeta,
    LedgerIndex const& ledgerIndex);
/** @} */

}  // namespace RPC
}  // namespace xrpl

#endif
