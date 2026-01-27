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

#ifndef XRPL_CORE_FAILHARD_H_INCLUDED
#define XRPL_CORE_FAILHARD_H_INCLUDED

namespace xrpl {

/** Specifies whether a transaction submission should fail hard.

    When FailHard::yes, the transaction will not be retried or held
    for later submission if it fails initially.
*/
enum class FailHard : unsigned char { no, yes };

/** Convert a boolean to FailHard enum.
    @param noMeansDont If true, returns FailHard::yes
*/
inline FailHard
doFailHard(bool noMeansDont)
{
    return noMeansDont ? FailHard::yes : FailHard::no;
}

}  // namespace xrpl

#endif
