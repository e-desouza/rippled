#ifndef XRPL_APP_RPC_MAKE_SERVERHANDLER_H_INCLUDED
#define XRPL_APP_RPC_MAKE_SERVERHANDLER_H_INCLUDED

#include <xrpld/app/rpc/IServerComponent.h>

#include <xrpl/beast/utility/Journal.h>
#include <xrpl/server/Port.h>

#include <boost/asio/io_context.hpp>

#include <iosfwd>
#include <memory>
#include <vector>

namespace xrpl {

class Application;
class CollectorManager;
class Config;
class JobQueue;
class NetworkOPs;

namespace Resource {
class Manager;
}

/**
 * Configuration for the HTTP/WebSocket server.
 * This is a simplified version that doesn't require the full ServerHandler.h
 */
struct ServerHandlerSetup
{
    explicit ServerHandlerSetup() = default;

    std::vector<Port> ports;

    // Configuration when acting in client role
    struct client_t
    {
        explicit client_t() = default;

        bool secure = false;
        std::string ip;
        std::uint16_t port = 0;
        std::string user;
        std::string password;
        std::string admin_user;
        std::string admin_password;
    };

    client_t client;

    // Configuration for the Overlay
    boost::asio::ip::tcp::endpoint overlay;

    void
    makeContexts();
};

/**
 * Parse server handler configuration from Config.
 */
ServerHandlerSetup
setup_ServerHandlerConfig(Config const& c, std::ostream&& log);

/**
 * Create an HTTP/WebSocket server handler.
 * Returns IHTTPServer interface to avoid app→rpc dependency.
 */
std::unique_ptr<IHTTPServer>
make_HTTPServer(
    Application& app,
    boost::asio::io_context& io_context,
    JobQueue& jobQueue,
    NetworkOPs& networkOPs,
    Resource::Manager& resourceManager,
    CollectorManager& cm);

/**
 * Setup the server handler with configuration.
 * This is called after make_HTTPServer to configure the server.
 */
void
setupHTTPServer(
    IHTTPServer& server,
    ServerHandlerSetup const& setup,
    beast::Journal journal);

}  // namespace xrpl

#endif
