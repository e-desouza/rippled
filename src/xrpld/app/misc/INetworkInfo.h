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

#ifndef XRPL_APP_MISC_INETWORKINFO_H_INCLUDED
#define XRPL_APP_MISC_INETWORKINFO_H_INCLUDED

#include <xrpl/json/json_value.h>
#include <xrpl/ledger/ReadView.h>
#include <xrpl/protocol/AccountID.h>
#include <xrpl/protocol/Book.h>

#include <memory>

namespace xrpl {

/**
 * @brief Interface for network information retrieval.
 *
 * This interface encapsulates the network information portion of the former
 * NetworkOPs god object. It provides methods for retrieving server status,
 * ledger fetch information, owner information, and order book data.
 *
 * Thread-safe: All implementations must be thread-safe.
 *
 * @see NetworkOPs (legacy facade)
 */
class INetworkInfo
{
public:
    virtual ~INetworkInfo() = default;

    //--------------------------------------------------------------------------
    // Server Information
    //--------------------------------------------------------------------------

    /**
     * @brief Get comprehensive server information.
     *
     * Returns a JSON object containing server status, configuration,
     * and runtime information.
     *
     * @param human If true, format output for human readability.
     * @param admin If true, include admin-level details.
     * @param counters If true, include performance counters.
     * @return JSON object with server information.
     */
    virtual Json::Value
    getServerInfo(bool human, bool admin, bool counters) = 0;

    //--------------------------------------------------------------------------
    // Ledger Fetch Information
    //--------------------------------------------------------------------------

    /**
     * @brief Clear ledger fetch statistics.
     *
     * Resets any accumulated ledger fetch metrics and state.
     */
    virtual void
    clearLedgerFetch() = 0;

    /**
     * @brief Get ledger fetch information.
     *
     * Returns a JSON object containing statistics and status about
     * ongoing or recent ledger fetch operations.
     *
     * @return JSON object with ledger fetch information.
     */
    virtual Json::Value
    getLedgerFetchInfo() = 0;

    //--------------------------------------------------------------------------
    // Owner Information
    //--------------------------------------------------------------------------

    /**
     * @brief Get owner information for an account.
     *
     * Returns details about objects owned by the specified account
     * in the given ledger view.
     *
     * @param lpLedger The ledger view to query.
     * @param account The account to get owner information for.
     * @return JSON object with owner information.
     */
    virtual Json::Value
    getOwnerInfo(
        std::shared_ptr<ReadView const> lpLedger,
        AccountID const& account) = 0;

    //--------------------------------------------------------------------------
    // Order Book Information
    //--------------------------------------------------------------------------

    /**
     * @brief Get a page of order book data.
     *
     * Retrieves offers from the order book matching the specified
     * currency pair, with pagination support.
     *
     * @param lpLedger The ledger view to query (may be updated).
     * @param book The order book (currency pair) to query.
     * @param uTakerID The account ID of the taker for quality calculations.
     * @param bProof If true, include proof information.
     * @param iLimit Maximum number of offers to return.
     * @param jvMarker Pagination marker from previous call.
     * @param jvResult Output parameter for the result JSON.
     */
    virtual void
    getBookPage(
        std::shared_ptr<ReadView const>& lpLedger,
        Book const& book,
        AccountID const& uTakerID,
        bool const bProof,
        unsigned int iLimit,
        Json::Value const& jvMarker,
        Json::Value& jvResult) = 0;
};

}  // namespace xrpl

#endif
