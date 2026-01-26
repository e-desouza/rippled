#ifndef XRPL_OVERLAY_IVALIDATOROPS_H_INCLUDED
#define XRPL_OVERLAY_IVALIDATOROPS_H_INCLUDED

#include <xrpl/json/json_value.h>

#include <cstdint>
#include <optional>
#include <string_view>

namespace xrpl {

class PublicKey;

/** Interface for validator operations needed by the overlay.

    This interface abstracts the ValidatorList and ValidatorSite
    functionality that the overlay module needs, allowing the overlay
    to be decoupled from the app/validators module.
    The app module provides the implementation.
*/
class IValidatorOps
{
public:
    virtual ~IValidatorOps() = default;

    /** Get an available validator list by public key.
        @param pubKey The public key to look up
        @param forceVersion Optional version to force
        @return The validator list JSON if found
    */
    virtual std::optional<Json::Value>
    getValidatorListAvailable(
        std::string_view pubKey,
        std::optional<std::uint32_t> forceVersion = {}) = 0;

    /** Get JSON representation of validator sites.
        @return JSON object with validator_sites member
    */
    virtual Json::Value
    getValidatorSitesJson() const = 0;

    /** Check if a validator is listed (included on any lists).
        @param identity Validation public key
        @return true if the key is listed
    */
    virtual bool
    isValidatorListed(PublicKey const& identity) const = 0;

    /** Get JSON representation of validators (master keys, signing keys, UNL).
        @return JSON object with validator information
    */
    virtual Json::Value
    getValidatorsJson() const = 0;
};

}  // namespace xrpl

#endif
