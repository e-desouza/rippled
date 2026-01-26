// DEPRECATED: This header is deprecated. Include <xrpl/protocol/MPTokenIssuanceID.h> instead.
// This file is kept for backward compatibility during the transition.
#ifndef XRPL_RPC_MPTOKENISSUANCEID_H_INCLUDED
#define XRPL_RPC_MPTOKENISSUANCEID_H_INCLUDED

#include <xrpl/protocol/MPTokenIssuanceID.h>

// Provide backward compatibility aliases in RPC namespace
namespace xrpl {
namespace RPC {

using xrpl::canHaveMPTokenIssuanceID;
using xrpl::getIDFromCreatedIssuance;
using xrpl::insertMPTokenIssuanceID;

}  // namespace RPC
}  // namespace xrpl

#endif
