# Phase 2 Task 2.2: Overlay/ServerHandler Decoupling

## Overview

### Problem Statement
The `overlay ↔ rpc` cycle contains 9 includes creating a bidirectional dependency between these modules. The Overlay module depends on ServerHandler but only uses a single piece of data: the peer port number (`serverHandler_.setup().overlay.port()`).

### Success Criteria
- Remove the `overlay → rpc` dependency completely
- Eliminate `overlay ↔ rpc` cycle from `loops.txt`
- Maintain identical runtime behavior for peer discovery configuration
- No performance regression in overlay initialization

---

## Implementation Notes

**Completed:** 2026-01-22

### What Was Done
- Added `peerPort` field to `Overlay::Setup` struct in `Overlay.h`
- Removed `ServerHandler&` parameter from `make_Overlay()` signature in `make_Overlay.h`
- Removed `ServerHandler&` member from `OverlayImpl` class in `OverlayImpl.h`
- Updated `OverlayImpl` constructor to no longer take or store `ServerHandler&`
- Updated `setup_Overlay()` function to parse peer port directly from config
- Updated `OverlayImpl::start()` to use `setup_.peerPort` instead of `serverHandler_.setup().overlay.port()`
- Updated `Application.cpp` to call `make_Overlay()` without `ServerHandler&` parameter

### Remaining Work
The following includes in `OverlayImpl.cpp` still exist but are **utility dependencies**, not the problematic ServerHandler coupling:
- `GetCounts.h` - Used for `getCountsJson()` in traffic counting/crawl endpoint
- `json_body.h` - Used for HTTP response handling in crawl/health endpoints

These can be addressed in a future task by moving these utilities to a shared location outside the `rpc` module.

### Summary
The primary goal of removing the `ServerHandler&` dependency from Overlay was achieved. The Overlay module no longer depends on `ServerHandler.h` in its public interface or implementation headers.

---

## Deep Code Analysis

### Complete Dependency List (overlay → rpc)

| # | File | Include | Line | Purpose |
|---|------|---------|------|---------|
| 1 | `src/xrpld/overlay/make_Overlay.h` | `ServerHandler.h` | 5 | ServerHandler& parameter type |
| 2 | `src/xrpld/overlay/detail/OverlayImpl.h` | `ServerHandler.h` | 12 | ServerHandler& member/parameter |
| 3 | `src/xrpld/overlay/detail/OverlayImpl.cpp` | `GetCounts.h` | 14 | getCountsJson() for crawl endpoint |
| 4 | `src/xrpld/overlay/detail/OverlayImpl.cpp` | `json_body.h` | 15 | json_body for HTTP responses |

### Actual Usage in OverlayImpl::start()

```cpp
// src/xrpld/overlay/detail/OverlayImpl.cpp:464-472
void
OverlayImpl::start()
{
    PeerFinder::Config config = PeerFinder::Config::makeConfig(
        app_.config(),
        serverHandler_.setup().overlay.port(),  // <-- ONLY USAGE OF ServerHandler!
        app_.getValidationPublicKey().has_value(),
        setup_.ipLimit);

    m_peerFinder->setConfig(config);
    m_peerFinder->start();
    // ...
}
```

### Application.cpp Initialization Sequence

```cpp
// src/xrpld/app/main/Application.cpp:1373-1398
overlay_ = make_Overlay(           // Line 1373: Overlay created FIRST
    *this,
    setup_Overlay(*config_),
    *serverHandler_,               // Line 1376: ServerHandler passed (not yet setup)
    *m_resourceManager,
    *m_resolver,
    get_io_context(),
    *config_,
    m_collectorManager->collector());
add(*overlay_);

// ...

auto setup = setup_ServerHandler(  // Line 1395: ServerHandler setup LATER
    *config_, beast::logstream{m_journal.error()});
setup.makeContexts();
serverHandler_->setup(setup, m_journal);  // Line 1398: Port is determined HERE
```

### Why Late Binding Exists
- `make_Overlay()` is called at line 1373
- `serverHandler_->setup()` is called at line 1398
- The port is only available AFTER `setup_ServerHandler()` parses the config
- `OverlayImpl::start()` is called even later, so the port IS available at that time
- The current design passes ServerHandler& to defer port access until `start()`

### ServerHandler::Setup Structure

```cpp
// src/xrpld/rpc/ServerHandler.h:34-62
struct Setup {
    std::vector<Port> ports;
    struct client_t { /* ... */ };
    client_t client;
    boost::asio::ip::tcp::endpoint overlay;  // Contains the port we need
    void makeContexts();
};
```

### Port Extraction Logic in ServerHandler

```cpp
// src/xrpld/rpc/detail/ServerHandler.cpp:1234-1247
static void
setup_Overlay(ServerHandler::Setup& setup)
{
    auto const iter = std::find_if(
        setup.ports.cbegin(), setup.ports.cend(), [](Port const& port) {
            return port.protocol.count("peer") != 0;
        });
    if (iter == setup.ports.cend())
    {
        setup.overlay = {};
        return;
    }
    setup.overlay = {iter->ip, iter->port};
}
```

---

## Design Considerations

### Option A: Pass Port to Overlay::Setup (RECOMMENDED)

**Approach**: Add a `peerPort` field to `Overlay::Setup` and populate it during config parsing.

**Pros**:
- Value semantics - no indirection or runtime overhead
- Simple, testable - port is just a `uint16_t`
- Follows existing pattern of `Overlay::Setup` struct
- No behavioral changes - only structural refactoring

**Cons**:
- Requires duplicating port-finding logic or creating shared utility

### Option B: Pass std::function<uint16_t()> Port Getter

**Approach**: Pass a lambda that captures config and returns the port.

**Pros**:
- Defers port resolution until needed
- Minimal changes to initialization order

**Cons**:
- Runtime overhead (function call indirection)
- More complex to test
- Captures state that may be invalidated

### Option C: Restructure Initialization Order

**Approach**: Call `setup_ServerHandler()` before `make_Overlay()`.

**Pros**:
- Cleaner overall architecture

**Cons**:
- Significant refactoring of Application.cpp
- May have cascading effects on other subsystems
- Higher risk of introducing bugs

### Recommendation: Option A

Option A provides the best balance of simplicity, testability, and minimal risk. The port-finding logic can be duplicated in `setup_Overlay()` (in OverlayImpl.cpp) since it's only ~10 lines and the configuration format is stable.

---

## Implementation Plan

### Step 1: Add peerPort Field to Overlay::Setup

```cpp
// src/xrpld/overlay/Overlay.h
struct Setup {
    explicit Setup() = default;
    std::shared_ptr<boost::asio::ssl::context> context;
    beast::IP::Address public_ip;
    int ipLimit = 0;
    std::uint32_t crawlOptions = 0;
    std::optional<std::uint32_t> networkID;
    bool vlEnabled = true;
    std::uint16_t peerPort = 0;  // NEW: peer listening port
};
```

### Step 2: Extract Port-Finding Logic to setup_Overlay()

Duplicate the port-finding logic in `setup_Overlay()` (OverlayImpl.cpp):

```cpp
// src/xrpld/overlay/detail/OverlayImpl.cpp - in setup_Overlay()
Overlay::Setup
setup_Overlay(BasicConfig const& config)
{
    Overlay::Setup setup;
    // ... existing setup code ...

    // NEW: Extract peer port from [server] configuration
    try
    {
        auto const& serverSection = config.section("server");
        for (auto const& name : serverSection.values())
        {
            auto const& portSection = config.section(name);
            auto const protocol = portSection.get<std::string>("protocol");
            if (protocol && protocol->find("peer") != std::string::npos)
            {
                if (auto port = portSection.get<std::uint16_t>("port"))
                {
                    setup.peerPort = *port;
                    break;
                }
            }
        }
    }
    catch (...)
    {
        // Port will remain 0 if not configured
    }

    return setup;
}
```

**Note**: This logic mirrors `parse_Ports()` in ServerHandler.cpp but only extracts the peer port.

### Step 3: Update OverlayImpl to Use setup_.peerPort

```cpp
// src/xrpld/overlay/detail/OverlayImpl.cpp:464-472
void
OverlayImpl::start()
{
    PeerFinder::Config config = PeerFinder::Config::makeConfig(
        app_.config(),
        setup_.peerPort,  // CHANGED: Use setup_ instead of serverHandler_
        app_.getValidationPublicKey().has_value(),
        setup_.ipLimit);

    m_peerFinder->setConfig(config);
    m_peerFinder->start();
    // ...
}
```

### Step 4: Remove ServerHandler& from make_Overlay Signature

**File: src/xrpld/overlay/make_Overlay.h**
```cpp
// Before:
std::unique_ptr<Overlay>
make_Overlay(
    Application& app,
    Overlay::Setup const& setup,
    ServerHandler& serverHandler,  // REMOVE THIS
    Resource::Manager& resourceManager,
    // ...

// After:
std::unique_ptr<Overlay>
make_Overlay(
    Application& app,
    Overlay::Setup const& setup,
    Resource::Manager& resourceManager,
    // ...
```

Remove include: `#include <xrpld/rpc/ServerHandler.h>`

### Step 5: Update OverlayImpl Constructor

**File: src/xrpld/overlay/detail/OverlayImpl.h**
- Remove `#include <xrpld/rpc/ServerHandler.h>` (line 12)
- Remove `ServerHandler& serverHandler` parameter from constructor (line 127)
- Remove `ServerHandler& serverHandler_` member (line 96)

**File: src/xrpld/overlay/detail/OverlayImpl.cpp**
- Update constructor to not take or store ServerHandler&
- Remove `serverHandler_` initialization

### Step 6: Update Application.cpp Call Site

```cpp
// src/xrpld/app/main/Application.cpp:1373-1381
overlay_ = make_Overlay(
    *this,
    setup_Overlay(*config_),
    // REMOVED: *serverHandler_,
    *m_resourceManager,
    *m_resolver,
    get_io_context(),
    *config_,
    m_collectorManager->collector());
```

### Step 7: Address GetCounts.h and json_body.h Dependencies

These are NOT part of the ServerHandler dependency but contribute to the cycle:

**GetCounts.h (line 14)**:
- Used in `OverlayImpl::getServerCounts()` which calls `getCountsJson(app_, 10)`
- Options:
  a. Move `getCountsJson()` to a utility module outside rpc (e.g., `app/misc/`)
  b. Keep the include - this is a handler utility, not ServerHandler itself
  c. Forward declare and use function pointer

**json_body.h (line 15)**:
- Used for HTTP response bodies in crawl/health endpoints
- Options:
  a. Move `json_body.h` to a shared location (e.g., `xrpl/beast/http/`)
  b. Keep the include - it's a lightweight utility

**Recommendation**: Address these in a separate task if the primary cycle (overlay ↔ rpc via ServerHandler) is broken. These utilities don't create the problematic architectural coupling.

---

## Risk Assessment

### Port Parsing Logic Duplication
- **Risk**: Logic divergence between ServerHandler and Overlay port parsing
- **Mitigation**:
  - Extract to shared utility function in `xrpl/basics/` if needed
  - Comprehensive test coverage for port configuration
  - Config schema validation

### Initialization Order Assumptions
- **Risk**: Future changes might call `start()` before port is configured
- **Mitigation**:
  - Assert `setup_.peerPort != 0` in `start()` if networking is enabled
  - Document the requirement in Overlay::Setup

### Test Coverage
- **Risk**: Existing tests may mock ServerHandler for overlay tests
- **Mitigation**: Update test fixtures to use Overlay::Setup with peerPort

### Rollback Strategy

1. **Git Revert**: All changes are atomic commits that can be reverted
2. **Verification**: After revert, run full test suite to confirm working state
3. **Time to Rollback**: < 5 minutes

---

## Validation Criteria

### Functional Validation
- [ ] `rippled` starts successfully with standard configuration
- [ ] Peer connections are established correctly
- [ ] Crawl endpoint returns correct information
- [ ] Health endpoint works correctly

### Cycle Validation
- [ ] Run dependency analysis: `overlay → rpc` cycle should be REMOVED from loops.txt
- [ ] Verify no new cycles introduced

### Test Validation
- [ ] All existing unit tests pass
- [ ] All integration tests pass
- [ ] Manual peer connectivity test on testnet

---

## Files to Modify

| File | Changes |
|------|---------|
| `src/xrpld/overlay/Overlay.h` | Add `peerPort` to Setup struct |
| `src/xrpld/overlay/make_Overlay.h` | Remove ServerHandler.h include, update signature |
| `src/xrpld/overlay/detail/OverlayImpl.h` | Remove ServerHandler.h include, member, parameter |
| `src/xrpld/overlay/detail/OverlayImpl.cpp` | Add port parsing to setup_Overlay(), update start(), constructor |
| `src/xrpld/app/main/Application.cpp` | Update make_Overlay() call |

---

## Estimated Effort

- **Implementation**: 2-3 hours
- **Testing**: 1-2 hours
- **Review adjustments**: 1 hour

**Total**: ~4-6 hours

