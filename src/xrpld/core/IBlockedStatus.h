#ifndef XRPL_CORE_IBLOCKEDSTATUS_H_INCLUDED
#define XRPL_CORE_IBLOCKEDSTATUS_H_INCLUDED

#include <xrpld/core/OperatingMode.h>

#include <string>

namespace xrpl {

/**
 * Interface for checking blocked status and operating mode conditions.
 * This allows RPC handlers to check node status without depending on
 * the full NetworkOPs interface.
 */
class IBlockedStatus
{
public:
    virtual ~IBlockedStatus() = default;

    /**
     * Check if the node is blocked due to a missing amendment.
     * @return true if the node is amendment blocked
     */
    virtual bool
    isAmendmentBlocked() = 0;

    /**
     * Check if the node is blocked due to an expired UNL.
     * @return true if the node is UNL blocked
     */
    virtual bool
    isUNLBlocked() = 0;

    /**
     * Get the current operating mode of the node.
     * @return the current OperatingMode
     */
    virtual OperatingMode
    getOperatingMode() const = 0;

    /**
     * Get a string representation of the operating mode.
     * @param admin if true, include additional admin-only information
     * @return string describing the operating mode
     */
    virtual std::string
    strOperatingMode(bool admin = false) const = 0;
};

}  // namespace xrpl

#endif

