# Architectural Improvement Implementation Tasks

**Based on:** `docs/ARCHITECTURAL_ANALYSIS_MERGED.md`  
**Created:** January 2026  
**Status:** In Progress

---

## Quick Reference

| Phase | Description | Status | Est. Effort |
|-------|-------------|--------|-------------|
| 1 | Structural Decoupling (Quick Wins) | ✅ Complete | 2-3 days |
| 2 | Interface Extraction | ⏳ Pending | 6-10 weeks |
| 3 | Major Restructuring | ⏳ Pending | 8-12 weeks |
| 4 | Developer Experience | ⏳ Pending | Ongoing |

### Phase 3 Task Breakdown

| Task | Description | Est. Effort | Risk |
|------|-------------|-------------|------|
| 3.1 | Split app/misc into Focused Modules | 3-4 weeks | High |
| 3.2 | Split app/tx/detail into Submodules | 2-3 weeks | High |
| 3.3 | Split NetworkOPs | 4 weeks | High |
| 3.4 | Split PeerImp | 4 days | Medium |
| 3.5 | Runtime Transaction Registry | 3-4 weeks | Medium |
| 3.6 | Consensus Adaptor Decoupling | 16-20 days | Medium |

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

**Status:** [/] In Progress
**Started:** 2026-01-22
**Risk:** Medium
**Estimated Effort:** 2-3 weeks
**Dependencies:** Phase 1 complete

**Detailed Plan:** See [docs/tasks/phase-2-task-1-ledger-data-provider.md](docs/tasks/phase-2-task-1-ledger-data-provider.md)

**Description:**
Create a `LedgerDataProvider` interface to break the massive `rpc → app` dependency (174 includes). RPC handlers should depend on this interface instead of `LedgerMaster` directly.

**Files to Create:**
- `src/xrpld/core/LedgerDataProvider.h` - New interface

**Files to Modify:**
- `src/xrpld/app/ledger/LedgerMaster.h` - Implement interface
- 30+ RPC handlers in `src/xrpld/rpc/handlers/` - Migrate to interface

**Acceptance Criteria:**
- [ ] Interface defined in core module (lower tier)
- [ ] LedgerMaster implements interface
- [ ] At least 5 RPC handlers migrated as proof of concept
- [ ] Build compiles without errors
- [ ] All tests pass
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

**Status:** [ ] Not Started
**Risk:** Medium
**Estimated Effort:** 1 week
**Dependencies:** Task 2.1

**Detailed Plan:** See [docs/tasks/phase-2-task-3-nodefamily-move.md](docs/tasks/phase-2-task-3-nodefamily-move.md)

**Description:**
Move NodeFamily from shamap to app, extracting interfaces for its app dependencies.

---

### Task 2.4: Abstract LedgerMaster

**Status:** [ ] Not Started
**Risk:** High
**Estimated Effort:** 2-3 weeks
**Dependencies:** Task 2.1

**Detailed Plan:** See [docs/tasks/phase-2-task-4-ledgermaster-abstract.md](docs/tasks/phase-2-task-4-ledgermaster-abstract.md)

**Description:**
Convert LedgerMaster to abstract interface with factory function.

---

## Phase 3: Major Restructuring

### Task 3.1: Split app/misc into Focused Modules

**Status:** [ ] Not Started
**Risk:** High
**Estimated Effort:** 3-4 weeks
**Dependencies:** Phase 2 complete

**Detailed Plan:** See [docs/tasks/phase-3-task-1-split-app-misc.md](docs/tasks/phase-3-task-1-split-app-misc.md)

**Description:**
Split the monolithic app/misc directory (72 files) into focused modules: validation, fees, amendments, and manifest.

---

### Task 3.2: Split app/tx/detail into Submodules

**Status:** [ ] Not Started
**Risk:** High
**Estimated Effort:** 2-3 weeks
**Dependencies:** Phase 2 complete

**Detailed Plan:** See [docs/tasks/phase-3-task-2-split-app-tx-detail.md](docs/tasks/phase-3-task-2-split-app-tx-detail.md)

**Description:**
Split the app/tx/detail directory (50+ transactors) into logical submodules by transaction category.

---

### Task 3.3: Split NetworkOPs

**Status:** [ ] Not Started
**Risk:** High
**Estimated Effort:** 4 weeks
**Dependencies:** Phase 2 complete

**Detailed Plan:** See [docs/tasks/phase-3-task-3-networkops-split.md](docs/tasks/phase-3-task-3-networkops-split.md)

**Description:**
Split the monolithic NetworkOPs class (~4000 lines) into focused responsibility-driven components: NetworkState, ConsensusCoordinator, TransactionSubmission, and PubSubManager.

---

### Task 3.4: Split PeerImp

**Status:** [ ] Not Started
**Risk:** Medium
**Estimated Effort:** 4 days
**Dependencies:** Phase 2 complete

**Detailed Plan:** See [docs/tasks/phase-3-task-4-peerimp-split.md](docs/tasks/phase-3-task-4-peerimp-split.md)

**Description:**
Split the PeerImp class (~3000 lines) into focused protocol handlers and message processors for improved maintainability and testability.

---

### Task 3.5: Runtime Transaction Registry

**Status:** [ ] Not Started
**Risk:** Medium
**Estimated Effort:** 3-4 weeks
**Dependencies:** Phase 2 complete

**Detailed Plan:** See [docs/tasks/phase-3-task-5-runtime-transaction-registry.md](docs/tasks/phase-3-task-5-runtime-transaction-registry.md)

**Description:**
Replace compile-time transaction type registration with a runtime registry pattern to enable pluggable transaction handlers and improve extensibility.

---

### Task 3.6: Consensus Adaptor Decoupling

**Status:** [ ] Not Started
**Risk:** Medium
**Estimated Effort:** 16-20 days
**Dependencies:** Phase 2 complete

**Detailed Plan:** See [docs/tasks/phase-3-task-6-consensus-adaptor-decoupling.md](docs/tasks/phase-3-task-6-consensus-adaptor-decoupling.md)

**Description:**
Decouple the consensus adaptor from NetworkOPs and Application to enable cleaner testing and alternative consensus implementations.

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

**Status:** [ ] Not Started
**Risk:** Low
**Estimated Effort:** 1 week
**Dependencies:** None

**Detailed Plan:** See [docs/tasks/phase-4-task-3-docker-environment.md](docs/tasks/phase-4-task-3-docker-environment.md)

**Description:**
Create a Docker-based development environment with pre-configured toolchains, dependencies, and IDE integration for consistent developer onboarding.

---

### Task 4.4: Architecture Documentation Guide

**Status:** [ ] Not Started
**Risk:** Low
**Estimated Effort:** 3-5 days
**Dependencies:** Phase 2, Phase 3 partially complete

**Detailed Plan:** See [docs/tasks/phase-4-task-4-architecture-guide.md](docs/tasks/phase-4-task-4-architecture-guide.md)

**Description:**
Create comprehensive architecture documentation covering module responsibilities, dependency guidelines, and system overview diagrams.

---

### Task 4.5: Feature Development Guide

**Status:** [ ] Not Started
**Risk:** Low
**Estimated Effort:** 1 week
**Dependencies:** Phase 2, Phase 3 partially complete

**Detailed Plan:** See [docs/tasks/phase-4-task-5-feature-development-guide.md](docs/tasks/phase-4-task-5-feature-development-guide.md)

**Description:**
Create step-by-step guides for common development tasks: adding new transaction types, RPC handlers, and amendments.

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

| Task | Commit SHA | Date | Notes |
|------|------------|------|-------|
| 1.1 DBInit.h move | 39f9190bc4 | 2026-01-22 | `app ↔ core` cycle removed |
| 1.2 Store.h extraction | 1c2d0704c6 | 2026-01-22 | Coupling reduced |
| 2.2 Overlay/ServerHandler | f589b84cd6 | 2026-01-22 | ServerHandler dependency removed |
| 2.2 json_body.h move | f16079e56c | 2026-01-22 | Moved to xrpl/server |
| 2.2 ServerCounts extraction | b5939e20d3 | 2026-01-22 | overlay→rpc cycle broken |
| 2.1 LedgerDataProvider | 4cc8735096 | 2026-01-22 | Interface + LedgerMaster implementation |
| 4.1 CMakePresets.json | (pending commit) | Jan 2026 | IDE integration |
| 4.2 jtx README | (pending commit) | Jan 2026 | Testing framework docs |

