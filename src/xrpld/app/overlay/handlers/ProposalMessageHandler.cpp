#include <xrpld/app/consensus/RCLCxPeerPos.h>
#include <xrpld/app/main/Application.h>
#include <xrpld/app/misc/LoadFeeTrack.h>
#include <xrpld/app/misc/NetworkOPs.h>
#include <xrpld/app/txqueue/HashRouter.h>
#include <xrpld/app/validators/ValidatorList.h>
#include <xrpld/overlay/IOverlayServices.h>
#include <xrpld/overlay/Message.h>
#include <xrpld/overlay/ReduceRelayCommon.h>
#include <xrpld/overlay/detail/OverlayImpl.h>
#include <xrpld/overlay/detail/PeerImp.h>
#include <xrpld/overlay/detail/TrafficCount.h>
#include <xrpld/overlay/detail/handlers/ProposalMessageHandler.h>

#include <xrpl/protocol/HashPrefix.h>
#include <xrpl/protocol/PublicKey.h>
#include <xrpl/protocol/digest.h>

namespace xrpl {

// Helper function to check for valid uint256 values in protobuf buffers
static bool
stringIsUint256Sized(std::string const& pBuffStr)
{
    return pBuffStr.size() == uint256::size();
}

void
ProposalMessageHandler::onMessage(
    std::shared_ptr<protocol::TMProposeSet> const& m,
    PeerImp& peer)
{
    protocol::TMProposeSet& set = *m;
    auto& journal = peer.p_journal_;
    auto& app = peer.overlay_.services().app();
    auto& overlay = peer.overlay_;

    auto const sig = makeSlice(set.signature());

    // Preliminary check for the validity of the signature: A DER encoded
    // signature can't be longer than 72 bytes.
    if ((std::clamp<std::size_t>(sig.size(), 64, 72) != sig.size()) ||
        (publicKeyType(makeSlice(set.nodepubkey())) != KeyType::secp256k1))
    {
        JLOG(journal.warn()) << "Proposal: malformed";
        peer.fee_.update(
            Resource::feeInvalidSignature,
            " signature can't be longer than 72 bytes");
        return;
    }

    if (!stringIsUint256Sized(set.currenttxhash()) ||
        !stringIsUint256Sized(set.previousledger()))
    {
        JLOG(journal.warn()) << "Proposal: malformed";
        peer.fee_.update(Resource::feeMalformedRequest, "bad hashes");
        return;
    }

    // RH TODO: when isTrusted = false we should probably also cache a key
    // suppression for 30 seconds to avoid doing a relatively expensive lookup
    // every time a spam packet is received
    PublicKey const publicKey{makeSlice(set.nodepubkey())};
    auto const isTrusted = app.validators().trusted(publicKey);

    // If the operator has specified that untrusted proposals be dropped then
    // this happens here I.e. before further wasting CPU verifying the signature
    // of an untrusted key
    if (!isTrusted)
    {
        // report untrusted proposal messages
        overlay.reportInboundTraffic(
            TrafficCount::category::proposal_untrusted,
            Message::messageSize(*m));

        if (app.config().RELAY_UNTRUSTED_PROPOSALS == -1)
            return;
    }

    uint256 const proposeHash{set.currenttxhash()};
    uint256 const prevLedger{set.previousledger()};

    NetClock::time_point const closeTime{NetClock::duration{set.closetime()}};

    uint256 const suppression = proposalUniqueId(
        proposeHash,
        prevLedger,
        set.proposeseq(),
        closeTime,
        publicKey.slice(),
        sig);

    if (auto [added, relayed] =
            app.getHashRouter().addSuppressionPeerWithStatus(suppression, peer.id());
        !added)
    {
        // Count unique messages (Slots has it's own 'HashRouter'), which a peer
        // receives within IDLED seconds since the message has been relayed.
        if (relayed && (stopwatch().now() - *relayed) < reduce_relay::IDLED)
            overlay.updateSlotAndSquelch(
                suppression, publicKey, peer.id(), protocol::mtPROPOSE_LEDGER);

        // report duplicate proposal messages
        overlay.reportInboundTraffic(
            TrafficCount::category::proposal_duplicate,
            Message::messageSize(*m));

        JLOG(journal.trace()) << "Proposal: duplicate";

        return;
    }

    if (!isTrusted)
    {
        if (peer.tracking_.load() == PeerImp::Tracking::diverged)
        {
            JLOG(journal.debug())
                << "Proposal: Dropping untrusted (peer divergence)";
            return;
        }

        if (!peer.cluster() && app.getFeeTrack().isLoadedLocal())
        {
            JLOG(journal.debug()) << "Proposal: Dropping untrusted (load)";
            return;
        }
    }

    JLOG(journal.trace()) << "Proposal: " << (isTrusted ? "trusted" : "untrusted");

    auto proposal = RCLCxPeerPos(
        publicKey,
        sig,
        suppression,
        RCLCxPeerPos::Proposal{
            prevLedger,
            set.proposeseq(),
            proposeHash,
            closeTime,
            app.timeKeeper().closeTime(),
            calcNodeID(app.validatorManifests().getMasterKey(publicKey))});

    std::weak_ptr<PeerImp> weak = peer.shared_from_this();
    app.getJobQueue().addJob(
        isTrusted ? jtPROPOSAL_t : jtPROPOSAL_ut,
        "checkPropose",
        [weak, isTrusted, m, proposal]() {
            if (auto p = weak.lock())
                ProposalMessageHandler::checkPropose(*p, isTrusted, m, proposal);
        });
}

void
ProposalMessageHandler::checkPropose(
    PeerImp& peer,
    bool isTrusted,
    std::shared_ptr<protocol::TMProposeSet> const& packet,
    RCLCxPeerPos peerPos)
{
    auto& journal = peer.p_journal_;
    auto& app = peer.overlay_.services().app();
    auto& overlay = peer.overlay_;

    JLOG(journal.trace())
        << "Checking " << (isTrusted ? "trusted" : "UNTRUSTED") << " proposal";

    XRPL_ASSERT(packet, "xrpl::ProposalMessageHandler::checkPropose : non-null packet");

    if (!peer.cluster() && !peerPos.checkSign())
    {
        std::string desc{"Proposal fails sig check"};
        JLOG(journal.warn()) << desc;
        peer.charge(Resource::feeInvalidSignature, desc);
        return;
    }

    bool relay;

    if (isTrusted)
        relay = app.getOPs().processTrustedProposal(peerPos);
    else
        relay = app.config().RELAY_UNTRUSTED_PROPOSALS == 1 || peer.cluster();

    if (relay)
    {
        // haveMessage contains peers, which are suppressed; i.e. the peers
        // are the source of the message, consequently the message should
        // not be relayed to these peers. But the message must be counted
        // as part of the squelch logic.
        auto haveMessage = app.overlay().relay(
            *packet, peerPos.suppressionID(), peerPos.publicKey());
        if (!haveMessage.empty())
            overlay.updateSlotAndSquelch(
                peerPos.suppressionID(),
                peerPos.publicKey(),
                std::move(haveMessage),
                protocol::mtPROPOSE_LEDGER);
    }
}

}  // namespace xrpl

