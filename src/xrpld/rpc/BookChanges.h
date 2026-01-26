// DEPRECATED: This header is deprecated. Include <xrpl/protocol/BookChanges.h> instead.
// This file is kept for backward compatibility during the transition.
#ifndef XRPL_RPC_BOOKCHANGES_H_INCLUDED
#define XRPL_RPC_BOOKCHANGES_H_INCLUDED

#include <xrpl/protocol/BookChanges.h>

// Provide backward compatibility alias in RPC namespace
namespace xrpl {
namespace RPC {

using xrpl::computeBookChanges;

}  // namespace RPC
}  // namespace xrpl

#endif
