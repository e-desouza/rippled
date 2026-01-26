# Levelization Tasks - Cycle Removal Execution Plan

**Created:** 2026-01-26
**Last Updated:** 2026-01-26 (continued session)
**Status:** In Progress - Cycle 4 at 80%, Cycle 2 at 7%

## Overview

This document tracks the execution of the refined implementation plans for removing the remaining dependency cycles:
- **Cycle 4: app↔rpc** — Started at 15 deps, **now at 3 deps** (80% reduction)
- **Cycle 2: app↔overlay** — Started at 29 deps, **now at 27 deps** (7% reduction)

---

## Phase 1: Cycle 4 (app↔rpc)

### Step 1.1: Extract Path Tuning Constants
- [x] Create `src/xrpld/app/paths/PathTuning.h` with path constants
- [x] Update `PathRequest.cpp` to use new header
- [x] Build and verify
- [x] Run levelization check
- [x] Commit

**Status:** ✅ COMPLETE
**Commit:** `3cffec86a0`
**Notes:** Created PathTuning.h with maxSourceCurrencies and maxAutoSourceCurrencies. PathRequest.cpp no longer includes rpc/detail/Tuning.h.

### Step 1.2: Extract/Split JSON Helpers
- [x] Analyze `rpc/BookChanges.h` - confirm context-free
- [x] Move `BookChanges` to `include/xrpl/protocol/BookChanges.h`
- [x] Split `DeliveredAmount.h` into core (ledger module) + RPC wiring
- [x] Split `MPTokenIssuanceID.h` into core (protocol module) + RPC wiring
- [x] Update all includers
- [x] Build and verify
- [x] Run levelization check
- [x] Commit

**Status:** ✅ COMPLETE
**Commit:** `1e859a0ec7`
**Notes:**
- BookChanges moved to `include/xrpl/protocol/BookChanges.h` (template function)
- MPTokenIssuanceID split: core in `include/xrpl/protocol/MPTokenIssuanceID.h`, RPC wrapper remains
- DeliveredAmount: Tried protocol module first but it can't depend on ledger. Moved core to `include/xrpl/ledger/DeliveredAmount.h`
- RPC headers now act as backward-compatibility wrappers with `using` declarations

### Step 1.3: Decouple LedgerToJson from RPC::Context
- [x] Refactor LedgerFill struct to not depend on RPC::Context
- [x] Add primary constructor with explicit parameters (LedgerMaster*, apiVersion, journal)
- [x] Add apiVersion, j, validated fields directly to LedgerFill
- [x] Keep backward-compatible constructor for RPC context
- [x] Update all fill.context-> usages to use new fields
- [x] Build and verify
- [x] Run levelization check
- [x] Commit

**Status:** ✅ COMPLETE
**Commit:** `016920268a`
**Notes:**
- LedgerToJson.h no longer includes rpc/Context.h (only forward declaration)
- LedgerToJson.cpp still includes it for backward-compat constructor (acceptable)
- This was the key header dependency from app→rpc

### Step 1.4: Refactor HTTP/gRPC Server Wiring
- [x] Move GRPCServer.h/.cpp to rpc module
- [x] Update include paths in Application.cpp
- [x] Build and verify
- [x] Run levelization check
- [x] Commit

**Status:** ✅ COMPLETE
**Commit:** `72377f773b`
**Notes:**
- Moved `GRPCServer.h` from `app/main/` to `rpc/` module
- Moved `GRPCServer.cpp` from `app/main/` to `rpc/detail/`
- GRPCServer belongs in rpc module since it's RPC infrastructure

### Step 1.5: Decouple Main.cpp from RPCCall
- [x] Create `src/xrpld/app/main/RpcCliMain.h`
- [x] Implement in `src/xrpld/rpc/detail/RpcCliMain.cpp`
- [x] Update `Main.cpp` to use RpcCliMain.h
- [x] Complete LedgerFill refactoring (remove backward-compat constructor)
- [x] Update all LedgerFill callers to use new constructors
- [x] Build and verify
- [x] Run levelization check
- [x] Commit

**Status:** ✅ COMPLETE
**Commit:** `72377f773b`
**Notes:**
- Created RpcCliMain façade for Main.cpp
- Removed backward-compatible LedgerFill constructor that took RPC::Context*
- Added simple constructor for non-RPC uses
- Updated all callers to use explicit constructors:
  - Non-RPC: `LedgerFill{*ledger}` or `LedgerFill{*ledger, options}`
  - RPC: `LedgerFill(*ledger, &context.ledgerMaster, apiVersion, journal, options)`

### Cycle 4 Current Status
- [x] Run levelization check
- [ ] Create server interfaces for remaining deps (optional)
- [ ] Final verification

**Status:** ⏸ PAUSED - Significant progress made
**Notes:**
- **app→rpc dependencies: 15 → 3 (80% reduction)**
- Remaining dependencies:
  1. `Application.cpp` → `rpc/GRPCServer.h` (creates GRPCServer)
  2. `Application.cpp` → `rpc/ServerHandler.h` (creates ServerHandler)
  3. `NetworkOPs.cpp` → `rpc/ServerHandler.h` (uses `setup().ports`)
- These are "wiring" dependencies in Application's infrastructure code
- Further reduction would require factory interfaces, which may not be worth the complexity
- The cycle loop still appears because there are bidirectional dependencies (rpc→app: 144 deps)
- The app→rpc direction is now minimal and localized to Application.cpp

---

## Phase 2: Cycle 2 (app↔overlay)

### Step 2.1: Classify Overlay→App Usage (Analysis)
- [x] Audit PeerImp.cpp
- [x] Audit OverlayImpl.cpp
- [x] Audit PeerSet.cpp
- [x] Audit Handshake.cpp
- [x] Audit PeerReservationTable.cpp
- [x] Document interface method lists

**Status:** ✅ COMPLETE
**Notes:**

#### PeerImp.cpp (11 app includes, 87 usage sites)
| Interface Category | App Methods Used |
|-------------------|------------------|
| **IOverlayLedgerOps** | `LedgerMaster::getValidatedLedgerAge`, `getValidLedgerIndex`, `haveLedger`, `getLedgerByHash`, `getLedgerBySeq`, `getClosedLedger`, `addFetchPack`, `gotFetchPack`, `makeFetchPack`, `getEarliestFetch`, `getValidatedRules` |
| **IOverlayTxOps** | `HashRouter::shouldProcess`, `addSuppressionPeer`, `addSuppressionPeerWithStatus`, `shouldRelay`, `setFlags`; `NetworkOPs::processTransaction`; `InboundTransactions::gotData`, `getSet` |
| **IOverlayValidationOps** | `ValidatorList::for_each_available`, `sendValidatorList`, `parseBlobs` |
| **IOverlayInboundOps** | `InboundLedgers::gotLedgerData` |
| **IOverlayFeeOps** | `LoadFeeTrack::isLoadedLocal` |

#### OverlayImpl.cpp (8 app includes)
| Interface Category | App Methods Used |
|-------------------|------------------|
| **IOverlayAppInfo** | `Application` (general), `ServerCounts`, `NetworkOPs` |
| **IOverlayValidatorOps** | `ValidatorList`, `ValidatorSite` |
| **IOverlayHashOps** | `HashRouter` |
| **IOverlayDbOps** | `RelationalDatabase`, `Wallet` |

#### Handshake.cpp (2 app includes)
| Interface Category | App Methods Used |
|-------------------|------------------|
| **IOverlayLedgerOps** | `LedgerMaster::getValidatedLedger`, `getClosedLedger` |
| **IOverlayAppInfo** | `Application` (for ledger master access) |

#### PeerReservationTable.cpp (2 app includes)
| Interface Category | App Methods Used |
|-------------------|------------------|
| **IOverlayDbOps** | `RelationalDatabase`, `Wallet` |

#### PeerSet.h/cpp (1 app include)
| Interface Category | App Methods Used |
|-------------------|------------------|
| **IOverlayAppInfo** | `Application&` member for general access |

---

**Conclusion:** Need 6-7 interfaces to fully decouple:
1. `IOverlayLedgerOps` - Ledger access and management
2. `IOverlayTxOps` - Transaction routing, hash router, inbound txs
3. `IOverlayValidationOps` - Validator list operations
4. `IOverlayInboundOps` - Inbound ledger data handling
5. `IOverlayFeeOps` - Load fee tracking
6. `IOverlayDbOps` - Database and wallet access
7. `IOverlayAppInfo` - General app info (optional, may inline)

### Step 2.2: Define Overlay Dependency Interfaces
- [x] Extract `HashRouterFlags` to `include/xrpl/basics/HashRouterFlags.h`
- [x] Update `PeerImp.h` to use shared flags header (removes 1 app include)
- [x] Create `IHashRouterOps.h` interface skeleton
- [x] Create `IFeeTrackOps.h` interface skeleton
- [x] Replace `Application.h` include with forward declaration in `PeerSet.h`
- [ ] Create `IOverlayLedgerOps.h`
- [ ] Create `IOverlayTxOps.h`
- [ ] Create `IOverlayValidationOps.h`
- [ ] Create `IOverlayReservationOps.h`
- [ ] Create `OverlayDeps.h` aggregate
- [ ] Build and verify
- [ ] Commit

**Status:** 🔄 IN PROGRESS (29→27 deps, 7% reduction)
**Notes:**
- Committed `6a77611371`: HashRouterFlags extraction, dependency 29→28
- Committed `62d7deb388`: PeerSet.h forward declaration, dependency 28→27
- Committed `f8a0878fc9`: LedgerReplayMsgHandler forward declaration in PeerImp.h
- Key challenges identified:
  1. `Ledger` type returned by LedgerMaster methods (would need `ReadView const*`)
  2. `makeFetchPack(weak_ptr<Peer>)` creates reverse dependency app→overlay
  3. `RCLCxPeerPos` passed by value (requires full type)
- Header-level deps remaining in PeerImp.h: RCLCxPeerPos.h, Application.h
- Alternative approach: focus on moving more includes from headers to .cpp files

**Completed Refactorings:**
- **SUCCESS: unique_ptr for LedgerReplayMsgHandler** - Refactored to use forward declaration:
  - Changed `LedgerReplayMsgHandler ledgerReplayMsgHandler_` to `std::unique_ptr<LedgerReplayMsgHandler>`
  - Moved template constructor implementation from header to `PeerImp.cpp`
  - Added explicit template instantiation for `boost::beast::basic_multi_buffer<std::allocator<char>>::subrange<true>`
  - Removed `LedgerReplayMsgHandler.h` include from `PeerImp.h`
  - **Important:** This reduces header coupling but does NOT reduce levelization count
    because `PeerImp.cpp` still needs the complete type (include moved from .h to .cpp)

**Analysis: Why Further Header Reduction is Difficult:**

The remaining header-level dependencies in `PeerImp.h` cannot be easily removed:

1. **`Application.h`** - Required because:
   - `Ledger` type is used in method signatures (`sendLedgerBase`, `getLedger`)
   - `.cpp` files that include `PeerImp.h` access `app_.config()`, `app_.cluster()`, etc.
   - Would require extracting 50+ methods to interfaces

2. **`RCLCxPeerPos.h`** - Required because:
   - `RCLCxPeerPos` is passed by value to `checkPropose()`
   - Changing to `const&` would require changes across 5+ interfaces (NetworkOPs, IConsensusCoordinator, etc.)

**Conclusion for Step 2.2:**
The low-hanging fruit has been picked (29→27 deps). Further reduction requires the interface extraction approach in Steps 2.3-2.5, which is a larger architectural change.

### Step 2.3: Move Message Handlers to App
- [ ] Create `app/overlay/handlers/` directory
- [ ] Extract validation message handling
- [ ] Extract transaction message handling
- [ ] Update PeerImp.cpp to use interfaces
- [ ] Build and verify
- [ ] Run levelization check
- [ ] Commit

**Status:** Not Started  
**Notes:**

### Step 2.4: Introduce OverlayDeps and Refactor Constructors
- [ ] Update PeerImp constructor
- [ ] Update PeerSet constructor
- [ ] Update OverlayImpl constructor
- [ ] Update Handshake
- [ ] Build and verify
- [ ] Run levelization check
- [ ] Commit

**Status:** Not Started  
**Notes:**

### Step 2.5: Implement App-Side Adapters
- [ ] Create OverlayLedgerOpsImpl
- [ ] Create OverlayTxOpsImpl
- [ ] Create OverlayValidationOpsImpl
- [ ] Create OverlayReservationOpsImpl
- [ ] Update Application.cpp
- [ ] Build and verify
- [ ] Run levelization check
- [ ] Commit

**Status:** Not Started  
**Notes:**

### Step 2.6: Clean Up and Verify
- [ ] Remove remaining direct app includes
- [ ] Run full levelization
- [ ] Confirm no "Loop: xrpld.app xrpld.overlay"
- [ ] Final commit for Cycle 2

**Status:** Not Started  
**Notes:**

---

## Phase 3: Final Verification

- [ ] Run full levelization
- [ ] Run full build
- [ ] Run tests
- [ ] Final commit

**Status:** Not Started  
**Notes:**

---

## Error Recovery Log

| Step | Error | Resolution | Date |
|------|-------|------------|------|
| | | | |

---

## Commit Log

| Commit | Message | Date |
|--------|---------|------|
| `3cffec86a0` | [Levelization] Extract path tuning constants to app/paths | 2026-01-26 |
| `1e859a0ec7` | [Levelization] Extract JSON helpers to lower-level modules | 2026-01-26 |
| `016920268a` | [Levelization] Decouple LedgerToJson.h from RPC::Context | 2026-01-26 |

