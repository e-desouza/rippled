# XRP Ledger (rippled) Architectural Analysis & Improvement Plan

**Date**: January 2026
**Version**: 1.1
**Authors**: Architectural Review Team

---

## Executive Summary

This document presents a comprehensive architectural analysis of the rippled codebase, identifying opportunities to improve modularity, reduce coupling, and enhance developer experience. The analysis covers the existing tier-based levelization system, circular dependencies, interface abstractions, and developer onboarding barriers.

### Key Findings

| Category                 | Status                     | Priority Issues                                                         |
| ------------------------ | -------------------------- | ----------------------------------------------------------------------- |
| **Modularity**           | ⚠️ Good with exceptions    | `app/misc` (51 files) needs splitting; `LedgerMaster` lacks abstraction |
| **Coupling**             | ⚠️ 6 circular dependencies | `app↔rpc` most severe (174 includes); `app↔overlay` (51 includes)     |
| **Developer Experience** | ❌ High barrier to entry   | Missing onboarding docs, complex Conan setup, no CMakePresets           |
| **Core Protocol**        | ✅ Well-designed           | Template-based consensus; clean abstractions                            |

### Quantitative Overview

| Metric                           | Count |
| -------------------------------- | ----- |
| Files in `app/misc/`             | 51    |
| Files in `app/tx/detail/`        | 136   |
| RPC handlers                     | 66    |
| Files including `Application.h`  | 90    |
| Files including `LedgerMaster.h` | 57    |
| Total test files                 | 282   |
| Manual test suites               | 24    |

---

## Table of Contents

1. [Current State Analysis](#1-current-state-analysis)
   - [Architecture Overview](#11-architecture-overview)
   - [Levelization System](#12-levelization-system)
   - [Circular Dependencies](#13-circular-dependencies-identified)
2. [Proposed Improvements](#2-proposed-improvements)
   - [Coupling Reduction](#21-coupling-reduction)
   - [Modularity Enhancements](#22-modularity-enhancements)
   - [Developer Experience](#23-developer-experience-improvements)
3. [Implementation Roadmap](#3-implementation-roadmap)
4. [Developer Guidelines](#4-developer-guidelines)

---

## 1. Current State Analysis

### 1.1 Architecture Overview

The rippled codebase follows a two-tier architecture:

```
┌─────────────────────────────────────────────────────────────────┐
│                        xrpld (Application)                       │
│  ┌─────────┬──────────┬─────────┬──────────┬──────────────────┐ │
│  │   app   │  overlay │  rpc    │consensus │   peerfinder     │ │
│  │ (T8)    │  (T7)    │  (T9)   │  (T5)    │     (T6)         │ │
│  └─────────┴──────────┴─────────┴──────────┴──────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│                      libxrpl (Reusable Library)                  │
│  ┌────────┬─────────┬──────────┬──────────┬─────────┬────────┐ │
│  │protocol│ ledger  │ nodestore│  shamap  │ server  │ basics │ │
│  │  (T4)  │  (T6)   │   (T6)   │   (T7)   │  (T5)   │  (T2)  │ │
│  └────────┴─────────┴──────────┴──────────┴─────────┴────────┘ │
│  ┌────────┬─────────┬──────────┬──────────┬─────────┐          │
│  │  json  │ crypto  │   core   │ resource │   net   │          │
│  │  (T3)  │  (T3)   │   (T5)   │   (T5)   │  (T6)   │          │
│  └────────┴─────────┴──────────┴──────────┴─────────┘          │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                        beast (T1)                          │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

**Tier Hierarchy Rules:**

- Lower-tier modules MUST NOT include headers from higher-tier modules
- Implementation details (`detail/` directories) MUST NOT be included across modules
- Each module should focus on a single responsibility

### 1.2 Levelization System

The codebase uses an automated levelization checking system defined in `.github/scripts/levelization/`:

| Library     | Tier  | Modules                                                                                        |
| ----------- | ----- | ---------------------------------------------------------------------------------------------- |
| **libxrpl** | 01-07 | beast → basics → json/crypto → protocol → core/resource/server → ledger/nodestore/net → shamap |
| **xrpld**   | 05-10 | conditions/consensus → core/peerfinder → shamap/overlay → app → rpc → perflog                  |

**Current Enforcement:** CI workflow validates levelization on every PR. Violations are tracked in `loops.txt`.

### 1.3 Circular Dependencies Identified

**Critical Cycles (from `loops.txt`):**

| Cycle                           | Severity    | Include Count | Breakdown                            | Root Cause                                                                              |
| ------------------------------- | ----------- | ------------- | ------------------------------------ | --------------------------------------------------------------------------------------- |
| `xrpld.app ↔ xrpld.rpc`        | 🔴 Critical | **174**       | rpc→app: 153, app→rpc: 21            | RPC handlers depend on Application, LedgerMaster; NetworkOPs implements InfoSub::Source |
| `xrpld.app ↔ xrpld.overlay`    | 🔴 Critical | **51**        | overlay→app: 28, app→overlay: 23     | OverlayImpl → Application, PeerImp → multiple app components                            |
| `xrpld.overlay ↔ xrpld.rpc`    | 🟠 High     | **9**         | overlay→rpc: 4, rpc→overlay: 5       | make_Overlay.h includes ServerHandler.h                                                 |
| `xrpld.app ↔ xrpld.shamap`     | 🟡 Medium   | **5**         | shamap→app: 4, app→shamap: 1         | NodeFamily.cpp includes LedgerMaster, Application, CollectorManager, Tuning             |
| `xrpld.app ↔ xrpld.peerfinder` | 🟢 Low      | **3**         | peerfinder→app: 1, app→peerfinder: 2 | StoreSqdb.h includes app/rdb/PeerFinder.h                                               |
| `xrpld.app ↔ xrpld.core`       | 🟢 Low      | **1**         | core→app: 1                          | core/DatabaseCon.h includes app/main/DBInit.h                                           |

**Test-related cycles (lower priority):**

- `test.jtx ↔ test.toplevel`
- `test.jtx ↔ test.unit_test`

**Note:** The `app↔rpc` cycle is the most severe with 174 total cross-module includes, primarily due to 30 of 66 RPC handlers directly including `Application.h` and 26 including `LedgerMaster.h`.

---

## 2. Proposed Improvements

### 2.1 Coupling Reduction

#### Priority 1: Quick Fixes (Single File Moves)

| File       | Current Location     | Proposed Location | Breaks Cycle        | Risk | Feasibility                                                        |
| ---------- | -------------------- | ----------------- | ------------------- | ---- | ------------------------------------------------------------------ |
| `DBInit.h` | `app/main/`          | `core/`           | `app ↔ core`       | Low  | ✅ High - only contains inline constexpr data, no app dependencies |
| `Store.h`  | `peerfinder/detail/` | `peerfinder/`     | `app ↔ peerfinder` | Low  | ✅ High - extract interface to public directory                    |

**Note:** The original recommendation to move `NodeFamily` to `app/main/` is reclassified to Phase 2 as it has 4 includes from app modules and requires careful planning.

#### Priority 2: Interface Extraction (Medium Effort)

**Break `app ↔ overlay` cycle:**

The cycle exists because `overlay/make_Overlay.h` includes `rpc/ServerHandler.h`. The fix involves forward-declaring or minimizing what Overlay needs from ServerHandler:

```cpp
// NEW: src/xrpld/overlay/OverlayServerHandler.h
namespace xrpl {

/** Minimal interface for Overlay's server handling needs.
    @note This interface breaks the overlay→rpc dependency.
*/
class OverlayServerHandler
{
protected:
    OverlayServerHandler() = default;

public:
    virtual ~OverlayServerHandler() = default;

    virtual Handoff
    onHandoff(
        std::unique_ptr<boost::beast::ssl_stream<boost::beast::tcp_stream>>&& bundle,
        http_request_type&& request,
        boost::asio::ip::tcp::endpoint remote_address) = 0;
};

}  // namespace xrpl
```

**Break `app ↔ rpc` cycle (highest priority - 174 includes):**

```cpp
// NEW: src/xrpld/core/LedgerDataProvider.h
namespace xrpl {

/** Provides ledger data for RPC queries.
    @note This interface breaks the rpc→app dependency by allowing
    RPC handlers to depend on this interface instead of LedgerMaster directly.
*/
class LedgerDataProvider
{
public:
    virtual ~LedgerDataProvider() = default;

    virtual std::optional<LedgerInfo>
    getLedgerInfo(LedgerIndex seq) = 0;

    virtual std::vector<AccountTx>
    getAccountTransactions(
        AccountID const& account,
        LedgerIndex minLedger,
        LedgerIndex maxLedger) = 0;
};

}  // namespace xrpl
```

#### Priority 3: Application Decomposition (High Effort)

Extract service interfaces from the `Application` god object:

| Service Group        | Methods to Extract                  | New Interface        |
| -------------------- | ----------------------------------- | -------------------- |
| Ledger Services      | `getLedgerMaster()`, `openLedger()` | `LedgerService`      |
| Network Services     | `overlay()`, `cluster()`            | `NetworkService`     |
| Transaction Services | `getTxQ()`, `getHashRouter()`       | `TransactionService` |
| Consensus Services   | `getOPs()`, `validators()`          | `ConsensusService`   |

### 2.2 Modularity Enhancements

#### Split `app/misc/` Module

The `app/misc/` directory contains **51 files** violating Single Responsibility Principle:

| New Module        | Files to Move                                                         | Purpose               |
| ----------------- | --------------------------------------------------------------------- | --------------------- |
| `app/validators/` | `Manifest.*`, `ValidatorList.*`, `ValidatorSite.*`, `ValidatorKeys.*` | Validator management  |
| `app/amendments/` | `AmendmentTable.*`, `NegativeUNLVote.*`                               | Protocol amendments   |
| `app/fees/`       | `FeeVote.*`, `LoadFeeTrack.*`                                         | Fee management        |
| `app/network/`    | `NetworkOPs.*`, `Transaction.*`                                       | Network operations    |
| `app/txqueue/`    | `HashRouter.*`, `CanonicalTXSet.*`, `TxQ.*`, `DeliverMax.*`           | Transaction handling  |
| `app/amm/`        | `AMMHelpers.*`, `AMMUtils.*`                                          | AMM utilities         |
| `app/nodestore/`  | `SHAMapStore.*`, `SHAMapStoreImp.*`                                   | Node store management |

**Additional splitting opportunity:** `app/tx/detail/` contains **136 files** and should be split into:

- `app/tx/amm/` - AMM transactions (7 files)
- `app/tx/nft/` - NFT transactions (6 files)
- `app/tx/defi/` - Escrow, PayChan, XChainBridge, Loan, Vault transactions
- `app/tx/core/` - Transactor, ApplyContext, InvariantCheck, applySteps

#### Abstract `LedgerMaster`

Convert concrete class (currently 2,651 lines across .h/.cpp) to abstract interface pattern:

```cpp
// NEW: src/xrpld/app/ledger/LedgerMaster.h (interface)
namespace xrpl {

/** Abstract interface for ledger management.
    @note LedgerMaster is included by 57 files - abstracting it
    enables better testability and reduces concrete dependencies.
*/
class LedgerMaster : public AbstractFetchPackContainer
{
public:
    virtual ~LedgerMaster() = default;

    virtual std::shared_ptr<Ledger const>
    getValidatedLedger() = 0;

    virtual std::shared_ptr<Ledger const>
    getClosedLedger() = 0;

    virtual LedgerIndex
    getValidLedgerIndex() = 0;
    // ... other pure virtual methods
};

// Factory function matching existing pattern
std::unique_ptr<LedgerMaster>
make_LedgerMaster(
    Application& app,
    Stopwatch& stopwatch,
    beast::insight::Collector::ptr const& collector,
    beast::Journal journal);

}  // namespace xrpl
```

#### Split Large "God Object" Files

| File                 | Current Lines | Recommendation                                                      |
| -------------------- | ------------- | ------------------------------------------------------------------- |
| `NetworkOPs.cpp`     | 4,893         | Split into: TransactionProcessor, SubscriptionManager, ServerStatus |
| `PeerImp.cpp`        | 3,650         | Split into: PeerConnection, PeerProtocol, PeerMetrics               |
| `InvariantCheck.cpp` | 3,569         | Split into one file per invariant class                             |
| `LedgerMaster.cpp`   | 2,245         | Extract abstract interface first                                    |
| `Application.cpp`    | 2,208         | Acceptable (assembles entire application)                           |

### 2.3 Developer Experience Improvements

#### Documentation Gaps to Address

| Document                      | Location                        | Content                                       | Priority |
| ----------------------------- | ------------------------------- | --------------------------------------------- | -------- |
| **Architecture Guide**        | `docs/ARCHITECTURE.md`          | Module dependencies, data flow, key patterns  | High     |
| **Feature Development Guide** | `docs/FEATURE_DEVELOPMENT.md`   | How to add amendments, transaction types      | High     |
| **jtx Testing Guide**         | `src/test/jtx/README.md`        | DSL syntax for 128 helper files               | High     |
| **Consensus Internals**       | `src/xrpld/consensus/README.md` | Template adaptor pattern, phase transitions   | Medium   |
| **Manual Tests Guide**        | `src/test/MANUAL_TESTS.md`      | Purpose of 24 manual test suites, when to run | Low      |

#### Build System Improvements

**Current Pain Points (not in original doc):**

| Issue                                 | Impact                                              | Solution                        |
| ------------------------------------- | --------------------------------------------------- | ------------------------------- |
| No `CMakePresets.json`                | IDE integration requires manual setup               | Create standard presets         |
| Multi-step Conan profile setup        | 10+ potential config issues before first build      | Provide pre-configured profiles |
| Patched recipes on `conan.ripplex.io` | Builds fail if remote unavailable                   | Document fallback procedure     |
| 27+ runtime dependencies              | First `conan install --build missing` takes 30+ min | Provide Docker with cached deps |
| Global `link_libraries()` in CMake    | Less modular build                                  | Migrate to per-target linking   |

**Recommended improvements:**

1. **CMakePresets.json** (missing - critical for IDE support)

   ```json
   {
     "version": 3,
     "configurePresets": [
       {
         "name": "conan-release",
         "displayName": "Conan Release",
         "toolchainFile": "${sourceDir}/build/generators/conan_toolchain.cmake",
         "binaryDir": "${sourceDir}/build",
         "cacheVariables": {
           "CMAKE_BUILD_TYPE": "Release"
         }
       }
     ]
   }
   ```

2. **Docker Development Environment**

   ```dockerfile
   # Development container with pre-installed Conan cache
   FROM ubuntu:24.04
   RUN apt-get update && apt-get install -y build-essential cmake python3-pip
   RUN pip3 install conan
   COPY conan-cache/ /root/.conan2/
   ```

3. **Simplified Build Script**

   ```bash
   # scripts/dev-build.sh
   #!/bin/bash
   set -e
   mkdir -p build && cd build
   conan install .. --output-folder . --build missing
   cmake -DCMAKE_TOOLCHAIN_FILE=generators/conan_toolchain.cmake ..
   cmake --build . --parallel
   ```

4. **Pre-configured IDE Settings**
   - `.vscode/settings.json` with CMake integration
   - `.vscode/launch.json` for debugger configurations
   - `.clangd` sample configuration for LSP

#### Testing Framework Documentation

Create `src/test/jtx/README.md` (the framework has 128 files with no documentation):

```markdown
# jtx Testing Framework Guide

## Core Classes

- `Env`: Simulated ledger environment with genesis accounts
- `Account`: Named account abstraction with automatic key generation
- `JTx`: JSON transaction builder with fluent interface

## Common Patterns

\`\`\`cpp
// Basic payment test
using namespace jtx;
Env env(\*this);
Account alice("alice"), bob("bob");
env.fund(XRP(10000), alice, bob);
env(pay(alice, bob, XRP(100)));
env.close();
BEAST_EXPECT(env.balance(bob) == XRP(10100));
\`\`\`

## Test Base Class Selection

- Use `beast::unit_test::suite` for simple tests
- Use `jtx::TestSuite` for tests needing `expectEquals` helpers
- Use `jtx::AMMTest` for AMM-related tests
```

**Testing Gaps Identified:**

| Gap                                        | Details                                              |
| ------------------------------------------ | ---------------------------------------------------- |
| No integration vs unit test separation     | All 282 test files in single directory               |
| Missing crypto/net module tests            | Security-critical modules lack dedicated tests       |
| 41 of 66 RPC handlers lack dedicated tests | Including critical: Submit, PathFind, RipplePathFind |

---

## 3. Implementation Roadmap

### Phase 1: Quick Wins (1-2 weeks, 1 developer)

| Task                                                  | Files Changed | Risk | Benefit                                     |
| ----------------------------------------------------- | ------------- | ---- | ------------------------------------------- |
| Move `DBInit.h` to core                               | 1             | Low  | Breaks `app↔core` cycle (1 include)        |
| Extract `Store.h` interface from `peerfinder/detail/` | 2-3           | Low  | Breaks `app↔peerfinder` cycle (3 includes) |

**Validation:** Run the full levelization check CI workflow after each change.

> **Note:** The NodeFamily move was originally listed as Phase 1 but is reclassified to Phase 2. NodeFamily.cpp has 4 includes from app modules (LedgerMaster, Application, CollectorManager, Tuning) and requires interface extraction first.

### Phase 2: Interface Extraction (6-10 weeks, 2-3 developers)

| Task                                               | Complexity | Cycle Fixed                 | Priority    |
| -------------------------------------------------- | ---------- | --------------------------- | ----------- |
| Create `LedgerDataProvider` interface              | Medium     | `app↔rpc` (174 includes)   | **Highest** |
| Create `OverlayServerHandler` interface            | Medium     | `overlay↔rpc` (9 includes) | High        |
| Move `NodeFamily` to app (with interface for deps) | Medium     | `app↔shamap` (5 includes)  | Medium      |
| Extract `LedgerMaster` to abstract interface       | High       | Multiple cycles             | Medium      |
| Update all call sites (30+ RPC handlers)           | High       | Above interfaces            | Required    |

**Validation:** Full test suite (282 files) must pass; `loops.txt` must show fewer cycles.

### Phase 3: Major Restructuring (8-12 weeks, 3+ developers)

| Task                                   | Complexity | Breaking Changes    | Files Affected |
| -------------------------------------- | ---------- | ------------------- | -------------- |
| Split `app/misc/` into focused modules | High       | Header path changes | 51 files       |
| Split `app/tx/detail/` into submodules | High       | Header path changes | 136 files      |
| Extract Application service interfaces | Very High  | API changes         | 90+ consumers  |
| Split `NetworkOPs.cpp` (4,893 lines)   | High       | None (internal)     | 1 file → 3     |
| Split `PeerImp.cpp` (3,650 lines)      | High       | None (internal)     | 1 file → 3     |

**Migration Strategy:**

1. Create new interfaces alongside existing code
2. Add `[[deprecated]]` attributes to old APIs
3. Migrate callers incrementally
4. Remove deprecated code after 2 releases

### Phase 4: Developer Experience (Ongoing)

| Task                                              | Owner        | Timeline  | Impact                 |
| ------------------------------------------------- | ------------ | --------- | ---------------------- |
| Create `CMakePresets.json`                        | Build team   | 1 day     | High - IDE integration |
| Write Architecture Guide                          | Core team    | 2 weeks   | High - onboarding      |
| Create jtx documentation (128 files undocumented) | Test team    | 1 week    | High - testing         |
| Docker dev environment with cached deps           | DevOps       | 1 week    | Medium - build time    |
| VSCode/CLion configurations                       | Contributors | Ongoing   | Medium - DX            |
| Add dedicated tests for 41 untested RPC handlers  | QA team      | 4-6 weeks | High - coverage        |

---

## 4. Developer Guidelines

### 4.1 Module Creation Best Practices

When creating new modules:

1. **Place in correct tier** - Check `.github/scripts/levelization/results/ordering.txt`
2. **Use abstract interfaces** - Follow the factory pattern (`make_*` functions)
3. **Hide implementation details** - Use `detail/` subdirectories
4. **One class per header** - As stated in `src/xrpld/README.md`
5. **Include what you use** - Don't rely on transitive includes

### 4.2 Avoiding New Cycles

Before adding includes, manually verify the levelization rules:

1. **Check tier assignment** in `.github/scripts/levelization/results/ordering.txt`
2. **Verify the include direction** - lower tier should not include from higher tier
3. **Run CI levelization check** after PR submission

> **Note:** There is no `--check-include` flag on the levelization scripts. Verification must be done by:
>
> - Checking `ordering.txt` for tier assignments
> - Reviewing `loops.txt` after running the full workflow

**Red Flags:**

- Including `app/` headers from `overlay/`, `rpc/`, or `consensus/`
- Including any header from a higher-tier module
- Including `detail/` headers from outside the module

### 4.3 Testing Requirements

All new code must include:

1. **Unit tests** using jtx framework for transaction logic
2. **Consensus tests** for any protocol changes
3. **Integration tests** for RPC endpoints

```cpp
// Preferred test structure
class NewFeature_test : public beast::unit_test::suite
{
    void testBasicFunctionality()
    {
        testcase("Basic functionality");
        using namespace jtx;
        Env env(*this);
        // ... test code
    }

    void run() override
    {
        testBasicFunctionality();
        testEdgeCases();
        testFailureModes();
    }
};
BEAST_DEFINE_TESTSUITE(NewFeature, app, xrpl);  // Note: use 'xrpl' not 'ripple'
```

### 4.4 Commit Message Convention

Follow the format in `CONTRIBUTING.md` (Conventional Commits style):

```
type: Brief description (max 72 chars)

Detailed explanation of the change, wrapped at 72 characters.
Explain WHY, not just WHAT.

Fixes #1234
```

**Type prefixes:**

- `fix:` - Bug fixes
- `refactor:` - Code changes that neither fix bugs nor add features
- `test:` - Adding or updating tests
- `docs:` - Documentation changes
- `build:` - Build system or dependency changes
- `chore:` - Other changes that don't modify src or test files
- `feat:` - New features

---

## Appendix A: Design Patterns Reference

| Pattern                      | Usage                         | Example                                                       |
| ---------------------------- | ----------------------------- | ------------------------------------------------------------- |
| **Factory**                  | Component creation            | `make_Overlay()`, `make_Application()`                        |
| **Pimpl**                    | Implementation hiding         | `*Impl` classes in `detail/`                                  |
| **Template Adaptor**         | Consensus decoupling          | `Consensus<Adaptor>`                                          |
| **X-Macro (Features)**       | Amendment registration        | `XRPL_FIX()`, `XRPL_FEATURE()` macros in `protocol/Feature.h` |
| **X-Macro (Transactions)**   | Transaction type registration | `TRANSACTION()` macro in `protocol/TxFormats.h`               |
| **X-Macro (Ledger Entries)** | Ledger entry registration     | `LEDGER_ENTRY()` macro in `protocol/LedgerFormats.h`          |
| **Service Locator**          | Dependency access             | `Application::get*()` methods                                 |

## Appendix B: Key Files Reference

| File                                                | Purpose                                     |
| --------------------------------------------------- | ------------------------------------------- |
| `.github/scripts/levelization/`                     | Levelization checking system                |
| `.github/scripts/levelization/results/loops.txt`    | Current circular dependencies               |
| `.github/scripts/levelization/results/ordering.txt` | Module tier assignments                     |
| `cmake/XrplCore.cmake`                              | Module definitions and dependencies         |
| `CONTRIBUTING.md`                                   | Coding standards and contribution process   |
| `BUILD.md`                                          | Build instructions and Conan setup          |
| `src/xrpld/README.md`                               | Source organization guidelines              |
| `src/test/jtx/`                                     | Testing framework (128 files, undocumented) |

---

## Revision History

| Version | Date         | Changes                                                                                                                                                                                                                                                                                                                                                          |
| ------- | ------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1.0     | January 2026 | Initial comprehensive analysis                                                                                                                                                                                                                                                                                                                                   |
| 1.1     | January 2026 | Validated against codebase; corrected include counts (app↔rpc: 174, not 15+); fixed severity rankings; updated file counts (app/misc: 51, app/tx/detail: 136); added quantitative metrics; corrected NodeFamily to Phase 2; fixed test library name to `xrpl`; corrected commit message format to Conventional Commits style; removed non-existent script flags |

---

_This document should be reviewed and updated quarterly to reflect architectural changes._
