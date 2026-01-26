#ifndef XRPL_APP_OVERLAY_ADAPTERS_HANDSHAKEPARAMSADAPTER_H_INCLUDED
#define XRPL_APP_OVERLAY_ADAPTERS_HANDSHAKEPARAMSADAPTER_H_INCLUDED

#include <xrpld/app/ledger/LedgerMaster.h>
#include <xrpld/app/main/Application.h>
#include <xrpld/overlay/IHandshakeParams.h>

namespace xrpl {

/** Adapter implementing IHandshakeParams by wrapping Application.

    This class lives in the app module and provides handshake parameters
    to the overlay module without creating a direct dependency.
*/
class HandshakeParamsAdapter final : public IHandshakeParams
{
private:
    Application& app_;

public:
    explicit HandshakeParamsAdapter(Application& app) : app_(app)
    {
    }

    std::chrono::seconds
    networkTime() const override
    {
        return std::chrono::duration_cast<std::chrono::seconds>(
            app_.timeKeeper().now().time_since_epoch());
    }

    std::pair<PublicKey, SecretKey> const&
    nodeIdentity() const override
    {
        return app_.nodeIdentity();
    }

    std::uint64_t
    instanceID() const override
    {
        return app_.instanceID();
    }

    std::string const&
    serverDomain() const override
    {
        return app_.config().SERVER_DOMAIN;
    }

    std::optional<uint256>
    closedLedgerHash() const override
    {
        if (auto const cl = app_.getLedgerMaster().getClosedLedger())
            return cl->header().hash;
        return std::nullopt;
    }

    std::optional<uint256>
    previousLedgerHash() const override
    {
        if (auto const cl = app_.getLedgerMaster().getClosedLedger())
            return cl->header().parentHash;
        return std::nullopt;
    }

    bool
    compressionEnabled() const override
    {
        return app_.config().COMPRESSION;
    }

    bool
    ledgerReplayEnabled() const override
    {
        return app_.config().LEDGER_REPLAY;
    }

    bool
    txReduceRelayEnabled() const override
    {
        return app_.config().TX_REDUCE_RELAY_ENABLE;
    }

    bool
    vpReduceRelayEnabled() const override
    {
        return app_.config().VP_REDUCE_RELAY_BASE_SQUELCH_ENABLE;
    }
};

}  // namespace xrpl

#endif

