#ifndef XRPL_APP_MAIN_RPCCLIMAIN_H_INCLUDED
#define XRPL_APP_MAIN_RPCCLIMAIN_H_INCLUDED

#include <xrpld/core/Config.h>

#include <xrpl/basics/Log.h>

#include <string>
#include <vector>

namespace xrpl {

/**
 * Process RPC commands from the command line.
 *
 * This is a façade function that delegates to the RPC module's
 * RPCCall::fromCommandLine() implementation, allowing Main.cpp
 * to avoid direct dependencies on the rpc module.
 *
 * @param config The server configuration
 * @param vCmd The command-line arguments
 * @param logs The logging facility
 * @return Exit code (0 for success)
 */
int
rpcCliMain(
    Config const& config,
    std::vector<std::string> const& vCmd,
    Logs& logs);

}  // namespace xrpl

#endif

