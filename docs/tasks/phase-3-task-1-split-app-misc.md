# Phase 3 Task 3.1: Split app/misc into Focused Modules

## Overview

### Problem Statement

The `src/xrpld/app/misc/` directory has grown into a **catch-all dumping ground** containing 51 files covering many unrelated concerns:
- AMM (Automated Market Maker) utilities
- Validator management
- Transaction queue and routing
- Network operations
- Fee management
- Amendment voting
- SHAMap store management

This creates several problems:
1. **Implicit coupling**: Files with unrelated functionality live together, making it easy to add inappropriate dependencies
2. **Navigation difficulty**: Developers must search through 51 files to find relevant code
3. **Unclear module boundaries**: No clear ownership or responsibility for the directory
4. **Hidden dependencies**: The flat structure obscures which components actually depend on each other

### Why Phase 3?

This task depends on Phase 2 interface work because:
1. **NetworkOPs decoupling** (Phase 2): NetworkOPs.cpp is the most-imported file (34 includes) and has deep dependencies on many other misc files
2. **LedgerMaster abstraction** (Phase 2 Task 2.4): Several misc files depend on LedgerMaster which needs abstraction first
3. **Cleaner boundaries**: Phase 2 interfaces make it clearer which files belong together

### Success Criteria

1. ✅ `app/misc/` reduced to ≤15 general-purpose utility files
2. ✅ New focused modules created: `app/amm/`, `app/validators/`, `app/txqueue/`
3. ✅ All include paths updated throughout codebase
4. ✅ CMakeLists.txt updated with new module organization
5. ✅ Build compiles successfully
6. ✅ All tests pass
7. ✅ Levelization check passes (no new cycles introduced)

---

## Deep Code Analysis

### Current File Inventory (51 files)

#### Public Headers in `app/misc/` (21 files)

| File | Category | Purpose | External Imports |
|------|----------|---------|------------------|
| `AMMHelpers.h` | AMM | LP token calculations, AMM math | 8 |
| `AMMUtils.h` | AMM | AMM pool queries, balance helpers | 12 |
| `LendingHelpers.h` | AMM | Lending pool helpers | 10 |
| `PermissionedDEXHelpers.h` | AMM | Permissioned DEX utilities | 4 |
| `AmendmentTable.h` | Amendments | Amendment tracking and voting | 6 |
| `FeeVote.h` | Fees | Fee voting mechanism | 2 |
| `NegativeUNLVote.h` | Amendments | Negative UNL voting | 3 |
| `ValidatorKeys.h` | Validators | Validator key management | 4 |
| `ValidatorList.h` | Validators | Trusted validator list | 9 |
| `ValidatorSite.h` | Validators | Validator site fetching | 3 |
| `Manifest.h` | Validators | Validator manifest handling | 4 |
| `Transaction.h` | Transactions | Transaction wrapper class | 15 |
| `CanonicalTXSet.h` | Transactions | Canonical transaction ordering | 5 |
| `TxQ.h` | TxQueue | Transaction queue management | 13 |
| `DeliverMax.h` | Transactions | DeliverMax field handling | 7 |
| `HashRouter.h` | TxQueue | Transaction hash routing | 12 |
| `NetworkOPs.h` | Network | Network operations interface | 34 |
| `LoadFeeTrack.h` | Fees | Load-based fee tracking | 14 |
| `SHAMapStore.h` | Storage | SHAMap store interface | 4 |
| `SHAMapStoreImp.h` | Storage | SHAMap store implementation | 1 |
| `DelegateUtils.h` | Utilities | Delegation utilities | 5 |

#### Detail Implementation Files in `app/misc/detail/` (17 files)

| File | Category | Purpose |
|------|----------|---------|
| `AMMHelpers.cpp` | AMM | AMM calculation implementations |
| `AMMUtils.cpp` | AMM | AMM utility implementations |
| `LendingHelpers.cpp` | AMM | Lending helper implementations |
| `AmendmentTable.cpp` | Amendments | Amendment table implementation |
| `ValidatorKeys.cpp` | Validators | Validator key loading |
| `ValidatorList.cpp` | Validators | Validator list management |
| `ValidatorSite.cpp` | Validators | Site fetching implementation |
| `Manifest.cpp` | Validators | Manifest parsing/verification |
| `Transaction.cpp` | Transactions | Transaction implementation |
| `TxQ.cpp` | TxQueue | Transaction queue implementation |
| `LoadFeeTrack.cpp` | Fees | Fee tracking implementation |
| `DelegateUtils.cpp` | Utilities | Delegation implementation |
| `DeliverMax.cpp` | Transactions | DeliverMax implementation |
| `AccountTxPaging.h/.cpp` | Utilities | Account transaction paging |
| `Work.h` | Network | HTTP work base interface |
| `WorkBase.h` | Network | HTTP work base class |
| `WorkPlain.h` | Network | Plain HTTP client |
| `WorkSSL.h/.cpp` | Network | SSL HTTP client |

#### Other Files

| File | Category | Purpose |
|------|----------|---------|
| `NetworkOPs.cpp` | Network | NetworkOPs implementation (large!) |
| `HashRouter.cpp` | TxQueue | Hash router implementation |
| `PermissionedDEXHelpers.cpp` | AMM | Permissioned DEX implementation |
| `FeeVoteImpl.cpp` | Fees | Fee vote implementation |
| `NegativeUNLVote.cpp` | Amendments | Negative UNL implementation |
| `SHAMapStoreImp.cpp` | Storage | SHAMap store implementation |
| `FeeEscalation.md` | Docs | Fee escalation documentation |
| `README.md` | Docs | Directory README |

### Dependency Analysis

#### Most-Imported Files (from app/misc)

```
34 includes - NetworkOPs.h      (VERY HIGH - special handling needed)
15 includes - Transaction.h
14 includes - LoadFeeTrack.h
13 includes - TxQ.h
12 includes - HashRouter.h
12 includes - AMMUtils.h
10 includes - LendingHelpers.h
 9 includes - ValidatorList.h
 8 includes - AMMHelpers.h
 7 includes - DeliverMax.h
 6 includes - AmendmentTable.h
 5 includes - CanonicalTXSet.h
 5 includes - DelegateUtils.h
 4 includes - ValidatorKeys.h
 4 includes - SHAMapStore.h
 4 includes - Manifest.h
 4 includes - PermissionedDEXHelpers.h
 3 includes - ValidatorSite.h
 3 includes - NegativeUNLVote.h
 2 includes - FeeVote.h
```

#### Internal Dependencies (within app/misc)

| File | Depends On (within misc) |
|------|--------------------------|
| `AMMUtils.cpp` | `AMMHelpers.h` |
| `ValidatorList.h` | `Manifest.h` |
| `ValidatorList.cpp` | `HashRouter.h`, `NetworkOPs.h` |
| `ValidatorSite.h` | `ValidatorList.h` |
| `ValidatorSite.cpp` | `ValidatorList.h`, `Manifest.h`, `NetworkOPs.h` |
| `NetworkOPs.cpp` | `AmendmentTable.h`, `HashRouter.h`, `LoadFeeTrack.h`, `TxQ.h`, `Transaction.h`, `ValidatorKeys.h`, `ValidatorList.h` |
| `TxQ.cpp` | `LoadFeeTrack.h`, `Transaction.h`, `HashRouter.h` |
| `SHAMapStoreImp.cpp` | `NetworkOPs.h` |

#### External Consumers (who imports from app/misc)

| Module | Files Importing from app/misc |
|--------|-------------------------------|
| `app/tx/` | 15+ files (AMM transactions, apply.cpp) |
| `app/ledger/` | 8+ files (LedgerMaster, OpenLedger) |
| `app/consensus/` | 4 files (RCLConsensus, RCLValidations) |
| `app/paths/` | 5 files (AMMLiquidity, BookStep) |
| `app/main/` | 3 files (Application.cpp, LoadManager) |
| `overlay/` | 4 files (OverlayImpl, PeerImp) |
| `rpc/` | 15+ files (handlers, helpers) |

---

## Design Considerations

### Option A: Subdirectories Under app/misc/

Create subdirectories within the existing misc directory:
- `app/misc/amm/`
- `app/misc/validators/`
- `app/misc/txqueue/`
- `app/misc/fees/`

**Pros:**
- Minimal disruption to existing include paths
- Gradual migration possible
- Easy rollback

**Cons:**
- Doesn't solve the fundamental "misc is a dumping ground" problem
- Still implies these are miscellaneous utilities
- Maintains the confusing naming

### Option B: Promote to Full Modules (RECOMMENDED)

Create new top-level modules under `app/`:
- `app/amm/` - AMM-related utilities
- `app/validators/` - Validator management
- `app/txqueue/` - Transaction queue and routing

**Pros:**
- Clear module boundaries with semantic meaning
- Matches existing patterns (`app/ledger/`, `app/consensus/`, `app/tx/`)
- Enables proper levelization analysis
- Future-proof architecture

**Cons:**
- More include path changes required
- Larger initial migration effort
- Must update CMakeLists.txt

### Option C: Move to Existing Modules

Move files to modules where they logically belong:
- AMM files → could create `app/amm/` or stay with paths
- Validator files → could join `overlay/` or new module
- TxQ files → could join `app/tx/`

**Cons:**
- Increases size of already-large modules
- May introduce new cycles
- Less clear ownership

### Recommendation: Option B with Staged Migration

**Rationale:**
1. Creates clear, semantic module boundaries
2. Follows existing `app/*` module naming convention
3. Allows independent compilation and testing of modules
4. NetworkOPs stays in misc temporarily (separate future task due to complexity)

### Special Case: NetworkOPs

`NetworkOPs.h/cpp` is a special case requiring separate handling:
- **34 external includes** - most-imported file in misc
- **Depends on 7+ other misc files** internally
- **~4000+ lines** of implementation code
- **Tightly coupled** to consensus, ledger, overlay, and RPC

**Recommendation:** Leave NetworkOPs in `app/misc/` for this task. Create a separate Phase 3 task for NetworkOPs refactoring after other files are moved.

---

## Implementation Plan

### Prerequisites

- [ ] Phase 2 Task 2.4 (LedgerMaster abstraction) complete
- [ ] All Phase 2 interface work complete
- [ ] Clean build on develop branch

### Step 1: Create New Directory Structure

Create the new module directories:

```
src/xrpld/app/amm/
src/xrpld/app/amm/detail/
src/xrpld/app/validators/
src/xrpld/app/validators/detail/
src/xrpld/app/txqueue/
src/xrpld/app/txqueue/detail/
```

**Estimated time:** 5 minutes

### Step 2: Move AMM Files (Lowest Risk)

AMM files have the cleanest boundaries - mostly used by `app/tx/` and `app/paths/`.

#### File Mapping: AMM Module

| Old Path | New Path |
|----------|----------|
| `app/misc/AMMHelpers.h` | `app/amm/AMMHelpers.h` |
| `app/misc/AMMUtils.h` | `app/amm/AMMUtils.h` |
| `app/misc/LendingHelpers.h` | `app/amm/LendingHelpers.h` |
| `app/misc/PermissionedDEXHelpers.h` | `app/amm/PermissionedDEXHelpers.h` |
| `app/misc/PermissionedDEXHelpers.cpp` | `app/amm/PermissionedDEXHelpers.cpp` |
| `app/misc/detail/AMMHelpers.cpp` | `app/amm/detail/AMMHelpers.cpp` |
| `app/misc/detail/AMMUtils.cpp` | `app/amm/detail/AMMUtils.cpp` |
| `app/misc/detail/LendingHelpers.cpp` | `app/amm/detail/LendingHelpers.cpp` |

**Files requiring include updates:**
- `src/xrpld/app/tx/detail/AMMVote.cpp`
- `src/xrpld/app/tx/detail/AMMWithdraw.cpp`
- `src/xrpld/app/tx/detail/AMMDelete.cpp`
- `src/xrpld/app/tx/detail/AMMClawback.cpp`
- `src/xrpld/app/tx/detail/AMMDeposit.cpp`
- `src/xrpld/app/tx/detail/AMMCreate.cpp`
- `src/xrpld/app/tx/detail/AMMBid.cpp`
- `src/xrpld/app/tx/detail/InvariantCheck.cpp`
- `src/xrpld/app/paths/AMMLiquidity.h`
- `src/xrpld/app/paths/detail/BookStep.cpp`
- `src/xrpld/app/ledger/OrderBookDB.cpp`
- `src/xrpld/rpc/handlers/AMMInfo.cpp`

**Estimated time:** 30 minutes

### Step 3: Move Validator Files

Validator files have moderate coupling - primarily used by overlay and consensus.

#### File Mapping: Validators Module

| Old Path | New Path |
|----------|----------|
| `app/misc/Manifest.h` | `app/validators/Manifest.h` |
| `app/misc/ValidatorKeys.h` | `app/validators/ValidatorKeys.h` |
| `app/misc/ValidatorList.h` | `app/validators/ValidatorList.h` |
| `app/misc/ValidatorSite.h` | `app/validators/ValidatorSite.h` |
| `app/misc/detail/Manifest.cpp` | `app/validators/detail/Manifest.cpp` |
| `app/misc/detail/ValidatorKeys.cpp` | `app/validators/detail/ValidatorKeys.cpp` |
| `app/misc/detail/ValidatorList.cpp` | `app/validators/detail/ValidatorList.cpp` |
| `app/misc/detail/ValidatorSite.cpp` | `app/validators/detail/ValidatorSite.cpp` |
| `app/misc/detail/Work.h` | `app/validators/detail/Work.h` |
| `app/misc/detail/WorkBase.h` | `app/validators/detail/WorkBase.h` |
| `app/misc/detail/WorkPlain.h` | `app/validators/detail/WorkPlain.h` |
| `app/misc/detail/WorkSSL.h` | `app/validators/detail/WorkSSL.h` |
| `app/misc/detail/WorkSSL.cpp` | `app/validators/detail/WorkSSL.cpp` |

**Files requiring include updates:**
- `src/xrpld/overlay/detail/OverlayImpl.cpp`
- `src/xrpld/overlay/detail/PeerImp.cpp`
- `src/xrpld/app/consensus/RCLConsensus.cpp`
- `src/xrpld/app/consensus/RCLValidations.cpp`
- `src/xrpld/app/ledger/detail/LedgerMaster.cpp`
- `src/xrpld/app/main/Application.cpp`
- `src/xrpld/app/misc/NetworkOPs.cpp`
- `src/xrpld/rpc/handlers/Validators.cpp`
- `src/xrpld/rpc/handlers/UnlList.cpp`

**Estimated time:** 45 minutes

### Step 4: Move Transaction Queue Files

TxQ files are moderately coupled - used by ledger, consensus, and RPC.

#### File Mapping: TxQueue Module

| Old Path | New Path |
|----------|----------|
| `app/misc/TxQ.h` | `app/txqueue/TxQ.h` |
| `app/misc/HashRouter.h` | `app/txqueue/HashRouter.h` |
| `app/misc/HashRouter.cpp` | `app/txqueue/HashRouter.cpp` |
| `app/misc/CanonicalTXSet.h` | `app/txqueue/CanonicalTXSet.h` |
| `app/misc/CanonicalTXSet.cpp` | `app/txqueue/CanonicalTXSet.cpp` |
| `app/misc/detail/TxQ.cpp` | `app/txqueue/detail/TxQ.cpp` |

**Files requiring include updates:**
- `src/xrpld/app/ledger/detail/LedgerMaster.cpp`
- `src/xrpld/app/ledger/detail/OpenLedger.cpp`
- `src/xrpld/app/ledger/LedgerToJson.h`
- `src/xrpld/app/consensus/RCLConsensus.cpp`
- `src/xrpld/app/main/Application.cpp`
- `src/xrpld/app/misc/NetworkOPs.cpp`
- `src/xrpld/app/misc/detail/ValidatorList.cpp` (→ moves to validators)
- `src/xrpld/rpc/detail/RPCHelpers.h`
- `src/xrpld/rpc/detail/TransactionSign.cpp`
- `src/xrpld/rpc/handlers/AccountInfo.cpp`
- `src/xrpld/rpc/handlers/Simulate.cpp`
- `src/xrpld/rpc/handlers/Fee1.cpp`

**Estimated time:** 30 minutes

### Step 5: Update Remaining Files in app/misc/

Files that remain in `app/misc/` after reorganization:

| File | Reason to Keep in misc |
|------|------------------------|
| `NetworkOPs.h/cpp` | Too complex, separate task |
| `Transaction.h/cpp` | General transaction wrapper, widely used |
| `AmendmentTable.h/cpp` | Could move to `app/amendments/` later |
| `FeeVote.h/cpp` | Could move to `app/fees/` later |
| `NegativeUNLVote.h/cpp` | Could move to `app/amendments/` later |
| `LoadFeeTrack.h/cpp` | Could move to `app/fees/` later |
| `SHAMapStore.h/cpp` | Could move to `nodestore/` later |
| `DeliverMax.h/cpp` | Small utility, can stay |
| `DelegateUtils.h/cpp` | Small utility, can stay |
| `AccountTxPaging.h/cpp` | RPC utility, could move to rpc/ |
| `FeeEscalation.md` | Documentation |
| `README.md` | Documentation |

**Estimated time:** 15 minutes (update README.md)

### Step 6: Update All Include Paths

Run comprehensive search and replace for all moved files:

```bash
# Example commands to find files needing updates
grep -r "xrpld/app/misc/AMMHelpers" src/ --include="*.cpp" --include="*.h"
grep -r "xrpld/app/misc/AMMUtils" src/ --include="*.cpp" --include="*.h"
grep -r "xrpld/app/misc/ValidatorList" src/ --include="*.cpp" --include="*.h"
grep -r "xrpld/app/misc/TxQ" src/ --include="*.cpp" --include="*.h"
```

**Include path changes:**

| Old Include | New Include |
|-------------|-------------|
| `<xrpld/app/misc/AMMHelpers.h>` | `<xrpld/app/amm/AMMHelpers.h>` |
| `<xrpld/app/misc/AMMUtils.h>` | `<xrpld/app/amm/AMMUtils.h>` |
| `<xrpld/app/misc/LendingHelpers.h>` | `<xrpld/app/amm/LendingHelpers.h>` |
| `<xrpld/app/misc/PermissionedDEXHelpers.h>` | `<xrpld/app/amm/PermissionedDEXHelpers.h>` |
| `<xrpld/app/misc/Manifest.h>` | `<xrpld/app/validators/Manifest.h>` |
| `<xrpld/app/misc/ValidatorKeys.h>` | `<xrpld/app/validators/ValidatorKeys.h>` |
| `<xrpld/app/misc/ValidatorList.h>` | `<xrpld/app/validators/ValidatorList.h>` |
| `<xrpld/app/misc/ValidatorSite.h>` | `<xrpld/app/validators/ValidatorSite.h>` |
| `<xrpld/app/misc/TxQ.h>` | `<xrpld/app/txqueue/TxQ.h>` |
| `<xrpld/app/misc/HashRouter.h>` | `<xrpld/app/txqueue/HashRouter.h>` |
| `<xrpld/app/misc/CanonicalTXSet.h>` | `<xrpld/app/txqueue/CanonicalTXSet.h>` |

**Estimated time:** 60 minutes

### Step 7: Update CMakeLists.txt

Update the build configuration to reflect new module structure.

**File:** `CMakeLists.txt` or `src/xrpld/CMakeLists.txt`

Add new source file lists for each module:

```cmake
# AMM Module
target_sources(xrpld PRIVATE
    src/xrpld/app/amm/AMMHelpers.h
    src/xrpld/app/amm/AMMUtils.h
    src/xrpld/app/amm/LendingHelpers.h
    src/xrpld/app/amm/PermissionedDEXHelpers.h
    src/xrpld/app/amm/PermissionedDEXHelpers.cpp
    src/xrpld/app/amm/detail/AMMHelpers.cpp
    src/xrpld/app/amm/detail/AMMUtils.cpp
    src/xrpld/app/amm/detail/LendingHelpers.cpp
)

# Validators Module
target_sources(xrpld PRIVATE
    src/xrpld/app/validators/Manifest.h
    src/xrpld/app/validators/ValidatorKeys.h
    src/xrpld/app/validators/ValidatorList.h
    src/xrpld/app/validators/ValidatorSite.h
    src/xrpld/app/validators/detail/Manifest.cpp
    src/xrpld/app/validators/detail/ValidatorKeys.cpp
    src/xrpld/app/validators/detail/ValidatorList.cpp
    src/xrpld/app/validators/detail/ValidatorSite.cpp
    src/xrpld/app/validators/detail/Work.h
    src/xrpld/app/validators/detail/WorkBase.h
    src/xrpld/app/validators/detail/WorkPlain.h
    src/xrpld/app/validators/detail/WorkSSL.h
    src/xrpld/app/validators/detail/WorkSSL.cpp
)

# TxQueue Module
target_sources(xrpld PRIVATE
    src/xrpld/app/txqueue/TxQ.h
    src/xrpld/app/txqueue/HashRouter.h
    src/xrpld/app/txqueue/HashRouter.cpp
    src/xrpld/app/txqueue/CanonicalTXSet.h
    src/xrpld/app/txqueue/CanonicalTXSet.cpp
    src/xrpld/app/txqueue/detail/TxQ.cpp
)
```

Remove moved files from existing `app/misc` source list.

**Estimated time:** 30 minutes

---

## Complete File Mapping Table

### AMM Module (8 files)

| # | Old Path | New Path | Status |
|---|----------|----------|--------|
| 1 | `app/misc/AMMHelpers.h` | `app/amm/AMMHelpers.h` | ⬜ |
| 2 | `app/misc/AMMUtils.h` | `app/amm/AMMUtils.h` | ⬜ |
| 3 | `app/misc/LendingHelpers.h` | `app/amm/LendingHelpers.h` | ⬜ |
| 4 | `app/misc/PermissionedDEXHelpers.h` | `app/amm/PermissionedDEXHelpers.h` | ⬜ |
| 5 | `app/misc/PermissionedDEXHelpers.cpp` | `app/amm/PermissionedDEXHelpers.cpp` | ⬜ |
| 6 | `app/misc/detail/AMMHelpers.cpp` | `app/amm/detail/AMMHelpers.cpp` | ⬜ |
| 7 | `app/misc/detail/AMMUtils.cpp` | `app/amm/detail/AMMUtils.cpp` | ⬜ |
| 8 | `app/misc/detail/LendingHelpers.cpp` | `app/amm/detail/LendingHelpers.cpp` | ⬜ |

### Validators Module (13 files)

| # | Old Path | New Path | Status |
|---|----------|----------|--------|
| 1 | `app/misc/Manifest.h` | `app/validators/Manifest.h` | ⬜ |
| 2 | `app/misc/ValidatorKeys.h` | `app/validators/ValidatorKeys.h` | ⬜ |
| 3 | `app/misc/ValidatorList.h` | `app/validators/ValidatorList.h` | ⬜ |
| 4 | `app/misc/ValidatorSite.h` | `app/validators/ValidatorSite.h` | ⬜ |
| 5 | `app/misc/detail/Manifest.cpp` | `app/validators/detail/Manifest.cpp` | ⬜ |
| 6 | `app/misc/detail/ValidatorKeys.cpp` | `app/validators/detail/ValidatorKeys.cpp` | ⬜ |
| 7 | `app/misc/detail/ValidatorList.cpp` | `app/validators/detail/ValidatorList.cpp` | ⬜ |
| 8 | `app/misc/detail/ValidatorSite.cpp` | `app/validators/detail/ValidatorSite.cpp` | ⬜ |
| 9 | `app/misc/detail/Work.h` | `app/validators/detail/Work.h` | ⬜ |
| 10 | `app/misc/detail/WorkBase.h` | `app/validators/detail/WorkBase.h` | ⬜ |
| 11 | `app/misc/detail/WorkPlain.h` | `app/validators/detail/WorkPlain.h` | ⬜ |
| 12 | `app/misc/detail/WorkSSL.h` | `app/validators/detail/WorkSSL.h` | ⬜ |
| 13 | `app/misc/detail/WorkSSL.cpp` | `app/validators/detail/WorkSSL.cpp` | ⬜ |

### TxQueue Module (6 files)

| # | Old Path | New Path | Status |
|---|----------|----------|--------|
| 1 | `app/misc/TxQ.h` | `app/txqueue/TxQ.h` | ⬜ |
| 2 | `app/misc/HashRouter.h` | `app/txqueue/HashRouter.h` | ⬜ |
| 3 | `app/misc/HashRouter.cpp` | `app/txqueue/HashRouter.cpp` | ⬜ |
| 4 | `app/misc/CanonicalTXSet.h` | `app/txqueue/CanonicalTXSet.h` | ⬜ |
| 5 | `app/misc/CanonicalTXSet.cpp` | `app/txqueue/CanonicalTXSet.cpp` | ⬜ |
| 6 | `app/misc/detail/TxQ.cpp` | `app/txqueue/detail/TxQ.cpp` | ⬜ |

### Remaining in app/misc/ (17 files)

| # | File | Reason |
|---|------|--------|
| 1 | `NetworkOPs.h` | Too complex, separate task |
| 2 | `NetworkOPs.cpp` | Too complex, separate task |
| 3 | `Transaction.h` | Widely used general utility |
| 4 | `detail/Transaction.cpp` | Widely used general utility |
| 5 | `AmendmentTable.h` | Future: app/amendments/ |
| 6 | `detail/AmendmentTable.cpp` | Future: app/amendments/ |
| 7 | `FeeVote.h` | Future: app/fees/ |
| 8 | `FeeVoteImpl.cpp` | Future: app/fees/ |
| 9 | `NegativeUNLVote.h` | Future: app/amendments/ |
| 10 | `NegativeUNLVote.cpp` | Future: app/amendments/ |
| 11 | `LoadFeeTrack.h` | Future: app/fees/ |
| 12 | `detail/LoadFeeTrack.cpp` | Future: app/fees/ |
| 13 | `SHAMapStore.h` | Future: nodestore/ |
| 14 | `SHAMapStoreImp.h` | Future: nodestore/ |
| 15 | `SHAMapStoreImp.cpp` | Future: nodestore/ |
| 16 | `DeliverMax.h` | Small utility |
| 17 | `detail/DeliverMax.cpp` | Small utility |
| 18 | `DelegateUtils.h` | Small utility |
| 19 | `detail/DelegateUtils.cpp` | Small utility |
| 20 | `detail/AccountTxPaging.h` | RPC utility |
| 21 | `detail/AccountTxPaging.cpp` | RPC utility |
| 22 | `FeeEscalation.md` | Documentation |
| 23 | `README.md` | Documentation |

---

## Risk Assessment

### High Risk

| Risk | Mitigation |
|------|------------|
| **Many external includes will break** | Perform comprehensive grep search before moving; use sed/awk for batch updates |
| **Build failures during migration** | Move one module at a time; verify build after each step |
| **Merge conflicts with active development** | Coordinate with team; perform migration during low-activity period |

### Medium Risk

| Risk | Mitigation |
|------|------------|
| **CMake module boundary changes** | Test CMake configuration after each module move |
| **NetworkOPs is heavily coupled** | Keep NetworkOPs in misc for now; separate dedicated task |
| **Test file include paths** | Search test directories for includes; update simultaneously |
| **IDE/tooling disruption** | Communicate changes to team; allow time for tooling updates |

### Low Risk

| Risk | Mitigation |
|------|------------|
| **Header-only files easier to move** | Start with header-only files as proof of concept |
| **Documentation updates needed** | Update README.md files after migration |
| **Header guards need updating** | Update `#ifndef` guards to match new paths |

---

## Validation Criteria

### Build Validation

- [ ] `cmake --build build` completes without errors
- [ ] All object files compile correctly
- [ ] Linking succeeds

### Test Validation

- [ ] `ctest --test-dir build` passes all tests
- [ ] AMM-related unit tests pass
- [ ] Validator-related unit tests pass
- [ ] TxQ-related unit tests pass
- [ ] Integration tests pass

### Static Analysis

- [ ] Levelization check passes: `.github/scripts/levelization/levelization.sh` shows no new cycles
- [ ] Include-what-you-use passes (if configured)
- [ ] Clang-tidy passes (if configured)

### Code Quality

- [ ] All header guards updated to match new paths
- [ ] No duplicate includes introduced
- [ ] README.md files updated for new and old directories
- [ ] Git history preserved with `git mv`

### Documentation

- [ ] `app/misc/README.md` updated to reflect remaining files
- [ ] `app/amm/README.md` created explaining AMM module
- [ ] `app/validators/README.md` created explaining validators module
- [ ] `app/txqueue/README.md` created explaining txqueue module

---

## Estimated Total Time

| Step | Time |
|------|------|
| Step 1: Create directories | 5 min |
| Step 2: Move AMM files | 30 min |
| Step 3: Move Validator files | 45 min |
| Step 4: Move TxQueue files | 30 min |
| Step 5: Update remaining misc files | 15 min |
| Step 6: Update all include paths | 60 min |
| Step 7: Update CMakeLists.txt | 30 min |
| Verification and testing | 60 min |
| **Total** | **~4.5 hours** |

---

## Future Work (Out of Scope)

1. **NetworkOPs Refactoring** - Separate Phase 3 task to break apart NetworkOPs into smaller components
2. **app/amendments/ Module** - Move AmendmentTable, NegativeUNLVote, FeeVote
3. **app/fees/ Module** - Move LoadFeeTrack, fee-related utilities
4. **SHAMapStore to nodestore/** - Move SHAMapStore files to nodestore module
5. **Transaction.h/cpp** - Evaluate if this should move to app/tx/