#include <xrpld/app/main/RpcCliMain.h>
#include <xrpld/rpc/RPCCall.h>

namespace xrpl {

int
rpcCliMain(
    Config const& config,
    std::vector<std::string> const& vCmd,
    Logs& logs)
{
    return RPCCall::fromCommandLine(config, vCmd, logs);
}

}  // namespace xrpl

