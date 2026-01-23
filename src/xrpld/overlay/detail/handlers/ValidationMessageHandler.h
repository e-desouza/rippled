#ifndef XRPL_OVERLAY_HANDLERS_VALIDATIONMESSAGEHANDLER_H_INCLUDED
#define XRPL_OVERLAY_HANDLERS_VALIDATIONMESSAGEHANDLER_H_INCLUDED

#include <xrpl/protocol/messages.h>

#include <memory>

namespace xrpl {

class PeerImp;
class STValidation;
struct ValidatorBlobInfo;

/**
 * @brief Handles validation-related protocol messages.
 *
 * This handler processes the following message types:
 * - TMValidation: Individual validator signatures
 * - TMValidatorList: Validator list updates (v1)
 * - TMValidatorListCollection: Validator list updates (v2)
 *
 * The handler is a friend of PeerImp and has access to its internals.
 * All handler methods are static and stateless.
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
     * @param peer The peer that received this message
     */
    static void
    onMessage(std::shared_ptr<protocol::TMValidation> const& m, PeerImp& peer);

    /**
     * @brief Process a TMValidatorList message (v1 format).
     *
     * Handles validator list propagation for peers using protocol version
     * that supports ValidatorListPropagation feature.
     *
     * @param m The validator list message
     * @param peer The peer that received this message
     */
    static void
    onMessage(
        std::shared_ptr<protocol::TMValidatorList> const& m,
        PeerImp& peer);

    /**
     * @brief Process a TMValidatorListCollection message (v2 format).
     *
     * Handles validator list propagation for peers using protocol version
     * that supports ValidatorList2Propagation feature.
     *
     * @param m The validator list collection message
     * @param peer The peer that received this message
     */
    static void
    onMessage(
        std::shared_ptr<protocol::TMValidatorListCollection> const& m,
        PeerImp& peer);

private:
    /**
     * @brief Common processing for validator list messages.
     *
     * @param peer The peer that received the message
     * @param messageType Description of the message type for logging
     * @param manifest The manifest string from the message
     * @param version The protocol version
     * @param blobs The validator blob information
     */
    static void
    processValidatorListMessage(
        PeerImp& peer,
        std::string const& messageType,
        std::string const& manifest,
        std::uint32_t version,
        std::vector<ValidatorBlobInfo> const& blobs);

    // No state - all methods are static
    ValidationMessageHandler() = delete;
};

}  // namespace xrpl

#endif

