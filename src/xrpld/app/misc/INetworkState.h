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

#ifndef XRPL_APP_MISC_INETWORKSTATE_H_INCLUDED
#define XRPL_APP_MISC_INETWORKSTATE_H_INCLUDED

#include <xrpld/app/misc/NetworkOPs.h>  // For OperatingMode enum

#include <string>

namespace xrpl {

/**
 * @brief Interface for network operating state management.
 *
 * This interface encapsulates the network state portion of the former
 * NetworkOPs god object. It handles operating mode management, amendment
 * blocking states, and network ledger state flags.
 *
 * Thread-safe: All implementations must be thread-safe. State changes
 * use atomic operations internally.
 *
 * @see NetworkOPs (legacy facade)
 * @see OperatingMode enum
 */
class INetworkState
{
public:
    virtual ~INetworkState() = default;

    //--------------------------------------------------------------------------
    // Operating Mode
    //--------------------------------------------------------------------------

    /** Get the current operating mode of the server. */
    virtual OperatingMode
    getOperatingMode() const = 0;

    /** Convert an operating mode to a human-readable string.
     *  @param mode The operating mode to convert.
     *  @param admin If true, include admin-level details.
     *  @return A string description of the mode.
     */
    virtual std::string
    strOperatingMode(OperatingMode mode, bool admin) const = 0;

    /** Get string representation of current operating mode.
     *  @param admin If true, include admin-level details.
     *  @return A string description of the current mode.
     */
    virtual std::string
    strOperatingMode(bool admin = false) const = 0;

    /** Set the operating mode. */
    virtual void
    setMode(OperatingMode om) = 0;

    /** Check if the server is in FULL mode. */
    virtual bool
    isFull() = 0;

    /** Check if the server is blocked (amendment or UNL). */
    virtual bool
    isBlocked() = 0;

    //--------------------------------------------------------------------------
    // Amendment State
    //--------------------------------------------------------------------------

    /** Check if the server is blocked due to an unsupported amendment. */
    virtual bool
    isAmendmentBlocked() = 0;

    /** Mark the server as amendment-blocked. */
    virtual void
    setAmendmentBlocked() = 0;

    /** Check if an amendment warning has been issued. */
    virtual bool
    isAmendmentWarned() = 0;

    /** Set the amendment warning flag. */
    virtual void
    setAmendmentWarned() = 0;

    /** Clear the amendment warning flag. */
    virtual void
    clearAmendmentWarned() = 0;

    //--------------------------------------------------------------------------
    // UNL State
    //--------------------------------------------------------------------------

    /** Check if the server is blocked due to UNL issues. */
    virtual bool
    isUNLBlocked() = 0;

    /** Mark the server as UNL-blocked. */
    virtual void
    setUNLBlocked() = 0;

    /** Clear the UNL-blocked state. */
    virtual void
    clearUNLBlocked() = 0;

    //--------------------------------------------------------------------------
    // Network Ledger State
    //--------------------------------------------------------------------------

    /** Check if the server needs a network ledger to sync. */
    virtual bool
    isNeedNetworkLedger() = 0;

    /** Mark that the server needs a network ledger. */
    virtual void
    setNeedNetworkLedger() = 0;

    /** Clear the need-network-ledger flag. */
    virtual void
    clearNeedNetworkLedger() = 0;

    /** Configure the server for standalone operation. */
    virtual void
    setStandAlone() = 0;
};

}  // namespace xrpl

#endif
