#include <xrpld/app/ledger/InboundLedgers.h>
#include <xrpld/app/ledger/InboundTransactions.h>
#include <xrpld/app/main/Application.h>
#include <xrpld/overlay/detail/handlers/LedgerDataOpsHandler.h>

namespace xrpl {

class LedgerDataOpsHandler : public ILedgerDataOps
{
public:
    explicit LedgerDataOpsHandler(Application& app) : app_(app)
    {
    }

    bool
    gotLedgerData(
        uint256 const& ledgerHash,
        std::shared_ptr<Peer> peer,
        std::shared_ptr<protocol::TMLedgerData> message) override
    {
        return app_.getInboundLedgers().gotLedgerData(
            ledgerHash, std::move(peer), std::move(message));
    }

    void
    gotTransactionData(
        uint256 const& setHash,
        std::shared_ptr<Peer> peer,
        std::shared_ptr<protocol::TMLedgerData> message) override
    {
        app_.getInboundTransactions().gotData(
            setHash, std::move(peer), std::move(message));
    }

    std::shared_ptr<SHAMap>
    getTransactionSet(uint256 const& setHash, bool acquire) override
    {
        return app_.getInboundTransactions().getSet(setHash, acquire);
    }

private:
    Application& app_;
};

std::unique_ptr<ILedgerDataOps>
make_LedgerDataOpsHandler(Application& app)
{
    return std::make_unique<LedgerDataOpsHandler>(app);
}

}  // namespace xrpl
