#ifndef XRPL_OVERLAY_HANDLERS_VALIDATIONMESSAGEHANDLER_H_INCLUDED
#define XRPL_OVERLAY_HANDLERS_VALIDATIONMESSAGEHANDLER_H_INCLUDED

#include <xrpld/overlay/detail/MessageHandlerContext.h>

#include <xrpl/protocol/messages.h>

#include <memory>

namespace xrpl {

class STValidation;

/**
 * @brief Handles validation-related protocol messages.
 *
 * This handler processes the following message types:
 * - TMValidation: Individual validator signatures
 * - TMValidatorList: Validator list updates (v1)
 * - TMValidatorListCollection: Validator list updates (v2)
 *
 * Thread Safety:
 * - All handler methods are called on the PeerImp strand
 * - Handler does not maintain any state between calls
 */
class ValidationMessageHandler
{
public:
    /**
     * @brief Process a TMValidation message.
     *
     * Validates incoming validator signatures, checks for duplicates,
     * and schedules signature verification on the job queue.
     *
     * @param m The validation message
     * @param ctx Handler context with dependencies
     */
    static void
    onMessage(
        std::shared_ptr<protocol::TMValidation> const& m,
        MessageHandlerContext& ctx);

    /**
     * @brief Process a TMValidatorList message (v1 format).
     *
     * Handles validator list propagation for peers using protocol version
     * that supports ValidatorListPropagation feature.
     *
     * @param m The validator list message
     * @param ctx Handler context with dependencies
     */
    static void
    onMessage(
        std::shared_ptr<protocol::TMValidatorList> const& m,
        MessageHandlerContext& ctx);

    /**
     * @brief Process a TMValidatorListCollection message (v2 format).
     *
     * Handles validator list propagation for peers using protocol version
     * that supports ValidatorList2Propagation feature.
     *
     * @param m The validator list collection message
     * @param ctx Handler context with dependencies
     */
    static void
    onMessage(
        std::shared_ptr<protocol::TMValidatorListCollection> const& m,
        MessageHandlerContext& ctx);

private:
    // No state - all methods are static
    ValidationMessageHandler() = delete;
};

}  // namespace xrpl

#endif

