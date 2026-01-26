#include <xrpld/overlay/Overlay.h>
#include <xrpld/overlay/PeerSet.h>

#include <xrpl/basics/Log.h>

namespace xrpl {

class PeerSetImpl : public PeerSet
{
public:
    explicit PeerSetImpl(Overlay& overlay);

    void
    addPeers(
        std::size_t limit,
        std::function<bool(std::shared_ptr<Peer> const&)> hasItem,
        std::function<void(std::shared_ptr<Peer> const&)> onPeerAdded) override;

    /** Send a message to one or all peers. */
    void
    sendRequest(
        ::google::protobuf::Message const& message,
        protocol::MessageType type,
        std::shared_ptr<Peer> const& peer) override;

    std::set<Peer::id_t> const&
    getPeerIds() const override;

private:
    Overlay& overlay_;

    /** The identifiers of the peers we are tracking. */
    std::set<Peer::id_t> peers_;
};

PeerSetImpl::PeerSetImpl(Overlay& overlay) : overlay_(overlay)
{
}

void
PeerSetImpl::addPeers(
    std::size_t limit,
    std::function<bool(std::shared_ptr<Peer> const&)> hasItem,
    std::function<void(std::shared_ptr<Peer> const&)> onPeerAdded)
{
    using ScoredPeer = std::pair<int, std::shared_ptr<Peer>>;

    std::vector<ScoredPeer> pairs;
    pairs.reserve(overlay_.size());

    overlay_.foreach([&](auto const& peer) {
        auto const score = peer->getScore(hasItem(peer));
        pairs.emplace_back(score, std::move(peer));
    });

    std::sort(
        pairs.begin(),
        pairs.end(),
        [](ScoredPeer const& lhs, ScoredPeer const& rhs) {
            return lhs.first > rhs.first;
        });

    std::size_t accepted = 0;
    for (auto const& pair : pairs)
    {
        auto const peer = pair.second;
        if (!peers_.insert(peer->id()).second)
            continue;
        onPeerAdded(peer);
        if (++accepted >= limit)
            break;
    }
}

void
PeerSetImpl::sendRequest(
    ::google::protobuf::Message const& message,
    protocol::MessageType type,
    std::shared_ptr<Peer> const& peer)
{
    auto packet = std::make_shared<Message>(message, type);
    if (peer)
    {
        peer->send(packet);
        return;
    }

    for (auto id : peers_)
    {
        if (auto p = overlay_.findPeerByShortID(id))
            p->send(packet);
    }
}

std::set<Peer::id_t> const&
PeerSetImpl::getPeerIds() const
{
    return peers_;
}

class PeerSetBuilderImpl : public PeerSetBuilder
{
public:
    explicit PeerSetBuilderImpl(Overlay& overlay) : overlay_(&overlay)
    {
    }

    virtual std::unique_ptr<PeerSet>
    build() override
    {
        return std::make_unique<PeerSetImpl>(*overlay_);
    }

private:
    Overlay* overlay_;
};

class LazyPeerSetBuilderImpl : public PeerSetBuilder
{
public:
    explicit LazyPeerSetBuilderImpl(IOverlayProvider& provider)
        : provider_(provider)
    {
    }

    virtual std::unique_ptr<PeerSet>
    build() override
    {
        return std::make_unique<PeerSetImpl>(provider_.getOverlay());
    }

private:
    IOverlayProvider& provider_;
};

std::unique_ptr<PeerSetBuilder>
make_PeerSetBuilder(IOverlayProvider& provider)
{
    return std::make_unique<LazyPeerSetBuilderImpl>(provider);
}

std::unique_ptr<PeerSetBuilder>
make_PeerSetBuilder(Overlay& overlay)
{
    return std::make_unique<PeerSetBuilderImpl>(overlay);
}

class DummyPeerSet : public PeerSet
{
public:
    DummyPeerSet(beast::Journal j) : j_(j)
    {
    }

    void
    addPeers(
        std::size_t limit,
        std::function<bool(std::shared_ptr<Peer> const&)> hasItem,
        std::function<void(std::shared_ptr<Peer> const&)> onPeerAdded) override
    {
        JLOG(j_.error()) << "DummyPeerSet addPeers should not be called";
    }

    void
    sendRequest(
        ::google::protobuf::Message const& message,
        protocol::MessageType type,
        std::shared_ptr<Peer> const& peer) override
    {
        JLOG(j_.error()) << "DummyPeerSet sendRequest should not be called";
    }

    std::set<Peer::id_t> const&
    getPeerIds() const override
    {
        static std::set<Peer::id_t> emptyPeers;
        JLOG(j_.error()) << "DummyPeerSet getPeerIds should not be called";
        return emptyPeers;
    }

private:
    beast::Journal j_;
};

std::unique_ptr<PeerSet>
make_DummyPeerSet(Logs& logs)
{
    return std::make_unique<DummyPeerSet>(logs.journal("DummyPeerSet"));
}

}  // namespace xrpl
