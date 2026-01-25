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

#ifndef XRPLD_CONSENSUS_DETAIL_OPERATINGMODEIMPL_H_INCLUDED
#define XRPLD_CONSENSUS_DETAIL_OPERATINGMODEIMPL_H_INCLUDED

#include <xrpld/consensus/IOperatingMode.h>

namespace xrpl {

class Application;
class NetworkOPs;

/**
 * Implementation of IOperatingMode that delegates to NetworkOPs.
 */
class OperatingModeImpl final : public IOperatingMode
{
public:
    explicit OperatingModeImpl(Application& app);

    OperatingMode
    getOperatingMode() const override;

    bool
    isFull() const override;

    bool
    isBlocked() const override;

    void
    setMode(OperatingMode mode) override;

    void
    consensusViewChange() override;

    void
    endConsensus(std::unique_ptr<std::stringstream> const& clog) override;

    void
    reportFeeChange() override;

    void
    pubValidation(std::shared_ptr<STValidation> const& val) override;

private:
    Application& app_;
    NetworkOPs& networkOPs_;
};

}  // namespace xrpl

#endif
