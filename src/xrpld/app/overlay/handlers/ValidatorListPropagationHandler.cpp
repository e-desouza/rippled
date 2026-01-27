#include <xrpld/overlay/detail/handlers/ValidatorListPropagationHandler.h>

#include <xrpld/app/main/Application.h>
#include <xrpld/app/txqueue/HashRouter.h>
#include <xrpld/app/validators/ValidatorList.h>
#include <xrpld/overlay/IOverlayServices.h>
#include <xrpld/overlay/detail/OverlayImpl.h>
#include <xrpld/overlay/detail/PeerImp.h>

namespace xrpl {

void
ValidatorListPropagationHandler::sendValidatorLists(PeerImp& peer)
{
    auto& app = peer.overlay_.services().app();
    auto& hashRouter = app.getHashRouter();

    app.validators().for_each_available(
        [&](std::string const& manifest,
            std::uint32_t version,
            std::map<std::size_t, ValidatorBlobInfo> const& blobInfos,
            PublicKey const& pubKey,
            std::size_t maxSequence,
            uint256 const& hash) {
            ValidatorList::sendValidatorList(
                peer,
                0,
                pubKey,
                maxSequence,
                version,
                manifest,
                blobInfos,
                hashRouter,
                peer.pJournal());

            // Don't send it next time.
            hashRouter.addSuppressionPeer(hash, peer.id());
        });
}

}  // namespace xrpl

