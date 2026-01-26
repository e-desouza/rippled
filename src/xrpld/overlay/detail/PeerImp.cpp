#include <xrpld/app/ledger/InboundLedgers.h>
#include <xrpld/app/ledger/InboundTransactions.h>
#include <xrpld/app/ledger/LedgerMaster.h>
#include <xrpld/app/ledger/detail/LedgerReplayMsgHandler.h>
#include <xrpld/app/misc/LoadFeeTrack.h>
#include <xrpld/app/txqueue/HashRouter.h>
#include <xrpld/overlay/Cluster.h>
#include <xrpld/overlay/detail/PeerImp.h>
#include <xrpld/overlay/detail/Tuning.h>
#include <xrpld/overlay/detail/handlers/ProposalMessageHandler.h>
#include <xrpld/overlay/detail/handlers/StatusChangeMessageHandler.h>
#include <xrpld/overlay/detail/handlers/TransactionMessageHandler.h>
#include <xrpld/overlay/detail/handlers/ValidationMessageHandler.h>
#include <xrpld/overlay/detail/handlers/ValidatorListPropagationHandler.h>

#include <xrpl/basics/UptimeClock.h>
#include <xrpl/basics/base64.h>
#include <xrpl/basics/random.h>
#include <xrpl/basics/safe_cast.h>
#include <xrpl/core/JobQueue.h>
#include <xrpl/core/PerfLog.h>
#include <xrpl/json/to_string.h>
#include <xrpl/protocol/TxFlags.h>
#include <xrpl/protocol/digest.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http/write.hpp>

#include <algorithm>
#include <chrono>
#include <memory>
#include <mutex>
#include <numeric>
#include <sstream>

using namespace std::chrono_literals;

namespace xrpl {

namespace {
/** The threshold above which we treat a peer connection as high latency */
std::chrono::milliseconds constexpr peerHighLatency{300};

/** How often we PING the peer to check for latency and sendq probe */
std::chrono::seconds constexpr peerTimerInterval{60};

/** The timeout for a shutdown timer */
std::chrono::seconds constexpr shutdownTimerInterval{5};

}  // namespace

// TODO: Remove this exclusion once unit tests are added after the hotfix
// release.

PeerImp::PeerImp(
    Application& app,
    id_t id,
    std::shared_ptr<PeerFinder::Slot> const& slot,
    http_request_type&& request,
    PublicKey const& publicKey,
    ProtocolVersion protocol,
    Resource::Consumer consumer,
    std::unique_ptr<stream_type>&& stream_ptr,
    OverlayImpl& overlay)
    : Child(overlay)
    , app_(app)
    , id_(id)
    , fingerprint_(
          getFingerprint(slot->remote_endpoint(), publicKey, to_string(id)))
    , prefix_(makePrefix(fingerprint_))
    , sink_(app_.journal("Peer"), prefix_)
    , p_sink_(app_.journal("Protocol"), prefix_)
    , journal_(sink_)
    , p_journal_(p_sink_)
    , stream_ptr_(std::move(stream_ptr))
    , socket_(stream_ptr_->next_layer().socket())
    , stream_(*stream_ptr_)
    , strand_(boost::asio::make_strand(socket_.get_executor()))
    , timer_(waitable_timer{socket_.get_executor()})
    , remote_address_(slot->remote_endpoint())
    , overlay_(overlay)
    , inbound_(true)
    , protocol_(protocol)
    , tracking_(Tracking::unknown)
    , trackingTime_(clock_type::now())
    , publicKey_(publicKey)
    , lastPingTime_(clock_type::now())
    , creationTime_(clock_type::now())
    , squelch_(app_.journal("Squelch"))
    , usage_(consumer)
    , fee_{Resource::feeTrivialPeer, ""}
    , slot_(slot)
    , request_(std::move(request))
    , headers_(request_)
    , compressionEnabled_(
          peerFeatureEnabled(
              headers_,
              FEATURE_COMPR,
              "lz4",
              app_.config().COMPRESSION)
              ? Compressed::On
              : Compressed::Off)
    , txReduceRelayEnabled_(peerFeatureEnabled(
          headers_,
          FEATURE_TXRR,
          app_.config().TX_REDUCE_RELAY_ENABLE))
    , ledgerReplayEnabled_(peerFeatureEnabled(
          headers_,
          FEATURE_LEDGER_REPLAY,
          app_.config().LEDGER_REPLAY))
    , ledgerReplayMsgHandler_(
          std::make_unique<LedgerReplayMsgHandler>(app, app.getLedgerReplayer()))
{
    JLOG(journal_.info())
        << "compression enabled " << (compressionEnabled_ == Compressed::On)
        << " vp reduce-relay base squelch enabled "
        << peerFeatureEnabled(
               headers_,
               FEATURE_VPRR,
               app_.config().VP_REDUCE_RELAY_BASE_SQUELCH_ENABLE)
        << " tx reduce-relay enabled " << txReduceRelayEnabled_;
}

PeerImp::~PeerImp()
{
    bool const inCluster{cluster()};

    overlay_.deletePeer(id_);
    overlay_.onPeerDeactivate(id_);
    overlay_.peerFinder().on_closed(slot_);
    overlay_.remove(slot_);

    if (inCluster)
    {
        JLOG(journal_.warn()) << name() << " left cluster";
    }
}

// Helper function to check for valid uint256 values in protobuf buffers
static bool
stringIsUint256Sized(std::string const& pBuffStr)
{
    return pBuffStr.size() == uint256::size();
}

void
PeerImp::run()
{
    if (!strand_.running_in_this_thread())
        return post(strand_, std::bind(&PeerImp::run, shared_from_this()));

    auto parseLedgerHash =
        [](std::string_view value) -> std::optional<uint256> {
        if (uint256 ret; ret.parseHex(value))
            return ret;

        if (auto const s = base64_decode(value); s.size() == uint256::size())
            return uint256{s};

        return std::nullopt;
    };

    std::optional<uint256> closed;
    std::optional<uint256> previous;

    if (auto const iter = headers_.find("Closed-Ledger");
        iter != headers_.end())
    {
        closed = parseLedgerHash(iter->value());

        if (!closed)
            fail("Malformed handshake data (1)");
    }

    if (auto const iter = headers_.find("Previous-Ledger");
        iter != headers_.end())
    {
        previous = parseLedgerHash(iter->value());

        if (!previous)
            fail("Malformed handshake data (2)");
    }

    if (previous && !closed)
        fail("Malformed handshake data (3)");

    {
        std::lock_guard<std::mutex> sl(recentLock_);
        if (closed)
            closedLedgerHash_ = *closed;
        if (previous)
            previousLedgerHash_ = *previous;
    }

    if (inbound_)
        doAccept();
    else
        doProtocolStart();

    // Anything else that needs to be done with the connection should be
    // done in doProtocolStart
}

void
PeerImp::stop()
{
    if (!strand_.running_in_this_thread())
        return post(strand_, std::bind(&PeerImp::stop, shared_from_this()));

    if (!socket_.is_open())
        return;

    // The rationale for using different severity levels is that
    // outbound connections are under our control and may be logged
    // at a higher level, but inbound connections are more numerous and
    // uncontrolled so to prevent log flooding the severity is reduced.
    JLOG(journal_.debug()) << "stop: Stop";

    shutdown();
}

//------------------------------------------------------------------------------

void
PeerImp::send(std::shared_ptr<Message> const& m)
{
    if (!strand_.running_in_this_thread())
        return post(strand_, std::bind(&PeerImp::send, shared_from_this(), m));

    if (!socket_.is_open())
        return;

    // we are in progress of closing the connection
    if (shutdown_)
        return tryAsyncShutdown();

    auto validator = m->getValidatorKey();
    if (validator && !squelch_.expireSquelch(*validator))
    {
        overlay_.reportOutboundTraffic(
            TrafficCount::category::squelch_suppressed,
            static_cast<int>(m->getBuffer(compressionEnabled_).size()));
        return;
    }

    // report categorized outgoing traffic
    overlay_.reportOutboundTraffic(
        safe_cast<TrafficCount::category>(m->getCategory()),
        static_cast<int>(m->getBuffer(compressionEnabled_).size()));

    // report total outgoing traffic
    overlay_.reportOutboundTraffic(
        TrafficCount::category::total,
        static_cast<int>(m->getBuffer(compressionEnabled_).size()));

    auto sendq_size = send_queue_.size();

    if (sendq_size < Tuning::targetSendQueue)
    {
        // To detect a peer that does not read from their
        // side of the connection, we expect a peer to have
        // a small senq periodically
        large_sendq_ = 0;
    }
    else if (auto sink = journal_.debug();
             sink && (sendq_size % Tuning::sendQueueLogFreq) == 0)
    {
        std::string const n = name();
        sink << n << " sendq: " << sendq_size;
    }

    send_queue_.push(m);

    if (sendq_size != 0)
        return;

    writePending_ = true;
    boost::asio::async_write(
        stream_,
        boost::asio::buffer(
            send_queue_.front()->getBuffer(compressionEnabled_)),
        bind_executor(
            strand_,
            std::bind(
                &PeerImp::onWriteMessage,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2)));
}

void
PeerImp::sendTxQueue()
{
    if (!strand_.running_in_this_thread())
        return post(
            strand_, std::bind(&PeerImp::sendTxQueue, shared_from_this()));

    if (!txQueue_.empty())
    {
        protocol::TMHaveTransactions ht;
        std::for_each(txQueue_.begin(), txQueue_.end(), [&](auto const& hash) {
            ht.add_hashes(hash.data(), hash.size());
        });
        JLOG(p_journal_.trace()) << "sendTxQueue " << txQueue_.size();
        txQueue_.clear();
        send(std::make_shared<Message>(ht, protocol::mtHAVE_TRANSACTIONS));
    }
}

void
PeerImp::addTxQueue(uint256 const& hash)
{
    if (!strand_.running_in_this_thread())
        return post(
            strand_, std::bind(&PeerImp::addTxQueue, shared_from_this(), hash));

    if (txQueue_.size() == reduce_relay::MAX_TX_QUEUE_SIZE)
    {
        JLOG(p_journal_.warn()) << "addTxQueue exceeds the cap";
        sendTxQueue();
    }

    txQueue_.insert(hash);
    JLOG(p_journal_.trace()) << "addTxQueue " << txQueue_.size();
}

void
PeerImp::removeTxQueue(uint256 const& hash)
{
    if (!strand_.running_in_this_thread())
        return post(
            strand_,
            std::bind(&PeerImp::removeTxQueue, shared_from_this(), hash));

    auto removed = txQueue_.erase(hash);
    JLOG(p_journal_.trace()) << "removeTxQueue " << removed;
}

void
PeerImp::charge(Resource::Charge const& fee, std::string const& context)
{
    if ((usage_.charge(fee, context) == Resource::drop) &&
        usage_.disconnect(p_journal_) && strand_.running_in_this_thread())
    {
        // Sever the connection
        overlay_.incPeerDisconnectCharges();
        fail("charge: Resources");
    }
}

//------------------------------------------------------------------------------

bool
PeerImp::crawl() const
{
    auto const iter = headers_.find("Crawl");
    if (iter == headers_.end())
        return false;
    return boost::iequals(iter->value(), "public");
}

bool
PeerImp::cluster() const
{
    return static_cast<bool>(app_.cluster().member(publicKey_));
}

std::string
PeerImp::getVersion() const
{
    if (inbound_)
        return headers_["User-Agent"];
    return headers_["Server"];
}

Json::Value
PeerImp::json()
{
    Json::Value ret(Json::objectValue);

    ret[jss::public_key] = toBase58(TokenType::NodePublic, publicKey_);
    ret[jss::address] = remote_address_.to_string();

    if (inbound_)
        ret[jss::inbound] = true;

    if (cluster())
    {
        ret[jss::cluster] = true;

        if (auto const n = name(); !n.empty())
            // Could move here if Json::Value supported moving from a string
            ret[jss::name] = n;
    }

    if (auto const d = domain(); !d.empty())
        ret[jss::server_domain] = std::string{d};

    if (auto const nid = headers_["Network-ID"]; !nid.empty())
        ret[jss::network_id] = std::string{nid};

    ret[jss::load] = usage_.balance();

    if (auto const version = getVersion(); !version.empty())
        ret[jss::version] = std::string{version};

    ret[jss::protocol] = to_string(protocol_);

    {
        std::lock_guard sl(recentLock_);
        if (latency_)
            ret[jss::latency] = static_cast<Json::UInt>(latency_->count());
    }

    ret[jss::uptime] = static_cast<Json::UInt>(
        std::chrono::duration_cast<std::chrono::seconds>(uptime()).count());

    std::uint32_t minSeq, maxSeq;
    ledgerRange(minSeq, maxSeq);

    if ((minSeq != 0) || (maxSeq != 0))
        ret[jss::complete_ledgers] =
            std::to_string(minSeq) + " - " + std::to_string(maxSeq);

    switch (tracking_.load())
    {
        case Tracking::diverged:
            ret[jss::track] = "diverged";
            break;

        case Tracking::unknown:
            ret[jss::track] = "unknown";
            break;

        case Tracking::converged:
            // Nothing to do here
            break;
    }

    uint256 closedLedgerHash;
    protocol::TMStatusChange last_status;
    {
        std::lock_guard sl(recentLock_);
        closedLedgerHash = closedLedgerHash_;
        last_status = last_status_;
    }

    if (closedLedgerHash != beast::zero)
        ret[jss::ledger] = to_string(closedLedgerHash);

    if (last_status.has_newstatus())
    {
        switch (last_status.newstatus())
        {
            case protocol::nsCONNECTING:
                ret[jss::status] = "connecting";
                break;

            case protocol::nsCONNECTED:
                ret[jss::status] = "connected";
                break;

            case protocol::nsMONITORING:
                ret[jss::status] = "monitoring";
                break;

            case protocol::nsVALIDATING:
                ret[jss::status] = "validating";
                break;

            case protocol::nsSHUTTING:
                ret[jss::status] = "shutting";
                break;

            default:
                JLOG(p_journal_.warn())
                    << "Unknown status: " << last_status.newstatus();
        }
    }

    ret[jss::metrics] = Json::Value(Json::objectValue);
    ret[jss::metrics][jss::total_bytes_recv] =
        std::to_string(metrics_.recv.total_bytes());
    ret[jss::metrics][jss::total_bytes_sent] =
        std::to_string(metrics_.sent.total_bytes());
    ret[jss::metrics][jss::avg_bps_recv] =
        std::to_string(metrics_.recv.average_bytes());
    ret[jss::metrics][jss::avg_bps_sent] =
        std::to_string(metrics_.sent.average_bytes());

    return ret;
}

bool
PeerImp::supportsFeature(ProtocolFeature f) const
{
    switch (f)
    {
        case ProtocolFeature::ValidatorListPropagation:
            return protocol_ >= make_protocol(2, 1);
        case ProtocolFeature::ValidatorList2Propagation:
            return protocol_ >= make_protocol(2, 2);
        case ProtocolFeature::LedgerReplay:
            return ledgerReplayEnabled_;
    }
    return false;
}

//------------------------------------------------------------------------------

bool
PeerImp::hasLedger(uint256 const& hash, std::uint32_t seq) const
{
    {
        std::lock_guard sl(recentLock_);
        if ((seq != 0) && (seq >= minLedger_) && (seq <= maxLedger_) &&
            (tracking_.load() == Tracking::converged))
            return true;
        if (std::find(recentLedgers_.begin(), recentLedgers_.end(), hash) !=
            recentLedgers_.end())
            return true;
    }
    return false;
}

void
PeerImp::ledgerRange(std::uint32_t& minSeq, std::uint32_t& maxSeq) const
{
    std::lock_guard sl(recentLock_);

    minSeq = minLedger_;
    maxSeq = maxLedger_;
}

bool
PeerImp::hasTxSet(uint256 const& hash) const
{
    std::lock_guard sl(recentLock_);
    return std::find(recentTxSets_.begin(), recentTxSets_.end(), hash) !=
        recentTxSets_.end();
}

void
PeerImp::cycleStatus()
{
    // Operations on closedLedgerHash_ and previousLedgerHash_ must be
    // guarded by recentLock_.
    std::lock_guard sl(recentLock_);
    previousLedgerHash_ = closedLedgerHash_;
    closedLedgerHash_.zero();
}

bool
PeerImp::hasRange(std::uint32_t uMin, std::uint32_t uMax)
{
    std::lock_guard sl(recentLock_);
    return (tracking_ != Tracking::diverged) && (uMin >= minLedger_) &&
        (uMax <= maxLedger_);
}

//------------------------------------------------------------------------------

void
PeerImp::fail(std::string const& name, error_code ec)
{
    XRPL_ASSERT(
        strand_.running_in_this_thread(),
        "xrpl::PeerImp::fail : strand in this thread");

    if (!socket_.is_open())
        return;

    JLOG(journal_.warn()) << name << ": " << ec.message();

    shutdown();
}

void
PeerImp::fail(std::string const& reason)
{
    if (!strand_.running_in_this_thread())
        return post(
            strand_,
            std::bind(
                (void(Peer::*)(std::string const&)) & PeerImp::fail,
                shared_from_this(),
                reason));

    if (!socket_.is_open())
        return;

    // Call to name() locks, log only if the message will be outputted
    if (journal_.active(beast::severities::kWarning))
    {
        std::string const n = name();
        JLOG(journal_.warn()) << n << " failed: " << reason;
    }

    shutdown();
}

void
PeerImp::tryAsyncShutdown()
{
    XRPL_ASSERT(
        strand_.running_in_this_thread(),
        "xrpl::PeerImp::tryAsyncShutdown : strand in this thread");

    if (!shutdown_ || shutdownStarted_)
        return;

    if (readPending_ || writePending_)
        return;

    shutdownStarted_ = true;

    setTimer(shutdownTimerInterval);

    // gracefully shutdown the SSL socket, performing a shutdown handshake
    stream_.async_shutdown(bind_executor(
        strand_,
        std::bind(
            &PeerImp::onShutdown, shared_from_this(), std::placeholders::_1)));
}

void
PeerImp::shutdown()
{
    XRPL_ASSERT(
        strand_.running_in_this_thread(),
        "xrpl::PeerImp::shutdown: strand in this thread");

    if (!socket_.is_open() || shutdown_)
        return;

    shutdown_ = true;

    boost::beast::get_lowest_layer(stream_).cancel();

    tryAsyncShutdown();
}

void
PeerImp::onShutdown(error_code ec)
{
    cancelTimer();
    if (ec)
    {
        // - eof: the stream was cleanly closed
        // - operation_aborted: an expired timer (slow shutdown)
        // - stream_truncated: the tcp connection closed (no handshake) it could
        // occur if a peer does not perform a graceful disconnect
        // - broken_pipe: the peer is gone
        bool shouldLog =
            (ec != boost::asio::error::eof &&
             ec != boost::asio::error::operation_aborted &&
             ec.message().find("application data after close notify") ==
                 std::string::npos);

        if (shouldLog)
        {
            JLOG(journal_.debug()) << "onShutdown: " << ec.message();
        }
    }

    close();
}

void
PeerImp::close()
{
    XRPL_ASSERT(
        strand_.running_in_this_thread(),
        "xrpl::PeerImp::close : strand in this thread");

    if (!socket_.is_open())
        return;

    cancelTimer();

    error_code ec;
    socket_.close(ec);

    overlay_.incPeerDisconnect();

    // The rationale for using different severity levels is that
    // outbound connections are under our control and may be logged
    // at a higher level, but inbound connections are more numerous and
    // uncontrolled so to prevent log flooding the severity is reduced.
    JLOG((inbound_ ? journal_.debug() : journal_.info())) << "close: Closed";
}

//------------------------------------------------------------------------------

void
PeerImp::setTimer(std::chrono::seconds interval)
{
    try
    {
        timer_.expires_after(interval);
    }
    catch (std::exception const& ex)
    {
        JLOG(journal_.error()) << "setTimer: " << ex.what();
        return shutdown();
    }

    timer_.async_wait(bind_executor(
        strand_,
        std::bind(
            &PeerImp::onTimer, shared_from_this(), std::placeholders::_1)));
}

//------------------------------------------------------------------------------

std::string
PeerImp::makePrefix(std::string const& fingerprint)
{
    std::stringstream ss;
    ss << "[" << fingerprint << "] ";
    return ss.str();
}

void
PeerImp::onTimer(error_code const& ec)
{
    XRPL_ASSERT(
        strand_.running_in_this_thread(),
        "xrpl::PeerImp::onTimer : strand in this thread");

    if (!socket_.is_open())
        return;

    if (ec)
    {
        // do not initiate shutdown, timers are frequently cancelled
        if (ec == boost::asio::error::operation_aborted)
            return;

        // This should never happen
        JLOG(journal_.error()) << "onTimer: " << ec.message();
        return close();
    }

    // the timer expired before the shutdown completed
    // force close the connection
    if (shutdown_)
    {
        JLOG(journal_.debug()) << "onTimer: shutdown timer expired";
        return close();
    }

    if (large_sendq_++ >= Tuning::sendqIntervals)
        return fail("Large send queue");

    if (auto const t = tracking_.load(); !inbound_ && t != Tracking::converged)
    {
        clock_type::duration duration;

        {
            std::lock_guard sl(recentLock_);
            duration = clock_type::now() - trackingTime_;
        }

        if ((t == Tracking::diverged &&
             (duration > app_.config().MAX_DIVERGED_TIME)) ||
            (t == Tracking::unknown &&
             (duration > app_.config().MAX_UNKNOWN_TIME)))
        {
            overlay_.peerFinder().on_failure(slot_);
            return fail("Not useful");
        }
    }

    // Already waiting for PONG
    if (lastPingSeq_)
        return fail("Ping Timeout");

    lastPingTime_ = clock_type::now();
    lastPingSeq_ = rand_int<std::uint32_t>();

    protocol::TMPing message;
    message.set_type(protocol::TMPing::ptPING);
    message.set_seq(*lastPingSeq_);

    send(std::make_shared<Message>(message, protocol::mtPING));

    setTimer(peerTimerInterval);
}

void
PeerImp::cancelTimer() noexcept
{
    try
    {
        timer_.cancel();
    }
    catch (std::exception const& ex)
    {
        JLOG(journal_.error()) << "cancelTimer: " << ex.what();
    }
}

//------------------------------------------------------------------------------
void
PeerImp::doAccept()
{
    XRPL_ASSERT(
        read_buffer_.size() == 0,
        "xrpl::PeerImp::doAccept : empty read buffer");

    JLOG(journal_.debug()) << "doAccept";

    // a shutdown was initiated before the handshake, there is nothing to do
    if (shutdown_)
        return tryAsyncShutdown();

    auto const sharedValue = makeSharedValue(*stream_ptr_, journal_);

    // This shouldn't fail since we already computed
    // the shared value successfully in OverlayImpl
    if (!sharedValue)
        return fail("makeSharedValue: Unexpected failure");

    JLOG(journal_.debug()) << "Protocol: " << to_string(protocol_);

    if (auto member = app_.cluster().member(publicKey_))
    {
        {
            std::unique_lock lock{nameMutex_};
            name_ = *member;
        }
        JLOG(journal_.info()) << "Cluster name: " << *member;
    }

    overlay_.activate(shared_from_this());

    // XXX Set timer: connection is in grace period to be useful.
    // XXX Set timer: connection idle (idle may vary depending on connection
    // type.)

    auto write_buffer = std::make_shared<boost::beast::multi_buffer>();

    boost::beast::ostream(*write_buffer) << makeResponse(
        !overlay_.peerFinder().config().peerPrivate,
        request_,
        overlay_.setup().public_ip,
        remote_address_.address(),
        *sharedValue,
        overlay_.setup().networkID,
        protocol_,
        app_);

    // Write the whole buffer and only start protocol when that's done.
    boost::asio::async_write(
        stream_,
        write_buffer->data(),
        boost::asio::transfer_all(),
        bind_executor(
            strand_,
            [this, write_buffer, self = shared_from_this()](
                error_code ec, std::size_t bytes_transferred) {
                if (!socket_.is_open())
                    return;
                if (ec == boost::asio::error::operation_aborted)
                    return tryAsyncShutdown();
                if (ec)
                    return fail("onWriteResponse", ec);
                if (write_buffer->size() == bytes_transferred)
                    return doProtocolStart();
                return fail("Failed to write header");
            }));
}

std::string
PeerImp::name() const
{
    std::shared_lock read_lock{nameMutex_};
    return name_;
}

std::string
PeerImp::domain() const
{
    return headers_["Server-Domain"];
}

//------------------------------------------------------------------------------

// Protocol logic

void
PeerImp::doProtocolStart()
{
    // a shutdown was initiated before the handshare, there is nothing to do
    if (shutdown_)
        return tryAsyncShutdown();

    onReadMessage(error_code(), 0);

    // Send all the validator lists that have been loaded
    if (inbound_ && supportsFeature(ProtocolFeature::ValidatorListPropagation))
    {
        // Delegate to ValidatorListPropagationHandler which has access to
        // ValidatorList in the app module to avoid cycle dependencies
        ValidatorListPropagationHandler::sendValidatorLists(*this);
    }

    if (auto m = overlay_.getManifestsMessage())
        send(m);

    setTimer(peerTimerInterval);
}

// Called repeatedly with protocol message data
void
PeerImp::onReadMessage(error_code ec, std::size_t bytes_transferred)
{
    XRPL_ASSERT(
        strand_.running_in_this_thread(),
        "xrpl::PeerImp::onReadMessage : strand in this thread");

    readPending_ = false;

    if (!socket_.is_open())
        return;

    if (ec)
    {
        if (ec == boost::asio::error::eof)
        {
            JLOG(journal_.debug()) << "EOF";
            return shutdown();
        }

        if (ec == boost::asio::error::operation_aborted)
            return tryAsyncShutdown();

        return fail("onReadMessage", ec);
    }
    // we started shutdown, no reason to process further data
    if (shutdown_)
        return tryAsyncShutdown();

    if (auto stream = journal_.trace())
    {
        stream << "onReadMessage: "
               << (bytes_transferred > 0
                       ? to_string(bytes_transferred) + " bytes"
                       : "");
    }

    metrics_.recv.add_message(bytes_transferred);

    read_buffer_.commit(bytes_transferred);

    auto hint = Tuning::readBufferBytes;

    while (read_buffer_.size() > 0)
    {
        std::size_t bytes_consumed;

        using namespace std::chrono_literals;
        std::tie(bytes_consumed, ec) = perf::measureDurationAndLog(
            [&]() {
                return invokeProtocolMessage(read_buffer_.data(), *this, hint);
            },
            "invokeProtocolMessage",
            350ms,
            journal_);

        if (!socket_.is_open())
            return;

        // the error_code is produced by invokeProtocolMessage
        // it could be due to a bad message
        if (ec)
            return fail("onReadMessage", ec);

        if (bytes_consumed == 0)
            break;

        read_buffer_.consume(bytes_consumed);
    }

    // check if a shutdown was initiated while processing messages
    if (shutdown_)
        return tryAsyncShutdown();

    readPending_ = true;

    XRPL_ASSERT(
        !shutdownStarted_, "xrpl::PeerImp::onReadMessage : shutdown started");

    // Timeout on writes only
    stream_.async_read_some(
        read_buffer_.prepare(std::max(Tuning::readBufferBytes, hint)),
        bind_executor(
            strand_,
            std::bind(
                &PeerImp::onReadMessage,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2)));
}

void
PeerImp::onWriteMessage(error_code ec, std::size_t bytes_transferred)
{
    XRPL_ASSERT(
        strand_.running_in_this_thread(),
        "xrpl::PeerImp::onWriteMessage : strand in this thread");

    writePending_ = false;

    if (!socket_.is_open())
        return;

    if (ec)
    {
        if (ec == boost::asio::error::operation_aborted)
            return tryAsyncShutdown();

        return fail("onWriteMessage", ec);
    }

    if (auto stream = journal_.trace())
    {
        stream << "onWriteMessage: "
               << (bytes_transferred > 0
                       ? to_string(bytes_transferred) + " bytes"
                       : "");
    }

    metrics_.sent.add_message(bytes_transferred);

    XRPL_ASSERT(
        !send_queue_.empty(),
        "xrpl::PeerImp::onWriteMessage : non-empty send buffer");
    send_queue_.pop();

    if (shutdown_)
        return tryAsyncShutdown();

    if (!send_queue_.empty())
    {
        writePending_ = true;
        XRPL_ASSERT(
            !shutdownStarted_,
            "xrpl::PeerImp::onWriteMessage : shutdown started");

        // Timeout on writes only
        return boost::asio::async_write(
            stream_,
            boost::asio::buffer(
                send_queue_.front()->getBuffer(compressionEnabled_)),
            bind_executor(
                strand_,
                std::bind(
                    &PeerImp::onWriteMessage,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2)));
    }
}

//------------------------------------------------------------------------------
//
// ProtocolHandler
//
//------------------------------------------------------------------------------

void
PeerImp::onMessageUnknown(std::uint16_t type)
{
    // TODO
}

void
PeerImp::onMessageBegin(
    std::uint16_t type,
    std::shared_ptr<::google::protobuf::Message> const& m,
    std::size_t size,
    std::size_t uncompressed_size,
    bool isCompressed)
{
    auto const name = protocolMessageName(type);
    load_event_ = app_.getJobQueue().makeLoadEvent(jtPEER, name);
    fee_ = {Resource::feeTrivialPeer, name};

    auto const category = TrafficCount::categorize(
        *m, static_cast<protocol::MessageType>(type), true);

    // report total incoming traffic
    overlay_.reportInboundTraffic(
        TrafficCount::category::total, static_cast<int>(size));

    // increase the traffic received for a specific category
    overlay_.reportInboundTraffic(category, static_cast<int>(size));

    using namespace protocol;
    if ((type == MessageType::mtTRANSACTION ||
         type == MessageType::mtHAVE_TRANSACTIONS ||
         type == MessageType::mtTRANSACTIONS ||
         // GET_OBJECTS
         category == TrafficCount::category::get_transactions ||
         // GET_LEDGER
         category == TrafficCount::category::ld_tsc_get ||
         category == TrafficCount::category::ld_tsc_share ||
         // LEDGER_DATA
         category == TrafficCount::category::gl_tsc_share ||
         category == TrafficCount::category::gl_tsc_get) &&
        (txReduceRelayEnabled() || app_.config().TX_REDUCE_RELAY_METRICS))
    {
        overlay_.addTxMetrics(
            static_cast<MessageType>(type), static_cast<std::uint64_t>(size));
    }
    JLOG(journal_.trace()) << "onMessageBegin: " << type << " " << size << " "
                           << uncompressed_size << " " << isCompressed;
}

void
PeerImp::onMessageEnd(
    std::uint16_t,
    std::shared_ptr<::google::protobuf::Message> const&)
{
    load_event_.reset();
    charge(fee_.fee, fee_.context);
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMManifests> const& m)
{
    auto const s = m->list_size();

    if (s == 0)
    {
        fee_.update(Resource::feeUselessData, "empty");
        return;
    }

    if (s > 100)
        fee_.update(Resource::feeModerateBurdenPeer, "oversize");

    app_.getJobQueue().addJob(
        jtMANIFEST, "RcvManifests", [this, that = shared_from_this(), m]() {
            overlay_.onManifests(m, that);
        });
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMPing> const& m)
{
    if (m->type() == protocol::TMPing::ptPING)
    {
        // We have received a ping request, reply with a pong
        fee_.update(Resource::feeModerateBurdenPeer, "ping request");
        m->set_type(protocol::TMPing::ptPONG);
        send(std::make_shared<Message>(*m, protocol::mtPING));
        return;
    }

    if (m->type() == protocol::TMPing::ptPONG && m->has_seq())
    {
        // Only reset the ping sequence if we actually received a
        // PONG with the correct cookie. That way, any peers which
        // respond with incorrect cookies will eventually time out.
        if (m->seq() == lastPingSeq_)
        {
            lastPingSeq_.reset();

            // Update latency estimate
            auto const rtt = std::chrono::round<std::chrono::milliseconds>(
                clock_type::now() - lastPingTime_);

            std::lock_guard sl(recentLock_);

            if (latency_)
                latency_ = (*latency_ * 7 + rtt) / 8;
            else
                latency_ = rtt;
        }

        return;
    }
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMCluster> const& m)
{
    // VFALCO NOTE I think we should drop the peer immediately
    if (!cluster())
    {
        fee_.update(Resource::feeUselessData, "unknown cluster");
        return;
    }

    for (int i = 0; i < m->clusternodes().size(); ++i)
    {
        protocol::TMClusterNode const& node = m->clusternodes(i);

        std::string name;
        if (node.has_nodename())
            name = node.nodename();

        auto const publicKey =
            parseBase58<PublicKey>(TokenType::NodePublic, node.publickey());

        // NIKB NOTE We should drop the peer immediately if
        // they send us a public key we can't parse
        if (publicKey)
        {
            auto const reportTime =
                NetClock::time_point{NetClock::duration{node.reporttime()}};

            app_.cluster().update(
                *publicKey, name, node.nodeload(), reportTime);
        }
    }

    int loadSources = m->loadsources().size();
    if (loadSources != 0)
    {
        Resource::Gossip gossip;
        gossip.items.reserve(loadSources);
        for (int i = 0; i < m->loadsources().size(); ++i)
        {
            protocol::TMLoadSource const& node = m->loadsources(i);
            Resource::Gossip::Item item;
            item.address = beast::IP::Endpoint::from_string(node.name());
            item.balance = node.cost();
            if (item.address != beast::IP::Endpoint())
                gossip.items.push_back(item);
        }
        overlay_.resourceManager().importConsumers(name(), gossip);
    }

    // Calculate the cluster fee:
    auto const thresh = app_.timeKeeper().now() - 90s;
    std::uint32_t clusterFee = 0;

    std::vector<std::uint32_t> fees;
    fees.reserve(app_.cluster().size());

    app_.cluster().for_each([&fees, thresh](ClusterNode const& status) {
        if (status.getReportTime() >= thresh)
            fees.push_back(status.getLoadFee());
    });

    if (!fees.empty())
    {
        auto const index = fees.size() / 2;
        std::nth_element(fees.begin(), fees.begin() + index, fees.end());
        clusterFee = fees[index];
    }

    app_.getFeeTrack().setClusterFee(clusterFee);
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMEndpoints> const& m)
{
    // Don't allow endpoints from peers that are not known tracking or are
    // not using a version of the message that we support:
    if (tracking_.load() != Tracking::converged || m->version() != 2)
        return;

    // The number is arbitrary and doesn't have any real significance or
    // implication for the protocol.
    if (m->endpoints_v2().size() >= 1024)
    {
        fee_.update(Resource::feeUselessData, "endpoints too large");
        return;
    }

    std::vector<PeerFinder::Endpoint> endpoints;
    endpoints.reserve(m->endpoints_v2().size());

    auto malformed = 0;
    for (auto const& tm : m->endpoints_v2())
    {
        auto result = beast::IP::Endpoint::from_string_checked(tm.endpoint());

        if (!result)
        {
            JLOG(p_journal_.error()) << "failed to parse incoming endpoint: {"
                                     << tm.endpoint() << "}";
            malformed++;
            continue;
        }

        // If hops == 0, this Endpoint describes the peer we are connected
        // to -- in that case, we take the remote address seen on the
        // socket and store that in the IP::Endpoint. If this is the first
        // time, then we'll verify that their listener can receive incoming
        // by performing a connectivity test.  if hops > 0, then we just
        // take the address/port we were given
        if (tm.hops() == 0)
            result = remote_address_.at_port(result->port());

        endpoints.emplace_back(*result, tm.hops());
    }

    // Charge the peer for each malformed endpoint. As there still may be
    // multiple valid endpoints we don't return early.
    if (malformed > 0)
    {
        fee_.update(
            Resource::feeInvalidData * malformed,
            std::to_string(malformed) + " malformed endpoints");
    }

    if (!endpoints.empty())
        overlay_.peerFinder().on_endpoints(slot_, endpoints);
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMTransaction> const& m)
{
    // Delegate to TransactionMessageHandler which has the implementation
    // in the app module to avoid cycle dependencies
    TransactionMessageHandler::onMessage(m, *this);
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMGetLedger> const& m)
{
    auto badData = [&](std::string const& msg) {
        fee_.update(Resource::feeInvalidData, "get_ledger " + msg);
        JLOG(p_journal_.warn()) << "TMGetLedger: " << msg;
    };
    auto const itype{m->itype()};

    // Verify ledger info type
    if (itype < protocol::liBASE || itype > protocol::liTS_CANDIDATE)
        return badData("Invalid ledger info type");

    auto const ltype = [&m]() -> std::optional<::protocol::TMLedgerType> {
        if (m->has_ltype())
            return m->ltype();
        return std::nullopt;
    }();

    if (itype == protocol::liTS_CANDIDATE)
    {
        if (!m->has_ledgerhash())
            return badData("Invalid TX candidate set, missing TX set hash");
    }
    else if (
        !m->has_ledgerhash() && !m->has_ledgerseq() &&
        !(ltype && *ltype == protocol::ltCLOSED))
    {
        return badData("Invalid request");
    }

    // Verify ledger type
    if (ltype && (*ltype < protocol::ltACCEPTED || *ltype > protocol::ltCLOSED))
        return badData("Invalid ledger type");

    // Verify ledger hash
    if (m->has_ledgerhash() && !stringIsUint256Sized(m->ledgerhash()))
        return badData("Invalid ledger hash");

    // Verify ledger sequence
    if (m->has_ledgerseq())
    {
        auto const ledgerSeq{m->ledgerseq()};

        // Check if within a reasonable range
        using namespace std::chrono_literals;
        if (app_.getLedgerMaster().getValidatedLedgerAge() <= 10s &&
            ledgerSeq > app_.getLedgerMaster().getValidLedgerIndex() + 10)
        {
            return badData(
                "Invalid ledger sequence " + std::to_string(ledgerSeq));
        }
    }

    // Verify ledger node IDs
    if (itype != protocol::liBASE)
    {
        if (m->nodeids_size() <= 0)
            return badData("Invalid ledger node IDs");

        for (auto const& nodeId : m->nodeids())
        {
            if (deserializeSHAMapNodeID(nodeId) == std::nullopt)
                return badData("Invalid SHAMap node ID");
        }
    }

    // Verify query type
    if (m->has_querytype() && m->querytype() != protocol::qtINDIRECT)
        return badData("Invalid query type");

    // Verify query depth
    if (m->has_querydepth())
    {
        if (m->querydepth() > Tuning::maxQueryDepth ||
            itype == protocol::liBASE)
        {
            return badData("Invalid query depth");
        }
    }

    // Queue a job to process the request
    std::weak_ptr<PeerImp> weak = shared_from_this();
    app_.getJobQueue().addJob(jtLEDGER_REQ, "RcvGetLedger", [weak, m]() {
        if (auto peer = weak.lock())
            peer->processLedgerRequest(m);
    });
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMProofPathRequest> const& m)
{
    JLOG(p_journal_.trace()) << "onMessage, TMProofPathRequest";
    if (!ledgerReplayEnabled_)
    {
        fee_.update(
            Resource::feeMalformedRequest, "proof_path_request disabled");
        return;
    }

    fee_.update(
        Resource::feeModerateBurdenPeer, "received a proof path request");
    std::weak_ptr<PeerImp> weak = shared_from_this();
    app_.getJobQueue().addJob(jtREPLAY_REQ, "RcvProofPReq", [weak, m]() {
        if (auto peer = weak.lock())
        {
            auto reply =
                peer->ledgerReplayMsgHandler_->processProofPathRequest(m);
            if (reply.has_error())
            {
                if (reply.error() == protocol::TMReplyError::reBAD_REQUEST)
                    peer->charge(
                        Resource::feeMalformedRequest, "proof_path_request");
                else
                    peer->charge(
                        Resource::feeRequestNoReply, "proof_path_request");
            }
            else
            {
                peer->send(std::make_shared<Message>(
                    reply, protocol::mtPROOF_PATH_RESPONSE));
            }
        }
    });
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMProofPathResponse> const& m)
{
    if (!ledgerReplayEnabled_)
    {
        fee_.update(
            Resource::feeMalformedRequest, "proof_path_response disabled");
        return;
    }

    if (!ledgerReplayMsgHandler_->processProofPathResponse(m))
    {
        fee_.update(Resource::feeInvalidData, "proof_path_response");
    }
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMReplayDeltaRequest> const& m)
{
    JLOG(p_journal_.trace()) << "onMessage, TMReplayDeltaRequest";
    if (!ledgerReplayEnabled_)
    {
        fee_.update(
            Resource::feeMalformedRequest, "replay_delta_request disabled");
        return;
    }

    fee_.fee = Resource::feeModerateBurdenPeer;
    std::weak_ptr<PeerImp> weak = shared_from_this();
    app_.getJobQueue().addJob(jtREPLAY_REQ, "RcvReplDReq", [weak, m]() {
        if (auto peer = weak.lock())
        {
            auto reply =
                peer->ledgerReplayMsgHandler_->processReplayDeltaRequest(m);
            if (reply.has_error())
            {
                if (reply.error() == protocol::TMReplyError::reBAD_REQUEST)
                    peer->charge(
                        Resource::feeMalformedRequest, "replay_delta_request");
                else
                    peer->charge(
                        Resource::feeRequestNoReply, "replay_delta_request");
            }
            else
            {
                peer->send(std::make_shared<Message>(
                    reply, protocol::mtREPLAY_DELTA_RESPONSE));
            }
        }
    });
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMReplayDeltaResponse> const& m)
{
    if (!ledgerReplayEnabled_)
    {
        fee_.update(
            Resource::feeMalformedRequest, "replay_delta_response disabled");
        return;
    }

    if (!ledgerReplayMsgHandler_->processReplayDeltaResponse(m))
    {
        fee_.update(Resource::feeInvalidData, "replay_delta_response");
    }
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMLedgerData> const& m)
{
    auto badData = [&](std::string const& msg) {
        fee_.update(Resource::feeInvalidData, msg);
        JLOG(p_journal_.warn()) << "TMLedgerData: " << msg;
    };

    // Verify ledger hash
    if (!stringIsUint256Sized(m->ledgerhash()))
        return badData("Invalid ledger hash");

    // Verify ledger sequence
    {
        auto const ledgerSeq{m->ledgerseq()};
        if (m->type() == protocol::liTS_CANDIDATE)
        {
            if (ledgerSeq != 0)
            {
                return badData(
                    "Invalid ledger sequence " + std::to_string(ledgerSeq));
            }
        }
        else
        {
            // Check if within a reasonable range
            using namespace std::chrono_literals;
            if (app_.getLedgerMaster().getValidatedLedgerAge() <= 10s &&
                ledgerSeq > app_.getLedgerMaster().getValidLedgerIndex() + 10)
            {
                return badData(
                    "Invalid ledger sequence " + std::to_string(ledgerSeq));
            }
        }
    }

    // Verify ledger info type
    if (m->type() < protocol::liBASE || m->type() > protocol::liTS_CANDIDATE)
        return badData("Invalid ledger info type");

    // Verify reply error
    if (m->has_error() &&
        (m->error() < protocol::reNO_LEDGER ||
         m->error() > protocol::reBAD_REQUEST))
    {
        return badData("Invalid reply error");
    }

    // Verify ledger nodes.
    if (m->nodes_size() <= 0 || m->nodes_size() > Tuning::hardMaxReplyNodes)
    {
        return badData(
            "Invalid Ledger/TXset nodes " + std::to_string(m->nodes_size()));
    }

    // If there is a request cookie, attempt to relay the message
    if (m->has_requestcookie())
    {
        if (auto peer = overlay_.findPeerByShortID(m->requestcookie()))
        {
            m->clear_requestcookie();
            peer->send(std::make_shared<Message>(*m, protocol::mtLEDGER_DATA));
        }
        else
        {
            JLOG(p_journal_.info()) << "Unable to route TX/ledger data reply";
        }
        return;
    }

    uint256 const ledgerHash{m->ledgerhash()};

    // Otherwise check if received data for a candidate transaction set
    if (m->type() == protocol::liTS_CANDIDATE)
    {
        std::weak_ptr<PeerImp> weak{shared_from_this()};
        app_.getJobQueue().addJob(
            jtTXN_DATA, "RcvPeerData", [weak, ledgerHash, m]() {
                if (auto peer = weak.lock())
                {
                    peer->app_.getInboundTransactions().gotData(
                        ledgerHash, peer, m);
                }
            });
        return;
    }

    // Consume the message
    app_.getInboundLedgers().gotLedgerData(ledgerHash, shared_from_this(), m);
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMProposeSet> const& m)
{
    // Delegate to the ProposalMessageHandler which has the implementation
    // in the app module to avoid cycle dependencies
    ProposalMessageHandler::onMessage(m, *this);
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMStatusChange> const& m)
{
    JLOG(p_journal_.trace()) << "Status: Change";

    if (!m->has_networktime())
        m->set_networktime(app_.timeKeeper().now().time_since_epoch().count());

    {
        std::lock_guard sl(recentLock_);
        if (!last_status_.has_newstatus() || m->has_newstatus())
            last_status_ = *m;
        else
        {
            // preserve old status
            protocol::NodeStatus status = last_status_.newstatus();
            last_status_ = *m;
            m->set_newstatus(status);
        }
    }

    if (m->newevent() == protocol::neLOST_SYNC)
    {
        bool outOfSync{false};
        {
            // Operations on closedLedgerHash_ and previousLedgerHash_ must be
            // guarded by recentLock_.
            std::lock_guard sl(recentLock_);
            if (!closedLedgerHash_.isZero())
            {
                outOfSync = true;
                closedLedgerHash_.zero();
            }
            previousLedgerHash_.zero();
        }
        if (outOfSync)
        {
            JLOG(p_journal_.debug()) << "Status: Out of sync";
        }
        return;
    }

    {
        uint256 closedLedgerHash{};
        bool const peerChangedLedgers{
            m->has_ledgerhash() && stringIsUint256Sized(m->ledgerhash())};

        {
            // Operations on closedLedgerHash_ and previousLedgerHash_ must be
            // guarded by recentLock_.
            std::lock_guard sl(recentLock_);
            if (peerChangedLedgers)
            {
                closedLedgerHash_ = m->ledgerhash();
                closedLedgerHash = closedLedgerHash_;
                addLedger(closedLedgerHash, sl);
            }
            else
            {
                closedLedgerHash_.zero();
            }

            if (m->has_ledgerhashprevious() &&
                stringIsUint256Sized(m->ledgerhashprevious()))
            {
                previousLedgerHash_ = m->ledgerhashprevious();
                addLedger(previousLedgerHash_, sl);
            }
            else
            {
                previousLedgerHash_.zero();
            }
        }
        if (peerChangedLedgers)
        {
            JLOG(p_journal_.debug()) << "LCL is " << closedLedgerHash;
        }
        else
        {
            JLOG(p_journal_.debug()) << "Status: No ledger";
        }
    }

    if (m->has_firstseq() && m->has_lastseq())
    {
        std::lock_guard sl(recentLock_);

        minLedger_ = m->firstseq();
        maxLedger_ = m->lastseq();

        if ((maxLedger_ < minLedger_) || (minLedger_ == 0) || (maxLedger_ == 0))
            minLedger_ = maxLedger_ = 0;
    }

    if (m->has_ledgerseq() &&
        app_.getLedgerMaster().getValidatedLedgerAge() < 2min)
    {
        checkTracking(
            m->ledgerseq(), app_.getLedgerMaster().getValidLedgerIndex());
    }

    // Get the closed ledger hash while holding the lock
    uint256 closedLedgerHash{};
    {
        std::lock_guard sl(recentLock_);
        closedLedgerHash = closedLedgerHash_;
    }

    // Delegate status publishing to handler which has access to NetworkOPs
    StatusChangeMessageHandler::publishPeerStatus(m, *this, closedLedgerHash);
}

void
PeerImp::checkTracking(std::uint32_t validationSeq)
{
    std::uint32_t serverSeq;
    {
        // Extract the sequence number of the highest
        // ledger this peer has
        std::lock_guard sl(recentLock_);

        serverSeq = maxLedger_;
    }
    if (serverSeq != 0)
    {
        // Compare the peer's ledger sequence to the
        // sequence of a recently-validated ledger
        checkTracking(serverSeq, validationSeq);
    }
}

void
PeerImp::checkTracking(std::uint32_t seq1, std::uint32_t seq2)
{
    int diff = std::max(seq1, seq2) - std::min(seq1, seq2);

    if (diff < Tuning::convergedLedgerLimit)
    {
        // The peer's ledger sequence is close to the validation's
        tracking_ = Tracking::converged;
    }

    if ((diff > Tuning::divergedLedgerLimit) &&
        (tracking_.load() != Tracking::diverged))
    {
        // The peer's ledger sequence is way off the validation's
        std::lock_guard sl(recentLock_);

        tracking_ = Tracking::diverged;
        trackingTime_ = clock_type::now();
    }
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMHaveTransactionSet> const& m)
{
    if (!stringIsUint256Sized(m->hash()))
    {
        fee_.update(Resource::feeMalformedRequest, "bad hash");
        return;
    }

    uint256 const hash{m->hash()};

    if (m->status() == protocol::tsHAVE)
    {
        std::lock_guard sl(recentLock_);

        if (std::find(recentTxSets_.begin(), recentTxSets_.end(), hash) !=
            recentTxSets_.end())
        {
            fee_.update(Resource::feeUselessData, "duplicate (tsHAVE)");
            return;
        }

        recentTxSets_.push_back(hash);
    }
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMValidatorList> const& m)
{
    // Delegate to the ValidationMessageHandler which has the implementation
    // in the app module to avoid cycle dependencies
    ValidationMessageHandler::onMessage(m, *this);
}

void
PeerImp::onMessage(
    std::shared_ptr<protocol::TMValidatorListCollection> const& m)
{
    // Delegate to the ValidationMessageHandler which has the implementation
    // in the app module to avoid cycle dependencies
    ValidationMessageHandler::onMessage(m, *this);
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMValidation> const& m)
{
    // Delegate to the ValidationMessageHandler which has the implementation
    // in the app module to avoid cycle dependencies
    ValidationMessageHandler::onMessage(m, *this);
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMGetObjectByHash> const& m)
{
    protocol::TMGetObjectByHash& packet = *m;

    JLOG(p_journal_.trace()) << "received TMGetObjectByHash " << packet.type()
                             << " " << packet.objects_size();

    if (packet.query())
    {
        // this is a query
        if (send_queue_.size() >= Tuning::dropSendQueue)
        {
            JLOG(p_journal_.debug()) << "GetObject: Large send queue";
            return;
        }

        if (packet.type() == protocol::TMGetObjectByHash::otFETCH_PACK)
        {
            doFetchPack(m);
            return;
        }

        if (packet.type() == protocol::TMGetObjectByHash::otTRANSACTIONS)
        {
            if (!txReduceRelayEnabled())
            {
                JLOG(p_journal_.error())
                    << "TMGetObjectByHash: tx reduce-relay is disabled";
                fee_.update(Resource::feeMalformedRequest, "disabled");
                return;
            }

            std::weak_ptr<PeerImp> weak = shared_from_this();
            app_.getJobQueue().addJob(jtREQUESTED_TXN, "DoTxs", [weak, m]() {
                if (auto peer = weak.lock())
                    TransactionMessageHandler::doTransactions(*peer, m);
            });
            return;
        }

        protocol::TMGetObjectByHash reply;

        reply.set_query(false);

        if (packet.has_seq())
            reply.set_seq(packet.seq());

        reply.set_type(packet.type());

        if (packet.has_ledgerhash())
        {
            if (!stringIsUint256Sized(packet.ledgerhash()))
            {
                fee_.update(Resource::feeMalformedRequest, "ledger hash");
                return;
            }

            reply.set_ledgerhash(packet.ledgerhash());
        }

        fee_.update(
            Resource::feeModerateBurdenPeer,
            " received a get object by hash request");

        // This is a very minimal implementation
        for (int i = 0; i < packet.objects_size(); ++i)
        {
            auto const& obj = packet.objects(i);
            if (obj.has_hash() && stringIsUint256Sized(obj.hash()))
            {
                uint256 const hash{obj.hash()};
                // VFALCO TODO Move this someplace more sensible so we dont
                //             need to inject the NodeStore interfaces.
                std::uint32_t seq{obj.has_ledgerseq() ? obj.ledgerseq() : 0};
                auto nodeObject{app_.getNodeStore().fetchNodeObject(hash, seq)};
                if (nodeObject)
                {
                    protocol::TMIndexedObject& newObj = *reply.add_objects();
                    newObj.set_hash(hash.begin(), hash.size());
                    newObj.set_data(
                        &nodeObject->getData().front(),
                        nodeObject->getData().size());

                    if (obj.has_nodeid())
                        newObj.set_index(obj.nodeid());
                    if (obj.has_ledgerseq())
                        newObj.set_ledgerseq(obj.ledgerseq());

                    // VFALCO NOTE "seq" in the message is obsolete

                    // Check if by adding this object, reply has reached its
                    // limit
                    if (reply.objects_size() >= Tuning::hardMaxReplyNodes)
                    {
                        fee_.update(
                            Resource::feeModerateBurdenPeer,
                            " Reply limit reached. Truncating reply.");
                        break;
                    }
                }
            }
        }

        JLOG(p_journal_.trace()) << "GetObj: " << reply.objects_size() << " of "
                                 << packet.objects_size();
        send(std::make_shared<Message>(reply, protocol::mtGET_OBJECTS));
    }
    else
    {
        // this is a reply
        std::uint32_t pLSeq = 0;
        bool pLDo = true;
        bool progress = false;

        for (int i = 0; i < packet.objects_size(); ++i)
        {
            protocol::TMIndexedObject const& obj = packet.objects(i);

            if (obj.has_hash() && stringIsUint256Sized(obj.hash()))
            {
                if (obj.has_ledgerseq())
                {
                    if (obj.ledgerseq() != pLSeq)
                    {
                        if (pLDo && (pLSeq != 0))
                        {
                            JLOG(p_journal_.debug())
                                << "GetObj: Full fetch pack for " << pLSeq;
                        }
                        pLSeq = obj.ledgerseq();
                        pLDo = !app_.getLedgerMaster().haveLedger(pLSeq);

                        if (!pLDo)
                        {
                            JLOG(p_journal_.debug())
                                << "GetObj: Late fetch pack for " << pLSeq;
                        }
                        else
                            progress = true;
                    }
                }

                if (pLDo)
                {
                    uint256 const hash{obj.hash()};

                    app_.getLedgerMaster().addFetchPack(
                        hash,
                        std::make_shared<Blob>(
                            obj.data().begin(), obj.data().end()));
                }
            }
        }

        if (pLDo && (pLSeq != 0))
        {
            JLOG(p_journal_.debug())
                << "GetObj: Partial fetch pack for " << pLSeq;
        }
        if (packet.type() == protocol::TMGetObjectByHash::otFETCH_PACK)
            app_.getLedgerMaster().gotFetchPack(progress, pLSeq);
    }
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMHaveTransactions> const& m)
{
    if (!txReduceRelayEnabled())
    {
        JLOG(p_journal_.error())
            << "TMHaveTransactions: tx reduce-relay is disabled";
        fee_.update(Resource::feeMalformedRequest, "disabled");
        return;
    }

    std::weak_ptr<PeerImp> weak = shared_from_this();
    app_.getJobQueue().addJob(jtMISSING_TXN, "HandleHaveTxs", [weak, m]() {
        if (auto peer = weak.lock())
            TransactionMessageHandler::handleHaveTransactions(*peer, m);
    });
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMTransactions> const& m)
{
    // Delegate to TransactionMessageHandler which has the implementation
    // in the app module to avoid cycle dependencies
    TransactionMessageHandler::onMessage(m, *this);
}

void
PeerImp::onMessage(std::shared_ptr<protocol::TMSquelch> const& m)
{
    using on_message_fn =
        void (PeerImp::*)(std::shared_ptr<protocol::TMSquelch> const&);
    if (!strand_.running_in_this_thread())
        return post(
            strand_,
            std::bind(
                (on_message_fn)&PeerImp::onMessage, shared_from_this(), m));

    if (!m->has_validatorpubkey())
    {
        fee_.update(Resource::feeInvalidData, "squelch no pubkey");
        return;
    }
    auto validator = m->validatorpubkey();
    auto const slice{makeSlice(validator)};
    if (!publicKeyType(slice))
    {
        fee_.update(Resource::feeInvalidData, "squelch bad pubkey");
        return;
    }
    PublicKey key(slice);

    // Ignore the squelch for validator's own messages.
    if (key == app_.getValidationPublicKey())
    {
        JLOG(p_journal_.debug())
            << "onMessage: TMSquelch discarding validator's squelch " << slice;
        return;
    }

    std::uint32_t duration =
        m->has_squelchduration() ? m->squelchduration() : 0;
    if (!m->squelch())
        squelch_.removeSquelch(key);
    else if (!squelch_.addSquelch(key, std::chrono::seconds{duration}))
        fee_.update(Resource::feeInvalidData, "squelch duration");

    JLOG(p_journal_.debug())
        << "onMessage: TMSquelch " << slice << " " << id() << " " << duration;
}

//--------------------------------------------------------------------------

void
PeerImp::addLedger(
    uint256 const& hash,
    std::lock_guard<std::mutex> const& lockedRecentLock)
{
    // lockedRecentLock is passed as a reminder that recentLock_ must be
    // locked by the caller.
    (void)lockedRecentLock;

    if (std::find(recentLedgers_.begin(), recentLedgers_.end(), hash) !=
        recentLedgers_.end())
        return;

    recentLedgers_.push_back(hash);
}

void
PeerImp::doFetchPack(std::shared_ptr<protocol::TMGetObjectByHash> const& packet)
{
    // VFALCO TODO Invert this dependency using an observer and shared state
    // object. Don't queue fetch pack jobs if we're under load or we already
    // have some queued.
    if (app_.getFeeTrack().isLoadedLocal() ||
        (app_.getLedgerMaster().getValidatedLedgerAge() > 40s) ||
        (app_.getJobQueue().getJobCount(jtPACK) > 10))
    {
        JLOG(p_journal_.info()) << "Too busy to make fetch pack";
        return;
    }

    if (!stringIsUint256Sized(packet->ledgerhash()))
    {
        JLOG(p_journal_.warn()) << "FetchPack hash size malformed";
        fee_.update(Resource::feeMalformedRequest, "hash size");
        return;
    }

    fee_.fee = Resource::feeHeavyBurdenPeer;

    uint256 const hash{packet->ledgerhash()};

    std::weak_ptr<PeerImp> weak = shared_from_this();
    auto elapsed = UptimeClock::now();
    auto const pap = &app_;
    app_.getJobQueue().addJob(
        jtPACK, "MakeFetchPack", [pap, weak, packet, hash, elapsed]() {
            pap->getLedgerMaster().makeFetchPack(weak, packet, hash, elapsed);
        });
}

// Returns the set of peers that can help us get
// the TX tree with the specified root hash.
//
static std::shared_ptr<PeerImp>
getPeerWithTree(OverlayImpl& ov, uint256 const& rootHash, PeerImp const* skip)
{
    std::shared_ptr<PeerImp> ret;
    int retScore = 0;

    ov.for_each([&](std::shared_ptr<PeerImp>&& p) {
        if (p->hasTxSet(rootHash) && p.get() != skip)
        {
            auto score = p->getScore(true);
            if (!ret || (score > retScore))
            {
                ret = std::move(p);
                retScore = score;
            }
        }
    });

    return ret;
}

// Returns a random peer weighted by how likely to
// have the ledger and how responsive it is.
//
static std::shared_ptr<PeerImp>
getPeerWithLedger(
    OverlayImpl& ov,
    uint256 const& ledgerHash,
    LedgerIndex ledger,
    PeerImp const* skip)
{
    std::shared_ptr<PeerImp> ret;
    int retScore = 0;

    ov.for_each([&](std::shared_ptr<PeerImp>&& p) {
        if (p->hasLedger(ledgerHash, ledger) && p.get() != skip)
        {
            auto score = p->getScore(true);
            if (!ret || (score > retScore))
            {
                ret = std::move(p);
                retScore = score;
            }
        }
    });

    return ret;
}

void
PeerImp::sendLedgerBase(
    std::shared_ptr<Ledger const> const& ledger,
    protocol::TMLedgerData& ledgerData)
{
    JLOG(p_journal_.trace()) << "sendLedgerBase: Base data";

    Serializer s(sizeof(LedgerHeader));
    addRaw(ledger->header(), s);
    ledgerData.add_nodes()->set_nodedata(s.getDataPtr(), s.getLength());

    auto const& stateMap{ledger->stateMap()};
    if (stateMap.getHash() != beast::zero)
    {
        // Return account state root node if possible
        Serializer root(768);

        stateMap.serializeRoot(root);
        ledgerData.add_nodes()->set_nodedata(
            root.getDataPtr(), root.getLength());

        if (ledger->header().txHash != beast::zero)
        {
            auto const& txMap{ledger->txMap()};
            if (txMap.getHash() != beast::zero)
            {
                // Return TX root node if possible
                root.erase();
                txMap.serializeRoot(root);
                ledgerData.add_nodes()->set_nodedata(
                    root.getDataPtr(), root.getLength());
            }
        }
    }

    auto message{
        std::make_shared<Message>(ledgerData, protocol::mtLEDGER_DATA)};
    send(message);
}

std::shared_ptr<Ledger const>
PeerImp::getLedger(std::shared_ptr<protocol::TMGetLedger> const& m)
{
    JLOG(p_journal_.trace()) << "getLedger: Ledger";

    std::shared_ptr<Ledger const> ledger;

    if (m->has_ledgerhash())
    {
        // Attempt to find ledger by hash
        uint256 const ledgerHash{m->ledgerhash()};
        ledger = app_.getLedgerMaster().getLedgerByHash(ledgerHash);
        if (!ledger)
        {
            JLOG(p_journal_.trace())
                << "getLedger: Don't have ledger with hash " << ledgerHash;

            if (m->has_querytype() && !m->has_requestcookie())
            {
                // Attempt to relay the request to a peer
                if (auto const peer = getPeerWithLedger(
                        overlay_,
                        ledgerHash,
                        m->has_ledgerseq() ? m->ledgerseq() : 0,
                        this))
                {
                    m->set_requestcookie(id());
                    peer->send(
                        std::make_shared<Message>(*m, protocol::mtGET_LEDGER));
                    JLOG(p_journal_.debug())
                        << "getLedger: Request relayed to peer";
                    return ledger;
                }

                JLOG(p_journal_.trace())
                    << "getLedger: Failed to find peer to relay request";
            }
        }
    }
    else if (m->has_ledgerseq())
    {
        // Attempt to find ledger by sequence
        if (m->ledgerseq() < app_.getLedgerMaster().getEarliestFetch())
        {
            JLOG(p_journal_.debug())
                << "getLedger: Early ledger sequence request";
        }
        else
        {
            ledger = app_.getLedgerMaster().getLedgerBySeq(m->ledgerseq());
            if (!ledger)
            {
                JLOG(p_journal_.debug())
                    << "getLedger: Don't have ledger with sequence "
                    << m->ledgerseq();
            }
        }
    }
    else if (m->has_ltype() && m->ltype() == protocol::ltCLOSED)
    {
        ledger = app_.getLedgerMaster().getClosedLedger();
    }

    if (ledger)
    {
        // Validate retrieved ledger sequence
        auto const ledgerSeq{ledger->header().seq};
        if (m->has_ledgerseq())
        {
            if (ledgerSeq != m->ledgerseq())
            {
                // Do not resource charge a peer responding to a relay
                if (!m->has_requestcookie())
                    charge(
                        Resource::feeMalformedRequest, "get_ledger ledgerSeq");

                ledger.reset();
                JLOG(p_journal_.warn())
                    << "getLedger: Invalid ledger sequence " << ledgerSeq;
            }
        }
        else if (ledgerSeq < app_.getLedgerMaster().getEarliestFetch())
        {
            ledger.reset();
            JLOG(p_journal_.debug())
                << "getLedger: Early ledger sequence request " << ledgerSeq;
        }
    }
    else
    {
        JLOG(p_journal_.debug()) << "getLedger: Unable to find ledger";
    }

    return ledger;
}

std::shared_ptr<SHAMap const>
PeerImp::getTxSet(std::shared_ptr<protocol::TMGetLedger> const& m) const
{
    JLOG(p_journal_.trace()) << "getTxSet: TX set";

    uint256 const txSetHash{m->ledgerhash()};
    std::shared_ptr<SHAMap> shaMap{
        app_.getInboundTransactions().getSet(txSetHash, false)};
    if (!shaMap)
    {
        if (m->has_querytype() && !m->has_requestcookie())
        {
            // Attempt to relay the request to a peer
            if (auto const peer = getPeerWithTree(overlay_, txSetHash, this))
            {
                m->set_requestcookie(id());
                peer->send(
                    std::make_shared<Message>(*m, protocol::mtGET_LEDGER));
                JLOG(p_journal_.debug()) << "getTxSet: Request relayed";
            }
            else
            {
                JLOG(p_journal_.debug())
                    << "getTxSet: Failed to find relay peer";
            }
        }
        else
        {
            JLOG(p_journal_.debug()) << "getTxSet: Failed to find TX set";
        }
    }

    return shaMap;
}

void
PeerImp::processLedgerRequest(std::shared_ptr<protocol::TMGetLedger> const& m)
{
    // Do not resource charge a peer responding to a relay
    if (!m->has_requestcookie())
        charge(
            Resource::feeModerateBurdenPeer, "received a get ledger request");

    std::shared_ptr<Ledger const> ledger;
    std::shared_ptr<SHAMap const> sharedMap;
    SHAMap const* map{nullptr};
    protocol::TMLedgerData ledgerData;
    bool fatLeaves{true};
    auto const itype{m->itype()};

    if (itype == protocol::liTS_CANDIDATE)
    {
        if (sharedMap = getTxSet(m); !sharedMap)
            return;
        map = sharedMap.get();

        // Fill out the reply
        ledgerData.set_ledgerseq(0);
        ledgerData.set_ledgerhash(m->ledgerhash());
        ledgerData.set_type(protocol::liTS_CANDIDATE);
        if (m->has_requestcookie())
            ledgerData.set_requestcookie(m->requestcookie());

        // We'll already have most transactions
        fatLeaves = false;
    }
    else
    {
        if (send_queue_.size() >= Tuning::dropSendQueue)
        {
            JLOG(p_journal_.debug())
                << "processLedgerRequest: Large send queue";
            return;
        }
        if (app_.getFeeTrack().isLoadedLocal() && !cluster())
        {
            JLOG(p_journal_.debug()) << "processLedgerRequest: Too busy";
            return;
        }

        if (ledger = getLedger(m); !ledger)
            return;

        // Fill out the reply
        auto const ledgerHash{ledger->header().hash};
        ledgerData.set_ledgerhash(ledgerHash.begin(), ledgerHash.size());
        ledgerData.set_ledgerseq(ledger->header().seq);
        ledgerData.set_type(itype);
        if (m->has_requestcookie())
            ledgerData.set_requestcookie(m->requestcookie());

        switch (itype)
        {
            case protocol::liBASE:
                sendLedgerBase(ledger, ledgerData);
                return;

            case protocol::liTX_NODE:
                map = &ledger->txMap();
                JLOG(p_journal_.trace()) << "processLedgerRequest: TX map hash "
                                         << to_string(map->getHash());
                break;

            case protocol::liAS_NODE:
                map = &ledger->stateMap();
                JLOG(p_journal_.trace())
                    << "processLedgerRequest: Account state map hash "
                    << to_string(map->getHash());
                break;

            default:
                // This case should not be possible here
                JLOG(p_journal_.error())
                    << "processLedgerRequest: Invalid ledger info type";
                return;
        }
    }

    if (!map)
    {
        JLOG(p_journal_.warn()) << "processLedgerRequest: Unable to find map";
        return;
    }

    // Add requested node data to reply
    if (m->nodeids_size() > 0)
    {
        auto const queryDepth{
            m->has_querydepth() ? m->querydepth() : (isHighLatency() ? 2 : 1)};

        std::vector<std::pair<SHAMapNodeID, Blob>> data;

        for (int i = 0; i < m->nodeids_size() &&
             ledgerData.nodes_size() < Tuning::softMaxReplyNodes;
             ++i)
        {
            auto const shaMapNodeId{deserializeSHAMapNodeID(m->nodeids(i))};

            data.clear();
            data.reserve(Tuning::softMaxReplyNodes);

            try
            {
                if (map->getNodeFat(*shaMapNodeId, data, fatLeaves, queryDepth))
                {
                    JLOG(p_journal_.trace())
                        << "processLedgerRequest: getNodeFat got "
                        << data.size() << " nodes";

                    for (auto const& d : data)
                    {
                        if (ledgerData.nodes_size() >=
                            Tuning::hardMaxReplyNodes)
                            break;
                        protocol::TMLedgerNode* node{ledgerData.add_nodes()};
                        node->set_nodeid(d.first.getRawString());
                        node->set_nodedata(d.second.data(), d.second.size());
                    }
                }
                else
                {
                    JLOG(p_journal_.warn())
                        << "processLedgerRequest: getNodeFat returns false";
                }
            }
            catch (std::exception const& e)
            {
                std::string info;
                switch (itype)
                {
                    case protocol::liBASE:
                        // This case should not be possible here
                        info = "Ledger base";
                        break;

                    case protocol::liTX_NODE:
                        info = "TX node";
                        break;

                    case protocol::liAS_NODE:
                        info = "AS node";
                        break;

                    case protocol::liTS_CANDIDATE:
                        info = "TS candidate";
                        break;

                    default:
                        info = "Invalid";
                        break;
                }

                if (!m->has_ledgerhash())
                    info += ", no hash specified";

                JLOG(p_journal_.warn())
                    << "processLedgerRequest: getNodeFat with nodeId "
                    << *shaMapNodeId << " and ledger info type " << info
                    << " throws exception: " << e.what();
            }
        }

        JLOG(p_journal_.info())
            << "processLedgerRequest: Got request for " << m->nodeids_size()
            << " nodes at depth " << queryDepth << ", return "
            << ledgerData.nodes_size() << " nodes";
    }

    if (ledgerData.nodes_size() == 0)
        return;

    send(std::make_shared<Message>(ledgerData, protocol::mtLEDGER_DATA));
}

int
PeerImp::getScore(bool haveItem) const
{
    // Random component of score, used to break ties and avoid
    // overloading the "best" peer
    static int const spRandomMax = 9999;

    // Score for being very likely to have the thing we are
    // look for; should be roughly spRandomMax
    static int const spHaveItem = 10000;

    // Score reduction for each millisecond of latency; should
    // be roughly spRandomMax divided by the maximum reasonable
    // latency
    static int const spLatency = 30;

    // Penalty for unknown latency; should be roughly spRandomMax
    static int const spNoLatency = 8000;

    int score = rand_int(spRandomMax);

    if (haveItem)
        score += spHaveItem;

    std::optional<std::chrono::milliseconds> latency;
    {
        std::lock_guard sl(recentLock_);
        latency = latency_;
    }

    if (latency)
        score -= latency->count() * spLatency;
    else
        score -= spNoLatency;

    return score;
}

bool
PeerImp::isHighLatency() const
{
    std::lock_guard sl(recentLock_);
    return latency_ >= peerHighLatency;
}

void
PeerImp::Metrics::add_message(std::uint64_t bytes)
{
    using namespace std::chrono_literals;
    std::unique_lock lock{mutex_};

    totalBytes_ += bytes;
    accumBytes_ += bytes;
    auto const timeElapsed = clock_type::now() - intervalStart_;
    auto const timeElapsedInSecs =
        std::chrono::duration_cast<std::chrono::seconds>(timeElapsed);

    if (timeElapsedInSecs >= 1s)
    {
        auto const avgBytes = accumBytes_ / timeElapsedInSecs.count();
        rollingAvg_.push_back(avgBytes);

        auto const totalBytes =
            std::accumulate(rollingAvg_.begin(), rollingAvg_.end(), 0ull);
        rollingAvgBytes_ = totalBytes / rollingAvg_.size();

        intervalStart_ = clock_type::now();
        accumBytes_ = 0;
    }
}

std::uint64_t
PeerImp::Metrics::average_bytes() const
{
    std::shared_lock lock{mutex_};
    return rollingAvgBytes_;
}

std::uint64_t
PeerImp::Metrics::total_bytes() const
{
    std::shared_lock lock{mutex_};
    return totalBytes_;
}

//------------------------------------------------------------------------------
// Template constructor implementation for outgoing peers.
// This is defined in the .cpp file with explicit instantiation to avoid
// requiring LedgerReplayMsgHandler definition in the header.
//------------------------------------------------------------------------------

template <class Buffers>
PeerImp::PeerImp(
    Application& app,
    std::unique_ptr<stream_type>&& stream_ptr,
    Buffers const& buffers,
    std::shared_ptr<PeerFinder::Slot>&& slot,
    http_response_type&& response,
    Resource::Consumer usage,
    PublicKey const& publicKey,
    ProtocolVersion protocol,
    id_t id,
    OverlayImpl& overlay)
    : Child(overlay)
    , app_(app)
    , id_(id)
    , fingerprint_(
          getFingerprint(slot->remote_endpoint(), publicKey, to_string(id_)))
    , prefix_(makePrefix(fingerprint_))
    , sink_(app_.journal("Peer"), prefix_)
    , p_sink_(app_.journal("Protocol"), prefix_)
    , journal_(sink_)
    , p_journal_(p_sink_)
    , stream_ptr_(std::move(stream_ptr))
    , socket_(stream_ptr_->next_layer().socket())
    , stream_(*stream_ptr_)
    , strand_(boost::asio::make_strand(socket_.get_executor()))
    , timer_(waitable_timer{socket_.get_executor()})
    , remote_address_(slot->remote_endpoint())
    , overlay_(overlay)
    , inbound_(false)
    , protocol_(protocol)
    , tracking_(Tracking::unknown)
    , trackingTime_(clock_type::now())
    , publicKey_(publicKey)
    , lastPingTime_(clock_type::now())
    , creationTime_(clock_type::now())
    , squelch_(app_.journal("Squelch"))
    , usage_(usage)
    , fee_{Resource::feeTrivialPeer}
    , slot_(std::move(slot))
    , response_(std::move(response))
    , headers_(response_)
    , compressionEnabled_(
          peerFeatureEnabled(
              headers_,
              FEATURE_COMPR,
              "lz4",
              app_.config().COMPRESSION)
              ? Compressed::On
              : Compressed::Off)
    , txReduceRelayEnabled_(peerFeatureEnabled(
          headers_,
          FEATURE_TXRR,
          app_.config().TX_REDUCE_RELAY_ENABLE))
    , ledgerReplayEnabled_(peerFeatureEnabled(
          headers_,
          FEATURE_LEDGER_REPLAY,
          app_.config().LEDGER_REPLAY))
    , ledgerReplayMsgHandler_(
          std::make_unique<LedgerReplayMsgHandler>(app, app.getLedgerReplayer()))
{
    read_buffer_.commit(boost::asio::buffer_copy(
        read_buffer_.prepare(boost::asio::buffer_size(buffers)), buffers));
    JLOG(journal_.info())
        << "compression enabled " << (compressionEnabled_ == Compressed::On)
        << " vp reduce-relay base squelch enabled "
        << peerFeatureEnabled(
               headers_,
               FEATURE_VPRR,
               app_.config().VP_REDUCE_RELAY_BASE_SQUELCH_ENABLE)
        << " tx reduce-relay enabled " << txReduceRelayEnabled_ << " on "
        << remote_address_ << " " << id_;
}

// Explicit template instantiation for the type used by ConnectAttempt.cpp
// read_buf_.data() returns subrange<true> from boost::beast::multi_buffer
template PeerImp::PeerImp(
    Application&,
    std::unique_ptr<stream_type>&&,
    boost::beast::basic_multi_buffer<std::allocator<char>>::subrange<true>
        const&,
    std::shared_ptr<PeerFinder::Slot>&&,
    http_response_type&&,
    Resource::Consumer,
    PublicKey const&,
    ProtocolVersion,
    id_t,
    OverlayImpl&);

}  // namespace xrpl
