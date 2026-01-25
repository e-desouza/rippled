# Cycle 5: xrpld.consensus ↔ xrpld.overlay

## Current State

**Loop detected:** `xrpld.overlay ~= xrpld.consensus`

The `~=` indicates a weak/approximate cycle. Overlay has 1 include from consensus, and consensus has 2 includes from overlay.

## Dependency Analysis

### overlay → consensus Dependencies (1 include) - PROBLEMATIC

| File                                           | Includes                  | Purpose          |
| ---------------------------------------------- | ------------------------- | ---------------- |
| `detail/handlers/ValidationMessageHandler.cpp` | `consensus/Validations.h` | Validation types |

### consensus → overlay Dependencies (2 includes) - PROBLEMATIC

| File                                | Includes            | Purpose                 |
| ----------------------------------- | ------------------- | ----------------------- |
| `detail/OverlayBroadcasterImpl.cpp` | `overlay/Overlay.h` | Broadcasting messages   |
| `IOverlayBroadcaster.h`             | `overlay/Peer.h`    | Peer type for interface |

## Root Cause

The cycle exists because:

1. **Overlay needs consensus types**: `ValidationMessageHandler` needs `Validations.h` to process validation messages
2. **Consensus needs overlay for broadcasting**: `OverlayBroadcasterImpl` needs `Overlay.h` to broadcast messages
3. **Interface includes overlay type**: `IOverlayBroadcaster.h` includes `Peer.h` for the `Peer::ptr_t` type

## Removal Strategy

### Step 1: Remove Peer.h include from IOverlayBroadcaster.h

**Current:**

```cpp
// consensus/IOverlayBroadcaster.h
#include <xrpld/overlay/Peer.h>  // For Peer::id_t and std::shared_ptr<Peer>
```

**Problem:** The interface uses:

- `Peer::id_t` (which is `std::uint32_t`)
- `std::shared_ptr<Peer>` in the `foreach` callback

**Solution:** Use forward declaration and type alias:

```cpp
// consensus/IOverlayBroadcaster.h - AFTER
#include <memory>
#include <cstdint>

namespace ripple {

class Peer;  // Forward declaration

// Type alias to avoid including Peer.h
using PeerId = std::uint32_t;  // Same as Peer::id_t

class IOverlayBroadcaster {
public:
    virtual ~IOverlayBroadcaster() = default;

    virtual void broadcast(protocol::TMProposeSet const& m) = 0;
    virtual void broadcast(protocol::TMValidation const& m) = 0;

    // Use PeerId instead of Peer::id_t
    virtual std::set<PeerId> relay(
        protocol::TMProposeSet const& m,
        uint256 const& suppression,
        PublicKey const& validator) = 0;

    virtual void relay(
        uint256 const& hash,
        protocol::TMTransaction const& m,
        std::set<PeerId> const& skip) = 0;

    // Forward declaration allows shared_ptr<Peer>
    virtual void foreach(std::function<void(std::shared_ptr<Peer> const&)> f) = 0;
};

} // namespace ripple
```

**Note:** `std::shared_ptr<Peer>` works with forward declaration because `shared_ptr` only needs the complete type at destruction, not declaration.

### Step 2: Move OverlayBroadcasterImpl to app module

The `OverlayBroadcasterImpl` is an implementation that wraps `Overlay`. It should be in `app/` where it can include overlay headers.

**Move:** `src/xrpld/consensus/detail/OverlayBroadcasterImpl.cpp` → `src/xrpld/app/consensus/detail/OverlayBroadcasterImpl.cpp`

(Note: This is the same move proposed in Cycle 1 - app↔consensus)

### Step 3: Move Validations.h types to shared location

**Option A: Move validation types to xrpl/protocol**

If `Validations.h` contains protocol-level types, move them to `xrpl/protocol/`:

```
consensus/Validations.h → xrpl/protocol/Validations.h
```

**Option B: Create forward declaration header**

```cpp
// consensus/ValidationsFwd.h
namespace ripple {
template<class Adaptor> class Validations;
} // namespace ripple
```

### Step 4: Refactor ValidationMessageHandler

**Option A: Move to app module**

Move `ValidationMessageHandler.cpp` to `app/overlay/handlers/` where it can include both consensus and overlay headers.

**Option B: Use interface**

Create an interface for validation handling:

```cpp
// overlay/IValidationHandler.h
class IValidationHandler {
public:
    virtual ~IValidationHandler() = default;
    virtual void onValidation(protocol::TMValidation const& msg) = 0;
};
```

Then inject the implementation from app.

## Expected Result After Changes

```
xrpld.consensus > xrpl.*     (consensus depends on xrpl libs - ALLOWED)
xrpld.overlay > xrpl.*       (overlay depends on xrpl libs - ALLOWED)
```

No cycle between consensus and overlay.

## Verification Steps

1. Run levelization: `.github/scripts/levelization/generate.sh`
2. Verify no `Loop: xrpld.consensus xrpld.overlay` in output
3. Build: `cd .build && ninja -j4`
4. Run tests: `./xrpld --unittest=Consensus && ./xrpld --unittest=Overlay`

## Risk Assessment

| Risk                            | Likelihood | Impact | Mitigation               |
| ------------------------------- | ---------- | ------ | ------------------------ |
| Breaking validation propagation | Medium     | High   | Test validation flow     |
| Breaking message broadcasting   | Medium     | High   | Test consensus rounds    |
| Type compatibility issues       | Low        | Medium | Verify type aliases work |

## Estimated Effort

**Low-Medium effort (3-5 days)** - Only 3 dependencies to break.

## Recommended Approach

1. First fix `IOverlayBroadcaster.h` to use forward declaration (Step 1)
2. Move `OverlayBroadcasterImpl.cpp` to app (Step 2) - aligns with Cycle 1 fix
3. Move `ValidationMessageHandler.cpp` to app (Step 4, Option A)

This approach:

- Minimizes new abstractions
- Aligns with Cycle 1 fixes
- Keeps implementations in app where they have access to all subsystems
