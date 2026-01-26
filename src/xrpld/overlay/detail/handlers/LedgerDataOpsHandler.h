#ifndef XRPL_OVERLAY_DETAIL_HANDLERS_LEDGERDATAOPSHANDLER_H_INCLUDED
#define XRPL_OVERLAY_DETAIL_HANDLERS_LEDGERDATAOPSHANDLER_H_INCLUDED

#include <xrpld/overlay/ILedgerDataOps.h>

namespace xrpl {

class Application;

/** Creates a handler that implements ILedgerDataOps using Application.
 *
 * The implementation is in app/overlay/handlers/LedgerDataOpsHandler.cpp
 * to keep the app dependency in the app module.
 */
std::unique_ptr<ILedgerDataOps>
make_LedgerDataOpsHandler(Application& app);

}  // namespace xrpl

#endif
