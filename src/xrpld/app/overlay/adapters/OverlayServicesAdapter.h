#ifndef XRPL_APP_OVERLAY_ADAPTERS_OVERLAYSERVICESADAPTER_H_INCLUDED
#define XRPL_APP_OVERLAY_ADAPTERS_OVERLAYSERVICESADAPTER_H_INCLUDED

#include <xrpld/app/main/Application.h>
#include <xrpld/app/overlay/handlers/LedgerRequestHandler.h>
#include <xrpld/overlay/IOverlayServices.h>

namespace xrpl {

/** Adapter that wraps Application to implement IOverlayServices interface.

    This adapter allows the overlay module implementation files (PeerImp.cpp,
    OverlayImpl.cpp, ConnectAttempt.cpp) to access Application services
    without directly including Application.h, breaking the overlay→app
    dependency cycle.
*/
class OverlayServicesAdapter final : public IOverlayServices
{
private:
    Application& app_;
    LedgerRequestHandler ledgerRequestHandler_;

public:
    explicit OverlayServicesAdapter(Application& app)
        : app_(app)
        , ledgerRequestHandler_(
              app.getLedgerMaster(),
              app.journal("LedgerRequest"))
    {
    }

    Logs&
    logs() override
    {
        return app_.logs();
    }

    Config&
    config() override
    {
        return app_.config();
    }

    Cluster&
    cluster() override
    {
        return app_.cluster();
    }

    JobQueue&
    jobQueue() override
    {
        return app_.getJobQueue();
    }

    TimeKeeper&
    timeKeeper() override
    {
        return app_.timeKeeper();
    }

    NodeStore::Database&
    nodeStore() override
    {
        return app_.getNodeStore();
    }

    ManifestCache&
    validatorManifests() override
    {
        return app_.validatorManifests();
    }

    PeerReservationTable&
    peerReservations() override
    {
        return app_.peerReservations();
    }

    std::optional<PublicKey const>
    getValidationPublicKey() const override
    {
        return app_.getValidationPublicKey();
    }

    beast::Journal
    journal(std::string const& name) override
    {
        return app_.journal(name);
    }

    Application&
    app() override
    {
        return app_;
    }

    ILedgerRequestHandler&
    ledgerRequestHandler() override
    {
        return ledgerRequestHandler_;
    }
};

}  // namespace xrpl

#endif
