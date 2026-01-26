#ifndef XRPL_OVERLAY_IHANDSHAKEPARAMS_H_INCLUDED
#define XRPL_OVERLAY_IHANDSHAKEPARAMS_H_INCLUDED

#include <xrpl/basics/base_uint.h>
#include <xrpl/protocol/PublicKey.h>
#include <xrpl/protocol/SecretKey.h>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace xrpl {

/** Interface providing parameters needed for peer handshake.

    This interface abstracts the handshake's dependency on Application
    and LedgerMaster, allowing the overlay module to perform handshakes
    without directly depending on app module headers.
*/
class IHandshakeParams
{
public:
    virtual ~IHandshakeParams() = default;

    /** Get the current network time. */
    virtual std::chrono::seconds
    networkTime() const = 0;

    /** Get this node's public/private key pair. */
    virtual std::pair<PublicKey, SecretKey> const&
    nodeIdentity() const = 0;

    /** Get the instance ID (cookie). */
    virtual std::uint64_t
    instanceID() const = 0;

    /** Get the server domain, empty if not configured. */
    virtual std::string const&
    serverDomain() const = 0;

    /** Get the closed ledger hash, if available. */
    virtual std::optional<uint256>
    closedLedgerHash() const = 0;

    /** Get the parent ledger hash (previous ledger), if available. */
    virtual std::optional<uint256>
    previousLedgerHash() const = 0;

    /** Get compression configuration flag. */
    virtual bool
    compressionEnabled() const = 0;

    /** Get ledger replay configuration flag. */
    virtual bool
    ledgerReplayEnabled() const = 0;

    /** Get transaction reduce-relay configuration flag. */
    virtual bool
    txReduceRelayEnabled() const = 0;

    /** Get validation/proposal reduce-relay base squelch configuration flag. */
    virtual bool
    vpReduceRelayEnabled() const = 0;
};

}  // namespace xrpl

#endif

