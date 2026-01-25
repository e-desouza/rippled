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

#include <xrpld/app/main/Application.h>
#include <xrpld/consensus/detail/OverlayBroadcasterImpl.h>
#include <xrpld/overlay/Overlay.h>

namespace xrpl {

OverlayBroadcasterImpl::OverlayBroadcasterImpl(Application& app)
    : app_(app), overlay_(app.overlay())
{
}

void
OverlayBroadcasterImpl::broadcast(protocol::TMProposeSet const& m)
{
    // Overlay::broadcast takes non-const reference, so we need to make a copy
    protocol::TMProposeSet copy = m;
    overlay_.broadcast(copy);
}

void
OverlayBroadcasterImpl::broadcast(protocol::TMValidation const& m)
{
    // Overlay::broadcast takes non-const reference, so we need to make a copy
    protocol::TMValidation copy = m;
    overlay_.broadcast(copy);
}

std::set<PeerId>
OverlayBroadcasterImpl::relay(
    protocol::TMProposeSet const& m,
    uint256 const& suppression,
    PublicKey const& validator)
{
    // Overlay::relay signature:
    //   relay(TMProposeSet& m, uint256 const& uid, PublicKey const& validator)
    // Returns set of peers that already have this proposal
    protocol::TMProposeSet copy = m;
    return overlay_.relay(copy, suppression, validator);
}

void
OverlayBroadcasterImpl::relay(
    uint256 const& hash,
    protocol::TMTransaction const& m,
    std::set<PeerId> const& skip)
{
    // Overlay::relay for transactions has signature:
    //   relay(uint256 const& hash,
    //         std::optional<std::reference_wrapper<TMTransaction>> m,
    //         std::set<Peer::id_t> const& toSkip)
    protocol::TMTransaction copy = m;
    overlay_.relay(hash, std::ref(copy), skip);
}

void
OverlayBroadcasterImpl::foreach(
    std::function<void(std::shared_ptr<ripple::Peer> const&)> f)
{
    overlay_.foreach(std::move(f));
}

}  // namespace xrpl
