# Tier 1 Implementation - Lessons Learned

## Overview

Analysis of 5 completed Tier 1 interface implementations:
1. **IFeeTrackOps** (bbd065c7e7) - Fee tracking abstraction
2. **IHandshakeParams** (f2e798ee40) - Handshake parameters  
3. **IPeerReservationStorage** (246b19a463) - Database abstraction
4. **getServerPorts()** (1a8e49ca30) - Method migration to Application
5. **IOverlayOps** (a28f6142c7) - Network operations abstraction

---

## 1. Successful Patterns

### Pattern A: Interface in Overlay, Adapter in App

**File Locations:**
```
src/xrpld/overlay/IFoo.h              # Interface (pure virtual)
src/xrpld/app/overlay/adapters/FooAdapter.h   # Adapter (header-only when simple)
```

**Why it works:**
- Overlay module only sees the interface, breaking the dependency
- App module provides implementation, has access to app-layer types
- Clear separation of concerns

### Pattern B: Handler Split (Header in Overlay, Impl in App)

For handlers that need complex app dependencies in implementation:
```
src/xrpld/overlay/detail/handlers/FooHandler.h     # Interface header
src/xrpld/app/overlay/handlers/FooHandler.cpp      # Implementation
```

**Example:** OverlayOpsHandler uses this pattern because the implementation needs NetworkOPs, Manifest, etc.

### Pattern C: Constructor Injection via make_Overlay

All interfaces are passed through `make_Overlay()`:
```cpp
std::unique_ptr<Overlay>
make_Overlay(
    Application& app,
    ...
    IFeeTrackOps& feeTrackOps,
    IHandshakeParams& handshakeParams,
    IOverlayOps& overlayOps);
```

**Store as references in OverlayImpl:**
```cpp
IFeeTrackOps& feeTrackOps_;
IHandshakeParams& handshakeParams_;
IOverlayOps& overlayOps_;
```

### Pattern D: Forward Declarations in Headers

Use forward declarations in header files to minimize includes:
```cpp
namespace xrpl {
class Application;
class IFeeTrackOps;
class IOverlayOps;
}
```

---

## 2. Common Pitfalls to Avoid

### Pitfall 1: Forgetting jss.h Include

When removing `NetworkOPs.h` from OverlayImpl.cpp, the jss:: constants lose their include.
**Solution:** Add `#include <xrpl/protocol/jss.h>` explicitly.

### Pitfall 2: Test Files Need Adapter Includes

Tests that call functions like `makeResponse()` need the adapter:
```cpp
#include <xrpld/app/overlay/adapters/HandshakeParamsAdapter.h>

// In test code:
HandshakeParamsAdapter handshakeParams(env.app());
auto http_resp = xrpl::makeResponse(..., handshakeParams);
```

**Files affected in Tier 1:**
- `src/test/app/LedgerReplay_test.cpp`
- `src/test/overlay/compression_test.cpp`  
- `src/test/overlay/reduce_relay_test.cpp`

### Pitfall 3: Two-Phase Initialization Dependencies

Some adapters can't be created in the constructor because dependencies aren't ready.
**Example:** PeerReservationStorageAdapter needs DatabaseCon, created after `initRelationalDatabase()`.

```cpp
// In ApplicationImp constructor:
feeTrackAdapter_(std::make_unique<LoadFeeTrackAdapter>(*mFeeTrack))  // OK

// In ApplicationImp::setup():
peerReservationStorageAdapter_ = 
    std::make_unique<PeerReservationStorageAdapter>(getWalletDB());  // After DB init
```

### Pitfall 4: Interface Method Signatures

Keep interface methods simple with primitive/protocol-layer types only.
**Good:** `std::uint32_t`, `PublicKey`, `std::optional<uint256>`
**Avoid:** Complex app-layer types that would reintroduce dependencies

---

## 3. Checklist for Implementing New Interfaces

### Pre-Implementation
- [ ] Identify all methods/data the overlay needs from app
- [ ] Design interface with minimal, primitive parameter types
- [ ] Check if header-only adapter is sufficient or if split pattern needed

### Interface Creation
- [ ] Create `src/xrpld/overlay/IFoo.h`
- [ ] Add minimal includes (prefer forward declarations)
- [ ] Virtual destructor: `virtual ~IFoo() = default;`
- [ ] Pure virtual methods with `= 0`
- [ ] Doxygen comments for each method

### Adapter/Handler Creation  
- [ ] Create adapter in `src/xrpld/app/overlay/adapters/FooAdapter.h`
- [ ] Or split handler: header in overlay, impl in app
- [ ] Include the interface header
- [ ] Include necessary app-layer headers in adapter
- [ ] Implement all interface methods

### Integration
- [ ] Add forward declaration to `OverlayImpl.h`
- [ ] Add reference member: `IFoo& foo_;`
- [ ] Add constructor parameter
- [ ] Update `make_Overlay.h` signature
- [ ] Update `make_Overlay()` implementation
- [ ] Create adapter instance in Application.cpp
- [ ] Pass to make_Overlay call

### Cleanup
- [ ] Remove old app-layer includes from overlay files
- [ ] Add any transitively-needed includes (e.g., `jss.h`)
- [ ] Update test files with adapter includes
- [ ] Run build to verify

---

## 4. File Naming & Location Conventions

| Type | Location | Naming |
|------|----------|--------|
| Interface | `src/xrpld/overlay/` | `IFoo.h` |
| Simple Adapter | `src/xrpld/app/overlay/adapters/` | `FooAdapter.h` |
| Handler Header | `src/xrpld/overlay/detail/handlers/` | `FooHandler.h` |
| Handler Impl | `src/xrpld/app/overlay/handlers/` | `FooHandler.cpp` |

### Header Guards
```cpp
#ifndef XRPL_OVERLAY_IFOO_H_INCLUDED
#define XRPL_OVERLAY_IFOO_H_INCLUDED
```

### Namespace
All code in `namespace xrpl { }` (not `namespace ripple`).

---

## 5. Typical Files Changed Per Interface

From commit analysis, expect to modify:

1. **New files (2-3):**
   - Interface header
   - Adapter header (and optionally .cpp)

2. **Modified files (4-7):**
   - `src/xrpld/overlay/detail/OverlayImpl.h` - Add member reference
   - `src/xrpld/overlay/detail/OverlayImpl.cpp` - Constructor, usage
   - `src/xrpld/overlay/make_Overlay.h` - Signature update
   - `src/xrpld/app/main/Application.cpp` - Create adapter, pass to make_Overlay
   - Overlay consumers (PeerImp.cpp, ConnectAttempt.cpp, etc.)
   - Test files (if they call affected functions)

---

## 6. Dependency Reduction Results

| Step | Interface | Dependencies Removed |
|------|-----------|---------------------|
| 2.4.1 | IFeeTrackOps | 17→16 |
| 2.4.2 | IHandshakeParams | 16→14 |
| 2.4.3 | IPeerReservationStorage | 14→13 |
| 2.4.4 | getServerPorts() | app→rpc 3→2 |
| 2.4.5 | IOverlayOps | 13→12 |

**Total overlay→app reduction: 17 → 12 (29% reduction)**

