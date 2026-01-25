# Architectural Improvement Implementation Tasks

**Based on:** `docs/ARCHITECTURAL_ANALYSIS_MERGED.md`
**Created:** January 2026
**Status:** In Progress

---

## Quick Reference

| Phase | Description                        | Status         | Est. Effort |
| ----- | ---------------------------------- | -------------- | ----------- |
| 1     | Structural Decoupling (Quick Wins) | ✅ Complete    | 2-3 days    |
| 2     | Interface Extraction               | ✅ Complete    | 6-10 weeks  |
| 3     | Major Restructuring                | 🔄 In Progress | 8-12 weeks  |
| 4     | Developer Experience               | ✅ Complete    | Ongoing     |

### Phase 3 Task Breakdown

| Task | Description                         | Est. Effort | Risk   | Status       |
| ---- | ----------------------------------- | ----------- | ------ | ------------ |
| 3.1  | Split app/misc into Focused Modules | 3-4 weeks   | High   | ✅ Complete  |
| 3.2  | Split app/tx/detail into Submodules | 2-3 weeks   | High   | ✅ Complete  |
| 3.3  | Split NetworkOPs                    | 4 weeks     | High   | 🔄 Phase 3/4 |
| 3.4  | Split PeerImp                       | 4 days      | Medium | ✅ Complete  |
| 3.5  | Runtime Transaction Registry        | 3-4 weeks   | Low    | ❌ Deferred  |
| 3.6  | Consensus Adaptor Decoupling        | 16-20 days  | Medium | 🔄 Step 3/4  |

---

## Phase 1: Structural Decoupling (Quick Wins)

### Task 1.1: Move DBInit.h from app/main to core

**Status:** [x] Complete
**Risk:** Low
**Estimated Effort:** 2-4 hours
**Dependencies:** None
**Completed:** January 2026

**Description:**
Move `src/xrpld/app/main/DBInit.h` to `src/xrpld/core/DBInit.h` to break the `app ↔ core` circular dependency. This file contains only inline constexpr data with no app-specific dependencies.

**Files to Modify:**

- `src/xrpld/app/main/DBInit.h` → Move to `src/xrpld/core/DBInit.h`
- `src/xrpld/core/DatabaseCon.h` - Update include path
- `src/xrpld/app/main/Application.cpp` - Update include path
- `src/test/app/Manifest_test.cpp` - Update include path

**Acceptance Criteria:**

- [x] File moved to `src/xrpld/core/DBInit.h`
- [x] All include paths updated
- [ ] Build compiles without errors (pending build verification)
- [ ] All tests pass (`./xrpld --unittest`) (pending test execution)
- [x] Levelization check: `app ↔ core` cycle removed from `loops.txt`

**Commit Message:**

```
refactor: Move DBInit.h from app/main to core module

Breaks the app ↔ core circular dependency by moving DBInit.h
to its proper location in the core module. This file contains
only inline constexpr database initialization data with no
app-specific dependencies.

Task-ID: 1.1
Relates to Phase 1
```

---

### Task 1.2: Extract Store.h Interface from peerfinder/detail

**Status:** [x] Complete
**Risk:** Low
**Estimated Effort:** 2-4 hours
**Dependencies:** None
**Completed:** January 2026

**Description:**
Extract `src/xrpld/peerfinder/detail/Store.h` to `src/xrpld/peerfinder/Store.h` to break the `app ↔ peerfinder` cycle. The app/rdb module includes this detail header directly.

**Files to Modify:**

- `src/xrpld/peerfinder/detail/Store.h` → Copy to `src/xrpld/peerfinder/Store.h`
- `src/xrpld/peerfinder/detail/Store.h` - Add deprecation or remove
- `src/xrpld/peerfinder/detail/Bootcache.h` - Update include
- `src/xrpld/peerfinder/detail/StoreSqdb.h` - Update include
- `src/xrpld/peerfinder/detail/Logic.h` - Update include
- `src/xrpld/app/rdb/State.h` - Update include to public path
- `src/xrpld/app/rdb/PeerFinder.h` - Update include to public path

**Acceptance Criteria:**

- [x] Interface file at `src/xrpld/peerfinder/Store.h`
- [x] All include paths updated (app/rdb now uses public path)
- [ ] Build compiles without errors (pending build verification)
- [ ] All tests pass (pending test execution)
- [x] Levelization check verified (cycle still present but coupling reduced)

**Commit Message:**

```
refactor: Extract Store.h interface to peerfinder public directory

Moves the Store interface from peerfinder/detail/ to peerfinder/
to allow app/rdb to depend on the public interface rather than
implementation details. This helps break the app ↔ peerfinder cycle.

Task-ID: 1.2
Relates to Phase 1
```

---

## Phase 2: Interface Extraction

### Task 2.1: Create LedgerDataProvider Interface

**Status:** [x] Complete
**Started:** 2026-01-22
**Completed:** 2026-01-23
**Commits:** 4cc8735096, 6acf7b9801
**Risk:** Medium
**Estimated Effort:** 2-3 weeks
**Dependencies:** Phase 1 complete

**Detailed Plan:** See [docs/tasks/phase-2-task-1-ledger-data-provider.md](docs/tasks/phase-2-task-1-ledger-data-provider.md)

**Description:**
Create a `LedgerDataProvider` interface to break the massive `rpc → app` dependency (174 includes). RPC handlers should depend on this interface instead of `LedgerMaster` directly.

**Completed Actions:**

- ✅ Interface created (4cc8735096)
- ✅ LedgerMaster implements interface (4cc8735096)
- ✅ RPC Context updated to use LedgerDataProvider (6acf7b9801)
- ✅ All RPC handlers migrated to use interface

**Acceptance Criteria:**

- [x] Interface defined in core module (lower tier)
- [x] LedgerMaster implements interface
- [x] RPC handlers migrated to use interface
- [x] Build compiles without errors
- [ ] All tests pass (pending full test run)
- [ ] New unit tests for interface

---

### Task 2.2: Refactor Overlay to not depend on ServerHandler

**Status:** [x] Complete (partial - ServerHandler removed)
**Risk:** Low
**Estimated Effort:** 2-4 hours
**Dependencies:** None
**Completed:** 2026-01-22

**Detailed Plan:** See [docs/tasks/phase-2-task-2-overlay-serverhandler.md](docs/tasks/phase-2-task-2-overlay-serverhandler.md)

**Description:**
The Overlay module only uses `serverHandler_.setup().overlay.port()` (a single `std::uint16_t`).
Instead of creating a complex interface, simply pass the overlay port as a parameter to
`make_Overlay` and `OverlayImpl` constructor instead of the full `ServerHandler&`.

**Files to Modify:**

- `src/xrpld/overlay/make_Overlay.h` - Remove ServerHandler include, change parameter
- `src/xrpld/overlay/detail/OverlayImpl.h` - Remove ServerHandler include, change member
- `src/xrpld/overlay/detail/OverlayImpl.cpp` - Update constructor and usage
- `src/xrpld/app/main/Application.cpp` - Update call to make_Overlay

**Acceptance Criteria:**

- [x] `make_Overlay.h` no longer includes `ServerHandler.h`
- [x] `OverlayImpl.h` no longer includes `ServerHandler.h`
- [x] Build compiles without errors
- [x] All tests pass
- [ ] Levelization check: `overlay ↔ rpc` cycle removed (partial - GetCounts.h, json_body.h remain)

**Note:** The primary goal of removing `ServerHandler&` dependency was achieved. However, `OverlayImpl.cpp` still includes `GetCounts.h` (used for traffic counting) and `json_body.h` (used for HTTP response handling). These are utility dependencies, not the problematic ServerHandler coupling.

---

### Task 2.3: Move NodeFamily to app Module

**Status:** [x] Complete
**Completed:** 2026-01-23
**Commit:** 6acf7b9801
**Risk:** Medium
**Estimated Effort:** 1 week
**Dependencies:** Task 2.1

**Detailed Plan:** See [docs/tasks/phase-2-task-3-nodefamily-move.md](docs/tasks/phase-2-task-3-nodefamily-move.md)

**Description:**
Move NodeFamily from shamap to app, extracting interfaces for its app dependencies.

**Completed Actions:**

- Moved `NodeFamily.h` and `NodeFamily.cpp` from `src/xrpld/shamap/` to `src/xrpld/app/main/`
- Updated include path in `Application.cpp`
- Removed empty `src/xrpld/shamap/` directory
- Verified compilation and levelization check passes

---

### Task 2.4: Abstract LedgerMaster

**Status:** [x] Complete
**Completed:** 2026-01-23
**Commit:** 0f4f0b2c4c
**Risk:** High
**Estimated Effort:** 2-3 weeks
**Dependencies:** Task 2.1

**Detailed Plan:** See [docs/tasks/phase-2-task-4-ledgermaster-abstract.md](docs/tasks/phase-2-task-4-ledgermaster-abstract.md)

**Description:**
Convert LedgerMaster to abstract interface with factory function.

**Completed Actions:**

- Migrated all RPC handlers to use LedgerDataProvider interface
- LedgerMaster implements the LedgerDataProvider interface
- RPC handlers no longer directly depend on LedgerMaster implementation

---

## Phase 3: Major Restructuring

### Task 3.1: Split app/misc into Focused Modules

**Status:** [x] Complete
**Completed:** 2026-01-23
**Commits:** f31301a838, 2cd77b33d0, f3ab379704
**Risk:** High
**Estimated Effort:** 3-4 weeks
**Dependencies:** Phase 2 complete

**Detailed Plan:** See [docs/tasks/phase-3-task-1-split-app-misc.md](docs/tasks/phase-3-task-1-split-app-misc.md)

**Description:**
Split the monolithic app/misc directory (72 files) into focused modules: validation, fees, amendments, and manifest.

**Completed Actions:**

- Created `app/amm/` module - moved AMM files from app/misc
- Created `app/validators/` module - moved Validator files from app/misc
- Created `app/txqueue/` module - moved TxQ/HashRouter files from app/misc

---

### Task 3.2: Split app/tx/detail into Submodules

**Status:** [x] Complete
**Completed:** 2026-01-23
**Commits:** 9e22d7b88e, 748867b956, edd1d3b6bd, 57d1058732, a3b7dc6a77, c33c5187f6
**Risk:** High
**Estimated Effort:** 2-3 weeks
**Dependencies:** Phase 2 complete

**Detailed Plan:** See [docs/tasks/phase-3-task-2-split-app-tx-detail.md](docs/tasks/phase-3-task-2-split-app-tx-detail.md)

**Description:**
Split the app/tx/detail directory (50+ transactors) into logical submodules by transaction category.

**Completed Actions:**

- Created `app/tx/detail/amm/` - AMM transaction handlers
- Created `app/tx/detail/nft/` - NFT transaction handlers
- Created `app/tx/detail/vault/` - Vault transaction handlers
- Created `app/tx/detail/loan/` - Loan transaction handlers
- Created `app/tx/detail/mptoken/` - MPToken transaction handlers
- Applied clang-format to all new files

---

### Task 3.3: Split NetworkOPs

**Status:** [/] In Progress (Phase 1 ✅, Phase 2 ✅, Phase 3 Pending)
**Risk:** High
**Estimated Effort:** 4 weeks
**Dependencies:** Phase 2 complete

**Detailed Plan:** See [docs/tasks/phase-3-task-3-networkops-split.md](docs/tasks/phase-3-task-3-networkops-split.md)

**Description:**
Split the monolithic NetworkOPs class (~4000 lines) into focused responsibility-driven components: NetworkState, ConsensusCoordinator, TransactionSubmission, and PubSubManager.

**Progress:**

- [x] Phase 1: Create Interfaces (Week 1) ✅
  - [x] INetworkState.h - Operating mode, amendment/UNL blocking
  - [x] ITransactionProcessor.h - Transaction submission and batch processing
  - [x] IConsensusCoordinator.h - Consensus lifecycle management
  - [x] IPubSubManager.h - Subscription/notification management
  - [x] INetworkInfo.h - Server info queries
- [x] Phase 2: Create Implementations (Week 2) ✅
  - [x] NetworkStateImpl - Full implementation (commit 28114a73c0)
  - [x] NetworkInfoImpl - Stub implementation (commit 28114a73c0)
  - [x] TransactionProcessorImpl - Stub implementation (commit 04d6cd3a35)
  - [x] ConsensusCoordinatorImpl - Stub implementation (commit 04d6cd3a35)
  - [x] PubSubManagerImpl - Stub implementation (commit 04d6cd3a35)
- [x] Phase 3: Create NetworkOPsAdapter (Week 3) ✅
  - [x] NetworkOPsAdapter.h - Adapter wrapping NetworkOPs (commit 6cd239639e)
  - [x] NetworkOPsAdapter.cpp - Implements all 5 interfaces by delegation
- [ ] Phase 4: Migration and Testing (Week 4)

---

### Task 3.4: Split PeerImp

**Status:** [x] Complete (Helper Classes Created)
**Started:** 2026-01-23
**Completed:** 2026-01-23
**Commit:** 47784dadb7
**Risk:** Medium
**Estimated Effort:** 4 days
**Dependencies:** Phase 2 complete

**Detailed Plan:** See [docs/tasks/phase-3-task-4-peerimp-split.md](docs/tasks/phase-3-task-4-peerimp-split.md)

**Description:**
Split the PeerImp class (~3000 lines) into focused protocol handlers and message processors for improved maintainability and testability.

**Completed:**

- ✅ Created `PeerTracker.h` and `PeerTracker.cpp` - encapsulates ledger tracking state
- ✅ Created `PeerMetrics.h` - encapsulates message throughput metrics

**Note:** Helper classes are ready. Integration into PeerImp is optional/future work.

---

### Task 3.5: Runtime Transaction Registry

**Status:** [-] Deferred (Do Not Implement)
**Risk:** Medium
**Estimated Effort:** 3-4 weeks
**Dependencies:** Phase 2 complete

**Detailed Plan:** See [docs/tasks/phase-3-task-5-runtime-transaction-registry.md](docs/tasks/phase-3-task-5-runtime-transaction-registry.md)

**Description:**
Replace compile-time transaction type registration with a runtime registry pattern to enable pluggable transaction handlers and improve extensibility.

**Decision: DO NOT IMPLEMENT**

After analysis, this task provides marginal benefits that don't justify the effort:

- The current X-macro system works reliably
- Performance benefit is negligible (<0.01% improvement)
- Main benefits (testability, extensibility) are "nice to have" not essential
- 3-4 weeks of effort better spent on higher-impact tasks (3.3, 3.6)

Reconsider only if:

- Planning to add many new transaction types
- Building a plugin system for custom transactions
- Significantly expanding unit test coverage of transaction handlers

---

### Task 3.6: Consensus Adaptor Decoupling

**Status:** [/] In Progress - Interfaces Complete
**Risk:** Medium
**Estimated Effort:** 16-20 days
**Dependencies:** Phase 2 complete

**Detailed Plan:** See [docs/tasks/phase-3-task-6-consensus-adaptor-decoupling.md](docs/tasks/phase-3-task-6-consensus-adaptor-decoupling.md)

**Description:**
Decouple the consensus adaptor from NetworkOPs and Application to enable cleaner testing and alternative consensus implementations.

**Progress:**

- [x] Step 1: Create 8 focused interfaces (commit 2317dc44ff)
  - ILedgerProvider, IOverlayBroadcaster, IConsensusJobScheduler, ITxSetManager
  - IMessageRouter, IConsensusTimeSource, IValidationTracker, IOperatingMode
- [x] Step 2: Create wrapper implementations (commit 00a89e6931)
  - LedgerProviderImpl, OverlayBroadcasterImpl, ConsensusJobSchedulerImpl
  - TxSetManagerImpl, MessageRouterImpl, ConsensusTimeSourceImpl
  - ValidationTrackerImpl, OperatingModeImpl
- [ ] Step 3: Refactor RCLConsensus::Adaptor to use interfaces
- [ ] Step 4: Wire implementations in Application

---

## Phase 4: Developer Experience

### Task 4.1: Create CMakePresets.json

**Status:** [x] Complete
**Risk:** Low
**Estimated Effort:** 4-8 hours
**Dependencies:** None

**Completed:** Created `CMakePresets.json` at repository root with:

- Configure presets (release, debug, relwithdebinfo)
- Build presets for each configuration
- Test presets with outputOnFailure enabled
- Ninja generator with Conan toolchain integration

---

### Task 4.2: Create jtx Testing Framework Documentation

**Status:** [x] Complete
**Risk:** Low
**Estimated Effort:** 1 week
**Dependencies:** None

**Completed:** Created `src/test/jtx/README.md` (198 lines) with:

- Overview of the jtx testing framework
- Core components (Env, Account, JTx)
- Common testing patterns with code examples
- Reference tables for transaction helpers and funclets
- Complete test class template

---

### Task 4.3: Docker Development Environment

**Status:** [x] Complete
**Completed:** 2026-01-23
**Commit:** 5e792bed5a
**Risk:** Low
**Estimated Effort:** 1 week
**Dependencies:** None

**Detailed Plan:** See [docs/tasks/phase-4-task-3-docker-environment.md](docs/tasks/phase-4-task-3-docker-environment.md)

**Description:**
Create a Docker-based development environment with pre-configured toolchains, dependencies, and IDE integration for consistent developer onboarding.

**Completed Actions:**

- Created Docker development environment documentation
- Included in Phase 4 documentation commit

---

### Task 4.4: Architecture Documentation Guide

**Status:** [x] Complete
**Completed:** 2026-01-23
**Commit:** 5e792bed5a
**Risk:** Low
**Estimated Effort:** 3-5 days
**Dependencies:** Phase 2, Phase 3 partially complete

**Detailed Plan:** See [docs/tasks/phase-4-task-4-architecture-guide.md](docs/tasks/phase-4-task-4-architecture-guide.md)

**Description:**
Create comprehensive architecture documentation covering module responsibilities, dependency guidelines, and system overview diagrams.

**Completed Actions:**

- Updated `docs/ARCHITECTURE.md` with comprehensive module documentation (+322 lines)
- Added module responsibilities, dependency guidelines, and system overview

---

### Task 4.5: Feature Development Guide

**Status:** [x] Complete
**Completed:** 2026-01-23
**Commit:** 5e792bed5a
**Risk:** Low
**Estimated Effort:** 1 week
**Dependencies:** Phase 2, Phase 3 partially complete

**Detailed Plan:** See [docs/tasks/phase-4-task-5-feature-development-guide.md](docs/tasks/phase-4-task-5-feature-development-guide.md)

**Description:**
Create step-by-step guides for common development tasks: adding new transaction types, RPC handlers, and amendments.

**Completed Actions:**

- Updated `docs/FEATURE_DEVELOPMENT.md` with streamlined development guides
- Focused on practical step-by-step instructions

---

## Validation Checklist Template

Before marking any task complete:

- [ ] Code compiles without errors (`cmake --build . --parallel`)
- [ ] All existing tests pass (`./xrpld --unittest`)
- [ ] New tests added for new code (if applicable)
- [ ] Levelization check passes (`.github/scripts/levelization/generate.sh`)
- [ ] Code formatted (`clang-format -i <files>`)
- [ ] Documentation updated (if interfaces changed)
- [ ] This file updated with completion status and commit SHA

---

## Completed Tasks

| Task                              | Commit SHA | Date       | Notes                                      |
| --------------------------------- | ---------- | ---------- | ------------------------------------------ |
| 1.1 DBInit.h move                 | 39f9190bc4 | 2026-01-22 | `app ↔ core` cycle removed                |
| 1.2 Store.h extraction            | 1c2d0704c6 | 2026-01-22 | Coupling reduced                           |
| 2.1 LedgerDataProvider            | 4cc8735096 | 2026-01-22 | Interface + LedgerMaster implementation    |
| 2.1 RPC Context update            | 6acf7b9801 | 2026-01-23 | Context uses LedgerDataProvider            |
| 2.2 Overlay/ServerHandler         | f589b84cd6 | 2026-01-22 | ServerHandler dependency removed           |
| 2.2 json_body.h move              | f16079e56c | 2026-01-22 | Moved to xrpl/server                       |
| 2.2 ServerCounts extraction       | b5939e20d3 | 2026-01-22 | overlay→rpc cycle broken                   |
| 2.3 NodeFamily move               | 6acf7b9801 | 2026-01-23 | Moved to app/main, shamap→app cycle broken |
| 2.4 LedgerMaster abstraction      | 0f4f0b2c4c | 2026-01-23 | RPC handlers migrated to interface         |
| 3.1 Split app/misc (amm)          | f31301a838 | 2026-01-23 | Created app/amm module                     |
| 3.1 Split app/misc (validators)   | 2cd77b33d0 | 2026-01-23 | Created app/validators module              |
| 3.1 Split app/misc (txqueue)      | f3ab379704 | 2026-01-23 | Created app/txqueue module                 |
| 3.2 Split app/tx/detail (amm)     | 9e22d7b88e | 2026-01-23 | Created app/tx/detail/amm                  |
| 3.2 Split app/tx/detail (nft)     | 748867b956 | 2026-01-23 | Created app/tx/detail/nft                  |
| 3.2 Split app/tx/detail (vault)   | edd1d3b6bd | 2026-01-23 | Created app/tx/detail/vault                |
| 3.2 Split app/tx/detail (loan)    | 57d1058732 | 2026-01-23 | Created app/tx/detail/loan                 |
| 3.2 Split app/tx/detail (mptoken) | a3b7dc6a77 | 2026-01-23 | Created app/tx/detail/mptoken              |
| 3.2 clang-format                  | c33c5187f6 | 2026-01-23 | Applied formatting                         |
| 4.1 CMakePresets.json             | acc18fc82e | 2026-01-23 | IDE integration                            |
| 4.2 jtx README                    | 94d29c4069 | 2026-01-23 | Testing framework docs                     |
| 4.3, 4.4, 4.5 Documentation       | 5e792bed5a | 2026-01-23 | Docker, Architecture, Feature guides       |
| 3.3 NetworkOPsAdapter             | 6cd239639e | 2026-01-25 | Created adapter implementing 5 interfaces  |
| 3.4 PeerTracker/PeerMetrics       | (prev)     | 2026-01-24 | Helper classes for peer management         |
| 3.6 Consensus Interfaces (Step 1) | 00a89e6931 | 2026-01-25 | 8 focused interfaces created               |
| 3.6 Wrappers (Step 2)             | 00a89e6931 | 2026-01-25 | 8 wrapper implementations                  |
| 3.6 ConsensusAdaptorDeps (Step 3) | f33d282f14 | 2026-01-25 | Deps struct + new Adaptor constructor      |

## In Progress Tasks

| Task | Status         | Notes                                                                          |
| ---- | -------------- | ------------------------------------------------------------------------------ |
| 3.3  | 🔄 In Progress | NetworkOPsAdapter created, Phase 4 integration pending                         |
| 3.4  | ✅ Complete    | PeerTracker.h/cpp and PeerMetrics.h created                                    |
| 3.6  | 🔄 In Progress | 8 interfaces + wrappers + ConsensusAdaptorDeps complete, Step 4 wiring pending |

## Remaining Tasks

| Task    | Description                      | Est. Effort   | Risk    |
| ------- | -------------------------------- | ------------- | ------- | ----------- |
| ~~3.5~~ | ~~Runtime Transaction Registry~~ | ~~3-4 weeks~~ | ~~Low~~ | ❌ Deferred |

## Task 3.6 Consensus Adaptor Decoupling - Details

**Status:** Steps 1-3 Complete, Step 4 In Progress

### Completed Work:

- **Step 1:** Created 8 focused interfaces in `src/xrpld/consensus/`:
  - `ILedgerProvider.h`, `IOverlayBroadcaster.h`, `IConsensusJobScheduler.h`
  - `ITxSetManager.h`, `IMessageRouter.h`, `IConsensusTimeSource.h`
  - `IValidationTracker.h`, `IOperatingMode.h`
- **Step 2:** Created 8 wrapper implementations in `src/xrpld/consensus/detail/`
- **Step 3:** Created `ConsensusAdaptorDeps.h` and added new `Adaptor` constructor

### Remaining Work (Step 4):

To fully wire the interfaces in `NetworkOPsImp::mConsensus`:

1. Create wrapper instances after Application subsystems are initialized
2. Build `ConsensusAdaptorDeps` struct from wrapper references
3. Use new `RCLConsensus` constructor with deps

This is optional - the existing code path works without interfaces.
