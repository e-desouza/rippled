#ifndef XRPL_OVERLAY_HANDLERS_VALIDATORLISTPROPAGATIONHANDLER_H_INCLUDED
#define XRPL_OVERLAY_HANDLERS_VALIDATORLISTPROPAGATIONHANDLER_H_INCLUDED

namespace xrpl {

class PeerImp;

/** Handler for validator list propagation during peer connection setup.

    This class provides a static method to propagate validator lists to
    newly connected peers. The implementation is in the app module to
    avoid dependency cycles between overlay and app.
*/
class ValidatorListPropagationHandler
{
public:
    /** Send all available validator lists to a newly connected peer.

        This is called during peer connection setup for inbound peers
        that support validator list propagation.

        @param peer The peer to send validator lists to
    */
    static void
    sendValidatorLists(PeerImp& peer);

    ValidatorListPropagationHandler() = delete;
};

}  // namespace xrpl

#endif

