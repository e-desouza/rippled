# Levelization Tasks - Cycle Removal Execution Plan

**Created:** 2026-01-26
**Last Updated:** 2026-01-26 (Tier 1 complete, Tier 2 revised)
**Status:** SIGNIFICANT PROGRESS - Cycle 4 at 87%, Cycle 2 at 59%

## Overview

This document tracks the execution of the refined implementation plans for reducing the dependency cycles:
- **Cycle 4: app↔rpc** — Started at 15 app→rpc deps, **now at 2 deps** (87% reduction)
- **Cycle 2: app↔overlay** — Started at 29 overlay→app deps, **now at 12 deps** (59% reduction)

**Note:** Both cycles still exist in the levelization tool output because complete removal requires:
- Cycle 4: Moving remaining 2 deps (ServerHandler.h, GRPCServer.h) or making them interfaces
- Cycle 2: Interface-based dependency inversion for 12 remaining app components

## Current State (After Tier 1)

### Remaining 12 overlay→app Dependencies

| File | Include | Usage |
|------|---------|-------|
| OverlayImpl.cpp | Application.h | config, journal, validators, manifests |
| OverlayImpl.cpp | ServerCounts.h | `getCountsJson()` for crawl |
| OverlayImpl.cpp | Wallet.h | `addValidatorManifest()` |
| OverlayImpl.cpp | HashRouter.h | `shouldRelay()`, `addSuppression()` |
| OverlayImpl.cpp | ValidatorList.h | `listed()`, `getJson()`, `getAvailable()` |
| OverlayImpl.cpp | ValidatorSite.h | `getJson()` |
| PeerImp.cpp | LedgerReplayMsgHandler.h | `processXxx()` methods |
| PeerImp.cpp | InboundLedgers.h | `gotLedgerData()` |
| PeerImp.cpp | InboundTransactions.h | `gotData()`, `getSet()` |
| PeerImp.cpp | LedgerMaster.h | 15 calls, 7 distinct methods |
| PeerImp.h | Application.h | `Application& app_` member |
| PeerSet.cpp | Application.h | `overlay()`, `journal()` |

### Remaining 2 app→rpc Dependencies

| File | Include | Usage |
|------|---------|-------|
| Application.cpp | GRPCServer.h | Creates GRPCServer |
| Application.cpp | ServerHandler.h | Creates ServerHandler |

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

**Status:** ✅ COMPLETE (29→27 deps in Step 2.2, further reduced to 26 in Step 2.3)
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
- [x] Investigate existing ValidationMessageHandler pattern
- [ ] Wire up ValidationMessageHandler delegation
- [ ] Remove duplicate validation code from PeerImp.cpp
- [ ] Extract transaction message handling similarly
- [ ] Update PeerImp.cpp to use interfaces
- [ ] Build and verify
- [ ] Run levelization check
- [ ] Commit

**Status:** ✅ COMPLETE (27→20 deps)
**Commits:** `1531f3ad96`, `b9e14bca93`, `242bacd321`, `339111374b`, `3aa7e5ebb0`, `0b8794e537`, `6946f36d87`
**Notes:**

**Completed: Wired up ValidationMessageHandler delegation**
- Replaced ~160 lines of duplicate validation handling code in PeerImp.cpp
- Removed `PeerImp::onValidatorListMessage()` helper method entirely
- PeerImp now delegates to ValidationMessageHandler for all validation messages
- Removed `RCLValidations.h` include from PeerImp.cpp (no longer needed)
- **Dependencies reduced: 27→26**

**Completed: Created TransactionMessageHandler**
- Created `TransactionMessageHandler.h` in `overlay/detail/handlers/`
- Created `TransactionMessageHandler.cpp` in `app/overlay/handlers/`
- Extracted `handleTransaction` and `checkTransaction` methods (~290 lines)
- PeerImp now delegates TMTransaction and TMTransactions to handler
- Removed `apply.h` include from PeerImp.cpp (no longer needed)
- **Dependencies reduced: 26→25**

**Completed: Extended TransactionMessageHandler**
- Added `handleHaveTransactions` and `doTransactions` methods
- Moved these methods from PeerImp.cpp to TransactionMessageHandler.cpp
- Removed `Transaction.h` and `TransactionMaster.h` includes from PeerImp.cpp
- **Dependencies reduced: 25→23**

**Completed: Created ProposalMessageHandler**
- Extracted proposal message handling from PeerImp to ProposalMessageHandler
- Created `ProposalMessageHandler.h` in `overlay/detail/handlers/`
- Created `ProposalMessageHandler.cpp` in `app/overlay/handlers/`
- Moved `onMessage(TMProposeSet)` and `checkPropose` methods
- **Note:** `ValidatorList.h` still needed in PeerImp.cpp for validator list propagation (separate from proposals)
- Code organization improvement, enabled RCLCxPeerPos.h removal from header

**Completed: Remove RCLCxPeerPos.h from PeerImp.h**
- After moving `checkPropose` to handler, `RCLCxPeerPos` type no longer needed in header
- Removed include from `PeerImp.h`
- **Dependencies reduced: 23→22**

**Completed: Created ValidatorListPropagationHandler**
- Extracted validator list propagation code from `PeerImp::onActivate()` to handler
- Created `ValidatorListPropagationHandler.h` in `overlay/detail/handlers/`
- Created `ValidatorListPropagationHandler.cpp` in `app/overlay/handlers/`
- Removed `ValidatorList.h` include from PeerImp.cpp
- **Dependencies reduced: 22→21**
- **Commit:** `0b8794e537`

**Completed: Created StatusChangeMessageHandler**
- Extracted `pubPeerStatus` call from `PeerImp::onMessage(TMStatusChange)` to handler
- Created `StatusChangeMessageHandler.h` in `overlay/detail/handlers/`
- Created `StatusChangeMessageHandler.cpp` in `app/overlay/handlers/`
- Removed `NetworkOPs.h` include from PeerImp.cpp
- Added explicit `JobQueue.h` include (xrpl.core, not app dependency)
- **Dependencies reduced: 21→20**
- **Commit:** `6946f36d87`

**Completed: Removed unused includes**
- Removed unused `HashRouter.h` from PeerImp.cpp (20→19) - `0a7d5b55a5`
- Removed unused `RelationalDatabase.h` from OverlayImpl.cpp (19→18) - `16fb83dcab`
- Removed unused `RelationalDatabase.h` from PeerReservationTable.cpp (18→17) - `4705d78880`

**Remaining overlay→app dependencies: 17 (41% reduction from 29)**

**Breakdown by file:**
- `OverlayImpl.cpp` (7): Application.h, NetworkOPs.h, ServerCounts.h, Wallet.h, HashRouter.h, ValidatorList.h, ValidatorSite.h
- `PeerImp.cpp` (5): InboundLedgers.h, InboundTransactions.h, LedgerMaster.h, LedgerReplayMsgHandler.h, LoadFeeTrack.h
- `Handshake.cpp` (2): LedgerMaster.h, Application.h
- `PeerReservationTable.cpp` (1): Wallet.h
- `PeerImp.h` (1): Application.h
- `PeerSet.cpp` (1): Application.h

**Analysis: Remaining dependencies are deeply integrated:**
- LedgerMaster.h used 14 times in PeerImp.cpp for ledger state access
- InboundLedgers/InboundTransactions for active fetching
- HashRouter for relay logic in OverlayImpl.cpp (shouldRelay calls)
- ValidatorList/ValidatorSite for validator UNL queries
- These would require interface-based dependency inversion to remove

---

## Refined Implementation Plan (2026-01-26 Re-Analysis)

After 5 comprehensive re-analysis passes, the following refined plan addresses the remaining 17 overlay→app and 3 app→rpc dependencies.

### Key Insights from Re-Analysis

1. **Levelization tool detects DIRECT includes only** - not transitive. Breaking direct includes breaks the cycle.
2. **Interfaces in consumer module break cycles** - If overlay defines `IFoo` and app implements it, app→overlay is allowed (L8→L7).
3. **Existing interfaces are underutilized** - `IHashRouterOps`, `IFeeTrackOps`, `LedgerDataProvider` exist but aren't fully wired.
4. **Handler pattern works** - Moving message handlers to app module successfully reduced deps (29→17).
5. **Some proposed solutions were flawed** - Moving peer reservation DB to overlay would create new cycles.

### Prioritized Implementation (Tier 1: Low Risk, High Impact) ✅ COMPLETE

#### Step 2.4.1: Wire up IFeeTrackOps ✅
- [x] Extended `IFeeTrackOps` with `setClusterFee()` method
- [x] Created `LoadFeeTrackAdapter` in `app/overlay/adapters/`
- [x] Injected into OverlayImpl via constructor
- [x] PeerImp uses `overlay_.feeTrackOps()`
- [x] Removed `LoadFeeTrack.h` include from PeerImp.cpp

**Commit:** `bbd065c7e7` | **Impact:** 17→16 deps

#### Step 2.4.2: Create IHandshakeParams interface ✅
- [x] Created `IHandshakeParams` interface in `overlay/`
- [x] Created `HandshakeParamsAdapter` in `app/overlay/adapters/`
- [x] Refactored Handshake.h/cpp to use interface
- [x] Updated callers: OverlayImpl, PeerImp, ConnectAttempt
- [x] Updated test files: compression_test, LedgerReplay_test, reduce_relay_test

**Commit:** `f2e798ee40` | **Impact:** 16→14 deps

#### Step 2.4.3: Inject IPeerReservationStorage ✅
- [x] Created `IPeerReservationStorage` interface in `overlay/`
- [x] Created `PeerReservationStorageAdapter` in `app/overlay/adapters/`
- [x] Updated PeerReservationTable to use interface
- [x] Removed `Wallet.h` include from PeerReservationTable.cpp

**Commit:** `246b19a463` | **Impact:** 14→13 deps

#### Step 2.4.4: Add getServerPorts() to Application ✅
- [x] Added `getServerPorts()` to Application interface
- [x] Implemented in ApplicationImpl
- [x] Updated NetworkOPs.cpp to use `app_.getServerPorts()`
- [x] Removed `ServerHandler.h` include from NetworkOPs.cpp

**Commit:** `1a8e49ca30` | **Impact:** app→rpc 3→2 deps

#### Step 2.4.5: Create IOverlayOps interface ✅
- [x] Created `IOverlayOps` interface with `pubManifest()` and `getServerInfo()`
- [x] Created `OverlayOpsHandler` (header in overlay, impl in app)
- [x] Injected into OverlayImpl via constructor
- [x] Removed `NetworkOPs.h` include from OverlayImpl.cpp
- [x] Added `jss.h` include for JSON constants

**Commit:** `a28f6142c7` | **Impact:** 13→12 deps

**Tier 1 Total:** ✅ COMPLETE - Removed 5 dependencies (17→12 overlay→app, 3→2 app→rpc)

---

### Prioritized Implementation (Tier 2: Medium Risk) - REVISED

Based on re-analysis of remaining 12 dependencies, Tier 2 is reorganized by risk/complexity.

#### Step 2.5.1: Extend IOverlayOps (removes ServerCounts.h, Wallet.h) ⭐ EASIEST
- [ ] Add `getServerCounts(int minObjectCount)` to IOverlayOps interface
- [ ] Add `saveValidatorManifest(std::string const& serialized)` to IOverlayOps
- [ ] Update OverlayOpsHandler implementation
- [ ] Update OverlayImpl.cpp to use new methods
- [ ] Remove `ServerCounts.h` and `Wallet.h` includes
- [ ] Build and verify
- [ ] Commit

**Estimated:** 2 hours | **Risk:** LOW | **Impact:** 2 deps removed

#### Step 2.5.2: Create IHashRouterOps interface (removes HashRouter.h)
- [ ] Create `IHashRouterOps` interface in `overlay/` with:
  - `shouldRelay(uint256) -> std::optional<std::set<Peer::id_t>>`
  - `addSuppression(uint256)`
- [ ] Create `HashRouterAdapter` in `app/overlay/adapters/`
- [ ] Add to OverlayImpl constructor injection
- [ ] Update relay methods in OverlayImpl.cpp
- [ ] Remove `HashRouter.h` include
- [ ] Build and verify
- [ ] Commit

**Estimated:** 3 hours | **Risk:** LOW-MEDIUM | **Impact:** 1 dep removed

#### Step 2.5.3: Create IValidatorOps interface (removes ValidatorList.h, ValidatorSite.h)
- [ ] Create `IValidatorOps` interface in `overlay/` with:
  - `isValidatorListed(PublicKey) -> bool`
  - `getValidatorsJson() -> Json::Value`
  - `getValidatorSitesJson() -> Json::Value`
  - `getValidatorListAvailable(key, version) -> std::optional<...>`
- [ ] Create `ValidatorOpsAdapter` in `app/overlay/adapters/`
- [ ] Add to OverlayImpl constructor injection
- [ ] Update `getUnlInfo()`, `processValidatorList()`, `onManifests()`
- [ ] Remove `ValidatorList.h` and `ValidatorSite.h` includes
- [ ] Build and verify
- [ ] Commit

**Estimated:** 4 hours | **Risk:** MEDIUM | **Impact:** 2 deps removed

#### Step 2.5.4: Forward declare Application in PeerImp.h (removes Application.h from header)
- [ ] Replace `#include <xrpld/app/main/Application.h>` with forward declaration
- [ ] Move include to PeerImp.cpp
- [ ] Verify no inline methods use Application methods
- [ ] Build and verify
- [ ] Commit

**Estimated:** 1 hour | **Risk:** LOW | **Impact:** 1 dep removed (header→cpp)

#### Step 2.5.5: Inject Overlay& into PeerSet (removes Application.h from PeerSet.cpp)
- [ ] Change PeerSet constructor to take `Overlay&` and `beast::Journal` instead of `Application&`
- [ ] Update `make_PeerSetBuilder()` signature
- [ ] Update callers to pass `app.overlay()` and `app.journal("PeerSet")`
- [ ] Remove `Application.h` include from PeerSet.cpp
- [ ] Build and verify
- [ ] Commit

**Estimated:** 2 hours | **Risk:** LOW | **Impact:** 1 dep removed

#### Step 2.5.6: Create LedgerDataMessageHandler (removes InboundLedgers.h, InboundTransactions.h)
- [ ] Create `LedgerDataMessageHandler.h` in `overlay/detail/handlers/`
- [ ] Create `LedgerDataMessageHandler.cpp` in `app/overlay/handlers/`
- [ ] Extract `onMessage(TMLedgerData)` logic from PeerImp.cpp
- [ ] Handle both ledger data and TX set data paths
- [ ] Remove `InboundLedgers.h` and `InboundTransactions.h` includes
- [ ] Build and verify
- [ ] Commit

**Note:** `getSet()` usage in `getTxSet()` may need separate interface (ITxSetProvider)

**Estimated:** 5 hours | **Risk:** MEDIUM | **Impact:** 2 deps removed

**Tier 2 Total:** ~17 hours, removes 9 dependencies (12→3 overlay→app)

---

### Prioritized Implementation (Tier 3: Higher Complexity)

After Tier 2, remaining 3 dependencies:
- `LedgerMaster.h` in PeerImp.cpp (15 calls, 7 methods)
- `LedgerReplayMsgHandler.h` in PeerImp.cpp
- `Application.h` in OverlayImpl.cpp (after all other deps removed)

#### Step 2.6.1: Create ILedgerReplayHandler interface (removes LedgerReplayMsgHandler.h)
- [ ] Create `ILedgerReplayHandler` interface in `overlay/`
- [ ] Create factory function in app module
- [ ] Inject into PeerImp via OverlayImpl
- [ ] Remove `LedgerReplayMsgHandler.h` include from PeerImp.cpp
- [ ] Build and verify
- [ ] Commit

**Estimated:** 3 hours | **Risk:** MEDIUM | **Impact:** 1 dep removed

#### Step 2.6.2: Create ILedgerDataOps interface (removes LedgerMaster.h)
- [ ] Create `ILedgerDataOps` interface in `overlay/` with:
  - Validation state: `getValidatedLedgerAge()`, `getValidLedgerIndex()`, `getEarliestFetch()`
  - Lookups: `getLedgerByHash()`, `getLedgerBySeq()`, `getClosedLedger()`, `haveLedger()`
  - Fetch pack: `addFetchPack()`, `gotFetchPack()`, `makeFetchPack()`
- [ ] Create `LedgerDataOpsAdapter` in `app/overlay/adapters/`
- [ ] Inject into PeerImp via OverlayImpl
- [ ] Update all 15 LedgerMaster usages in PeerImp.cpp
- [ ] Remove `LedgerMaster.h` include
- [ ] Build and verify
- [ ] Commit

**Estimated:** 6 hours | **Risk:** HIGH | **Impact:** 1 dep removed

#### Step 2.6.3: Remove final Application.h from OverlayImpl.cpp
- [ ] After all other deps removed, analyze remaining Application usage
- [ ] Create minimal `IOverlayContext` if needed for config/journal/timeKeeper
- [ ] Or pass these directly via constructor
- [ ] Remove `Application.h` include
- [ ] Build and verify
- [ ] Commit

**Estimated:** 4 hours | **Risk:** MEDIUM | **Impact:** 1 dep removed

**Tier 3 Total:** ~13 hours, removes 3 dependencies (3→0 overlay→app)

---

### Cycle 4 Completion (app→rpc: 2→0) - OPTIONAL

#### Step 1.6: Remove remaining app→rpc dependencies
- [ ] Create `IRPCServerFactory` interface for ServerHandler/GRPCServer creation
- [ ] Move factory to rpc module, inject into Application
- [ ] Remove `ServerHandler.h` and `GRPCServer.h` includes from Application.cpp
- [ ] Build and verify
- [ ] Commit

**Estimated:** 6 hours | **Risk:** MEDIUM-HIGH | **Impact:** 2 deps removed

**Note:** This is optional. The 2 remaining deps in Application.cpp are "wiring" code that creates RPC infrastructure. They're localized and may be acceptable.

---

### Summary: Expected Final State (REVISED)

| Cycle | Start | After Tier 1 | After Tier 2 | After Tier 3 |
|-------|-------|--------------|--------------|--------------|
| overlay→app | 29 | 12 ✅ | 3 | 0 |
| app→rpc | 15 | 2 ✅ | 2 | 0 (optional) |

**Total Estimated Time:** ~36 hours for complete cycle removal (Tier 2 + Tier 3)

---

### Step 2.4: Tier 1 Implementation (Low Risk) ✅ COMPLETE
- [x] Step 2.4.1: Wire up IFeeTrackOps - `bbd065c7e7`
- [x] Step 2.4.2: Create IHandshakeParams interface - `f2e798ee40`
- [x] Step 2.4.3: Inject IPeerReservationStorage - `246b19a463`
- [x] Step 2.4.4: Add getServerPorts() to Application - `1a8e49ca30`
- [x] Step 2.4.5: Create IOverlayOps interface - `a28f6142c7`

**Status:** ✅ COMPLETE
**Notes:** Removed 5 dependencies (17→12 overlay→app, 3→2 app→rpc). See lessons learned in `TIER1_LESSONS_LEARNED.md`.

### Step 2.5: Tier 2 Implementation (Medium Risk)
- [ ] Step 2.5.1: Extend IOverlayOps (ServerCounts.h, Wallet.h)
- [ ] Step 2.5.2: Create IHashRouterOps (HashRouter.h)
- [ ] Step 2.5.3: Create IValidatorOps (ValidatorList.h, ValidatorSite.h)
- [ ] Step 2.5.4: Forward declare Application in PeerImp.h
- [ ] Step 2.5.5: Inject Overlay& into PeerSet
- [ ] Step 2.5.6: Create LedgerDataMessageHandler (InboundLedgers.h, InboundTransactions.h)

**Status:** Not Started
**Notes:** See detailed implementation plan in "Prioritized Implementation (Tier 2)" section above.

### Step 2.6: Tier 3 Implementation (Higher Complexity)
- [ ] Step 2.6.1: Create ILedgerReplayHandler (LedgerReplayMsgHandler.h)
- [ ] Step 2.6.2: Create ILedgerDataOps (LedgerMaster.h)
- [ ] Step 2.6.3: Remove final Application.h from OverlayImpl.cpp

**Status:** Not Started
**Notes:**

---

## Phase 3: Final Verification

- [ ] Run full levelization - confirm no cycles
- [ ] Run full build with `ninja -j$(nproc)`
- [ ] Run unit tests
- [ ] Run integration tests
- [ ] Final commit

**Status:** Not Started
**Notes:**

---

## Interface Design Conventions

Based on analysis of existing codebase patterns, follow these conventions:

### Naming
- **Interface prefix:** `I` (e.g., `ILedgerProvider`, `IFeeTrackOps`)
- **Implementation suffix:** `Impl` (e.g., `LedgerProviderImpl`)
- **Adapter suffix:** `Adapter` for wrappers (e.g., `LoadFeeTrackAdapter`)

### File Locations
| Type | Location |
|------|----------|
| Interface (consumer-owned) | `src/xrpld/<consumer-module>/I<Name>.h` |
| Implementation | `src/xrpld/<provider-module>/detail/<Name>Impl.h` |
| Adapter | `src/xrpld/app/overlay/adapters/<Name>Adapter.cpp` |
| Handler header | `src/xrpld/overlay/detail/handlers/<Name>Handler.h` |
| Handler impl | `src/xrpld/app/overlay/handlers/<Name>Handler.cpp` |

### Interface Template
```cpp
// src/xrpld/overlay/IFooOps.h
#ifndef XRPLD_OVERLAY_IFOOOPS_H_INCLUDED
#define XRPLD_OVERLAY_IFOOOPS_H_INCLUDED

namespace xrpl {

class IFooOps
{
public:
    virtual ~IFooOps() = default;

    [[nodiscard]] virtual bool
    someMethod() const = 0;

    virtual void
    anotherMethod(int param) = 0;
};

}  // namespace xrpl

#endif
```

### Dependency Aggregation Pattern
For modules needing multiple interfaces, use a deps struct:
```cpp
// src/xrpld/overlay/OverlayDeps.h
struct OverlayDeps
{
    IFeeTrackOps& feeTrackOps;
    IHashRouterOps& hashRouterOps;
    ILedgerAccess& ledgerAccess;
    // ... other interfaces
};
```

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
| `72377f773b` | [Levelization] Move GRPCServer to rpc module | 2026-01-26 |
| `6a77611371` | [Levelization] Extract HashRouterFlags | 2026-01-26 |
| `62d7deb388` | [Levelization] Forward-declare Application in PeerSet.h | 2026-01-26 |
| `f8a0878fc9` | [Levelization] Forward-declare LedgerReplayMsgHandler in PeerImp.h | 2026-01-26 |
| `1531f3ad96` | [Levelization] Wire up ValidationMessageHandler | 2026-01-26 |
| `b9e14bca93` | [Levelization] Create TransactionMessageHandler | 2026-01-26 |
| `242bacd321` | [Levelization] Extend TransactionMessageHandler (handleHaveTransactions, doTransactions) | 2026-01-26 |
| `9b0c01edee` | [Levelization] Move checkValidation to ValidationMessageHandler | 2026-01-26 |
| `339111374b` | [Levelization] Create ProposalMessageHandler | 2026-01-26 |
| `3aa7e5ebb0` | [Levelization] Remove RCLCxPeerPos.h from PeerImp.h (23→22 deps) | 2026-01-26 |
| `0b8794e537` | [Levelization] Create ValidatorListPropagationHandler (22→21 deps) | 2026-01-26 |
| `6946f36d87` | [Levelization] Create StatusChangeMessageHandler (21→20 deps) | 2026-01-26 |
| `0a7d5b55a5` | [Levelization] Remove unused HashRouter.h from PeerImp.cpp (20→19 deps) | 2026-01-26 |
| `16fb83dcab` | [Levelization] Remove unused RelationalDatabase.h from OverlayImpl.cpp (19→18 deps) | 2026-01-26 |
| `4705d78880` | [Levelization] Remove unused RelationalDatabase.h from PeerReservationTable.cpp (18→17 deps) | 2026-01-26 |
| `bbd065c7e7` | [Levelization] Step 2.4.1: Wire up IFeeTrackOps (17→16 deps) | 2026-01-26 |
| `f2e798ee40` | [Levelization] Step 2.4.2: Create IHandshakeParams interface (16→14 deps) | 2026-01-26 |
| `246b19a463` | [Levelization] Step 2.4.3: Inject IPeerReservationStorage (14→13 deps) | 2026-01-26 |
| `1a8e49ca30` | [Levelization] Step 2.4.4: Add getServerPorts() to Application (app→rpc 3→2) | 2026-01-26 |
| `a28f6142c7` | [Levelization] Step 2.4.5: Create IOverlayOps interface (13→12 deps) | 2026-01-26 |

