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

#ifndef XRPL_CORE_OPERATINGMODE_H_INCLUDED
#define XRPL_CORE_OPERATINGMODE_H_INCLUDED

namespace xrpl {

/** Specifies the mode under which the server believes it's operating.

    This has implications about how the server processes transactions and
    how it responds to requests (e.g. account balance request).

    @note Other code relies on the numerical values of these constants; do
          not change them without verifying each use and ensuring that it is
          not a breaking change.
*/
enum class OperatingMode {
    DISCONNECTED = 0,  //!< not ready to process requests
    CONNECTED = 1,     //!< convinced we are talking to the network
    SYNCING = 2,       //!< fallen slightly behind
    TRACKING = 3,      //!< convinced we agree with the network
    FULL = 4           //!< we have the ledger and can even validate
};

}  // namespace xrpl

#endif

