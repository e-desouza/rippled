# Phase 3 Task 3.6: Decouple RCLConsensus::Adaptor from Concrete Dependencies

## Overview

### Problem Statement

`RCLConsensus::Adaptor` is an inner class that implements the template interface required by `Consensus<Adaptor>`. Currently, it has direct dependencies on multiple concrete application components:

1. **Application& app_** - The monolithic Application object, used to access:
   - `JobQueue` for scheduling async work
   - `Overlay` for peer communication (relay, broadcast, foreach)
   - `HashRouter` for message suppression
   - `OpenLedger` for transaction state
   - `TimeKeeper` for time synchronization
   - `Validations` for validation tracking
   - `Config` for configuration
   - `TxQ` for transaction queue management
   - `NetworkOPs` for operating mode control
   - `InboundLedgers` for ledger acquisition

2. **LedgerMaster& ledgerMaster_** - For ledger access and validation range queries
3. **LocalTxs& localTxs_** - For local transaction management
4. **InboundTransactions& inboundTransactions_** - For transaction set management
5. **ValidatorKeys const& validatorKeys_** - For signing and identity

This tight coupling creates several problems:
- **Untestable**: Cannot unit test consensus logic without full Application mock
- **Circular Dependencies**: Consensus depends on app layer, which depends on consensus
- **Difficult Refactoring**: Changes to any subsystem require changes to Adaptor
- **No Isolation**: Consensus cannot be developed/tested independently

### Target Architecture

Create focused interfaces that represent the specific capabilities the Adaptor needs:

```
┌──────────────────────────────────────────────────────────────────────────┐
│                         Consensus<Adaptor>                                │
│                              (generic)                                    │
└───────────────────────────────────┬──────────────────────────────────────┘
                                    │
                                    ▼
┌──────────────────────────────────────────────────────────────────────────┐
│                          RCLConsensus::Adaptor                           │
│                                                                          │
│  Dependencies (injected interfaces):                                     │
│  ┌────────────────┐ ┌──────────────────┐ ┌────────────────────────┐     │
│  │ILedgerProvider │ │IOverlayBroadcaster│ │IConsensusJobScheduler │     │
│  └────────────────┘ └──────────────────┘ └────────────────────────┘     │
│  ┌────────────────┐ ┌──────────────────┐ ┌────────────────────────┐     │
│  │ITxSetManager   │ │IValidationTracker│ │IConsensusTimeSource    │     │
│  └────────────────┘ └──────────────────┘ └────────────────────────┘     │
│  ┌────────────────┐ ┌──────────────────┐                                │
│  │IOperatingMode  │ │IMessageRouter    │                                │
│  └────────────────┘ └──────────────────┘                                │
└──────────────────────────────────────────────────────────────────────────┘
```

### Success Criteria

1. All `app_.getXXX()` calls replaced with interface method calls
2. `LedgerMaster` accessed only through `ILedgerProvider` interface
3. Unit tests can verify consensus behavior with mock implementations
4. No compile-time dependency on `Application.h` from Adaptor (only interfaces)
5. All existing integration tests continue to pass

### Dependencies

- **Phase 2 Task 2.1**: LedgerDataProvider interface (provides foundation)
- **Phase 2 Task 2.4**: LedgerMaster abstraction (partially complete)
- **Phase 3 Task 3.1**: Split app/misc directory (reduces header size)

### Estimated Effort

| Component | Effort |
|-----------|--------|
| Interface design and header files | 2-3 days |
| LedgerMaster wrapper implementation | 1 day |
| Overlay/HashRouter wrapper implementation | 2 days |
| JobQueue/TimeKeeper wrapper implementation | 1 day |
| NetworkOPs wrapper implementation | 1 day |
| Adaptor refactoring | 3-4 days |
| Application wiring updates | 1 day |
| Unit test creation | 3-4 days |
| Integration testing | 2-3 days |
| **Total** | **16-20 days** |

---

## Deep Code Analysis

### Current Adaptor Structure

**File**: `src/xrpld/app/consensus/RCLConsensus.h` (554 lines)
**Implementation**: `src/xrpld/app/consensus/RCLConsensus.cpp` (1108 lines)

```cpp
class RCLConsensus {
    class Adaptor {
        Application& app_;                           // Line 42
        std::unique_ptr<FeeVote> feeVote_;          // Line 43
        LedgerMaster& ledgerMaster_;                 // Line 44
        LocalTxs& localTxs_;                         // Line 45
        InboundTransactions& inboundTransactions_;   // Line 46
        beast::Journal const j_;                     // Line 47
        ValidatorKeys const& validatorKeys_;         // Line 50
        // ... other members
    };
};
```

### Application Method Usage Summary

Summary of `app_.` method calls in `RCLConsensus.cpp` grouped by proposed interface:

| Proposed Interface | Methods Used | Occurrence Count |
|--------------------|--------------|------------------|
| `IConsensusJobScheduler` | `getJobQueue().addJob()` | 3 |
| `ILedgerProvider` | `getInboundLedgers().acquireAsync()`, ledger queries | 8 |
| `IMessageRouter` | `getHashRouter().shouldRelay()`, `addSuppression()` | 4 |
| `IConsensusTimeSource` | `timeKeeper().now()`, `closeTime()`, `adjustCloseTime()` | 5 |
| `IOverlayBroadcaster` | `overlay().relay()`, `broadcast()`, `foreach()` | 7 |
| `IOpenLedgerState` | `openLedger().empty()`, `current()`, `accept()` | 3 |
| `IValidationTracker` | `getValidations().*` | 6 |
| `IOperatingMode` | `getOPs().*` (mode, consensus, fee) | 8 |
| Pass at construction | `journal()`, `config()`, `getNodeFamily()` | 4 |
| Keep existing pattern | Amendment/fee voting pseudo-tx handling | 2 |

### LedgerMaster Method Usage

Key methods: `getLedgerByHash()`, `getValidLedgerIndex()`, `getValidatedLedger()`, `applyHeldTransactions()`, `setBuildingLedger()`, `consensusBuilt()`, `switchLCL()`, `getFullValidatedRange()`, `getEarliestFetch()`, `releaseReplay()`, `storeLedger()`, `haveValidated()`

### InboundTransactions Method Usage

Key methods: `newRound()`, `giveSet()`, `getSet()`

### LocalTxs Method Usage

Key method: `getTxSet()` (line 643)

---

## Design Considerations

### Option A: Interface Per Responsibility (RECOMMENDED)

Create focused, single-responsibility interfaces:

```cpp
// src/xrpld/consensus/interfaces/ILedgerProvider.h
class ILedgerProvider {
public:
    virtual ~ILedgerProvider() = default;

    virtual std::shared_ptr<Ledger const>
    getLedgerByHash(LedgerHash const& hash) = 0;

    virtual bool
    getFullValidatedRange(std::uint32_t& min, std::uint32_t& max) = 0;

    virtual LedgerIndex getEarliestFetch() = 0;
    virtual LedgerIndex getValidLedgerIndex() = 0;
    virtual std::shared_ptr<Ledger const> getValidatedLedger() = 0;
    virtual bool haveValidated() const = 0;
    virtual bool isCompatible(ReadView const&, beast::Journal::Stream, char const* reason) = 0;

    // Mutating operations needed by consensus
    virtual void applyHeldTransactions() = 0;
    virtual void setBuildingLedger(LedgerIndex) = 0;
    virtual bool storeLedger(std::shared_ptr<Ledger const>) = 0;
    virtual void switchLCL(std::shared_ptr<Ledger const>) = 0;
    virtual void consensusBuilt(std::shared_ptr<Ledger const>,
                                uint256 const&, Json::Value) = 0;
    virtual std::unique_ptr<LedgerReplay> releaseReplay() = 0;
};

// src/xrpld/consensus/interfaces/IOverlayBroadcaster.h
class IOverlayBroadcaster {
public:
    virtual ~IOverlayBroadcaster() = default;

    virtual void broadcast(protocol::TMProposeSet&) = 0;
    virtual void broadcast(protocol::TMValidation&) = 0;

    virtual std::set<Peer::id_t>
    relay(protocol::TMProposeSet&, uint256 const&, PublicKey const&) = 0;

    virtual void
    relay(uint256 const&, protocol::TMTransaction const&,
          std::set<Peer::id_t> const&) = 0;

    template<typename Func>
    void foreach(Func f);  // For status notifications
};

// src/xrpld/consensus/interfaces/IConsensusJobScheduler.h
class IConsensusJobScheduler {
public:
    virtual ~IConsensusJobScheduler() = default;

    virtual void addJob(JobType type, std::string const& name,
                        std::function<void()> job) = 0;
};

// src/xrpld/consensus/interfaces/ITxSetManager.h
class ITxSetManager {
public:
    virtual ~ITxSetManager() = default;

    virtual std::shared_ptr<SHAMap>
    getSet(uint256 const& hash, bool acquire) = 0;

    virtual void
    giveSet(uint256 const& hash, std::shared_ptr<SHAMap> const&, bool) = 0;

    virtual void newRound(std::uint32_t seq) = 0;
};

// src/xrpld/consensus/interfaces/IMessageRouter.h
class IMessageRouter {
public:
    virtual ~IMessageRouter() = default;

    virtual bool shouldRelay(uint256 const& key) = 0;
    virtual void addSuppression(uint256 const& key) = 0;
};

// src/xrpld/consensus/interfaces/IConsensusTimeSource.h
class IConsensusTimeSource {
public:
    virtual ~IConsensusTimeSource() = default;

    virtual NetClock::time_point now() const = 0;
    virtual NetClock::time_point closeTime() const = 0;
    virtual void adjustCloseTime(std::chrono::duration<std::int32_t>) = 0;
};

// src/xrpld/consensus/interfaces/IValidationTracker.h
class IValidationTracker {
public:
    virtual ~IValidationTracker() = default;

    virtual std::size_t numTrustedForLedger(LedgerHash const&) const = 0;
    virtual std::size_t getNodesAfter(RCLValidatedLedger const&,
                                      LedgerHash const&) const = 0;
    virtual uint256 getPreferred(RCLValidatedLedger const&,
                                 LedgerIndex) const = 0;
    virtual bool canValidateSeq(LedgerIndex) const = 0;
    virtual std::size_t laggards(LedgerIndex, hash_set<PublicKey>&) const = 0;
    virtual Json::Value getJsonTrie() const = 0;
};

// src/xrpld/consensus/interfaces/IOperatingMode.h
class IOperatingMode {
public:
    virtual ~IOperatingMode() = default;

    virtual OperatingMode getOperatingMode() const = 0;
    virtual bool isFull() const = 0;
    virtual bool isBlocked() const = 0;
    virtual void setMode(OperatingMode) = 0;
    virtual void consensusViewChange() = 0;
    virtual void endConsensus(std::unique_ptr<std::stringstream> const&) = 0;
    virtual void reportFeeChange() = 0;
    virtual void pubValidation(std::shared_ptr<STValidation> const&) = 0;
};

// src/xrpld/consensus/interfaces/IOpenLedgerState.h
class IOpenLedgerState {
public:
    virtual ~IOpenLedgerState() = default;

    virtual bool empty() const = 0;
    virtual std::shared_ptr<OpenView const> current() const = 0;
    virtual void accept(Application&, Rules const&,
                        std::shared_ptr<Ledger const>,
                        CanonicalTXSet const&, bool, CanonicalTXSet&,
                        ApplyFlags, char const*,
                        std::function<bool(OpenView&, beast::Journal)>) = 0;
};
```

**Advantages:**
- Single Responsibility Principle - each interface has one reason to change
- Easy to mock individual capabilities for testing
- Can evolve interfaces independently
- Clear documentation of consensus requirements
- Enables granular dependency injection

**Disadvantages:**
- More interface classes to maintain
- Adaptor constructor has many parameters
- Wrapper implementations needed for each interface

### Option B: Facade Pattern (Single Aggregated Interface)

Single `IConsensusServices` interface with 50+ methods aggregating all capabilities.

**Advantages:** Single dependency, simpler constructor
**Disadvantages:** Violates ISP, hard to mock all methods, large implementation

### Option C: Callback-Based Approach

Pass `std::function` callbacks for each capability instead of interface references.

**Advantages:** Maximum flexibility, minimal test mocks
**Disadvantages:** Unwieldy constructor (15+ callbacks), no type safety for related operations

### Recommendation: Option A (Interface Per Responsibility)

Option A is recommended because:

1. **Testability**: Each interface can be independently mocked
2. **Clarity**: Interfaces document exact consensus requirements
3. **Evolution**: Interfaces can evolve independently
4. **ISP Compliance**: Small, focused interfaces
5. **Implementation Reuse**: Wrapper implementations can be shared

To manage constructor complexity, use a factory pattern:

```cpp
struct ConsensusAdaptorDependencies {
    ILedgerProvider& ledgerProvider;
    IOverlayBroadcaster& broadcaster;
    IConsensusJobScheduler& scheduler;
    ITxSetManager& txSetManager;
    IMessageRouter& messageRouter;
    IConsensusTimeSource& timeSource;
    IValidationTracker& validationTracker;
    IOperatingMode& operatingMode;
    IOpenLedgerState& openLedgerState;
    ValidatorKeys const& validatorKeys;
    std::unique_ptr<FeeVote> feeVote;
    beast::Journal journal;
};

class Adaptor {
public:
    explicit Adaptor(ConsensusAdaptorDependencies deps);
};
```

---

## Thread Safety Considerations

### Current Thread Safety Model

From `RCLConsensus.h` comments (lines 512-515):
```cpp
// Since Consensus does not provide intrinsic thread-safety, this mutex
// guards all calls to consensus_. adaptor_ uses atomics internally
// to allow concurrent access of its data members that have getters.
mutable std::recursive_mutex mutex_;
```

Key atomic members in Adaptor:
- `std::atomic<bool> validating_` (line 64)
- `std::atomic<std::size_t> prevProposers_` (line 65)
- `std::atomic<std::chrono::milliseconds> prevRoundTime_` (line 66-67)
- `std::atomic<ConsensusMode> mode_` (line 68)

### Interface Thread Safety Requirements

Each interface implementation must handle thread safety internally:

| Interface | Thread Safety Requirement |
|-----------|--------------------------|
| `ILedgerProvider` | Must be thread-safe - called from job threads |
| `IOverlayBroadcaster` | Must be thread-safe - network operations |
| `IConsensusJobScheduler` | Must be thread-safe - job queue operations |
| `ITxSetManager` | Must be thread-safe - concurrent access |
| `IMessageRouter` | Must be thread-safe - hash router uses mutex |
| `IConsensusTimeSource` | Thread-safe reads, rare writes |
| `IValidationTracker` | Must be thread-safe - validation access |
| `IOperatingMode` | Uses atomics for mode state |
| `IOpenLedgerState` | Must coordinate with master mutex |

### Critical Section: `doAccept()`

The `doAccept()` method (lines 440-710) is called from a job queue and:
1. Does NOT hold the consensus mutex
2. Relies on consensus state not changing until `startRound()` is called
3. Must coordinate with `app_.getMasterMutex()` and `ledgerMaster_.peekMutex()`

Interface implementations must preserve this locking protocol.

---

## Implementation Plan

### Phase 1: Interface Definition (Week 1)

Create interface headers in `src/xrpld/consensus/interfaces/`: `ILedgerProvider.h`, `IOverlayBroadcaster.h`, `IConsensusJobScheduler.h`, `ITxSetManager.h`, `IMessageRouter.h`, `IConsensusTimeSource.h`, `IValidationTracker.h`, `IOperatingMode.h`, `IOpenLedgerState.h`, `ConsensusAdaptorDeps.h`

Implement interfaces as defined in **Option A** above.

**Verification**: All headers compile independently, unit tests pass

### Phase 2: Wrapper Implementations (Week 2)

Create thin wrapper classes in `src/xrpld/consensus/detail/`:

| Wrapper Class | Wraps | File |
|--------------|-------|------|
| `LedgerProviderImpl` | `LedgerMaster` + `InboundLedgers` | `LedgerProviderImpl.h` |
| `OverlayBroadcasterImpl` | `Overlay` | `OverlayBroadcasterImpl.h` |
| `JobSchedulerImpl` | `JobQueue` | `JobSchedulerImpl.h` |
| `TxSetManagerImpl` | `InboundTransactions` | `TxSetManagerImpl.h` |
| `MessageRouterImpl` | `HashRouter` | `MessageRouterImpl.h` |
| `TimeSourceImpl` | `TimeKeeper` | `TimeSourceImpl.h` |
| `ValidationTrackerImpl` | `RCLValidations` | `ValidationTrackerImpl.h` |
| `OperatingModeImpl` | `NetworkOPs` | `OperatingModeImpl.h` |
| `OpenLedgerStateImpl` | `OpenLedger` | `OpenLedgerStateImpl.h` |

Each wrapper holds reference to wrapped object, delegates all calls directly.

**Verification**: Each wrapper compiles and can be instantiated

### Phase 3: Adaptor Refactoring (Week 3)

**Step 3.1**: Replace member variables - change from concrete types (`Application&`, `LedgerMaster&`, etc.) to interface references (`ILedgerProvider&`, `IOverlayBroadcaster&`, etc.)

**Step 3.2**: Update constructor to accept `ConsensusAdaptorDependencies` struct. Keep deprecated constructor for backward compatibility.

**Step 3.3**: Refactor all method implementations - replace `app_.getXXX()` calls with interface method calls. Example: `app_.getJobQueue().addJob()` → `scheduler_.addJob()`.

**Step 3.4**: Update includes - remove `Application.h` and concrete subsystem headers, add focused interface includes.

**Verification**: Compilation succeeds, all tests pass

### Phase 4: Application Wiring (Week 4)

**Step 4.1**: In `ApplicationImp::setup()`, create wrapper instances after their wrapped objects:
- Create all `*Impl` wrapper instances
- Construct `ConsensusAdaptorDependencies` struct with wrapper references
- Pass struct to `RCLConsensus` constructor

**Step 4.2**: Initialization order: Subsystems → Wrappers → RCLConsensus

**Verification**: Application starts and runs correctly

### Phase 5: Testing (Week 5)

**Step 5.1**: Create mock implementations for all interfaces in `src/test/consensus/Mock*.h`

**Step 5.2**: Add unit tests in `src/test/consensus/RCLConsensusAdaptor_test.cpp`:
- Test `acquireLedger()` with found/not-found cases
- Test `share()`, `propose()`, `doAccept()` paths
- Verify interface method calls via mocks

**Verification**: Unit tests pass, coverage > 80% for Adaptor

---

## Risk Assessment

### High Risk: Breaking Consensus Logic

**Risk**: Refactoring introduces subtle bugs that break consensus behavior
**Probability**: Medium
**Impact**: Critical - could cause network forks or stuck consensus
**Mitigation**:
- Extensive integration testing with existing test suite
- Maintain backward-compatible constructor during transition
- Deploy to testnet before mainnet
- Monitor consensus metrics during rollout
**Rollback**: Revert to old constructor if issues detected

### Medium Risk: Thread Safety Violations

**Risk**: Interface implementations don't properly handle threading
**Probability**: Medium
**Impact**: High - race conditions, crashes, or data corruption
**Mitigation**:
- Document thread safety requirements in interfaces
- Wrapper implementations directly delegate (no new state)
- Thread sanitizer testing
- Code review focused on threading
**Rollback**: Wrapper implementations can be immediately reverted

### Medium Risk: Performance Regression

**Risk**: Additional virtual dispatch and wrapper indirection impacts consensus timing
**Probability**: Low
**Impact**: Medium - slower consensus rounds
**Mitigation**:
- Profile before and after implementation
- Benchmark critical paths (acquireLedger, doAccept)
- Monitor consensus round times in testing
- Use inline hints where appropriate
**Measurement**: Consensus round time variance < 5%

### Low Risk: Incomplete Interface Coverage

**Risk**: Some Application dependency missed during analysis
**Probability**: Low
**Impact**: Low - compile error catches this immediately
**Mitigation**:
- Exhaustive code analysis (completed above)
- Compile-time verification
- Incremental refactoring with testing at each step
**Resolution**: Add missing methods to appropriate interface

### Low Risk: Integration Complexity

**Risk**: Wiring up dependencies in Application becomes complex/fragile
**Probability**: Low
**Impact**: Medium - initialization failures
**Mitigation**:
- Clear initialization order documentation
- Factory pattern for dependency struct creation
- Startup validation checks
**Rollback**: Old constructor remains available

---

## Validation Criteria

### Functional Validation

- [ ] All existing consensus unit tests pass
- [ ] All integration tests pass (network simulation)
- [ ] Standalone mode consensus works correctly
- [ ] Multi-node testnet reaches consensus
- [ ] Ledger advancement continues normally
- [ ] Validations are properly broadcast and received
- [ ] Transaction sets are properly shared
- [ ] Mode transitions work correctly

### Architectural Validation

- [ ] `RCLConsensus.cpp` has no direct `app_.` calls (except deprecated path)
- [ ] `RCLConsensus.h` does not include `Application.h`
- [ ] Include graph shows reduced dependencies
- [ ] Each interface has single responsibility
- [ ] All interfaces have mock implementations
- [ ] Unit tests don't require full Application

### Performance Validation

- [ ] Consensus round time unchanged (< 5% variance)
- [ ] Memory usage unchanged
- [ ] No increase in lock contention
- [ ] No measurable latency increase for ledger acquisition

### Code Quality Validation

- [ ] All new code follows style guidelines
- [ ] Interface documentation complete
- [ ] Thread safety documented for each interface
- [ ] No new compiler warnings
- [ ] Code coverage > 80% for new mock-based tests

---

## Files to Modify

### New Files to Create

| File | Purpose |
|------|---------|
| `src/xrpld/consensus/interfaces/ILedgerProvider.h` | Ledger access interface |
| `src/xrpld/consensus/interfaces/IOverlayBroadcaster.h` | Peer communication interface |
| `src/xrpld/consensus/interfaces/IConsensusJobScheduler.h` | Job scheduling interface |
| `src/xrpld/consensus/interfaces/ITxSetManager.h` | Transaction set management |
| `src/xrpld/consensus/interfaces/IMessageRouter.h` | Message routing/suppression |
| `src/xrpld/consensus/interfaces/IConsensusTimeSource.h` | Time provider interface |
| `src/xrpld/consensus/interfaces/IValidationTracker.h` | Validation tracking |
| `src/xrpld/consensus/interfaces/IOperatingMode.h` | Operating mode control |
| `src/xrpld/consensus/interfaces/IOpenLedgerState.h` | Open ledger state |
| `src/xrpld/consensus/interfaces/ConsensusAdaptorDeps.h` | Dependency aggregate |
| `src/xrpld/consensus/detail/LedgerProviderImpl.h` | LedgerMaster wrapper |
| `src/xrpld/consensus/detail/OverlayBroadcasterImpl.h` | Overlay wrapper |
| `src/xrpld/consensus/detail/*.h` | Other wrapper implementations |
| `src/test/consensus/MockLedgerProvider.h` | Mock for testing |
| `src/test/consensus/Mock*.h` | Other mock implementations |
| `src/test/consensus/RCLConsensusAdaptor_test.cpp` | New unit tests |

### Files to Modify

| File | Change Type | Description |
|------|-------------|-------------|
| `src/xrpld/app/consensus/RCLConsensus.h` | Major | Update Adaptor members and constructor |
| `src/xrpld/app/consensus/RCLConsensus.cpp` | Major | Refactor all method implementations |
| `src/xrpld/app/main/Application.h` | Minor | Add wrapper instance getters (optional) |
| `src/xrpld/app/main/Application.cpp` | Medium | Create wrapper instances, wire dependencies |
| `CMakeLists.txt` | Minor | Add new source files |

---

## Related Tasks

- **Task 2.1**: LedgerDataProvider interface (foundation work)
- **Task 2.4**: LedgerMaster abstraction (overlapping ledger interface)
- **Task 3.1**: Split app/misc (reduces header complexity)
- **Future**: Further decouple consensus from XRPL-specific types

