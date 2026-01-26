#ifndef XRPL_OVERLAY_DETAIL_HANDLERS_LEDGERREPLAYMSGHANDLERADAPTER_H_INCLUDED
#define XRPL_OVERLAY_DETAIL_HANDLERS_LEDGERREPLAYMSGHANDLERADAPTER_H_INCLUDED

#include <xrpld/overlay/ILedgerReplayMsgHandler.h>

namespace xrpl {

class Application;

/**
 * Factory function to create a LedgerReplayMsgHandler adapter.
 *
 * This factory creates an adapter that wraps the LedgerReplayMsgHandler
 * from the app module, implementing the ILedgerReplayMsgHandler interface.
 *
 * @param app The Application instance
 * @return A unique_ptr to an ILedgerReplayMsgHandler implementation
 */
std::unique_ptr<ILedgerReplayMsgHandler>
make_LedgerReplayMsgHandlerAdapter(Application& app);

/**
 * Creates a factory function for creating LedgerReplayMsgHandler adapters.
 *
 * This is used to defer handler creation until the PeerImp is constructed,
 * allowing each PeerImp to have its own handler instance.
 *
 * @param app The Application instance
 * @return A factory function that creates ILedgerReplayMsgHandler instances
 */
LedgerReplayMsgHandlerFactory
make_LedgerReplayMsgHandlerFactory(Application& app);

}  // namespace xrpl

#endif
