# Cycle 4: xrpld.app ↔ xrpld.rpc

## ✅ HEADER DEPENDENCIES ELIMINATED

**Progress:** ALL header-level dependencies between app↔rpc eliminated!
- **Header deps APP→RPC:** 0 (was 1)
- **Header deps RPC→APP:** 0 (was 3)
- **Total APP→RPC:** 2 (impl-only, from Application.cpp)
- **Total RPC→APP:** 129 (impl-only, RPC handlers using app services)

**Note:** The levelization script still reports a cycle because it counts ALL source file
dependencies, not just headers. The remaining deps are implementation-level only.

### Changes Made (2026-01-27):

1. **Commit `ddca13afc0`:** Move InfoSub to xrpl library and extract FailHard enum
   - Moved `InfoSub` to `include/xrpl/subscription/InfoSub.h`
   - Created `src/xrpld/core/FailHard.h` following OperatingMode pattern
   - Added `subscription` module to xrpl library

2. **Commit `e2b6d878f8`:** Move Manifest to xrpl library and reduce rpc→app header deps
   - Moved `Manifest` to `include/xrpl/validators/Manifest.h`
   - Created `ManifestPersistence.h` for database operations
   - Removed obsolete `GetCounts.h`
   - Moved `Version.h` constructor to `.cpp`
   - Added `validators` module to xrpl library

3. **Earlier commits:** CTID.h, LedgerDataProvider.h, LedgerShortcut.h moves

4. **Latest changes (2026-01-27):**
   - Moved `LedgerDataProvider.h` to `include/xrpl/ledger/` (xrpl library)
   - Created `IBlockedStatus` interface in `src/xrpld/core/` (shared between app and rpc)
   - Moved `TxConsequences` class to `include/xrpl/protocol/TxConsequences.h`
   - Moved `TxDetails` struct to `include/xrpl/protocol/TxDetails.h`
   - Updated `Handler.h` to use `IBlockedStatus` interface instead of `NetworkOPs.h`
   - Updated `LedgerHandler.h` to use xrpl-library `TxDetails.h`

### Remaining Dependencies (Implementation-only):

| Direction | Count | Nature | Solution |
|-----------|-------|--------|----------|
| APP→RPC | 2 | `Application.cpp` → `GRPCServer.h`, `ServerHandler.h` | Factory pattern (architectural) |
| RPC→APP | 129 | RPC handlers using app services | Expected direction, no action needed |

### Proposed Solutions for Remaining 3 Header Dependencies

#### Solution 1: Move LedgerDataProvider to xrpl library (LOW EFFORT)

`LedgerDataProvider` is a pure interface with no xrpld dependencies:

```cpp
// Current: src/xrpld/app/ledger/LedgerDataProvider.h
// Proposed: include/xrpl/ledger/LedgerDataProvider.h
```

**Implementation:**
1. Move `LedgerDataProvider.h` to `include/xrpl/ledger/`
2. Update `cmake/XrplCore.cmake` to include it in ledger module
3. Update all include paths

**Result:** `Context.h` → `LedgerDataProvider.h` becomes xrpl→xrpl (no cycle)

#### Solution 2: Extract IBlockedStatus Interface (MEDIUM EFFORT)

Create interface for the blocked status checks used by `Handler.h`:

```cpp
// src/xrpld/rpc/IBlockedStatus.h (or xrpl/rpc/)
class IBlockedStatus {
public:
    virtual ~IBlockedStatus() = default;
    virtual bool isAmendmentBlocked() const = 0;
    virtual bool isUNLBlocked() const = 0;
};
```

**Implementation:**
1. Create `IBlockedStatus` interface in rpc module
2. Have `NetworkOPs` implement `IBlockedStatus`
3. Change `Handler.h` to use `IBlockedStatus&` instead of including `NetworkOPs.h`
4. Pass `IBlockedStatus&` through `Context`

**Result:** `Handler.h` → `NetworkOPs.h` becomes `Handler.h` → `IBlockedStatus.h` (no cycle)

#### Solution 3: Extract TxDetails to Standalone Header (HIGH EFFORT)

The `TxQ::TxDetails` nested type blocks extraction. Options:

**Option A: Move TxDetails outside TxQ class**
```cpp
// src/xrpld/app/txqueue/TxDetails.h (new file)
struct TxDetails {
    FeeLevel64 feeLevel;
    std::optional<LedgerIndex> lastValid;
    // ... rest of struct
};

// TxQ.h now includes TxDetails.h
```

**Option B: Use forward declaration + pointer**
```cpp
// LedgerHandler.h
class TxQ;
struct TxDetails;  // Forward declare

class LedgerHandler {
    std::vector<std::unique_ptr<TxDetails>> queueTxs_;  // Pointer instead of value
};
```

**Option C: Move LedgerHandler to app module**
Since `LedgerHandler` is tightly coupled to app types, move it:
```
src/xrpld/rpc/handlers/LedgerHandler.h → src/xrpld/app/rpc/LedgerHandler.h
```

**Recommended:** Option A (extract TxDetails) is cleanest and follows C++ best practices.

### Implementation Priority

| Priority | Solution | Effort | Impact |
|----------|----------|--------|--------|
| 1 | Move LedgerDataProvider to xrpl | Low | Removes 1 header dep |
| 2 | Extract IBlockedStatus | Medium | Removes 1 header dep |
| 3 | Extract TxDetails | High | Removes 1 header dep |

**If all 3 solutions implemented:** Cycle 4 would be fully broken (0 header deps).

### Refined Implementation Plan (2026-01-26)

---

## Original State (Before Fixes)

**Loop detected:** `xrpld.rpc > xrpld.app`

The rpc module had 133 includes from app, and app had 24 includes from rpc.

## Dependency Analysis

### rpc → app Dependencies (133 includes) - EXPECTED

RPC handlers naturally need to access app components to serve requests. This is the **expected** direction of dependency.

**Level Analysis:**

- xrpld.rpc is Level 9 (highest)
- xrpld.app is Level 8
- Level 9 → Level 8 is ALLOWED (higher level can depend on lower)

Key dependencies:

- `app/main/Application.h` - Access to subsystems
- `app/ledger/LedgerMaster.h` - Ledger queries
- `app/misc/NetworkOPs.h` - Network operations
- `app/txqueue/TxQ.h` - Transaction queue
- Various other app components for specific RPC handlers

### app → rpc Dependencies (24 includes) - PROBLEMATIC

**Level Analysis:**

- xrpld.app is Level 8
- xrpld.rpc is Level 9
- Level 8 → Level 9 is FORBIDDEN (lower level cannot depend on higher)

**These dependencies must be removed by moving items FROM rpc TO app/xrpl (lower levels).**

| File                                 | Includes                                                                                                     | Purpose                  |
| ------------------------------------ | ------------------------------------------------------------------------------------------------------------ | ------------------------ |
| `app/rdb/RelationalDatabase.h`       | `rpc/detail/RPCLedgerHelpers.h`                                                                              | Ledger JSON helpers      |
| `app/main/Main.cpp`                  | `rpc/RPCCall.h`                                                                                              | RPC client calls         |
| `app/main/GRPCServer.h`              | `rpc/Context.h`, `rpc/GRPCHandlers.h`, `rpc/InfoSub.h`, `rpc/Role.h`, `rpc/detail/Handler.h`                 | gRPC server              |
| `app/main/Application.cpp`           | `rpc/ServerHandler.h`                                                                                        | HTTP server              |
| `app/ledger/detail/LedgerToJson.cpp` | `rpc/Context.h`, `rpc/DeliveredAmount.h`, `rpc/MPTokenIssuanceID.h`                                          | JSON conversion          |
| `app/ledger/BookListeners.h`         | `rpc/InfoSub.h`                                                                                              | Subscription interface   |
| `app/ledger/LedgerMaster.h`          | `rpc/LedgerDataProvider.h`                                                                                   | Interface implementation |
| `app/ledger/LedgerToJson.h`          | `rpc/Context.h`                                                                                              | Context type             |
| `app/paths/PathRequest.h`            | `rpc/InfoSub.h`                                                                                              | Subscription interface   |
| `app/paths/PathRequest.cpp`          | `rpc/detail/Tuning.h`                                                                                        | Tuning constants         |
| `app/misc/detail/Transaction.cpp`    | `rpc/CTID.h`                                                                                                 | CTID utilities           |
| `app/misc/IPubSubManager.h`          | `rpc/InfoSub.h`                                                                                              | Subscription interface   |
| `app/misc/NetworkOPs.h`              | `rpc/InfoSub.h`                                                                                              | Subscription interface   |
| `app/misc/NetworkOPs.cpp`            | `rpc/BookChanges.h`, `rpc/CTID.h`, `rpc/DeliveredAmount.h`, `rpc/MPTokenIssuanceID.h`, `rpc/ServerHandler.h` | Various utilities        |

## Root Cause

The cycle exists because:

1. **InfoSub.h** is in rpc/, but app directly includes it for pub/sub subscriptions instead of going through app-level interfaces
2. **JSON utility functions** (DeliveredAmount, CTID, MPTokenIssuanceID, BookChanges) are in rpc/ but used by app and other lower-level code
3. **Context.h** is in rpc/ but used by app for ledger JSON conversion
4. **ServerHandler** is in rpc/ but created by Application

## Removal Strategy

### Step 1: Keep InfoSub in rpc and route app through app-level pub/sub interfaces

`InfoSub` is an RPC-specific subscription implementation. App code should depend only on app-level pub/sub interfaces (for example `IPubSubManager`) rather than on the concrete RPC type.

**Goal:** Remove direct `#include <xrpld/rpc/InfoSub.h>` from app code by:

- Defining or extending app-level pub/sub interfaces in `app/misc/` to capture the operations `InfoSub` provides.
- Updating app sites such as `BookListeners`, `PathRequest`, `IPubSubManager`, and `NetworkOPs` to depend only on those interfaces.
- Having the RPC layer implement those interfaces using `InfoSub` internally (no file moves required).

### Step 2: Move JSON utility functions to xrpl/json or xrpl/protocol

These are pure utility functions that don't depend on RPC and are used by both app and RPC:

**Move to `src/xrpl/json/` or `src/xrpl/protocol/` (protocol-level utilities):**

- `rpc/CTID.h` → `xrpl/json/CTID.h`
- `rpc/DeliveredAmount.h` → `xrpl/json/DeliveredAmount.h`
- `rpc/MPTokenIssuanceID.h` → `xrpl/json/MPTokenIssuanceID.h`
- `rpc/BookChanges.h` → `xrpl/json/BookChanges.h`

> Note (2026-01-26): In practice this will be implemented by splitting
> `DeliveredAmount` and `MPTokenIssuanceID` into rpc-agnostic core helpers and
> RPC wiring functions rather than moving the entire headers.

### Step 3: Move LedgerDataProvider interface

`LedgerDataProvider` is an interface that LedgerMaster implements. It should be in app, not rpc.

**Move:** `src/xrpld/rpc/LedgerDataProvider.h` → `src/xrpld/app/ledger/LedgerDataProvider.h`

### Step 4: Refactor Context.h usage

`Context.h` is used by `LedgerToJson`. Options:

1. Move `LedgerToJson` to rpc module (it's RPC-specific)
2. Create a minimal `LedgerJsonContext` in app that Context extends

**Recommended:** Move `LedgerToJson.h/cpp` to `rpc/detail/` since it's RPC-specific.

### Step 5: Refactor ServerHandler creation

`Application.cpp` includes `rpc/ServerHandler.h` to create the HTTP server.

**Solution:** Use forward declaration and factory function:

```cpp
// In Application.cpp
std::unique_ptr<ServerHandler> make_ServerHandler(...);  // Factory
```

## Expected Result After Changes

```
xrpld.rpc > xrpld.app  (rpc depends on app - ALLOWED)
xrpld.app > xrpl.*     (app depends on xrpl libs - ALLOWED)
```

No cycle between app and rpc.

## Verification Steps

1. Run levelization: `.github/scripts/levelization/generate.sh`
2. Verify no `Loop: xrpld.app xrpld.rpc` in output
3. Build: `cd .build && ninja -j4`
4. Run tests: `./xrpld --unittest`

## Risk Assessment

| Risk                    | Likelihood | Impact | Mitigation                 |
| ----------------------- | ---------- | ------ | -------------------------- |
| Breaking RPC handlers   | Medium     | High   | Test all RPC commands      |
| Missing include updates | High       | Low    | Grep for old paths         |
| Subscription issues     | Medium     | High   | Test pub/sub functionality |

## Estimated Effort

**Medium effort (1-2 weeks)** - 24 dependencies to break, but most are simple moves.
