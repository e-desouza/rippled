#ifndef XRPL_APP_LEDGER_LEDGERTOJSON_H_INCLUDED
#define XRPL_APP_LEDGER_LEDGERTOJSON_H_INCLUDED

#include <xrpld/app/ledger/Ledger.h>
#include <xrpld/app/ledger/LedgerMaster.h>
#include <xrpld/app/txqueue/TxQ.h>

#include <xrpl/basics/chrono.h>
#include <xrpl/beast/utility/Journal.h>
#include <xrpl/protocol/ApiVersion.h>
#include <xrpl/protocol/serialize.h>

namespace xrpl {

namespace RPC {
struct Context;  // Forward declaration for backward compatibility
}

struct LedgerFill
{
    // Primary constructor - takes explicit parameters (preferred)
    LedgerFill(
        ReadView const& l,
        LedgerMaster* lm,
        unsigned int apiVer,
        beast::Journal journal = beast::Journal{beast::Journal::getNullSink()},
        int o = 0,
        std::vector<TxQ::TxDetails> q = {})
        : ledger(l)
        , options(o)
        , txQueue(std::move(q))
        , apiVersion(apiVer)
        , j(journal)
    {
        if (lm)
        {
            closeTime = lm->getCloseTimeBySeq(ledger.seq());
            validated = lm->isValidated(ledger);
        }
    }

    // Backward-compatible constructor for RPC context
    // DEPRECATED: Use the primary constructor instead
    LedgerFill(
        ReadView const& l,
        RPC::Context const* ctx,
        int o = 0,
        std::vector<TxQ::TxDetails> q = {});

    enum Options {
        dumpTxrp = 1,
        dumpState = 2,
        expand = 4,
        full = 8,
        binary = 16,
        ownerFunds = 32,
        dumpQueue = 64
    };

    ReadView const& ledger;
    int options;
    std::vector<TxQ::TxDetails> txQueue;
    unsigned int apiVersion{RPC::apiMaximumSupportedVersion};
    beast::Journal j;
    std::optional<NetClock::time_point> closeTime;
    std::optional<bool> validated;
};

/** Given a Ledger and options, fill a Json::Value with a
    description of the ledger.
 */
void
addJson(Json::Value&, LedgerFill const&);

/** Return a new Json::Value representing the ledger with given options.*/
Json::Value
getJson(LedgerFill const&);

/** Copy all the keys and values from one object into another. */
void
copyFrom(Json::Value& to, Json::Value const& from);

}  // namespace xrpl

#endif
