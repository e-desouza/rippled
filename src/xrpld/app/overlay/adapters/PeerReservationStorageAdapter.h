#ifndef XRPL_APP_OVERLAY_ADAPTERS_PEERRESERVATIONSTORAGEADAPTER_H_INCLUDED
#define XRPL_APP_OVERLAY_ADAPTERS_PEERRESERVATIONSTORAGEADAPTER_H_INCLUDED

#include <xrpld/app/rdb/Wallet.h>
#include <xrpld/core/DatabaseCon.h>
#include <xrpld/overlay/IPeerReservationStorage.h>

namespace xrpl {

/**
 * @brief Adapter implementing IPeerReservationStorage using the wallet database.
 *
 * This adapter wraps DatabaseCon and provides the database operations
 * needed by PeerReservationTable.
 */
class PeerReservationStorageAdapter : public IPeerReservationStorage
{
public:
    explicit PeerReservationStorageAdapter(DatabaseCon& connection)
        : connection_(connection)
    {
    }

    std::unordered_set<PeerReservation, beast::uhash<>, KeyEqual>
    loadReservations(beast::Journal j) override
    {
        auto db = connection_.checkoutDb();
        return getPeerReservationTable(*db, j);
    }

    void
    insertReservation(
        PublicKey const& nodeId,
        std::string const& description) override
    {
        auto db = connection_.checkoutDb();
        insertPeerReservation(*db, nodeId, description);
    }

    void
    deleteReservation(PublicKey const& nodeId) override
    {
        auto db = connection_.checkoutDb();
        deletePeerReservation(*db, nodeId);
    }

private:
    DatabaseCon& connection_;
};

}  // namespace xrpl

#endif

