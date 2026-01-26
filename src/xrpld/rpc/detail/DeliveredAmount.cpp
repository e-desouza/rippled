// This file contains RPC-specific DeliveredAmount functions.
// The protocol-level insertDeliveredAmount(Json::Value&, ReadView const&, ...)
// is now implemented in src/libxrpl/protocol/DeliveredAmount.cpp

#include <xrpld/app/ledger/LedgerDataProvider.h>
#include <xrpld/app/misc/Transaction.h>
#include <xrpld/rpc/Context.h>
#include <xrpld/rpc/DeliveredAmount.h>

#include <xrpl/protocol/Feature.h>
#include <xrpl/protocol/RPCErr.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/TxFormats.h>

#include <chrono>

namespace xrpl {
namespace RPC {

namespace {

/*
  GetLedgerIndex and GetCloseTime are lambdas that allow the close time and
  ledger index to be lazily calculated.
 */
template <class GetLedgerIndex, class GetCloseTime>
std::optional<STAmount>
getDeliveredAmountImpl(
    GetLedgerIndex const& getLedgerIndex,
    GetCloseTime const& getCloseTime,
    std::shared_ptr<STTx const> const& serializedTx,
    TxMeta const& transactionMeta)
{
    if (!serializedTx)
        return {};

    if (auto const& deliveredAmount = transactionMeta.getDeliveredAmount();
        deliveredAmount.has_value())
    {
        return *deliveredAmount;
    }

    if (serializedTx->isFieldPresent(sfAmount))
    {
        using namespace std::chrono_literals;

        if (getLedgerIndex() >= 4594095 ||
            getCloseTime() > NetClock::time_point{446000000s})
        {
            return serializedTx->getFieldAmount(sfAmount);
        }
    }

    return {};
}

// Returns true if transaction meta could contain a delivered amount field
bool
canHaveDeliveredAmount(
    std::shared_ptr<STTx const> const& serializedTx,
    TxMeta const& transactionMeta)
{
    if (!serializedTx)
        return false;

    TxType const tt{serializedTx->getTxnType()};
    if ((tt == ttPAYMENT || tt == ttCHECK_CASH || tt == ttACCOUNT_DELETE) &&
        transactionMeta.getResultTER() == tesSUCCESS)
    {
        return true;
    }

    return false;
}

template <class GetLedgerIndex>
std::optional<STAmount>
getDeliveredAmountWithContext(
    RPC::Context const& context,
    std::shared_ptr<STTx const> const& serializedTx,
    TxMeta const& transactionMeta,
    GetLedgerIndex const& getLedgerIndex)
{
    if (canHaveDeliveredAmount(serializedTx, transactionMeta))
    {
        auto const getCloseTime =
            [&context,
             &getLedgerIndex]() -> std::optional<NetClock::time_point> {
            return context.ledgerDataProvider.getCloseTimeBySeq(
                getLedgerIndex());
        };
        return getDeliveredAmountImpl(
            getLedgerIndex, getCloseTime, serializedTx, transactionMeta);
    }

    return {};
}

}  // namespace

std::optional<STAmount>
getDeliveredAmount(
    RPC::Context const& context,
    std::shared_ptr<STTx const> const& serializedTx,
    TxMeta const& transactionMeta,
    LedgerIndex const& ledgerIndex)
{
    return getDeliveredAmountWithContext(
        context, serializedTx, transactionMeta, [&ledgerIndex]() {
            return ledgerIndex;
        });
}

void
insertDeliveredAmount(
    Json::Value& meta,
    RPC::JsonContext const& context,
    std::shared_ptr<Transaction> const& transaction,
    TxMeta const& transactionMeta)
{
    insertDeliveredAmount(
        meta, context, transaction->getSTransaction(), transactionMeta);
}

void
insertDeliveredAmount(
    Json::Value& meta,
    RPC::JsonContext const& context,
    std::shared_ptr<STTx const> const& transaction,
    TxMeta const& transactionMeta)
{
    if (canHaveDeliveredAmount(transaction, transactionMeta))
    {
        auto amt = getDeliveredAmountWithContext(
            context, transaction, transactionMeta, [&transactionMeta]() {
                return transactionMeta.getLgrSeq();
            });

        if (amt)
        {
            meta[jss::delivered_amount] =
                amt->getJson(JsonOptions::include_date);
        }
        else
        {
            // report "unavailable" which cannot be parsed into a sensible
            // amount.
            meta[jss::delivered_amount] = Json::Value("unavailable");
        }
    }
}

}  // namespace RPC
}  // namespace xrpl
