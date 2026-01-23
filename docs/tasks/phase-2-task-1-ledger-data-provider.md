# Phase 2 Task 2.1: LedgerDataProvider Interface

## Overview

## Implementation Progress

### Current Status: ✅ COMPLETE

| Step | Description | Status | Commit |
|------|-------------|--------|--------|
| 1 | Create interface file | ✅ COMPLETE | 4cc8735096 |
| 2 | Make LedgerMaster implement interface | ✅ COMPLETE | 4cc8735096 |
| 3 | Add LedgerDataProvider to RPC Context | ✅ COMPLETE | 2a7bfdcfb9 |
| 4 | Migrate RPC Handlers | ✅ COMPLETE | 3ca3d6748f |

**All steps are complete.** All RPC handlers have been migrated to use `LedgerDataProvider`.

### Migrated Handlers

The following RPC handlers have been migrated to use `LedgerDataProvider` instead of `LedgerMaster`:

#### Batch 1: Initial Handlers (Commit: 2a7bfdcfb9)
| Handler | Methods Used |
|---------|--------------|
| LedgerAccept.cpp | `getCurrentLedgerIndex()` |
| Submit.cpp | `getValidatedLedgerAge()` |
| SubmitMultiSigned.cpp | `getValidatedLedgerAge()` |

#### Batch 2: Ledger Index Handlers (Commit: 6486d699f6)
| Handler | Methods Used |
|---------|--------------|
| LedgerCurrent.cpp | `getCurrentLedgerIndex()` |
| LedgerClosed.cpp | `getClosedLedger()` |

#### Batch 3: Additional Handlers (Commit: 63d0481743)
| Handler | Methods Used |
|---------|--------------|
| AMMInfo.cpp | via `lookupLedger()` |
| AccountChannels.cpp | via `lookupLedger()` |
| AccountCurrencies.cpp | via `lookupLedger()` |
| AccountInfo.cpp | via `lookupLedger()` |
| AccountLines.cpp | via `lookupLedger()` |
| AccountNFTs.cpp | via `lookupLedger()` |
| AccountObjects.cpp | via `lookupLedger()` |
| AccountOffers.cpp | via `lookupLedger()` |
| BookOffers.cpp | via `lookupLedger()` |
| DepositAuthorized.cpp | via `lookupLedger()` |
| GatewayBalances.cpp | via `lookupLedger()` |
| LedgerData.cpp | via `lookupLedger()` |
| LedgerEntry.cpp | via `lookupLedger()` |
| LedgerHeader.cpp | via `lookupLedger()` |
| NFTBuyOffers.cpp | via `lookupLedger()` |
| NFTInfo.cpp | via `lookupLedger()` |
| NFTSellOffers.cpp | via `lookupLedger()` |
| NoRippleCheck.cpp | via `lookupLedger()` |
| OwnerInfo.cpp | `getClosedLedger()` |
| PathFind.cpp | `getClosedLedger()` |

#### Batch 4: Remaining Handlers (Commit: 3ca3d6748f)
| Handler | Methods Used |
|---------|--------------|
| AccountTx.cpp | `getLedgerBySeq()` |
| CanDelete.cpp | `getValidLedgerIndex()` |
| Crawl.cpp | `getValidatedLedger()` |
| Fee1.cpp | `getValidatedLedger()` |
| GetCounts.cpp | `getClosedLedger()`, `getValidLedgerIndex()` |
| GetAggregatePrice.cpp | via `lookupLedger()` |
| LedgerHandler.cpp | `getClosedLedger()`, `getCurrentLedger()`, `isValidated()` |
| LedgerDiff.cpp | `getLedgerBySeq()`, `getLedgerByHash()` |
| LedgerRequest.cpp | `getLedgerByHash()`, `getLedgerBySeq()`, `getValidatedLedger()` |
| Manifest.cpp | `getValidLedgerIndex()` |
| Peers.cpp | `getValidatedLedger()` |
| RipplePathFind.cpp | `getValidatedLedgerAge()`, `getClosedLedger()` |
| ServerInfo.cpp | Multiple methods |
| ServerState.cpp | Multiple methods |
| Simulate.cpp | `getValidatedLedgerAge()` |
| Tx.cpp | `getLedgerBySeq()` |
| TxHistory.cpp | `getClosedLedger()` |
| TxReduceRelay.cpp | `getLedgerBySeq()` |
| TransactionEntry.cpp | `getLedgerBySeq()`, `getLedgerByHash()` |
| ValidatorInfo.cpp | `getValidLedgerIndex()` |
| ValidatorListSites.cpp | `getValidLedgerIndex()` |
| Validators.cpp | `getValidLedgerIndex()` |

#### Helper Files (Commit: 14aae1361e)
| File | Methods Used |
|------|--------------|
| RPCHelpers.cpp | `getClosedLedger()`, `getLedgerByHash()`, `getLedgerBySeq()`, `isValidated()` |
| LookupLedger.cpp | `getLedgerByHash()`, `getLedgerBySeq()`, `isValidated()`, `getValidLedgerIndex()`, `getCurrentLedgerIndex()` |

### Key Commits

| Commit | Description |
|--------|-------------|
| 4cc8735096 | Create LedgerDataProvider interface |
| 2a7bfdcfb9 | Add LedgerDataProvider to RPC Context |
| 6486d699f6 | Migrate LedgerCurrent and LedgerClosed |
| 63d0481743 | Migrate more RPC handlers |
| 42739dd2e2 | Break InfoSub.h → Manifest.h dependency |
| ce6ac4a345 | Extend LedgerDataProvider with remaining methods |
| 3ca3d6748f | Migrate all remaining RPC handlers |
| 14aae1361e | Migrate RPC helper files |

The abstract interface class was created in `src/xrpld/core/LedgerDataProvider.h` with the following 12 methods:

1. `getCurrentLedgerIndex()` - Get current ledger sequence
2. `getValidLedgerIndex()` - Get validated ledger sequence
3. `getCurrentLedger()` - Get current open ledger
4. `getClosedLedger()` - Get last closed ledger
5. `getValidatedLedger()` - Get last validated ledger
6. `getLedgerBySeq()` - Get ledger by sequence number
7. `getLedgerByHash()` - Get ledger by hash
8. `isValidated()` - Check if ledger is validated
9. `getValidatedRange()` - Get validated ledger range
10. `getValidatedLedgerAge()` - Get age of validated ledger
11. `getHashBySeq()` - Get hash for sequence number
12. `getCompleteLedgers()` - Get complete ledger range string

---

### Problem Statement

The `app ↔ rpc` cycle is the most severe circular dependency in the rippled codebase with **174 includes** (rpc→app: 153, app→rpc: 21). RPC handlers currently access `LedgerMaster` directly via `context.ledgerMaster`, creating tight coupling between the RPC layer and the application layer.

This dependency violates the Dependency Inversion Principle (DIP) and creates several problems:
1. **Testing difficulty**: RPC handlers cannot be unit tested without instantiating the entire application
2. **Build time impact**: Changes to LedgerMaster force recompilation of all RPC handlers
3. **Architectural rigidity**: The RPC layer cannot be extracted to a separate module
4. **Circular dependencies**: Creates bidirectional coupling that complicates the build graph

### Success Criteria

1. ✅ RPC handlers depend on abstract `LedgerDataProvider` interface instead of concrete `LedgerMaster`
2. ✅ `loops.txt` shows reduction in `app ↔ rpc` cycle (target: reduce 21 app→rpc includes)
3. ✅ All existing RPC handler tests pass without modification
4. ✅ No performance regression in RPC handler benchmarks
5. ✅ Build remains successful at each incremental step

---

## Deep Code Analysis

### Current RPC Context Structure

**File**: `src/xrpld/rpc/Context.h`

```cpp
namespace xrpl {

class Application;
class NetworkOPs;
class LedgerMaster;  // Forward declaration - but concrete class used

namespace RPC {

/** The context of information needed to call an RPC. */
struct Context
{
    beast::Journal const j;
    Application& app;
    Resource::Charge& loadType;
    NetworkOPs& netOps;
    LedgerMaster& ledgerMaster;  // <-- DIRECT DEPENDENCY (Problem!)
    Resource::Consumer& consumer;
    Role role;
    std::shared_ptr<JobQueue::Coro> coro{};
    InfoSub::pointer infoSub{};
    unsigned int apiVersion;
};

struct JsonContext : public Context { /* ... */ };

template <class RequestType>
struct GRPCContext : public Context { /* ... */ };

}  // namespace RPC
}  // namespace xrpl
```

### LedgerMaster Methods Used by RPC Handlers

**File**: `src/xrpld/app/ledger/LedgerMaster.h`

The following methods are accessed by RPC handlers and need to be part of the interface:

| Method | Return Type | Description | RPC Usage Frequency |
|--------|-------------|-------------|---------------------|
| `getCurrentLedgerIndex()` | `LedgerIndex` | Get current ledger sequence | High |
| `getValidLedgerIndex()` | `LedgerIndex` | Get validated ledger sequence | High |
| `getClosedLedger()` | `shared_ptr<Ledger const>` | Get last closed ledger | High |
| `getValidatedLedger()` | `shared_ptr<Ledger const>` | Get last validated ledger | High |
| `getLedgerBySeq(uint32_t)` | `shared_ptr<Ledger const>` | Get ledger by sequence | High |
| `getLedgerByHash(uint256)` | `shared_ptr<Ledger const>` | Get ledger by hash | Medium |
| `getValidatedLedgerAge()` | `chrono::seconds` | Get age of validated ledger | Medium |
| `isValidated(ReadView&)` | `bool` | Check if ledger is validated | Medium |
| `getValidatedRange(min, max)` | `bool` | Get validated ledger range | Low |
| `getHashBySeq(uint32_t)` | `uint256` | Get hash for sequence | Low |
| `getCurrentLedger()` | `shared_ptr<ReadView const>` | Get current open ledger | Medium |
| `getCompleteLedgers()` | `string` | Get complete ledger range string | Low |

### RPC Handlers with LedgerMaster Dependencies

**Directory**: `src/xrpld/rpc/handlers/`

#### Direct `context.ledgerMaster` Usage (30+ handlers):

| Handler File | Methods Called | Priority |
|--------------|---------------|----------|
| `LedgerAccept.cpp` | `getCurrentLedgerIndex()` | High |
| `LedgerCurrent.cpp` | `getCurrentLedgerIndex()` | High |
| `LedgerClosed.cpp` | `getClosedLedger()` | High |
| `LedgerHandler.cpp` | `getClosedLedger()`, `getCurrentLedger()`, `isValidated()` | High |
| `OwnerInfo.cpp` | `getClosedLedger()` | Medium |
| `SubmitMultiSigned.cpp` | `getValidatedLedgerAge()` | Medium |
| `Submit.cpp` | `getValidatedLedgerAge()` | Medium |
| `PathFind.cpp` | `getClosedLedger()` | Medium |
| `RipplePathFind.cpp` | `getValidatedLedgerAge()`, `getClosedLedger()` | Medium |
| `ServerInfo.cpp` | Multiple methods | High |
| `ServerState.cpp` | Multiple methods | High |
| `AccountInfo.cpp` | Via `lookupLedger()` | High |
| `AccountLines.cpp` | Via `lookupLedger()` | High |
| `AccountObjects.cpp` | Via `lookupLedger()` | High |
| `AccountTx.cpp` | Via helper functions | Medium |
| `BookOffers.cpp` | Via `lookupLedger()` | Medium |
| `LedgerData.cpp` | Via helper functions | Medium |
| `LedgerEntry.cpp` | Via helper functions | Medium |
| `LedgerRequest.cpp` | Via `getOrAcquireLedger()` | Medium |
| `Tx.cpp` | `getLedgerBySeq()` | Medium |

#### Indirect Usage via Helper Functions:

**File**: `src/xrpld/rpc/detail/RPCLedgerHelpers.cpp`

```cpp
// Helper functions that internally use context.ledgerMaster:
template <class T>
Status getLedger(T& ledger, uint256 const& ledgerHash, Context const& context)
{
    ledger = context.ledgerMaster.getLedgerByHash(ledgerHash);  // <-- Direct access
    // ...
}

template <class T>
Status getLedger(T& ledger, uint32_t ledgerIndex, Context const& context)
{
    ledger = context.ledgerMaster.getLedgerBySeq(ledgerIndex);  // <-- Direct access
    auto cur = context.ledgerMaster.getCurrentLedger();         // <-- Direct access
    // ...
}

template <class T>
Status getLedger(T& ledger, LedgerShortcut shortcut, Context const& context)
{
    // Uses: getValidatedLedger(), getCurrentLedger(), getClosedLedger()
    // Also uses: getValidLedgerIndex(), getValidatedLedgerAge()
}

Status lookupLedger(shared_ptr<ReadView const>& ledger, JsonContext const& context, Json::Value& result)
{
    result[jss::validated] = context.ledgerMaster.isValidated(*ledger);  // <-- Direct access
}
```

### Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                            Current Architecture                          │
└─────────────────────────────────────────────────────────────────────────┘

  ┌──────────────┐         ┌──────────────────┐         ┌─────────────────┐
  │  RPC Client  │────────▶│   RPC Handler    │────────▶│  LedgerMaster   │
  │  (JSON/gRPC) │         │ (AccountInfo,    │ direct  │  (Concrete)     │
  └──────────────┘         │  LedgerCurrent,  │ access  │                 │
                           │  Submit, etc.)   │────────▶│  - getLedger*() │
                           └──────────────────┘         │  - isValidated()│
                                    │                   │  - getRange()   │
                                    ▼                   └─────────────────┘
                           ┌──────────────────┐                  │
                           │  RPC::Context    │                  │
                           │                  │                  │
                           │ ledgerMaster& ───┼──────────────────┘
                           └──────────────────┘

                                PROBLEM: Tight coupling creates circular deps
```

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           Proposed Architecture                          │
└─────────────────────────────────────────────────────────────────────────┘

  ┌──────────────┐         ┌──────────────────┐         ┌─────────────────┐
  │  RPC Client  │────────▶│   RPC Handler    │         │  LedgerMaster   │
  │  (JSON/gRPC) │         │ (AccountInfo,    │         │  (Concrete)     │
  └──────────────┘         │  LedgerCurrent,  │         │                 │
                           │  Submit, etc.)   │         │  implements     │
                           └──────────────────┘         │       ▲         │
                                    │                   └───────┼─────────┘
                                    ▼                           │
                           ┌──────────────────┐                 │
                           │  RPC::Context    │                 │
                           │                  │                 │
                           │ ledgerProvider& ─┼──▶ ┌────────────┴────────────┐
                           └──────────────────┘    │  LedgerDataProvider     │
                                                   │  (Abstract Interface)   │
                                                   │  src/xrpld/core/        │
                                                   │                         │
                                                   │  + getCurrentLedgerIdx()│
                                                   │  + getValidLedgerIdx()  │
                                                   │  + getClosedLedger()    │
                                                   │  + getValidatedLedger() │
                                                   │  + getLedgerBySeq()     │
                                                   │  + getLedgerByHash()    │
                                                   │  + isValidated()        │
                                                   │  + ...                  │
                                                   └─────────────────────────┘

                     SOLUTION: Dependency Inversion via abstract interface
```

---

## Design Considerations

### Option A: Create LedgerDataProvider Interface in Core Module (RECOMMENDED)

Create a pure virtual interface class that abstracts the read-only ledger access methods.

**Location**: `src/xrpld/core/LedgerDataProvider.h`

```cpp
#ifndef XRPL_CORE_LEDGERDATAPROVIDER_H_INCLUDED
#define XRPL_CORE_LEDGERDATAPROVIDER_H_INCLUDED

#include <xrpl/basics/chrono.h>
#include <xrpl/protocol/Protocol.h>
#include <xrpl/protocol/RippleLedgerHash.h>
#include <memory>
#include <chrono>
#include <string>

namespace xrpl {

class Ledger;
class ReadView;

/**
 * @brief Abstract interface for read-only ledger data access.
 *
 * This interface provides a clean abstraction layer between the RPC
 * subsystem and the ledger management implementation, enabling:
 * - Dependency inversion (RPC depends on abstraction, not concrete class)
 * - Improved testability (mock implementations for unit tests)
 * - Reduced coupling between app and rpc modules
 */
class LedgerDataProvider
{
public:
    virtual ~LedgerDataProvider() = default;

    // Ledger index accessors
    [[nodiscard]] virtual LedgerIndex
    getCurrentLedgerIndex() = 0;

    [[nodiscard]] virtual LedgerIndex
    getValidLedgerIndex() = 0;

    // Ledger retrieval by reference
    [[nodiscard]] virtual std::shared_ptr<ReadView const>
    getCurrentLedger() = 0;

    [[nodiscard]] virtual std::shared_ptr<Ledger const>
    getClosedLedger() = 0;

    [[nodiscard]] virtual std::shared_ptr<Ledger const>
    getValidatedLedger() = 0;

    // Ledger retrieval by identifier
    [[nodiscard]] virtual std::shared_ptr<Ledger const>
    getLedgerBySeq(std::uint32_t index) = 0;

    [[nodiscard]] virtual std::shared_ptr<Ledger const>
    getLedgerByHash(uint256 const& hash) = 0;

    // Validation status
    [[nodiscard]] virtual bool
    isValidated(ReadView const& ledger) = 0;

    [[nodiscard]] virtual bool
    getValidatedRange(std::uint32_t& minVal, std::uint32_t& maxVal) = 0;

    // Timing and age
    [[nodiscard]] virtual std::chrono::seconds
    getValidatedLedgerAge() = 0;

    // Hash lookup
    [[nodiscard]] virtual uint256
    getHashBySeq(std::uint32_t index) = 0;

    // Informational
    [[nodiscard]] virtual std::string
    getCompleteLedgers() = 0;
};

}  // namespace xrpl

#endif
```

**Pros**:
- Clean separation of concerns following SOLID principles
- Interface lives in `core` module (no circular dependencies)
- Easy to create mock implementations for testing
- Minimal changes to existing code structure
- Virtual dispatch overhead is negligible for RPC (network-bound)

**Cons**:
- Adds a new abstraction layer
- Slight virtual dispatch overhead (microseconds, irrelevant for RPC)

### Option B: Create LedgerReadView Interface (Read-Only Subset)

Create a minimal interface with only the most commonly used read methods.

**Pros**:
- Smaller interface, fewer methods to implement
- Even simpler abstraction

**Cons**:
- May need expansion later for edge cases
- Doesn't cover all current RPC needs

### Option C: Use std::function Callbacks

Replace direct LedgerMaster reference with std::function callbacks.

```cpp
struct Context {
    std::function<LedgerIndex()> getCurrentLedgerIndex;
    std::function<std::shared_ptr<Ledger const>()> getValidatedLedger;
    // ... more callbacks
};
```

**Pros**:
- No new interface class needed
- Maximum flexibility

**Cons**:
- Verbose context initialization
- Harder to mock for testing
- Less discoverable API
- Performance overhead from std::function

### Recommendation: Option A

**Option A is recommended** because:
1. **Follows established patterns**: rippled already uses abstract base classes (e.g., `AbstractFetchPackContainer`)
2. **Testability**: Easy to create `MockLedgerDataProvider` for unit tests
3. **Clarity**: Single interface documents all required ledger operations
4. **Minimal overhead**: Virtual dispatch is negligible for RPC handlers (network I/O dominates)
5. **Future-proof**: Easy to add new methods or create specialized interfaces

---

## Implementation Plan

### Step 1: Define LedgerDataProvider Interface

**Files to Create**:
- `src/xrpld/core/LedgerDataProvider.h`

**Actions**:
1. Create the abstract interface class as shown in Option A
2. Ensure the header has minimal dependencies (forward declarations where possible)
3. Add to CMakeLists.txt if necessary

**Compilation Order**: This change is additive and won't break the build.

```bash
# Verify build after step 1
cmake --build build --target rippled -j$(nproc)
```

### Step 2: Have LedgerMaster Implement LedgerDataProvider

**Files to Modify**:
- `src/xrpld/app/ledger/LedgerMaster.h`

**Changes**:
```cpp
// Before:
class LedgerMaster : public AbstractFetchPackContainer

// After:
class LedgerMaster : public AbstractFetchPackContainer, public LedgerDataProvider
```

**Notes**:
- All required methods already exist in LedgerMaster
- No method signature changes needed
- Add `override` specifiers to relevant methods
- Implementation in LedgerMaster.cpp remains unchanged

**Compilation Order**: Build should succeed; methods already exist.

```bash
# Verify build after step 2
cmake --build build --target rippled -j$(nproc)
```

### Step 3: Update RPC::Context to Use Interface

**Files to Modify**:
- `src/xrpld/rpc/Context.h`

**Changes**:
```cpp
// Before:
#include <...>
namespace xrpl {
class LedgerMaster;  // Forward declaration
namespace RPC {
struct Context {
    LedgerMaster& ledgerMaster;
    // ...
};

// After:
#include <xrpld/core/LedgerDataProvider.h>  // New include
namespace xrpl {
class LedgerMaster;  // Keep for backward compatibility during transition
namespace RPC {
struct Context {
    LedgerDataProvider& ledgerMaster;  // Changed type, same name for compatibility
    // ...
};
```

**Important**: Keep the member name as `ledgerMaster` initially to minimize changes to handlers.

**Files to Update** (Context construction sites):
- `src/xrpld/app/main/GRPCServer.cpp` - line ~163
- `src/xrpld/rpc/detail/ServerHandlerImp.cpp` - context creation
- Any test files that construct RPC::Context

**Compilation Order**: Build may have issues with files that expect LedgerMaster-specific methods.

```bash
# Verify build - may need fixes
cmake --build build --target rippled -j$(nproc) 2>&1 | head -100
```

### Step 4: Migrate RPC Handlers Incrementally

Migrate handlers in batches of 5-10 to ensure buildability at each step.

#### Batch 1: Simple Ledger Index Handlers (5 files)
- `LedgerAccept.cpp` - uses `getCurrentLedgerIndex()`
- `LedgerCurrent.cpp` - uses `getCurrentLedgerIndex()`
- `LedgerClosed.cpp` - uses `getClosedLedger()`
- `OwnerInfo.cpp` - uses `getClosedLedger()`
- `PathFind.cpp` - uses `getClosedLedger()`

**Changes**: Usually no changes needed if interface matches; verify compilation.

#### Batch 2: Submit Handlers (5 files)
- `Submit.cpp` - uses `getValidatedLedgerAge()`
- `SubmitMultiSigned.cpp` - uses `getValidatedLedgerAge()`
- `Simulate.cpp` - uses similar patterns
- `SignHandler.cpp` - uses similar patterns
- `SignFor.cpp` - uses similar patterns

#### Batch 3: Account Handlers (6 files)
- `AccountInfo.cpp` - via `lookupLedger()`
- `AccountLines.cpp` - via `lookupLedger()`
- `AccountObjects.cpp` - via `lookupLedger()`
- `AccountChannels.cpp` - via `lookupLedger()`
- `AccountOffers.cpp` - via `lookupLedger()`
- `AccountTx.cpp` - via helper functions

#### Batch 4: Ledger Handlers (6 files)
- `LedgerHandler.cpp` - multiple methods
- `LedgerData.cpp` - via helpers
- `LedgerEntry.cpp` - via helpers
- `LedgerRequest.cpp` - via `getOrAcquireLedger()`
- `LedgerHeader.cpp` - via `lookupLedger()`
- `LedgerDiff.cpp` - via helpers

#### Batch 5: Helper Functions Update
- `src/xrpld/rpc/detail/RPCLedgerHelpers.cpp` - update all `context.ledgerMaster` usages

#### Batch 6: Server Info Handlers (4 files)
- `ServerInfo.cpp` - multiple methods
- `ServerState.cpp` - multiple methods
- `Fee1.cpp` - status methods
- `GetCounts.cpp` - statistics methods

#### Batch 7: Transaction Handlers (5 files)
- `Tx.cpp` - uses `getLedgerBySeq()`
- `TxHistory.cpp` - uses range methods
- `TransactionEntry.cpp` - uses ledger lookup
- `BookOffers.cpp` - via helpers
- `GatewayBalances.cpp` - via helpers

#### Batch 8: Path Finding (3 files)
- `RipplePathFind.cpp` - uses age and closed ledger
- `PathFind.cpp` - uses closed ledger
- `NoRippleCheck.cpp` - uses ledger lookup

#### Batch 9: Remaining Handlers
- All remaining handlers using ledger access

### Step 5: Update Tests

**Files to Review**:
- `src/test/rpc/*.cpp` - RPC unit tests
- Any files creating mock contexts

**Actions**:
1. Create `MockLedgerDataProvider` class for testing
2. Update test context creation to use mock
3. Verify all RPC tests pass

```bash
# Run RPC tests
./build/rippled --unittest="rpc"
```

### Step 6: Final Verification

```bash
# Full build
cmake --build build --target rippled -j$(nproc)

# Run all unit tests
./build/rippled --unittest

# Check for remaining direct LedgerMaster includes in RPC
grep -r "LedgerMaster" src/xrpld/rpc/ --include="*.cpp" --include="*.h"

# Generate updated loops.txt
# (Use the include analysis tool from Phase 1)
```

---

## Risk Assessment

### High Risk: Breaking Changes to RPC Handler Signatures

**Risk**: If LedgerDataProvider interface doesn't match LedgerMaster method signatures exactly, handlers will fail to compile.

**Mitigation**:
1. Interface methods must match LedgerMaster signatures exactly (including const-ness)
2. Keep member name as `ledgerMaster` to avoid find/replace across all handlers
3. Use `override` specifiers in LedgerMaster to catch signature mismatches
4. Build after each batch to catch issues early

**Example of signature matching**:
```cpp
// LedgerMaster.h (existing)
std::shared_ptr<Ledger const> getValidatedLedger();

// LedgerDataProvider.h (new interface) - MUST MATCH
virtual std::shared_ptr<Ledger const> getValidatedLedger() = 0;
```

### Medium Risk: Test Coverage Gaps

**Risk**: Some handlers may have edge cases not covered by existing tests that could break.

**Mitigation**:
1. Run full test suite after each batch
2. Add tests for any discovered gaps
3. Create `MockLedgerDataProvider` to enable new unit tests
4. Test both standalone and network modes

**Test commands**:
```bash
# Run RPC-specific tests
./build/rippled --unittest="rpc"

# Run ledger-specific tests
./build/rippled --unittest="ledger"

# Run full test suite
./build/rippled --unittest
```

### Low Risk: Performance Implications of Virtual Dispatch

**Risk**: Virtual function calls add slight overhead (~1-2 nanoseconds per call).

**Mitigation**:
1. RPC handlers are network-bound (milliseconds), not CPU-bound
2. Measure before/after with benchmarks if concerned
3. Consider marking interface methods as `final` in LedgerMaster if needed

**Benchmark approach**:
```cpp
// Simple timing comparison (if needed)
auto start = std::chrono::high_resolution_clock::now();
for (int i = 0; i < 1000000; ++i) {
    ledgerMaster.getCurrentLedgerIndex();
}
auto end = std::chrono::high_resolution_clock::now();
// Compare direct vs interface call times
```

### Low Risk: gRPC Context Compatibility

**Risk**: `GRPCContext<T>` inherits from `Context` and may have special requirements.

**Mitigation**:
1. `GRPCContext` uses same `ledgerMaster` member - no changes needed
2. Verify gRPC handlers compile and work after migration
3. Test gRPC endpoints specifically

### Rollback Strategy

1. **Git Revert**: All changes are atomic commits that can be reverted
2. **Verification**: After revert, run full test suite to confirm working state
3. **Time to Rollback**: < 5 minutes

---

## Validation Criteria

### 1. Expected Reduction in loops.txt

**Before** (estimated from analysis):
```
app <-> rpc: 174 includes
  rpc -> app: 153
  app -> rpc: 21
```

**After** (expected):
```
app <-> rpc: ~160 includes (estimated)
  rpc -> app: 140-145 (reduced by ~10-15)
  app -> rpc: 21 (may reduce if app no longer needs RPC types)
```

**Specific includes that should be eliminated**:
- `src/xrpld/rpc/handlers/*.cpp` → `src/xrpld/app/ledger/LedgerMaster.h`

### 2. Build Verification

```bash
# Clean build must succeed
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target rippled -j$(nproc)
echo "Build verification: PASSED"
```

### 3. Test Verification

```bash
# All unit tests must pass
./build/rippled --unittest 2>&1 | tee test_results.txt
grep -E "(Passed|Failed)" test_results.txt | tail -5

# Specific RPC tests
./build/rippled --unittest="AccountInfo"
./build/rippled --unittest="Ledger"
./build/rippled --unittest="Submit"
```

### 4. Interface Completeness Check

```bash
# Verify no direct LedgerMaster access in RPC handlers
grep -r "LedgerMaster" src/xrpld/rpc/handlers/ --include="*.cpp" --include="*.h"
# Expected: No results (all should use LedgerDataProvider)

# Verify Context.h uses interface
grep "LedgerDataProvider" src/xrpld/rpc/Context.h
# Expected: Should find the interface reference
```

### 5. Include Graph Analysis

```bash
# Generate new include analysis
# (Use same tool from ARCHITECTURAL_ANALYSIS.md)
python3 tools/analyze_includes.py > loops_after.txt

# Compare before/after
diff loops_before.txt loops_after.txt | head -50
```

---

## Appendix: Complete File Change Summary

### New Files
| File | Purpose |
|------|---------|
| `src/xrpld/core/LedgerDataProvider.h` | Abstract interface for ledger data access |

### Modified Files
| File | Change Type | Description |
|------|-------------|-------------|
| `src/xrpld/app/ledger/LedgerMaster.h` | Inheritance | Add `LedgerDataProvider` base class |
| `src/xrpld/rpc/Context.h` | Type change | `LedgerMaster&` → `LedgerDataProvider&` |
| `src/xrpld/rpc/detail/RPCLedgerHelpers.cpp` | Uses interface | Update helper function implementations |
| `src/xrpld/app/main/GRPCServer.cpp` | Context creation | No changes if type matches |
| 30+ RPC handler files | Compilation | Should compile unchanged if interface matches |

### Test Files to Review
| File | Action |
|------|--------|
| `src/test/rpc/*.cpp` | Verify context creation, add mock if needed |
| `src/test/app/LedgerMaster_test.cpp` | Verify LedgerMaster still works |

---

## Timeline Estimate

| Phase | Duration | Cumulative |
|-------|----------|------------|
| Step 1: Define Interface | 0.5 days | 0.5 days |
| Step 2: LedgerMaster implements interface | 0.5 days | 1 day |
| Step 3: Update RPC::Context | 0.5 days | 1.5 days |
| Step 4: Migrate handlers (9 batches) | 2-3 days | 4.5 days |
| Step 5: Update tests | 1 day | 5.5 days |
| Step 6: Final verification | 0.5 days | 6 days |

**Total Estimated Time**: 5-6 working days

---

## Related Tasks

- **Phase 1**: Module structure analysis (prerequisite - completed)
- **Phase 2 Task 2.2**: NetworkOPs interface extraction (similar pattern)
- **Phase 2 Task 2.3**: Application interface refinement
- **Phase 3**: Build system integration with new module boundaries

