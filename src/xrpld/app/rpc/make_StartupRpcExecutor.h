#ifndef XRPL_APP_RPC_MAKE_STARTUPRPCEXECUTOR_H_INCLUDED
#define XRPL_APP_RPC_MAKE_STARTUPRPCEXECUTOR_H_INCLUDED

#include <xrpld/app/rpc/IStartupRpcExecutor.h>

#include <memory>

namespace xrpl {

/**
 * Factory function to create a startup RPC executor.
 *
 * The implementation is provided by the rpc module, allowing the app
 * module to use RPC functionality without directly depending on
 * RPC implementation headers.
 */
std::unique_ptr<IStartupRpcExecutor>
make_StartupRpcExecutor();

}  // namespace xrpl

#endif
