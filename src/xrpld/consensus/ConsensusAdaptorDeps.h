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

#ifndef XRPL_CONSENSUS_CONSENSUSADAPTORDEPS_H_INCLUDED
#define XRPL_CONSENSUS_CONSENSUSADAPTORDEPS_H_INCLUDED

namespace xrpl {

// Forward declarations
class ILedgerProvider;
class IOverlayBroadcaster;
class IConsensusJobScheduler;
class ITxSetManager;
class IMessageRouter;
class IConsensusTimeSource;
class IValidationTracker;
class IOperatingMode;

/**
 * @brief Aggregates the interface dependencies for the consensus adaptor.
 *
 * This struct provides a single point of dependency injection for the
 * RCLConsensus::Adaptor class. It allows the adaptor to be constructed
 * with focused interfaces rather than depending on the monolithic
 * Application object directly.
 *
 * Currently, the adaptor still requires Application& for some operations
 * not yet abstracted into interfaces. As the refactoring progresses,
 * additional interfaces can be added here.
 *
 * Usage:
 * @code
 * ConsensusAdaptorDeps deps{
 *     ledgerProvider,
 *     overlayBroadcaster,
 *     jobScheduler,
 *     txSetManager,
 *     messageRouter,
 *     timeSource,
 *     validationTracker,
 *     operatingMode
 * };
 * @endcode
 *
 * @note All references must remain valid for the lifetime of the adaptor.
 */
struct ConsensusAdaptorDeps
{
    ILedgerProvider& ledgerProvider;
    IOverlayBroadcaster& overlayBroadcaster;
    IConsensusJobScheduler& jobScheduler;
    ITxSetManager& txSetManager;
    IMessageRouter& messageRouter;
    IConsensusTimeSource& timeSource;
    IValidationTracker& validationTracker;
    IOperatingMode& operatingMode;
};

}  // namespace xrpl

#endif
