#include <xrpld/app/main/Application.h>
#include <xrpld/app/misc/NetworkOPs.h>
#include <xrpld/app/misc/ServerCounts.h>
#include <xrpld/app/rdb/Wallet.h>
#include <xrpld/app/validators/Manifest.h>
#include <xrpld/overlay/detail/handlers/OverlayOpsHandler.h>

namespace xrpl {

OverlayOpsHandler::OverlayOpsHandler(Application& app) : app_(app)
{
}

void
OverlayOpsHandler::pubManifest(
    PublicKey const& masterKey,
    std::optional<PublicKey> const& signingKey,
    std::uint32_t sequence,
    std::optional<std::vector<unsigned char>> const& signature,
    std::vector<unsigned char> const& masterSignature,
    std::string const& domain,
    std::string const& serialized)
{
    // Reconstruct the Manifest object from the provided fields
    Manifest manifest(serialized, masterKey, signingKey, sequence, domain);
    app_.getOPs().pubManifest(manifest);
}

Json::Value
OverlayOpsHandler::getServerInfo(bool humanReadable, bool admin, bool counters)
{
    return app_.getOPs().getServerInfo(humanReadable, admin, counters);
}

Json::Value
OverlayOpsHandler::getServerCounts(int minObjectCount)
{
    return getCountsJson(app_, minObjectCount);
}

void
OverlayOpsHandler::saveValidatorManifest(std::string const& serialized)
{
    auto db = app_.getWalletDB().checkoutDb();
    addValidatorManifest(*db, serialized);
}

}  // namespace xrpl
