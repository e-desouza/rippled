#ifndef XRPL_OVERLAY_IFEETRACKOPS_H_INCLUDED
#define XRPL_OVERLAY_IFEETRACKOPS_H_INCLUDED

#include <cstdint>

namespace xrpl {

/** Interface for load fee tracking operations needed by the overlay.

    This interface abstracts the LoadFeeTrack functionality that the overlay
    module needs, allowing the overlay to be decoupled from the app module.
    The app module provides the implementation.
*/
class IFeeTrackOps
{
public:
    virtual ~IFeeTrackOps() = default;

    /** Check if the local node is under load.
        @return true if the local node is experiencing high load
    */
    virtual bool
    isLoadedLocal() const = 0;

    /** Set the cluster fee.
        @param fee The fee to set for the cluster
    */
    virtual void
    setClusterFee(std::uint32_t fee) = 0;
};

}  // namespace xrpl

#endif

