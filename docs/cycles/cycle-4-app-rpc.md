# Cycle 4: xrpld.app ↔ xrpld.rpc

## ⚠️ IMPLEMENTATION STATUS: PARTIAL

**Progress:** app→rpc dependencies reduced from 24 to 15 (37.5% improvement)

### Changes Made:

1. **Commit `97ce488155`:** Move CTID.h and LedgerDataProvider.h
   - Moved `src/xrpld/rpc/CTID.h` → `include/xrpl/protocol/CTID.h`
   - Removed `RPC::` namespace wrapper, functions now in `xrpl::`
   - Moved `src/xrpld/rpc/LedgerDataProvider.h` → `src/xrpld/app/ledger/LedgerDataProvider.h`

2. **Commit `f595490a3c`:** Move InfoSub.h to app/misc
   - Moved `src/xrpld/rpc/InfoSub.h` → `src/xrpld/app/misc/InfoSub.h`
   - Moved `src/xrpld/rpc/detail/InfoSub.cpp` → `src/xrpld/app/misc/detail/InfoSub.cpp`
   - Updated all 10 files that include InfoSub.h

3. **Commit `d3f6303865`:** Extract LedgerShortcut enum
   - Created `src/xrpld/core/LedgerShortcut.h` with the enum definition
   - Updated `RelationalDatabase.h` to use core/LedgerShortcut.h

### Remaining Dependencies (15):

| File | Includes | Why Difficult |
|------|----------|---------------|
| `NetworkOPs.cpp` | `rpc/BookChanges.h` | Template uses ledger types |
| `NetworkOPs.cpp` | `rpc/DeliveredAmount.h` | Uses RPC::Context |
| `NetworkOPs.cpp` | `rpc/MPTokenIssuanceID.h` | Uses RPC::Context |
| `NetworkOPs.cpp` | `rpc/ServerHandler.h` | Creates HTTP server |
| `LedgerToJson.h` | `rpc/Context.h` | Core dependency |
| `LedgerToJson.cpp` | `rpc/Context.h`, `rpc/DeliveredAmount.h`, `rpc/MPTokenIssuanceID.h` | Uses RPC context |
| `PathRequest.cpp` | `rpc/detail/Tuning.h` | Path tuning constants |
| `GRPCServer.h` | 4 RPC headers | gRPC server implementation |
| `Application.cpp` | `rpc/ServerHandler.h` | Creates HTTP server |
| `Main.cpp` | `rpc/RPCCall.h` | RPC client calls |

### Remaining Work Required:

1. **Move LedgerToJson to rpc module** - It's RPC-specific but heavily used by app
2. **Create interfaces for Context usage** - Abstract RPC::Context dependencies
3. **Refactor GRPCServer** - Move to rpc or use forward declarations
4. **Extract path tuning constants** - Move to app/paths/

### Refined Implementation Plan (2026-01-26)

1. **Extract JSON helpers into lower-level modules**
   - Move `BookChanges` to `xrpl/json/BookChanges.h` (pure JSON + ledger types).
   - Split `DeliveredAmount` and `MPTokenIssuanceID` into:
     - Context-free helper functions in `xrpl/json/...` (no `RPC::Context`).
     - RPC wiring functions that remain in `src/xrpld/rpc/`, calling the helpers.

2. **Decouple `LedgerToJson` from `RPC::Context`**
   - Introduce an app-level `LedgerJsonOptions` struct in `app/ledger`.
   - Change `LedgerToJson` to take `ReadView` + `LedgerJsonOptions` instead of `RPC::Context`.
   - Add RPC-side helpers that read `RPC::Context` / request JSON, populate `LedgerJsonOptions`, and call the app-level ledger JSON functions.

3. **Extract path tuning constants to app**
   - Add `app/paths/PathTuning.h` with path-related limits/constants.
   - Replace `rpc/detail/Tuning.h` includes in app (e.g., `PathRequest.cpp`) with `PathTuning.h`.
   - Have rpc use `PathTuning.h` or a thin wrapper if needed.

4. **Refactor HTTP/gRPC server wiring**
   - Define small app-level interfaces (e.g., `IHttpServer`, `IGrpcServer`) that `Application` and `NetworkOPs` depend on.
   - Implement these interfaces in `src/xrpld/rpc/` using existing `ServerHandler` / `GRPCServer` logic.
   - Provide factory functions in rpc that construct concrete servers, returning `std::unique_ptr<IHttpServer>` / `std::unique_ptr<IGrpcServer>`; declare the factories in an app header, implement them in rpc.

5. **Decouple `Main.cpp` from `RPCCall`**
   - Introduce an app-visible façade function (e.g., `int rpcCliMain(int argc, char** argv);`) declared under `app/main/`.
   - Implement `rpcCliMain` in `src/xrpld/rpc/` using existing `RPCCall` utilities.
   - Update `Main.cpp` to call the façade instead of including `rpc/RPCCall.h`.

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
