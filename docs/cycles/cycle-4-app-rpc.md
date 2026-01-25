# Cycle 4: xrpld.app ↔ xrpld.rpc

## Current State

**Loop detected:** `xrpld.rpc > xrpld.app`

The rpc module has 133 includes from app, and app has 24 includes from rpc.

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

1. **InfoSub.h** is in rpc/ but used by app for pub/sub subscriptions
2. **JSON utility functions** (DeliveredAmount, CTID, MPTokenIssuanceID, BookChanges) are in rpc/ but used by app
3. **Context.h** is in rpc/ but used by app for ledger JSON conversion
4. **ServerHandler** is in rpc/ but created by Application

## Removal Strategy

### Step 1: Move InfoSub to app/misc

`InfoSub` is a subscription interface used throughout app. It belongs in app, not rpc.

**Move:** `src/xrpld/rpc/InfoSub.h` → `src/xrpld/app/misc/InfoSub.h`

**Update all consumers** (grep for `rpc/InfoSub.h`):

- `app/ledger/BookListeners.h`
- `app/paths/PathRequest.h`
- `app/misc/IPubSubManager.h`
- `app/misc/NetworkOPs.h`
- `rpc/handlers/Subscribe.cpp` (will now include from app)

### Step 2: Move JSON utility functions to xrpl/json or app/misc

These are pure utility functions that don't depend on RPC:

**Move to `src/xrpl/json/` (protocol-level utilities):**

- `rpc/CTID.h` → `xrpl/json/CTID.h`
- `rpc/DeliveredAmount.h` → `xrpl/json/DeliveredAmount.h`
- `rpc/MPTokenIssuanceID.h` → `xrpl/json/MPTokenIssuanceID.h`
- `rpc/BookChanges.h` → `xrpl/json/BookChanges.h`

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
