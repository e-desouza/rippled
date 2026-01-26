#ifndef XRPL_OVERLAY_IOVERLAYPROVIDER_H_INCLUDED
#define XRPL_OVERLAY_IOVERLAYPROVIDER_H_INCLUDED

namespace xrpl {

class Overlay;

/**
 * Interface for getting an Overlay reference lazily.
 * Used by PeerSetBuilder to defer overlay access until build() is called.
 * This allows PeerSetBuilder to be created before Overlay is initialized.
 */
class IOverlayProvider
{
public:
    virtual ~IOverlayProvider() = default;
    virtual Overlay&
    getOverlay() = 0;
};

}  // namespace xrpl

#endif
