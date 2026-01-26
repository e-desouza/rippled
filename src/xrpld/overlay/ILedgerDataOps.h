#ifndef XRPL_OVERLAY_ILEDGERDATAOPS_H_INCLUDED
#define XRPL_OVERLAY_ILEDGERDATAOPS_H_INCLUDED

#include <xrpl/basics/base_uint.h>

#include <memory>

namespace protocol {
class TMLedgerData;
}

namespace xrpl {

class Peer;
class SHAMap;

/** Interface for ledger data operations used by the overlay module.
 *
 * This interface abstracts the InboundLedgers and InboundTransactions
 * components to break the dependency cycle between overlay and app modules.
 */
class ILedgerDataOps
{
public:
    virtual ~ILedgerDataOps() = default;

    /** Process incoming ledger data from a peer.
     *
     * Forwards to InboundLedgers::gotLedgerData().
     *
     * @param ledgerHash The hash of the ledger.
     * @param peer The peer that sent the data.
     * @param message The TMLedgerData message.
     * @return true if the data was consumed.
     */
    virtual bool
    gotLedgerData(
        uint256 const& ledgerHash,
        std::shared_ptr<Peer> peer,
        std::shared_ptr<protocol::TMLedgerData> message) = 0;

    /** Process incoming transaction set data from a peer.
     *
     * Forwards to InboundTransactions::gotData().
     *
     * @param setHash The hash of the transaction set.
     * @param peer The peer that sent the data.
     * @param message The TMLedgerData message.
     */
    virtual void
    gotTransactionData(
        uint256 const& setHash,
        std::shared_ptr<Peer> peer,
        std::shared_ptr<protocol::TMLedgerData> message) = 0;

    /** Get a transaction set by hash.
     *
     * Forwards to InboundTransactions::getSet().
     *
     * @param setHash The hash of the transaction set.
     * @param acquire Whether to acquire the set if not found.
     * @return The transaction set, or nullptr if not found.
     */
    virtual std::shared_ptr<SHAMap>
    getTransactionSet(uint256 const& setHash, bool acquire) = 0;
};

}  // namespace xrpl

#endif
