//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

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

#ifndef XRPL_APP_PATHS_PATHTUNING_H_INCLUDED
#define XRPL_APP_PATHS_PATHTUNING_H_INCLUDED

namespace xrpl {

/** Path-finding tuning constants.
 *
 * These constants are used by the path-finding subsystem in the app module.
 * They were extracted from rpc/detail/Tuning.h to break the app->rpc
 * dependency cycle.
 */
namespace PathTuning {

/** Maximum number of source currencies allowed in a path find request. */
static int constexpr maxSourceCurrencies = 18;

/** Maximum number of auto source currencies in a path find request. */
static int constexpr maxAutoSourceCurrencies = 88;

}  // namespace PathTuning

}  // namespace xrpl

#endif

