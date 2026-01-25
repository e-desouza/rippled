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

#ifndef XRPLD_CONSENSUS_DETAIL_MESSAGEROUTERIMPL_H_INCLUDED
#define XRPLD_CONSENSUS_DETAIL_MESSAGEROUTERIMPL_H_INCLUDED

#include <xrpld/consensus/IMessageRouter.h>

namespace xrpl {

class Application;
class HashRouter;

/**
 * Implementation of IMessageRouter that delegates to HashRouter.
 */
class MessageRouterImpl final : public IMessageRouter
{
public:
    explicit MessageRouterImpl(Application& app);

    bool
    shouldRelay(uint256 const& hash) override;

    void
    addSuppression(uint256 const& hash) override;

private:
    Application& app_;
    HashRouter& hashRouter_;
};

}  // namespace xrpl

#endif
