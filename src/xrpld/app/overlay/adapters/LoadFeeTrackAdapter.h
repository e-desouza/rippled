#ifndef XRPL_APP_OVERLAY_ADAPTERS_LOADFEETRACKADAPTER_H_INCLUDED
#define XRPL_APP_OVERLAY_ADAPTERS_LOADFEETRACKADAPTER_H_INCLUDED

#include <xrpld/app/misc/LoadFeeTrack.h>
#include <xrpld/overlay/IFeeTrackOps.h>

namespace xrpl {

/** Adapter that wraps LoadFeeTrack to implement IFeeTrackOps interface.

    This adapter allows the overlay module to access load fee tracking
    functionality without directly depending on the app module's LoadFeeTrack
    class.
*/
class LoadFeeTrackAdapter final : public IFeeTrackOps
{
private:
    LoadFeeTrack& feeTrack_;

public:
    explicit LoadFeeTrackAdapter(LoadFeeTrack& feeTrack) : feeTrack_(feeTrack)
    {
    }

    bool
    isLoadedLocal() const override
    {
        return feeTrack_.isLoadedLocal();
    }

    void
    setClusterFee(std::uint32_t fee) override
    {
        feeTrack_.setClusterFee(fee);
    }
};

}  // namespace xrpl

#endif

