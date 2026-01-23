#ifndef RIPPLE_APP_MISC_SERVERCOUNTS_H_INCLUDED
#define RIPPLE_APP_MISC_SERVERCOUNTS_H_INCLUDED

#include <xrpld/app/main/Application.h>

namespace xrpl {

Json::Value
getCountsJson(Application& app, int minObjectCount);

}  // namespace xrpl

#endif
