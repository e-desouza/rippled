#ifndef XRPL_OVERLAY_IHASHROUTEROPS_H_INCLUDED
#define XRPL_OVERLAY_IHASHROUTEROPS_H_INCLUDED

#include <xrpl/basics/HashRouterFlags.h>
#include <xrpl/basics/base_uint.h>
#include <xrpl/basics/chrono.h>

#include <chrono>
#include <cstdint>
#include <optional>
#include <set>

namespace xrpl {

/** Interface for hash router operations needed by the overlay.

    This interface abstracts the HashRouter functionality that the overlay
    module needs, allowing the overlay to be decoupled from the app module.
    The app module provides the implementation.
*/
class IHashRouterOps
{
public:
    using PeerShortID = std::uint32_t;

    virtual ~IHashRouterOps() = default;

    /** Add a suppression entry for a hash. */
    virtual void
    addSuppression(uint256 const& key) = 0;

    /** Add a peer to the suppression set for a hash.
        @return true if the peer was added (not already present)
    */
    virtual bool
    addSuppressionPeer(uint256 const& key, PeerShortID peer) = 0;

    /** Add a peer and get the relay status.
        @return pair: (was peer added, optional relay time if already relayed)
    */
    virtual std::pair<bool, std::optional<Stopwatch::time_point>>
    addSuppressionPeerWithStatus(uint256 const& key, PeerShortID peer) = 0;

    /** Add a peer and check if the hash should be processed.
        @return true if the hash should be processed
    */
    virtual bool
    shouldProcess(
        uint256 const& key,
        PeerShortID peer,
        HashRouterFlags& flags,
        std::chrono::seconds tx_interval) = 0;

    /** Set flags on a hash entry.
        @return true if flags were changed
    */
    virtual bool
    setFlags(uint256 const& key, HashRouterFlags flags) = 0;

    /** Get the flags for a hash entry. */
    virtual HashRouterFlags
    getFlags(uint256 const& key) = 0;

    /** Check if a hash should be relayed.
        @return set of peers to skip, or nullopt if should not relay
    */
    virtual std::optional<std::set<PeerShortID>>
    shouldRelay(uint256 const& key) = 0;
};

}  // namespace xrpl

#endif

