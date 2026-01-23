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

