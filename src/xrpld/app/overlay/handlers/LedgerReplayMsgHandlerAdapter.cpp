#include <xrpld/app/ledger/detail/LedgerReplayMsgHandler.h>
#include <xrpld/app/main/Application.h>
#include <xrpld/overlay/detail/handlers/LedgerReplayMsgHandlerAdapter.h>

namespace xrpl {

/**
 * Adapter that wraps LedgerReplayMsgHandler to implement
 * ILedgerReplayMsgHandler.
 *
 * This adapter forwards all calls to the underlying LedgerReplayMsgHandler,
 * allowing the overlay module to use ledger replay functionality without
 * directly depending on the app module's LedgerReplayMsgHandler.
 */
class LedgerReplayMsgHandlerAdapter : public ILedgerReplayMsgHandler
{
public:
    LedgerReplayMsgHandlerAdapter(Application& app)
        : handler_(app, app.getLedgerReplayer())
    {
    }

    protocol::TMProofPathResponse
    processProofPathRequest(
        std::shared_ptr<protocol::TMProofPathRequest> const& msg) override
    {
        return handler_.processProofPathRequest(msg);
    }

    bool
    processProofPathResponse(
        std::shared_ptr<protocol::TMProofPathResponse> const& msg) override
    {
        return handler_.processProofPathResponse(msg);
    }

    protocol::TMReplayDeltaResponse
    processReplayDeltaRequest(
        std::shared_ptr<protocol::TMReplayDeltaRequest> const& msg) override
    {
        return handler_.processReplayDeltaRequest(msg);
    }

    bool
    processReplayDeltaResponse(
        std::shared_ptr<protocol::TMReplayDeltaResponse> const& msg) override
    {
        return handler_.processReplayDeltaResponse(msg);
    }

private:
    LedgerReplayMsgHandler handler_;
};

std::unique_ptr<ILedgerReplayMsgHandler>
make_LedgerReplayMsgHandlerAdapter(Application& app)
{
    return std::make_unique<LedgerReplayMsgHandlerAdapter>(app);
}

LedgerReplayMsgHandlerFactory
make_LedgerReplayMsgHandlerFactory(Application& app)
{
    return [&app]() { return make_LedgerReplayMsgHandlerAdapter(app); };
}

}  // namespace xrpl
