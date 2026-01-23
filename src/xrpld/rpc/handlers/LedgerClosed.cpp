#include <xrpld/app/ledger/Ledger.h>
#include <xrpld/rpc/Context.h>

#include <xrpl/json/json_value.h>
#include <xrpl/protocol/jss.h>

namespace xrpl {

Json::Value
doLedgerClosed(RPC::JsonContext& context)
{
    auto ledger = context.ledgerDataProvider.getClosedLedger();
    XRPL_ASSERT(ledger, "xrpl::doLedgerClosed : non-null closed ledger");

    Json::Value jvResult;
    jvResult[jss::ledger_index] = ledger->header().seq;
    jvResult[jss::ledger_hash] = to_string(ledger->header().hash);

    return jvResult;
}

}  // namespace xrpl
