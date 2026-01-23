# Phase 2, Task 2.3: Move NodeFamily to App Module

**Status:** ✅ COMPLETE
**Completed:** 2026-01-23
**Commit:** 6acf7b9801

## Overview

### Problem Statement
`NodeFamily` is currently located in `src/xrpld/shamap/` but has dependencies on the `app` module, contributing to the `app ↔ shamap` cycle. The implementation file (`NodeFamily.cpp`) includes 4 headers from the `app` module, creating an upward dependency that violates proper levelization.

### Success Criteria
- `NodeFamily` class is relocated to `src/xrpld/app/main/`
- All includes and usages are updated correctly
- `app ↔ shamap` cycle dependency is reduced (5 → 1 include)
- All tests pass
- Levelization check passes

---

## Deep Code Analysis

### Current File Locations

**Header:** `src/xrpld/shamap/NodeFamily.h` (89 lines)
```cpp
#include <xrpl/shamap/Family.h>  // Base class in libxrpl

namespace xrpl {
class Application;  // Forward declaration

class NodeFamily : public Family
{
public:
    NodeFamily(Application& app, CollectorManager& cm);
    // ... overrides db(), journal(), getFullBelowCache(), getTreeNodeCache()
    // ... sweep(), reset(), missingNodeAcquireBySeq(), missingNodeAcquireByHash()
private:
    Application& app_;
    NodeStore::Database& db_;
    beast::Journal const j_;
    std::shared_ptr<FullBelowCache> fbCache_;
    std::shared_ptr<TreeNodeCache> tnCache_;
    LedgerIndex maxSeq_{0};
    std::mutex maxSeqMutex_;
    void acquire(uint256 const& hash, std::uint32_t seq);
};
}
```

**Implementation:** `src/xrpld/shamap/NodeFamily.cpp` (91 lines)
```cpp
// These 4 includes create the shamap→app dependency:
#include <xrpld/app/ledger/LedgerMaster.h>
#include <xrpld/app/main/Application.h>
#include <xrpld/app/main/CollectorManager.h>
#include <xrpld/app/main/Tuning.h>
#include <xrpld/shamap/NodeFamily.h>
```

### Dependencies from shamap→app

| Include | Usage |
|---------|-------|
| `Application.h` | `app.getNodeStore()`, `app.journal()`, `app.config()`, `app.getInboundLedgers()`, `app.getLedgerMaster()` |
| `CollectorManager.h` | `cm.collector()` for metrics |
| `LedgerMaster.h` | `app_.getLedgerMaster().getHashBySeq(seq)` in `missingNodeAcquireBySeq()` |
| `Tuning.h` | `fullBelowTargetSize`, `fullBelowExpiration` constants |

### Family Base Class (libxrpl)

**Location:** `include/xrpl/shamap/Family.h` (71 lines)

The `Family` abstract base class is correctly placed in `libxrpl` (the library tier). It defines the interface:
- `db()` - Access to NodeStore::Database
- `journal()` - Logging
- `getFullBelowCache()` / `getTreeNodeCache()` - Cache access
- `sweep()` / `reset()` - Lifecycle management
- `missingNodeAcquireBySeq()` / `missingNodeAcquireByHash()` - Missing node handling

### Application Usage

**Location:** `src/xrpld/app/main/Application.cpp`

```cpp
#include <xrpld/shamap/NodeFamily.h>  // Line 36 - creates app→shamap dependency

class ApplicationImp : public Application ... {
    NodeFamily nodeFamily_;              // Line 171 - member variable
    // ...
    , nodeFamily_(*this, *m_collectorManager)  // Line 334 - constructor
    // ...
    Family& getNodeFamily() override { return nodeFamily_; }  // Lines 525-528
};
```

Also used in:
- Lines 967-984: Sweep operations for caches
- Line 1694: Genesis ledger creation
- Line 1832: Ledger loading

### TestNodeFamily (No Impact)

**Location:** `src/test/shamap/common.h` (115 lines)

`TestNodeFamily` extends `Family` directly (not `NodeFamily`). It provides a test-only implementation with no dependencies on the `app` module. **This class is NOT affected by moving `NodeFamily`.**

---

## Design Considerations

### Option A: Move NodeFamily to app/main/ (RECOMMENDED ✓)

**Approach:** Move both `NodeFamily.h` and `NodeFamily.cpp` to `src/xrpld/app/main/`.

**Justification:**
- `NodeFamily` is inherently app-specific (requires `Application`, `LedgerMaster`, `CollectorManager`)
- The `Family` base class in `libxrpl` provides the abstraction that `shamap` needs
- Moving to `app/main/` places it alongside its dependencies (`Application`, `CollectorManager`, `Tuning`)
- Eliminates the shamap→app dependency entirely
- Simple, minimal-risk change

**Impact:**
- One include path change in `Application.cpp`
- No interface changes
- No behavioral changes

### Option B: Create Interface in shamap, Implementation in app

**Approach:** Keep a minimal interface in `shamap`, move implementation to `app`.

**Why NOT recommended:**
- `Family` base class already provides the necessary abstraction
- Would add unnecessary complexity
- No additional benefit over Option A

### Option C: Remove app Dependencies via Callbacks

**Approach:** Inject dependencies as callbacks/interfaces to keep `NodeFamily` in `shamap`.

**Why NOT recommended:**
- Significant refactoring effort
- `NodeFamily` is conceptually an app-layer component
- Would obscure the natural ownership relationship

---

## Implementation Plan

### Step 1: Create New Files in app/main/

Create `src/xrpld/app/main/NodeFamily.h`:
```cpp
#ifndef XRPL_APP_MAIN_NODEFAMILY_H_INCLUDED
#define XRPL_APP_MAIN_NODEFAMILY_H_INCLUDED

#include <xrpl/shamap/Family.h>

namespace xrpl {

class Application;

class NodeFamily : public Family
{
    // ... (copy existing content from shamap/NodeFamily.h)
};

}  // namespace xrpl

#endif
```

Create `src/xrpld/app/main/NodeFamily.cpp`:
```cpp
#include <xrpld/app/main/NodeFamily.h>  // Updated include path
#include <xrpld/app/ledger/LedgerMaster.h>
#include <xrpld/app/main/Application.h>
#include <xrpld/app/main/CollectorManager.h>
#include <xrpld/app/main/Tuning.h>

namespace xrpl {
// ... (copy existing implementation)
}  // namespace xrpl
```

### Step 2: Update Include Path in Application.cpp

In `src/xrpld/app/main/Application.cpp`, change:
```cpp
// Before (line 36):
#include <xrpld/shamap/NodeFamily.h>

// After:
#include <xrpld/app/main/NodeFamily.h>
```

### Step 3: Check for Other Usages

Search for any other files that include `NodeFamily.h`:
```bash
grep -rn "shamap/NodeFamily.h" src/
```

Based on analysis, only `Application.cpp` includes `NodeFamily.h`.

### Step 4: CMakeLists.txt (No Changes Required)

The `xrpld` target uses `file(GLOB_RECURSE ...)` to include all `.cpp` files under `src/xrpld/`:
```cmake
file(GLOB_RECURSE sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/xrpld/*.cpp"
)
```

Moving files within `src/xrpld/` requires **no CMake changes**.

### Step 5: Delete Old Files

Remove the original files:
- `src/xrpld/shamap/NodeFamily.h`
- `src/xrpld/shamap/NodeFamily.cpp`

The `src/xrpld/shamap/` directory will become empty and can be removed.

### Step 6: Run Levelization Check

```bash
./.github/scripts/levelization/levelization.sh
```

Verify:
- No `shamap→app` dependencies remain
- The `app ↔ shamap` cycle is resolved

---

## Risk Assessment

### Low Risk Items

| Risk | Mitigation |
|------|------------|
| Include path changes | Only `Application.cpp` includes `NodeFamily.h` |
| Build breakage | GLOB_RECURSE handles file moves automatically |
| Test impact | `TestNodeFamily` uses `Family` base class, not `NodeFamily` |

### Considerations

1. **Empty shamap directory:** After the move, `src/xrpld/shamap/` will be empty. This is expected - all shamap code in libxrpl is in `include/xrpl/shamap/` and `src/libxrpl/shamap/`.

2. **Header include guards:** Update the include guard from `XRPL_SHAMAP_NODEFAMILY_H_INCLUDED` to `XRPL_APP_MAIN_NODEFAMILY_H_INCLUDED`.

3. **Namespace:** No change needed - `NodeFamily` stays in `namespace xrpl`.

### Rollback Strategy

1. **Git Revert**: All changes are atomic commits that can be reverted
2. **Verification**: After revert, run full test suite to confirm working state
3. **Time to Rollback**: < 5 minutes

---

## Validation Criteria

### Compilation
```bash
cmake --build build --target xrpld
```

### Unit Tests
```bash
./build/xrpld --unittest
```

### Levelization Check
```bash
./.github/scripts/levelization/levelization.sh
```

**Expected outcome:** The `app ↔ shamap` cycle should show 0 dependencies from `shamap→app`.

### Manual Verification

Check that these still work correctly:
- Node startup and ledger operations
- Missing node acquisition (via `missingNodeAcquireBySeq`/`missingNodeAcquireByHash`)
- Cache sweeping during runtime

---

## Dependencies

- **Blocked by:** None
- **Blocks:** Phase 2 completion

---

## Estimated Effort

- **Implementation:** 30 minutes
- **Testing:** 30 minutes
- **Total:** 1 hour

---

## Checklist

- [x] Create `src/xrpld/app/main/NodeFamily.h`
- [x] Create `src/xrpld/app/main/NodeFamily.cpp`
- [x] Update include in `Application.cpp`
- [x] Delete `src/xrpld/shamap/NodeFamily.h`
- [x] Delete `src/xrpld/shamap/NodeFamily.cpp`
- [x] Remove empty `src/xrpld/shamap/` directory
- [x] Run compilation
- [x] Run unit tests
- [x] Run levelization check
- [x] Verify cycle is resolved

