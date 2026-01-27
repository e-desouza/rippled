#ifndef XRPL_APP_RPC_ISERVERCOMPONENT_H_INCLUDED
#define XRPL_APP_RPC_ISERVERCOMPONENT_H_INCLUDED

#include <xrpl/server/Port.h>

#include <boost/asio/ip/tcp.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace xrpl {

// Type alias matching the one in xrpl/server/detail/ServerImpl.h
using Endpoints =
    std::unordered_map<std::string, boost::asio::ip::tcp::endpoint>;

/**
 * Minimal interface for server components (HTTP/WebSocket/gRPC servers).
 *
 * This interface allows Application to manage server lifecycle without
 * depending on the full RPC module headers, breaking the app→rpc dependency.
 */
class IServerComponent
{
public:
    virtual ~IServerComponent() = default;

    /** Stop the server component. */
    virtual void
    stop() = 0;
};

/**
 * Interface for the HTTP/WebSocket server handler.
 * Extends IServerComponent with setup and port information.
 */
class IHTTPServer : public IServerComponent
{
public:
    /** Get the configured server ports. */
    virtual std::vector<Port> const&
    getPorts() const = 0;

    /** Get the server endpoints (name -> endpoint mapping). */
    virtual Endpoints const&
    getEndpoints() const = 0;
};

/**
 * Interface for the gRPC server.
 * Extends IServerComponent with start capability.
 */
class IGRPCServer : public IServerComponent
{
public:
    /** Start the gRPC server.
     * @return true if server started successfully
     */
    virtual bool
    start() = 0;

    /** Get the endpoint the server is listening on. */
    virtual boost::asio::ip::tcp::endpoint
    getEndpoint() const = 0;
};

}  // namespace xrpl

#endif
