#include <xrpld/app/main/Application.h>
#include <xrpld/app/txqueue/HashRouter.h>
#include <xrpld/overlay/detail/handlers/HashRouterOpsHandler.h>

namespace xrpl {

HashRouterOpsHandler::HashRouterOpsHandler(Application& app) : app_(app)
{
}

void
HashRouterOpsHandler::addSuppression(uint256 const& key)
{
    app_.getHashRouter().addSuppression(key);
}

bool
HashRouterOpsHandler::addSuppressionPeer(uint256 const& key, PeerShortID peer)
{
    return app_.getHashRouter().addSuppressionPeer(key, peer);
}

std::pair<bool, std::optional<Stopwatch::time_point>>
HashRouterOpsHandler::addSuppressionPeerWithStatus(
    uint256 const& key,
    PeerShortID peer)
{
    return app_.getHashRouter().addSuppressionPeerWithStatus(key, peer);
}

bool
HashRouterOpsHandler::shouldProcess(
    uint256 const& key,
    PeerShortID peer,
    HashRouterFlags& flags,
    std::chrono::seconds tx_interval)
{
    return app_.getHashRouter().shouldProcess(key, peer, flags, tx_interval);
}

bool
HashRouterOpsHandler::setFlags(uint256 const& key, HashRouterFlags flags)
{
    return app_.getHashRouter().setFlags(key, flags);
}

HashRouterFlags
HashRouterOpsHandler::getFlags(uint256 const& key)
{
    return app_.getHashRouter().getFlags(key);
}

std::optional<std::set<IHashRouterOps::PeerShortID>>
HashRouterOpsHandler::shouldRelay(uint256 const& key)
{
    return app_.getHashRouter().shouldRelay(key);
}

}  // namespace xrpl
