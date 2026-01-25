//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012-2026 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef XRPLD_CONSENSUS_DETAIL_OVERLAYBROADCASTERIMPL_H_INCLUDED
#define XRPLD_CONSENSUS_DETAIL_OVERLAYBROADCASTERIMPL_H_INCLUDED

#include <xrpld/consensus/IOverlayBroadcaster.h>

namespace xrpl {

class Application;
class Overlay;

/**
 * Implementation of IOverlayBroadcaster that delegates to Overlay.
 */
class OverlayBroadcasterImpl final : public IOverlayBroadcaster
{
public:
    explicit OverlayBroadcasterImpl(Application& app);

    void
    broadcast(protocol::TMProposeSet const& m) override;

    void
    broadcast(protocol::TMValidation const& m) override;

    std::set<PeerId>
    relay(
        protocol::TMProposeSet const& m,
        uint256 const& suppression,
        PublicKey const& validator) override;

    void
    relay(
        uint256 const& hash,
        protocol::TMTransaction const& m,
        std::set<PeerId> const& skip) override;

    void
    foreach(std::function<void(std::shared_ptr<Peer> const&)> f) override;

private:
    Application& app_;
    Overlay& overlay_;
};

}  // namespace xrpl

#endif
