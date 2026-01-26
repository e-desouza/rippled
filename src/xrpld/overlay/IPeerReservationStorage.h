#ifndef XRPL_OVERLAY_IPEERRESERVATIONSTORAGE_H_INCLUDED
#define XRPL_OVERLAY_IPEERRESERVATIONSTORAGE_H_INCLUDED

#include <xrpld/overlay/PeerReservationTable.h>

#include <xrpl/beast/utility/Journal.h>
#include <xrpl/protocol/PublicKey.h>

#include <string>
#include <unordered_set>

namespace xrpl {

/**
 * @brief Interface for peer reservation database operations.
 *
 * This interface abstracts the database operations needed by
 * PeerReservationTable, allowing the overlay module to be decoupled
 * from the app module's Wallet.h.
 */
class IPeerReservationStorage
{
public:
    virtual ~IPeerReservationStorage() = default;

    /**
     * @brief Load peer reservations from the database.
     * @param j Journal for logging.
     * @return Set of peer reservations.
     */
    virtual std::unordered_set<PeerReservation, beast::uhash<>, KeyEqual>
    loadReservations(beast::Journal j) = 0;

    /**
     * @brief Insert a peer reservation into the database.
     * @param nodeId Public key of the node.
     * @param description Description of the node.
     */
    virtual void
    insertReservation(PublicKey const& nodeId, std::string const& description) = 0;

    /**
     * @brief Delete a peer reservation from the database.
     * @param nodeId Public key of the node to remove.
     */
    virtual void
    deleteReservation(PublicKey const& nodeId) = 0;
};

}  // namespace xrpl

#endif

