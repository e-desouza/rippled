#include <xrpld/app/rpc/make_GRPCServer.h>
#include <xrpld/rpc/GRPCServer.h>

namespace xrpl {

std::unique_ptr<IGRPCServer>
make_GRPCServer(Application& app)
{
    return std::make_unique<GRPCServer>(app);
}

}  // namespace xrpl
