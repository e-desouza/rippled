//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012-2024 Ripple Labs Inc.

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
#include <xrpld/app/txqueue/HashRouter.h>
#include <xrpld/consensus/detail/MessageRouterImpl.h>

namespace xrpl {

MessageRouterImpl::MessageRouterImpl(Application& app)
    : app_(app), hashRouter_(app.getHashRouter())
{
}

bool
MessageRouterImpl::shouldRelay(uint256 const& hash)
{
    // HashRouter::shouldRelay returns std::optional<std::set<PeerShortID>>
    // If the optional has a value, we should relay; otherwise we shouldn't.
    return hashRouter_.shouldRelay(hash).has_value();
}

void
MessageRouterImpl::addSuppression(uint256 const& hash)
{
    hashRouter_.addSuppression(hash);
}

}  // namespace xrpl
