#ifndef XRPL_APP_RPC_MAKE_GRPCSERVER_H_INCLUDED
#define XRPL_APP_RPC_MAKE_GRPCSERVER_H_INCLUDED

#include <xrpld/app/rpc/IServerComponent.h>

#include <memory>

namespace xrpl {

class Application;

/**
 * Create a gRPC server.
 * Returns IGRPCServer interface to avoid app→rpc dependency.
 */
std::unique_ptr<IGRPCServer>
make_GRPCServer(Application& app);

}  // namespace xrpl

#endif
