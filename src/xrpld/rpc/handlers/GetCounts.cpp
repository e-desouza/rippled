#include <xrpld/app/misc/ServerCounts.h>
#include <xrpld/rpc/Context.h>

#include <xrpl/protocol/jss.h>

namespace xrpl {

// {
//   min_count: <number>  // optional, defaults to 10
// }
Json::Value
doGetCounts(RPC::JsonContext& context)
{
    int minCount = 10;

    if (context.params.isMember(jss::min_count))
        minCount = context.params[jss::min_count].asUInt();

    return getCountsJson(context.app, minCount);
}

}  // namespace xrpl
