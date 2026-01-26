#ifndef XRPL_OVERLAY_DETAIL_HANDLERS_VALIDATOROPSHANDLER_H_INCLUDED
#define XRPL_OVERLAY_DETAIL_HANDLERS_VALIDATOROPSHANDLER_H_INCLUDED

#include <xrpld/overlay/IValidatorOps.h>

namespace xrpl {

class Application;

/**
 * Handler that implements IValidatorOps by delegating to ValidatorList
 * and ValidatorSite.
 * The header lives in overlay module, implementation in app module.
 */
class ValidatorOpsHandler final : public IValidatorOps
{
private:
    Application& app_;

public:
    explicit ValidatorOpsHandler(Application& app);

    std::optional<Json::Value>
    getValidatorListAvailable(
        std::string_view pubKey,
        std::optional<std::uint32_t> forceVersion = {}) override;

    Json::Value
    getValidatorSitesJson() const override;
};

}  // namespace xrpl

#endif
