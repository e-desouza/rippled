#ifndef XRPL_OVERLAY_DETAIL_HANDLERS_OVERLAYOPSHANDLER_H_INCLUDED
#define XRPL_OVERLAY_DETAIL_HANDLERS_OVERLAYOPSHANDLER_H_INCLUDED

#include <xrpld/overlay/IOverlayOps.h>

namespace xrpl {

class Application;

/**
 * Handler that implements IOverlayOps by delegating to NetworkOPs.
 * The header lives in overlay module, implementation in app module.
 */
class OverlayOpsHandler final : public IOverlayOps
{
private:
    Application& app_;

public:
    explicit OverlayOpsHandler(Application& app);

    void
    pubManifest(
        PublicKey const& masterKey,
        std::optional<PublicKey> const& signingKey,
        std::uint32_t sequence,
        std::optional<std::vector<unsigned char>> const& signature,
        std::vector<unsigned char> const& masterSignature,
        std::string const& domain,
        std::string const& serialized) override;

    Json::Value
    getServerInfo(bool humanReadable, bool admin, bool counters) override;

    Json::Value
    getServerCounts(int minObjectCount) override;

    void
    saveValidatorManifest(std::string const& serialized) override;
};

}  // namespace xrpl

#endif
