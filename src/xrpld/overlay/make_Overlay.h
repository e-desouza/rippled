#ifndef XRPL_OVERLAY_MAKE_OVERLAY_H_INCLUDED
#define XRPL_OVERLAY_MAKE_OVERLAY_H_INCLUDED

#include <xrpld/overlay/ILedgerReplayMsgHandler.h>
#include <xrpld/overlay/Overlay.h>

#include <xrpl/basics/Resolver.h>

#include <boost/asio/io_context.hpp>

namespace xrpl {

class IFeeTrackOps;
class IHandshakeParams;
class IHashRouterOps;
class ILedgerDataOps;
class ILedgerMasterOps;
class IOverlayOps;
class IValidatorOps;

Overlay::Setup
setup_Overlay(BasicConfig const& config);

/** Creates the implementation of Overlay. */
std::unique_ptr<Overlay>
make_Overlay(
    Application& app,
    Overlay::Setup const& setup,
    Resource::Manager& resourceManager,
    Resolver& resolver,
    boost::asio::io_context& io_context,
    BasicConfig const& config,
    beast::insight::Collector::ptr const& collector,
    IFeeTrackOps& feeTrackOps,
    IHandshakeParams& handshakeParams,
    IOverlayOps& overlayOps,
    IHashRouterOps& hashRouterOps,
    IValidatorOps& validatorOps,
    ILedgerDataOps& ledgerDataOps,
    ILedgerMasterOps& ledgerMasterOps,
    LedgerReplayMsgHandlerFactory ledgerReplayMsgHandlerFactory);

}  // namespace xrpl

#endif
