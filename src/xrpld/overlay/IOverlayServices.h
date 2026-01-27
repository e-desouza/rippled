#ifndef XRPL_OVERLAY_IOVERLAYSERVICES_H_INCLUDED
#define XRPL_OVERLAY_IOVERLAYSERVICES_H_INCLUDED

#include <xrpl/beast/utility/Journal.h>
#include <xrpl/protocol/PublicKey.h>

#include <optional>
#include <string>

namespace xrpl {

// Forward declarations
class Application;
class Cluster;
class Config;
class ILedgerRequestHandler;
class JobQueue;
class Logs;
class ManifestCache;
class PeerReservationTable;
class TimeKeeper;

namespace NodeStore {
class Database;
}

/**
 * @brief Interface providing Application services needed by the overlay module.
 *
 * This interface abstracts the dependency on Application.h, allowing overlay
 * implementation files (PeerImp.cpp, OverlayImpl.cpp, ConnectAttempt.cpp) to
 * access necessary services without directly including Application.h.
 *
 * The app module provides the implementation via OverlayServicesAdapter.
 */
class IOverlayServices
{
public:
    virtual ~IOverlayServices() = default;

    /** Get the Logs instance for creating journals. */
    virtual Logs&
    logs() = 0;

    /** Get the configuration. */
    virtual Config&
    config() = 0;

    /** Get the cluster management instance. */
    virtual Cluster&
    cluster() = 0;

    /** Get the job queue for scheduling work. */
    virtual JobQueue&
    jobQueue() = 0;

    /** Get the time keeper for network time. */
    virtual TimeKeeper&
    timeKeeper() = 0;

    /** Get the node store database. */
    virtual NodeStore::Database&
    nodeStore() = 0;

    /** Get the validator manifests cache. */
    virtual ManifestCache&
    validatorManifests() = 0;

    /** Get the peer reservation table. */
    virtual PeerReservationTable&
    peerReservations() = 0;

    /** Get the validation public key, if configured. */
    virtual std::optional<PublicKey const>
    getValidationPublicKey() const = 0;

    /** Create a journal with the given name. */
    virtual beast::Journal
    journal(std::string const& name) = 0;

    /**
     * Get the Application instance.
     * This is primarily used by message handlers in the app module
     * that need access to Application services not exposed through
     * other interface methods.
     */
    virtual Application&
    app() = 0;

    /** Get the ledger request handler.
     * This handler processes ledger data requests (liBASE, liTX_NODE,
     * liAS_NODE) without exposing Ledger.h to the overlay module.
     */
    virtual ILedgerRequestHandler&
    ledgerRequestHandler() = 0;
};

}  // namespace xrpl

#endif
