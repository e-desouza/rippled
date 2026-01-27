#ifndef XRPL_XRPL_RPC_HANDLERS_VERSION_H
#define XRPL_XRPL_RPC_HANDLERS_VERSION_H

#include <xrpld/rpc/Context.h>
#include <xrpld/rpc/Status.h>
#include <xrpld/rpc/detail/Handler.h>

#include <xrpl/protocol/ApiVersion.h>

namespace xrpl {
namespace RPC {

class VersionHandler
{
public:
    explicit VersionHandler(JsonContext& c);

    Status
    check()
    {
        return Status::OK;
    }

    void
    writeResult(Json::Value& obj)
    {
        setVersion(obj, apiVersion_, betaEnabled_);
    }

    static constexpr char const* name = "version";

    static constexpr unsigned minApiVer = RPC::apiMinimumSupportedVersion;

    static constexpr unsigned maxApiVer = RPC::apiMaximumValidVersion;

    static constexpr Role role = Role::USER;

    static constexpr Condition condition = NO_CONDITION;

private:
    unsigned int apiVersion_;
    bool betaEnabled_;
};

}  // namespace RPC
}  // namespace xrpl

#endif
