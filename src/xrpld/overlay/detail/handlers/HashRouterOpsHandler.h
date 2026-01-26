#ifndef XRPL_OVERLAY_DETAIL_HANDLERS_HASHROUTEROPSHANDLER_H_INCLUDED
#define XRPL_OVERLAY_DETAIL_HANDLERS_HASHROUTEROPSHANDLER_H_INCLUDED

#include <xrpld/overlay/IHashRouterOps.h>

namespace xrpl {

class Application;

/**
 * Handler that implements IHashRouterOps by delegating to HashRouter.
 * The header lives in overlay module, implementation in app module.
 */
class HashRouterOpsHandler final : public IHashRouterOps
{
private:
    Application& app_;

public:
    explicit HashRouterOpsHandler(Application& app);

    void
    addSuppression(uint256 const& key) override;

    bool
    addSuppressionPeer(uint256 const& key, PeerShortID peer) override;

    std::pair<bool, std::optional<Stopwatch::time_point>>
    addSuppressionPeerWithStatus(uint256 const& key, PeerShortID peer) override;

    bool
    shouldProcess(
        uint256 const& key,
        PeerShortID peer,
        HashRouterFlags& flags,
        std::chrono::seconds tx_interval) override;

    bool
    setFlags(uint256 const& key, HashRouterFlags flags) override;

    HashRouterFlags
    getFlags(uint256 const& key) override;

    std::optional<std::set<PeerShortID>>
    shouldRelay(uint256 const& key) override;
};

}  // namespace xrpl

#endif
