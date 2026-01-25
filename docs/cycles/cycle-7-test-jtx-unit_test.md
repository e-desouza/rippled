# Cycle 7: test.jtx ↔ test.unit_test

## Current State

**Loop detected:** `test.unit_test == test.jtx`

The `==` indicates a bidirectional dependency at the same level.

- `test.jtx` includes `test.unit_test` (1 include)
- `test.unit_test` includes `test.jtx` (1 include)

## Dependency Analysis

### jtx → unit_test Dependency (1 include)

| File             | Includes                        | Purpose                 |
| ---------------- | ------------------------------- | ----------------------- |
| `test/jtx/Env.h` | `test/unit_test/SuiteJournal.h` | Journal for test output |

### unit_test → jtx Dependency (1 include)

| File                            | Includes               | Purpose               |
| ------------------------------- | ---------------------- | --------------------- |
| `test/unit_test/FileDirGuard.h` | `test/jtx/TestSuite.h` | Test suite base class |

## Root Cause

The cycle exists because:

1. `Env.h` (test environment) needs `SuiteJournal.h` for logging
2. `FileDirGuard.h` (file/directory test helper) needs `TestSuite.h` for the base class

Both modules have legitimate needs for each other's functionality.

## Removal Strategy

### Option A: Move SuiteJournal to test.jtx (Recommended)

`SuiteJournal` is used by `Env.h` which is the core test environment. It makes sense to have the journal in the same module.

**Move:** `src/test/unit_test/SuiteJournal.h` → `src/test/jtx/SuiteJournal.h`

**Update includes:**

- `test/jtx/Env.h` - Change to `test/jtx/SuiteJournal.h`
- Any other files including `test/unit_test/SuiteJournal.h`

**Result:**

```
test.unit_test > test.jtx  (unit_test depends on jtx - ALLOWED)
```

### Option B: Move TestSuite to test.unit_test

Alternatively, move `TestSuite.h` to `test/unit_test/`:

**Move:** `src/test/jtx/TestSuite.h` → `src/test/unit_test/TestSuite.h`

**Update includes:**

- `test/unit_test/FileDirGuard.h` - Change to `test/unit_test/TestSuite.h`
- All files including `test/jtx/TestSuite.h`

**Result:**

```
test.jtx > test.unit_test  (jtx depends on unit_test - ALLOWED)
```

### Option C: Create shared test.base module

Create a new `test/base/` module for shared test infrastructure:

**New module:** `src/test/base/`

- `test/base/SuiteJournal.h` (moved from unit_test)
- `test/base/TestSuite.h` (moved from jtx)

**Result:**

```
test.jtx > test.base       (jtx depends on base - ALLOWED)
test.unit_test > test.base (unit_test depends on base - ALLOWED)
```

## Recommended Approach

**Option A (Move SuiteJournal to jtx)** is recommended because:

1. `SuiteJournal` is primarily used by `Env.h` in jtx
2. Minimal file moves (1 file)
3. `test.jtx` is the primary test framework, so it makes sense for it to be lower-level
4. `test.unit_test` can depend on `test.jtx` for test infrastructure

## Implementation Steps

### Step 1: Move SuiteJournal.h

```bash
git mv src/test/unit_test/SuiteJournal.h src/test/jtx/SuiteJournal.h
```

### Step 2: Update includes

**File:** `src/test/jtx/Env.h`

```cpp
// BEFORE
#include <test/unit_test/SuiteJournal.h>

// AFTER
#include <test/jtx/SuiteJournal.h>
```

### Step 3: Update any other consumers

```bash
grep -rn "test/unit_test/SuiteJournal.h" --include="*.h" --include="*.cpp" src/
```

Update all found files to use the new path.

## Expected Result After Changes

```
test.unit_test > test.jtx  (unit_test depends on jtx - ALLOWED)
test.jtx > xrpl.*          (jtx depends on xrpl libs - ALLOWED)
```

No cycle between test.jtx and test.unit_test.

## Test Impact

**Tests that use SuiteJournal (will need include path update):**

- `test/overlay/cluster_test.cpp`
- `test/jtx/impl/Env.cpp`
- `test/jtx/Env.h`
- `test/consensus/Validations_test.cpp`
- `test/consensus/Consensus_test.cpp`
- `test/server/Server_test.cpp`
- `test/peerfinder/PeerFinder_test.cpp`
- `test/peerfinder/Livecache_test.cpp`
- `test/basics/KeyCache_test.cpp`
- `test/basics/IntrusiveShared_test.cpp`

**Required test runs after changes:**

```bash
./xrpld --unittest  # All tests use jtx/Env
```

## Verification Steps

1. Run levelization: `.github/scripts/levelization/generate.sh`
2. Verify no `Loop: test.jtx test.unit_test` in output
3. Build: `cd .build && ninja -j4`
4. Run tests: `./xrpld --unittest`

## Risk Assessment

| Risk                    | Likelihood | Impact | Mitigation        |
| ----------------------- | ---------- | ------ | ----------------- |
| Missing include updates | Low        | Low    | Grep for old path |
| Build failures          | Low        | Low    | Easy to fix       |

## Estimated Effort

**Low effort (30 minutes)** - Single file move with include updates.

## Notes

This is a test-only cycle and doesn't affect production code. The fix is straightforward and improves the test module organization.
