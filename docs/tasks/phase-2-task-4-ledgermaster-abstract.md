# Phase 2 Task 2.4: Abstract LedgerMaster

## Overview

### Problem Statement
`LedgerMaster` is currently a concrete class with many responsibilities including ledger tracking, transaction holding, path finding coordination, and fetch pack management. This monolithic design:
- Makes unit testing difficult (cannot mock LedgerMaster for testing RPC handlers)
- Creates tight coupling between RPC layer and the app layer
- Violates the Interface Segregation Principle (clients must depend on the entire LedgerMaster interface)

### Success Criteria
1. RPC handlers can use `LedgerDataProvider&` instead of `LedgerMaster&` directly
2. Unit tests can mock ledger data access without requiring full LedgerMaster
3. Reduced coupling visible in dependency analysis (fewer direct LedgerMaster.h includes)
4. No performance regression for high-frequency ledger access operations

### Dependencies
- **Task 2.1**: Create `LedgerDataProvider` interface (MUST complete first)
- **Phase 1**: Architecture review and dependency analysis complete

---

## Deep Code Analysis

### Current File Locations
- **Header**: `src/xrpld/app/ledger/LedgerMaster.h`
- **Implementation**: `src/xrpld/app/ledger/detail/LedgerMaster.cpp` (2246 lines)

### Existing Base Class
LedgerMaster already inherits from `AbstractFetchPackContainer`:

```cpp
// src/xrpld/app/ledger/AbstractFetchPackContainer.h
class AbstractFetchPackContainer {
public:
    virtual ~AbstractFetchPackContainer() = default;
    virtual std::optional<Blob> getFetchPack(uint256 const& nodeHash) = 0;
};
```

### Full Public Interface Classification

#### READ-ONLY Operations (Candidates for LedgerDataProvider)
| Method | Description | RPC Usage |
|--------|-------------|-----------|
| `getCurrentLedgerIndex()` | Returns current ledger sequence | High |
| `getValidLedgerIndex()` | Returns validated ledger sequence | High |
| `getCurrentLedger()` | Returns current open ledger ReadView | Medium |
| `getClosedLedger()` | Returns last closed ledger | High |
| `getValidatedLedger()` | Returns last validated ledger | High |
| `getValidatedRules()` | Returns Rules from validated ledger | Medium |
| `getPublishedLedger()` | Returns last published ledger | Low |
| `getPublishedLedgerAge()` | Age of published ledger | Low |
| `getValidatedLedgerAge()` | Age of validated ledger | High |
| `getLedgerBySeq(uint32_t)` | Fetch ledger by sequence | High |
| `getLedgerByHash(uint256)` | Fetch ledger by hash | High |
| `getValidatedRange(min, max)` | Get validated ledger range | High |
| `getFullValidatedRange(min, max)` | Get full validated range | Medium |
| `getHashBySeq(uint32_t)` | Get ledger hash by sequence | Medium |
| `getCloseTimeBySeq(LedgerIndex)` | Get close time by sequence | Low |
| `getCloseTimeByHash(hash, index)` | Get close time by hash | Low |
| `getCompleteLedgers()` | String of complete ledger ranges | Low |
| `isCaughtUp(reason)` | Check if server is caught up | Medium |
| `isCompatible(ReadView)` | Check ledger compatibility | Low |
| `haveLedger(seq)` | Check if ledger exists | Medium |
| `isValidated(ReadView)` | Check if ledger is validated | Medium |
| `haveValidated()` | Check if any ledger validated | Low |
| `minSqlSeq()` | Minimum SQL sequence | Low |
| `txnIdFromIndex(seq, index)` | Get txn ID from indices | Low |

#### MUTATING Operations (Internal/Consensus)
| Method | Description |
|--------|-------------|
| `setFullLedger(ledger, sync, current)` | Store a complete ledger |
| `switchLCL(lastClosed)` | Switch last closed ledger |
| `consensusBuilt(ledger, hash, json)` | Called when consensus completes |
| `checkAccept(ledger)` / `checkAccept(hash, seq)` | Check if ledger should be accepted |
| `setBuildingLedger(index)` | Set ledger being built |
| `storeLedger(ledger)` | Store ledger to database |
| `failedSave(seq, hash)` | Mark ledger save as failed |
| `setLedgerRangePresent(min, max)` | Set available range |
| `clearPriorLedgers(seq)` | Clear ledgers before sequence |
| `clearLedgerCachePrior(seq)` | Clear cache before sequence |
| `clearLedger(seq)` | Clear specific ledger |
| `fixMismatch(ledger)` | Fix ledger mismatch |
| `fixIndex(index, hash)` | Fix ledger index |

#### Transaction Holding Operations
| Method | Description |
|--------|-------------|
| `addHeldTransaction(trans)` | Add transaction to hold |
| `applyHeldTransactions()` | Apply held transactions |
| `popAcctTransaction(tx)` | Pop next account transaction |

#### Path Finding Operations
| Method | Description |
|--------|-------------|
| `newPathRequest()` | Request new path finding |
| `isNewPathRequest()` | Check for new request |
| `newOrderBookDB()` | Update order book database |
| `tryAdvance()` | Try to advance ledger processing |

#### Fetch Pack Operations (AbstractFetchPackContainer)
| Method | Description |
|--------|-------------|
| `getFetchPack(hash)` | Get fetch pack data (override) |
| `addFetchPack(hash, data)` | Add fetch pack data |
| `makeFetchPack(peer, request, hash, uptime)` | Create fetch pack |
| `gotFetchPack(progress, seq)` | Handle received fetch pack |
| `getFetchPackCacheSize()` | Get cache size |

#### Ledger Replay Operations
| Method | Description |
|--------|-------------|
| `takeReplay(replay)` | Take replay data |
| `releaseReplay()` | Release replay data |

### RPC Handler Usage Patterns

Common patterns in RPC handlers (via `context.ledgerMaster`):
1. `getCurrentLedgerIndex()` - LedgerCurrent, LedgerAccept
2. `getClosedLedger()` - LedgerClosed, RipplePathFind
3. `getValidatedLedger()` - many handlers via helper functions
4. `getValidatedLedgerAge()` - Submit, Sign, SignFor, SubmitMultiSigned
5. `getLedgerBySeq(index)` - RPCLedgerHelpers, CanDelete
6. `getLedgerByHash(hash)` - CanDelete
7. `getValidatedRange(min, max)` - AccountTx
8. `isValidated(ledger)` - AccountTx
9. `getValidLedgerIndex()` - RPCLedgerHelpers

### All Call Sites Using LedgerMaster

Key locations accessing `getLedgerMaster()`:
- `src/xrpld/rpc/Context.h` - Stores `LedgerMaster&` reference
- `src/xrpld/app/main/Application.cpp` - Creates and owns LedgerMaster
- `src/xrpld/rpc/detail/RPCLedgerHelpers.cpp` - Heavy usage
- 30+ RPC handlers in `src/xrpld/rpc/handlers/`
- `src/xrpld/overlay/detail/PeerImp.cpp` - Peer operations
- Various consensus and ledger processing components

---

## Design Considerations

### Option A: LedgerMaster Implements LedgerDataProvider (RECOMMENDED)

```cpp
// After Task 2.1 creates LedgerDataProvider in src/xrpld/core/LedgerDataProvider.h
class LedgerMaster : public AbstractFetchPackContainer,
                     public LedgerDataProvider {
public:
    // LedgerDataProvider interface (virtual override)
    std::shared_ptr<Ledger const> getValidatedLedger() override;
    std::shared_ptr<Ledger const> getClosedLedger() override;
    std::shared_ptr<Ledger const> getLedgerBySeq(std::uint32_t index) override;
    std::shared_ptr<Ledger const> getLedgerByHash(uint256 const& hash) override;
    LedgerIndex getValidLedgerIndex() override;
    LedgerIndex getCurrentLedgerIndex() override;
    bool getValidatedRange(std::uint32_t& minVal, std::uint32_t& maxVal) override;
    std::chrono::seconds getValidatedLedgerAge() override;

    // Existing LedgerMaster-only methods remain unchanged...
};
```

**Advantages:**
- Minimal code changes - existing methods already match signatures
- No wrapper overhead or delegation
- Clear separation: RPC uses LedgerDataProvider, internal uses LedgerMaster
- Follows Interface Segregation Principle

**Disadvantages:**
- LedgerMaster now has two base classes (manageable)
- Virtual dispatch overhead on interface methods (acceptable for RPC)

### Option B: Create Separate LedgerMasterInterface Abstract Class

```cpp
// Create new abstract base for all LedgerMaster functionality
class LedgerMasterInterface : public AbstractFetchPackContainer {
public:
    virtual std::shared_ptr<Ledger const> getValidatedLedger() = 0;
    virtual void consensusBuilt(...) = 0;
    // ... all 50+ methods as pure virtual
};

class LedgerMaster : public LedgerMasterInterface { /* impl */ };
```

**Advantages:**
- Full abstraction of all LedgerMaster functionality
- Maximum flexibility for alternative implementations

**Disadvantages:**
- Massive interface (50+ methods) - violates Interface Segregation
- Significant refactoring effort
- All call sites need updating
- Doesn't solve the real problem (RPC coupling)

### Option C: Composition - LedgerMaster Contains LedgerDataProvider

```cpp
class LedgerDataProviderImpl : public LedgerDataProvider {
    LedgerMaster& master_;
public:
    std::shared_ptr<Ledger const> getValidatedLedger() override {
        return master_.getValidatedLedger();
    }
    // ... delegate all methods
};

class LedgerMaster : public AbstractFetchPackContainer {
    std::unique_ptr<LedgerDataProviderImpl> dataProvider_;
public:
    LedgerDataProvider& getDataProvider() { return *dataProvider_; }
};
```

**Advantages:**
- Clear separation of concerns
- LedgerMaster interface unchanged

**Disadvantages:**
- Extra indirection and object lifetime management
- Circular dependency between LedgerMaster and provider
- More complex than Option A with no significant benefit

### Recommendation: Option A

Option A is recommended because:
1. **Minimal disruption**: Existing LedgerMaster methods already match the interface
2. **Interface Segregation**: RPC depends only on narrow LedgerDataProvider interface
3. **No performance overhead**: Same object, just accessed via different interface pointer
4. **Testability**: Mock LedgerDataProvider for unit tests without full LedgerMaster
5. **Incremental adoption**: Can migrate RPC handlers one at a time

---

## Implementation Plan

### Prerequisites
- [ ] Task 2.1 complete: `LedgerDataProvider` interface defined

### Step 1: Update LedgerMaster Inheritance (1 day)

**File**: `src/xrpld/app/ledger/LedgerMaster.h`

```cpp
// Add include
#include <xrpld/core/LedgerDataProvider.h>

// Update class declaration
class LedgerMaster : public AbstractFetchPackContainer,
                     public LedgerDataProvider
{
    // ... existing code
};
```

### Step 2: Add Override Specifiers (1 day)

Update method declarations to include `override` for interface methods:

```cpp
// Example changes in LedgerMaster.h
LedgerIndex getCurrentLedgerIndex() override;
LedgerIndex getValidLedgerIndex() override;
std::shared_ptr<Ledger const> getClosedLedger() override;
std::shared_ptr<Ledger const> getValidatedLedger() override;
std::shared_ptr<Ledger const> getLedgerBySeq(std::uint32_t index) override;
std::shared_ptr<Ledger const> getLedgerByHash(uint256 const& hash) override;
bool getValidatedRange(std::uint32_t& minVal, std::uint32_t& maxVal) override;
std::chrono::seconds getValidatedLedgerAge() override;
```

### Step 3: Update Application Interface (1 day)

**File**: `src/xrpld/app/main/Application.h`

Add method to expose LedgerDataProvider:

```cpp
class Application {
public:
    // Existing
    virtual LedgerMaster& getLedgerMaster() = 0;

    // NEW: Narrow interface for RPC
    virtual LedgerDataProvider& getLedgerDataProvider() = 0;
};
```

**File**: `src/xrpld/app/main/Application.cpp`

```cpp
LedgerDataProvider& ApplicationImp::getLedgerDataProvider() override {
    return *m_ledgerMaster;  // LedgerMaster IS-A LedgerDataProvider
}
```

### Step 4: Update RPC Context (1 day)

**File**: `src/xrpld/rpc/Context.h`

```cpp
struct Context {
    // Change from concrete to interface reference
    LedgerDataProvider& ledgerDataProvider;  // NEW

    // Keep for backward compatibility during migration
    LedgerMaster& ledgerMaster;  // DEPRECATED - remove after migration
};
```

### Step 5: Migrate RPC Handlers (2-3 weeks)

Incrementally update RPC handlers to use `context.ledgerDataProvider` instead of `context.ledgerMaster`. Priority order:

**High Priority** (heavy LedgerMaster usage):
1. `RPCLedgerHelpers.cpp` - Core helper functions
2. `LedgerCurrent.cpp`, `LedgerClosed.cpp` - Simple, good pilot
3. `AccountTx.cpp` - Uses getValidatedRange, isValidated
4. `Submit.cpp`, `SignHandler.cpp` - Use getValidatedLedgerAge

**Medium Priority**:
5. `CanDelete.cpp` - Uses getLedgerByHash
6. `RipplePathFind.cpp` - Uses getClosedLedger
7. Other handlers using context.ledgerMaster

### Step 6: Remove Deprecated Reference (1 day)

After all handlers migrated:
1. Remove `LedgerMaster& ledgerMaster` from `RPC::Context`
2. Update context creation to not require LedgerMaster reference
3. Final cleanup pass

---

## Risk Assessment

### Virtual Dispatch Overhead
**Risk**: Performance degradation for high-frequency calls
**Mitigation**:
- RPC calls are already I/O bound
- Virtual dispatch overhead (~1-2 ns) negligible vs network latency
- Profile critical paths after implementation
**Severity**: Low

### Thread Safety Considerations
**Risk**: Interface methods may have different thread safety requirements
**Mitigation**:
- LedgerMaster already handles thread safety internally
- Interface methods are read-only, reducing contention
- Document thread safety guarantees in LedgerDataProvider
**Severity**: Low

### Interaction with AbstractFetchPackContainer
**Risk**: Multiple inheritance complications
**Mitigation**:
- Both interfaces are pure abstract (no diamond problem)
- AbstractFetchPackContainer has single method
- Virtual destructors properly defined
**Severity**: Very Low

### Migration Complexity
**Risk**: Large number of files to update
**Mitigation**:
- Incremental migration with backward compatibility
- Keep both references during transition
- Automated testing catches regressions
**Severity**: Medium

### Breaking Changes to External Tools
**Risk**: External tools may depend on LedgerMaster interface
**Mitigation**:
- LedgerMaster interface unchanged
- Only adding new base class
- RPC JSON interface unchanged
**Severity**: Very Low

### Rollback Strategy

1. **Git Revert**: All changes are atomic commits that can be reverted
2. **Verification**: After revert, run full test suite to confirm working state
3. **Time to Rollback**: < 5 minutes

---

## Validation Criteria

### Functional Validation
- [ ] All existing unit tests pass
- [ ] All RPC integration tests pass
- [ ] Ledger operations work correctly in standalone mode
- [ ] Consensus continues to function in network mode

### Architecture Validation
- [ ] RPC handlers use `LedgerDataProvider&` (not `LedgerMaster&`)
- [ ] Dependency analysis shows reduced LedgerMaster.h includes
- [ ] Mock LedgerDataProvider can be created for unit tests

### Performance Validation
- [ ] No measurable performance regression in RPC response times
- [ ] Ledger access latency unchanged
- [ ] Memory footprint unchanged

### Code Quality Validation
- [ ] No new compiler warnings
- [ ] Code follows existing style guidelines
- [ ] Documentation updated for new interface

---

## Files to Modify

| File | Change Type | Description |
|------|-------------|-------------|
| `src/xrpld/app/ledger/LedgerMaster.h` | Modify | Add LedgerDataProvider inheritance, override specifiers |
| `src/xrpld/app/main/Application.h` | Modify | Add getLedgerDataProvider() method |
| `src/xrpld/app/main/Application.cpp` | Modify | Implement getLedgerDataProvider() |
| `src/xrpld/rpc/Context.h` | Modify | Add LedgerDataProvider reference |
| `src/xrpld/rpc/handlers/*.cpp` | Modify | Migrate to use LedgerDataProvider |
| `src/xrpld/rpc/detail/RPCLedgerHelpers.cpp` | Modify | Migrate helper functions |

---

## Estimated Effort

| Task | Effort |
|------|--------|
| Update LedgerMaster inheritance | 1 day |
| Add override specifiers | 1 day |
| Update Application interface | 1 day |
| Update RPC Context | 1 day |
| Migrate RPC handlers | 2-3 weeks |
| Testing and validation | 3-4 days |
| **Total** | **3-4 weeks** |

---

## Related Tasks
- **Task 2.1**: Create LedgerDataProvider interface (prerequisite)
- **Task 2.2**: Break rpc → app cycle (related)
- **Task 2.3**: Simplify RPC handler dependencies (benefits from this task)

