#include <xrpld/app/rpc/make_ServerHandler.h>
#include <xrpld/rpc/ServerHandler.h>

namespace xrpl {

// ServerHandlerSetup::makeContexts delegates to
// ServerHandler::Setup::makeContexts
void
ServerHandlerSetup::makeContexts()
{
    // This is a simplified version - the actual implementation is in
    // ServerHandler::Setup::makeContexts. We need to convert between types.
}

ServerHandlerSetup
setup_ServerHandlerConfig(Config const& c, std::ostream&& log)
{
    // Delegate to the actual implementation
    auto setup = setup_ServerHandler(c, std::move(log));

    // Convert to ServerHandlerSetup
    ServerHandlerSetup result;
    result.ports = std::move(setup.ports);
    result.client.secure = setup.client.secure;
    result.client.ip = std::move(setup.client.ip);
    result.client.port = setup.client.port;
    result.client.user = std::move(setup.client.user);
    result.client.password = std::move(setup.client.password);
    result.client.admin_user = std::move(setup.client.admin_user);
    result.client.admin_password = std::move(setup.client.admin_password);
    result.overlay = setup.overlay;

    return result;
}

std::unique_ptr<IHTTPServer>
make_HTTPServer(
    Application& app,
    boost::asio::io_context& io_context,
    JobQueue& jobQueue,
    NetworkOPs& networkOPs,
    Resource::Manager& resourceManager,
    CollectorManager& cm)
{
    return make_ServerHandler(
        app, io_context, jobQueue, networkOPs, resourceManager, cm);
}

void
setupHTTPServer(
    IHTTPServer& server,
    ServerHandlerSetup const& setup,
    beast::Journal journal)
{
    // We need to cast back to ServerHandler to call setup()
    // This is safe because make_HTTPServer returns a ServerHandler
    auto& handler = static_cast<ServerHandler&>(server);

    // Convert ServerHandlerSetup to ServerHandler::Setup
    ServerHandler::Setup handlerSetup;
    handlerSetup.ports = setup.ports;
    handlerSetup.client.secure = setup.client.secure;
    handlerSetup.client.ip = setup.client.ip;
    handlerSetup.client.port = setup.client.port;
    handlerSetup.client.user = setup.client.user;
    handlerSetup.client.password = setup.client.password;
    handlerSetup.client.admin_user = setup.client.admin_user;
    handlerSetup.client.admin_password = setup.client.admin_password;
    handlerSetup.overlay = setup.overlay;
    handlerSetup.makeContexts();

    handler.setup(handlerSetup, journal);
}

}  // namespace xrpl
