#include <xrpld/app/main/Application.h>
#include <xrpld/app/validators/ValidatorList.h>
#include <xrpld/app/validators/ValidatorSite.h>
#include <xrpld/overlay/detail/handlers/ValidatorOpsHandler.h>

#include <xrpl/protocol/PublicKey.h>

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

bool
ValidatorOpsHandler::isValidatorListed(PublicKey const& identity) const
{
    return app_.validators().listed(identity);
}

Json::Value
ValidatorOpsHandler::getValidatorsJson() const
{
    return app_.validators().getJson();
}

}  // namespace xrpl
