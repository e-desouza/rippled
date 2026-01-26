#ifndef XRPLD_CORE_LEDGERSHORTCUT_H_INCLUDED
#define XRPLD_CORE_LEDGERSHORTCUT_H_INCLUDED

namespace xrpl {

/**
 * @brief Shortcuts for specifying common ledgers.
 *
 * Used throughout the codebase to refer to current, closed,
 * or validated ledgers without specifying a sequence number.
 */
enum class LedgerShortcut { Current, Closed, Validated };

}  // namespace xrpl

#endif

