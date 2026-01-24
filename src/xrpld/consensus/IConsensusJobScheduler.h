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

#ifndef XRPL_CONSENSUS_ICONSENSUSJOBSCHEDULER_H_INCLUDED
#define XRPL_CONSENSUS_ICONSENSUSJOBSCHEDULER_H_INCLUDED

#include <xrpl/core/Job.h>

#include <functional>
#include <string>

namespace xrpl {

/**
 * @brief Interface for scheduling consensus-related jobs.
 *
 * This interface abstracts job scheduling functionality used by the
 * consensus layer. It allows the consensus module to schedule work
 * without depending directly on the application's job queue.
 *
 * Thread-safe: Implementations must be thread-safe.
 */
class IConsensusJobScheduler
{
public:
    virtual ~IConsensusJobScheduler() = default;

    /**
     * @brief Schedule a job for execution.
     *
     * @param type The type of job to schedule (determines priority).
     * @param name A descriptive name for the job (used for logging).
     * @param job The function to execute.
     */
    virtual void
    addJob(
        JobType type,
        std::string const& name,
        std::function<void()> job) = 0;
};

}  // namespace xrpl

#endif
