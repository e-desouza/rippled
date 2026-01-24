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

#ifndef XRPL_APP_MISC_NETWORKINFOIMPL_H_INCLUDED
#define XRPL_APP_MISC_NETWORKINFOIMPL_H_INCLUDED

#include <xrpld/app/misc/INetworkInfo.h>

namespace xrpl {

class Application;

/**
 * @brief Implementation of the INetworkInfo interface.
 *
 * This class provides network information retrieval functionality,
 * including server status, ledger fetch information, owner information,
 * and order book data.
 *
 * This is a stub implementation that will be completed when wiring
 * together the NetworkOPs split components.
 *
 * Thread-safe: All methods are thread-safe.
 */
class NetworkInfoImpl final : public INetworkInfo
{
public:
    /**
     * @brief Construct a NetworkInfoImpl.
     *
     * @param app Reference to the Application instance.
     */
    explicit NetworkInfoImpl(Application& app);

    ~NetworkInfoImpl() override = default;

    // Non-copyable, non-movable
    NetworkInfoImpl(NetworkInfoImpl const&) = delete;
    NetworkInfoImpl&
    operator=(NetworkInfoImpl const&) = delete;
    NetworkInfoImpl(NetworkInfoImpl&&) = delete;
    NetworkInfoImpl&
    operator=(NetworkInfoImpl&&) = delete;

    //--------------------------------------------------------------------------
    // INetworkInfo interface implementation
    //--------------------------------------------------------------------------

    Json::Value
    getServerInfo(bool human, bool admin, bool counters) override;

    void
    clearLedgerFetch() override;

    Json::Value
    getLedgerFetchInfo() override;

    Json::Value
    getOwnerInfo(
        std::shared_ptr<ReadView const> lpLedger,
        AccountID const& account) override;

    void
    getBookPage(
        std::shared_ptr<ReadView const>& lpLedger,
        Book const& book,
        AccountID const& uTakerID,
        bool const bProof,
        unsigned int iLimit,
        Json::Value const& jvMarker,
        Json::Value& jvResult) override;

private:
    Application& app_;
};

}  // namespace xrpl

#endif
