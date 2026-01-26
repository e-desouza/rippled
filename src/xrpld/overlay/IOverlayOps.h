#ifndef XRPL_OVERLAY_IOVERLAYOPS_H_INCLUDED
#define XRPL_OVERLAY_IOVERLAYOPS_H_INCLUDED

#include <xrpl/json/json_value.h>
#include <xrpl/protocol/PublicKey.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace xrpl {

/**
 * Interface for overlay operations that need to interact with NetworkOPs.
 * This abstraction allows the overlay module to publish manifests and
 * get server info without directly depending on the app module.
 */
class IOverlayOps
{
public:
    virtual ~IOverlayOps() = default;

    /**
     * Publish a manifest to subscribers.
     *
     * @param masterKey The master public key from the manifest
     * @param signingKey The ephemeral signing key (optional if revoked)
     * @param sequence The manifest sequence number
     * @param signature The manifest signature (optional)
     * @param masterSignature The master key signature
     * @param domain The domain from the manifest
     * @param serialized The serialized manifest data
     */
    virtual void
    pubManifest(
        PublicKey const& masterKey,
        std::optional<PublicKey> const& signingKey,
        std::uint32_t sequence,
        std::optional<std::vector<unsigned char>> const& signature,
        std::vector<unsigned char> const& masterSignature,
        std::string const& domain,
        std::string const& serialized) = 0;

    /**
     * Get server information for crawler response.
     *
     * @param humanReadable Whether to format for human reading
     * @param admin Whether this is an admin request
     * @param counters Whether to include counters
     * @return JSON value with server information
     */
    virtual Json::Value
    getServerInfo(bool humanReadable, bool admin, bool counters) = 0;
};

}  // namespace xrpl

#endif

