#include <xrpld/app/main/Application.h>
#include <xrpld/app/validators/ValidatorList.h>
#include <xrpld/app/validators/ValidatorSite.h>
#include <xrpld/overlay/detail/handlers/ValidatorOpsHandler.h>

namespace xrpl {

ValidatorOpsHandler::ValidatorOpsHandler(Application& app) : app_(app)
{
}

std::optional<Json::Value>
ValidatorOpsHandler::getValidatorListAvailable(
    std::string_view pubKey,
    std::optional<std::uint32_t> forceVersion)
{
    return app_.validators().getAvailable(pubKey, forceVersion);
}

Json::Value
ValidatorOpsHandler::getValidatorSitesJson() const
{
    return app_.validatorSites().getJson();
}

}  // namespace xrpl
