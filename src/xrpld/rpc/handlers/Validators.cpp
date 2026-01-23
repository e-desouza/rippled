#include <xrpld/app/main/Application.h>
#include <xrpld/app/validators/ValidatorList.h>
#include <xrpld/rpc/Context.h>

#include <xrpl/protocol/ErrorCodes.h>

namespace xrpl {

Json::Value
doValidators(RPC::JsonContext& context)
{
    return context.app.validators().getJson();
}

}  // namespace xrpl
