#include <xrpld/app/ledger/LedgerMaster.h>
#include <xrpld/app/main/Application.h>
#include <xrpld/app/misc/NetworkOPs.h>
#include <xrpld/app/rpc/IStartupRpcExecutor.h>
#include <xrpld/app/rpc/make_StartupRpcExecutor.h>
#include <xrpld/rpc/Context.h>
#include <xrpld/rpc/RPCHandler.h>

#include <xrpl/protocol/ApiVersion.h>
#include <xrpl/resource/Charge.h>
#include <xrpl/resource/Consumer.h>
#include <xrpl/resource/Fees.h>

namespace xrpl {

namespace {

class StartupRpcExecutor : public IStartupRpcExecutor
{
public:
    Json::Value
    execute(Application& app, Json::Value const& command) override
    {
        Resource::Charge loadType = Resource::feeReferenceRPC;
        Resource::Consumer consumer;

        RPC::JsonContext context{
            {app.journal("RPCHandler"),
             app,
             loadType,
             app.getOPs(),
             app.getLedgerMaster(),
             app.getLedgerMaster(),
             app.getOPs(),
             consumer,
             Role::ADMIN,
             {},
             {},
             RPC::apiMaximumSupportedVersion},
            command};

        Json::Value result;
        RPC::doCommand(context, result);
        return result;
    }
};

}  // namespace

std::unique_ptr<IStartupRpcExecutor>
make_StartupRpcExecutor()
{
    return std::make_unique<StartupRpcExecutor>();
}

}  // namespace xrpl
