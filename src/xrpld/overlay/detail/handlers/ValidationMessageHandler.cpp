#include <xrpld/overlay/detail/handlers/ValidationMessageHandler.h>

#include <xrpld/app/consensus/RCLValidations.h>
#include <xrpld/app/main/Application.h>
#include <xrpld/app/misc/LoadFeeTrack.h>
#include <xrpld/app/misc/NetworkOPs.h>
#include <xrpld/app/txqueue/HashRouter.h>
#include <xrpld/app/validators/ValidatorList.h>
#include <xrpld/consensus/Validations.h>
#include <xrpld/overlay/Message.h>
#include <xrpld/overlay/detail/OverlayImpl.h>
#include <xrpld/overlay/detail/PeerImp.h>
#include <xrpld/overlay/detail/ProtocolVersion.h>
#include <xrpld/overlay/detail/TrafficCount.h>

#include <xrpl/basics/UptimeClock.h>
#include <xrpl/protocol/STValidation.h>
#include <xrpl/protocol/digest.h>

using namespace std::chrono_literals;

namespace xrpl {

void
ValidationMessageHandler::onMessage(
    std::shared_ptr<protocol::TMValidation> const& m,
    PeerImp& peer)
{
    auto& app = peer.app_;
    auto& overlay = peer.overlay_;
    auto const& journal = peer.p_journal_;

    if (m->validation().size() < 50)
    {
        JLOG(journal.warn()) << "Validation: Too small";
        peer.fee_.update(Resource::feeMalformedRequest, "too small");
        return;
    }

    try
    {
        auto const closeTime = app.timeKeeper().closeTime();

        std::shared_ptr<STValidation> val;
        {
            SerialIter sit(makeSlice(m->validation()));
            val = std::make_shared<STValidation>(
                std::ref(sit),
                [&app](PublicKey const& pk) {
                    return calcNodeID(app.validatorManifests().getMasterKey(pk));
                },
                false);
            val->setSeen(closeTime);
        }

        if (!isCurrent(
                app.getValidations().parms(),
                app.timeKeeper().closeTime(),
                val->getSignTime(),
                val->getSeenTime()))
        {
            JLOG(journal.trace()) << "Validation: Not current";
            peer.fee_.update(Resource::feeUselessData, "not current");
            return;
        }

        auto const isTrusted = app.validators().trusted(val->getSignerPublic());

        // If the operator has specified that untrusted validations be
        // dropped then this happens here
        if (!isTrusted)
        {
            overlay.reportInboundTraffic(
                TrafficCount::category::validation_untrusted,
                Message::messageSize(*m));

            if (app.config().RELAY_UNTRUSTED_VALIDATIONS == -1)
                return;
        }

        auto key = sha512Half(makeSlice(m->validation()));

        auto [added, relayed] =
            app.getHashRouter().addSuppressionPeerWithStatus(key, peer.id_);

        if (!added)
        {
            // Count unique messages for squelch logic
            if (relayed && (stopwatch().now() - *relayed) < reduce_relay::IDLED)
                overlay.updateSlotAndSquelch(
                    key, val->getSignerPublic(), peer.id_, protocol::mtVALIDATION);

            overlay.reportInboundTraffic(
                TrafficCount::category::validation_duplicate,
                Message::messageSize(*m));

            JLOG(journal.trace()) << "Validation: duplicate";
            return;
        }

        if (!isTrusted && (peer.tracking_.load() == PeerImp::Tracking::diverged))
        {
            JLOG(journal.debug())
                << "Dropping untrusted validation from diverged peer";
        }
        else if (isTrusted || !app.getFeeTrack().isLoadedLocal())
        {
            std::string const name = isTrusted ? "ChkTrust" : "ChkUntrust";

            std::weak_ptr<PeerImp> weak = peer.shared_from_this();
            app.getJobQueue().addJob(
                isTrusted ? jtVALIDATION_t : jtVALIDATION_ut,
                name,
                [weak, val, m, key]() {
                    if (auto p = weak.lock())
                        p->checkValidation(val, key, m);
                });
        }
        else
        {
            JLOG(journal.debug()) << "Dropping untrusted validation for load";
        }
    }
    catch (std::exception const& e)
    {
        JLOG(journal.warn())
            << "Exception processing validation: " << e.what();
        using namespace std::string_literals;
        peer.fee_.update(Resource::feeMalformedRequest, e.what());
    }
}

void
ValidationMessageHandler::onMessage(
    std::shared_ptr<protocol::TMValidatorList> const& m,
    PeerImp& peer)
{
    try
    {
        if (!peer.supportsFeature(ProtocolFeature::ValidatorListPropagation))
        {
            JLOG(peer.p_journal_.debug())
                << "ValidatorList: received validator list from peer using "
                << "protocol version " << to_string(peer.protocol_)
                << " which shouldn't support this feature.";
            peer.fee_.update(Resource::feeUselessData, "unsupported peer");
            return;
        }
        processValidatorListMessage(
            peer,
            "ValidatorList",
            m->manifest(),
            m->version(),
            ValidatorList::parseBlobs(*m));
    }
    catch (std::exception const& e)
    {
        JLOG(peer.p_journal_.warn()) << "ValidatorList: Exception, " << e.what();
        using namespace std::string_literals;
        peer.fee_.update(Resource::feeInvalidData, e.what());
    }
}

void
ValidationMessageHandler::onMessage(
    std::shared_ptr<protocol::TMValidatorListCollection> const& m,
    PeerImp& peer)
{
    try
    {
        if (!peer.supportsFeature(ProtocolFeature::ValidatorList2Propagation))
        {
            JLOG(peer.p_journal_.debug())
                << "ValidatorListCollection: received validator list from peer "
                << "using protocol version " << to_string(peer.protocol_)
                << " which shouldn't support this feature.";
            peer.fee_.update(Resource::feeUselessData, "unsupported peer");
            return;
        }
        else if (m->version() < 2)
        {
            JLOG(peer.p_journal_.debug())
                << "ValidatorListCollection: received invalid validator list "
                   "version "
                << m->version() << " from peer using protocol version "
                << to_string(peer.protocol_);
            peer.fee_.update(Resource::feeInvalidData, "wrong version");
            return;
        }
        processValidatorListMessage(
            peer,
            "ValidatorListCollection",
            m->manifest(),
            m->version(),
            ValidatorList::parseBlobs(*m));
    }
    catch (std::exception const& e)
    {
        JLOG(peer.p_journal_.warn())
            << "ValidatorListCollection: Exception, " << e.what();
        using namespace std::string_literals;
        peer.fee_.update(Resource::feeInvalidData, e.what());
    }
}

void
ValidationMessageHandler::processValidatorListMessage(
    PeerImp& peer,
    std::string const& messageType,
    std::string const& manifest,
    std::uint32_t version,
    std::vector<ValidatorBlobInfo> const& blobs)
{
    auto& app = peer.app_;
    auto const& journal = peer.p_journal_;

    // If there are no blobs, the message is malformed (possibly because of
    // ValidatorList class rules), so charge accordingly and skip processing.
    if (blobs.empty())
    {
        JLOG(journal.warn()) << "Ignored malformed " << messageType;
        // This shouldn't ever happen with a well-behaved peer
        peer.fee_.update(Resource::feeHeavyBurdenPeer, "no blobs");
        return;
    }

    auto const hash = sha512Half(manifest, blobs, version);

    JLOG(journal.debug()) << "Received " << messageType;

    if (!app.getHashRouter().addSuppressionPeer(hash, peer.id_))
    {
        JLOG(journal.debug())
            << messageType << ": received duplicate " << messageType;
        // Charging this fee here won't hurt the peer in the normal
        // course of operation (ie. refresh every 5 minutes), but
        // will add up if the peer is misbehaving.
        peer.fee_.update(Resource::feeUselessData, "duplicate");
        return;
    }

    auto const applyResult = app.validators().applyListsAndBroadcast(
        manifest,
        version,
        blobs,
        peer.remote_address_.to_string(),
        hash,
        app.overlay(),
        app.getHashRouter(),
        app.getOPs());

    JLOG(journal.debug())
        << "Processed " << messageType << " version " << version << " from "
        << (applyResult.publisherKey ? strHex(*applyResult.publisherKey)
                                     : "unknown or invalid publisher")
        << " with best result " << to_string(applyResult.bestDisposition());

    // Act based on the best result
    switch (applyResult.bestDisposition())
    {
        // New list
        case ListDisposition::accepted:
        // Newest list is expired, and that needs to be broadcast, too
        case ListDisposition::expired:
        // Future list
        case ListDisposition::pending: {
            std::lock_guard<std::mutex> sl(peer.recentLock_);

            XRPL_ASSERT(
                applyResult.publisherKey,
                "xrpl::ValidationMessageHandler::processValidatorListMessage : "
                "publisher key is set");
            auto const& pubKey = *applyResult.publisherKey;
#ifndef NDEBUG
            if (auto const iter = peer.publisherListSequences_.find(pubKey);
                iter != peer.publisherListSequences_.end())
            {
                XRPL_ASSERT(
                    iter->second < applyResult.sequence,
                    "xrpl::ValidationMessageHandler::processValidatorListMessage "
                    ": lower sequence");
            }
#endif
            peer.publisherListSequences_[pubKey] = applyResult.sequence;
        }
        break;
        case ListDisposition::same_sequence:
        case ListDisposition::known_sequence:
#ifndef NDEBUG
        {
            std::lock_guard<std::mutex> sl(peer.recentLock_);
            XRPL_ASSERT(
                applyResult.sequence && applyResult.publisherKey,
                "xrpl::ValidationMessageHandler::processValidatorListMessage : "
                "nonzero sequence and set publisher key");
            XRPL_ASSERT(
                peer.publisherListSequences_[*applyResult.publisherKey] <=
                    applyResult.sequence,
                "xrpl::ValidationMessageHandler::processValidatorListMessage : "
                "maximum sequence");
        }
#endif  // !NDEBUG

        break;
        case ListDisposition::stale:
        case ListDisposition::untrusted:
        case ListDisposition::invalid:
        case ListDisposition::unsupported_version:
            break;
        // LCOV_EXCL_START
        default:
            UNREACHABLE(
                "xrpl::ValidationMessageHandler::processValidatorListMessage : "
                "invalid best list disposition");
            // LCOV_EXCL_STOP
    }

    // Charge based on the worst result
    switch (applyResult.worstDisposition())
    {
        case ListDisposition::accepted:
        case ListDisposition::expired:
        case ListDisposition::pending:
            // No charges for good data
            break;
        case ListDisposition::same_sequence:
        case ListDisposition::known_sequence:
            // Charging this fee here won't hurt the peer in the normal
            // course of operation (ie. refresh every 5 minutes), but
            // will add up if the peer is misbehaving.
            peer.fee_.update(
                Resource::feeUselessData,
                " duplicate (same_sequence or known_sequence)");
            break;
        case ListDisposition::stale:
            // There are very few good reasons for a peer to send an
            // old list, particularly more than once.
            peer.fee_.update(Resource::feeInvalidData, "expired");
            break;
        case ListDisposition::untrusted:
            // Charging this fee here won't hurt the peer in the normal
            // course of operation (ie. refresh every 5 minutes), but
            // will add up if the peer is misbehaving.
            peer.fee_.update(Resource::feeUselessData, "untrusted");
            break;
        case ListDisposition::invalid:
            // This shouldn't ever happen with a well-behaved peer
            peer.fee_.update(
                Resource::feeInvalidSignature, "invalid list disposition");
            break;
        case ListDisposition::unsupported_version:
            // During a version transition, this may be legitimate.
            // If it happens frequently, that's probably bad.
            peer.fee_.update(Resource::feeInvalidData, "version");
            break;
        // LCOV_EXCL_START
        default:
            UNREACHABLE(
                "xrpl::ValidationMessageHandler::processValidatorListMessage : "
                "invalid worst list disposition");
            // LCOV_EXCL_STOP
    }

    // Log based on all the results.
    for (auto const& [disp, count] : applyResult.dispositions)
    {
        switch (disp)
        {
            // New list
            case ListDisposition::accepted:
                JLOG(journal.debug())
                    << "Applied " << count << " new " << messageType;
                break;
            // Newest list is expired, and that needs to be broadcast, too
            case ListDisposition::expired:
                JLOG(journal.debug())
                    << "Applied " << count << " expired " << messageType;
                break;
            // Future list
            case ListDisposition::pending:
                JLOG(journal.debug())
                    << "Processed " << count << " future " << messageType;
                break;
            case ListDisposition::same_sequence:
                JLOG(journal.warn())
                    << "Ignored " << count << " " << messageType
                    << "(s) with current sequence";
                break;
            case ListDisposition::known_sequence:
                JLOG(journal.warn())
                    << "Ignored " << count << " " << messageType
                    << "(s) with future sequence";
                break;
            case ListDisposition::stale:
                JLOG(journal.warn())
                    << "Ignored " << count << "stale " << messageType;
                break;
            case ListDisposition::untrusted:
                JLOG(journal.warn())
                    << "Ignored " << count << " untrusted " << messageType;
                break;
            case ListDisposition::unsupported_version:
                JLOG(journal.warn())
                    << "Ignored " << count << "unsupported version "
                    << messageType;
                break;
            case ListDisposition::invalid:
                JLOG(journal.warn())
                    << "Ignored " << count << "invalid " << messageType;
                break;
            // LCOV_EXCL_START
            default:
                UNREACHABLE(
                    "xrpl::ValidationMessageHandler::processValidatorListMessage "
                    ": invalid list disposition");
                // LCOV_EXCL_STOP
        }
    }
}

}  // namespace xrpl

