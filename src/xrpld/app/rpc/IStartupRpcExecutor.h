#ifndef XRPL_APP_RPC_ISTARTUPRPCEXECUTOR_H_INCLUDED
#define XRPL_APP_RPC_ISTARTUPRPCEXECUTOR_H_INCLUDED

#include <xrpl/json/json_value.h>

namespace xrpl {

class Application;

/**
 * Interface for executing RPC commands during application startup.
 *
 * This interface abstracts the RPC command execution to break the
 * dependency cycle between app and rpc modules. The app module
 * defines this interface, and the rpc module provides the implementation.
 */
class IStartupRpcExecutor
{
public:
    virtual ~IStartupRpcExecutor() = default;

    /**
     * Execute an RPC command and return the result.
     *
     * @param app The application instance
     * @param command The JSON command to execute
     * @return The JSON result of the command
     */
    virtual Json::Value
    execute(Application& app, Json::Value const& command) = 0;
};

}  // namespace xrpl

#endif
