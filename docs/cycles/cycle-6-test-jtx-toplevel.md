# Cycle 6: test.jtx ↔ test.toplevel

## Current State

**Loop detected:** `test.toplevel > test.jtx`

The `test.toplevel` module (files at `src/test/` root level) includes `test.jtx` (55 includes), and `test.jtx` includes `test.toplevel` (10 includes).

## Dependency Analysis

### What is test.toplevel?

`test.toplevel` refers to files directly in `src/test/` (not in subdirectories):

- `src/test/jtx.h` - Convenience header that includes all jtx headers
- `src/test/csf.h` - Consensus simulation framework header

### toplevel → jtx Dependencies (55 includes) - EXPECTED

`src/test/jtx.h` is a convenience header that includes all `test/jtx/*.h` files:

```cpp
// src/test/jtx.h
#include <test/jtx/AMM.h>
#include <test/jtx/Account.h>
#include <test/jtx/Env.h>
// ... 50+ more includes
```

This is the expected direction - the convenience header aggregates the module.

### jtx → toplevel Dependencies (10 includes) - PROBLEMATIC

Some files in `test/jtx/` include `test/jtx.h` (the convenience header):

| File                                     | Includes     | Purpose     |
| ---------------------------------------- | ------------ | ----------- |
| `test/jtx/Oracle.h`                      | `test/jtx.h` | Convenience |
| `test/jtx/PathSet.h`                     | `test/jtx.h` | Convenience |
| `test/jtx/impl/WSClient.cpp`             | `test/jtx.h` | Convenience |
| `test/jtx/impl/permissioned_dex.cpp`     | `test/jtx.h` | Convenience |
| `test/jtx/impl/permissioned_domains.cpp` | `test/jtx.h` | Convenience |
| `test/jtx/impl/mpt.cpp`                  | `test/jtx.h` | Convenience |
| `test/jtx/WSClient_test.cpp`             | `test/jtx.h` | Convenience |
| `test/jtx/Env_test.cpp`                  | `test/jtx.h` | Convenience |

## Root Cause

Files within `test/jtx/` are including the convenience header `test/jtx.h` instead of including only the specific headers they need. This creates a cycle because:

```
test/jtx.h → test/jtx/Oracle.h → test/jtx.h (cycle!)
```

## Removal Strategy

### Step 1: Replace convenience header includes with specific includes

For each file in `test/jtx/` that includes `test/jtx.h`, replace with specific includes:

**Example - test/jtx/Oracle.h:**

```cpp
// BEFORE
#include <test/jtx.h>

// AFTER
#include <test/jtx/Env.h>
#include <test/jtx/Account.h>
// ... only what's actually needed
```

### Step 2: Update all affected files

| File                                     | Action                                      |
| ---------------------------------------- | ------------------------------------------- |
| `test/jtx/Oracle.h`                      | Replace `test/jtx.h` with specific includes |
| `test/jtx/PathSet.h`                     | Replace `test/jtx.h` with specific includes |
| `test/jtx/impl/WSClient.cpp`             | Replace `test/jtx.h` with specific includes |
| `test/jtx/impl/permissioned_dex.cpp`     | Replace `test/jtx.h` with specific includes |
| `test/jtx/impl/permissioned_domains.cpp` | Replace `test/jtx.h` with specific includes |
| `test/jtx/impl/mpt.cpp`                  | Replace `test/jtx.h` with specific includes |
| `test/jtx/WSClient_test.cpp`             | Replace `test/jtx.h` with specific includes |
| `test/jtx/Env_test.cpp`                  | Replace `test/jtx.h` with specific includes |

### Step 3: Add include-what-you-use comments

Add comments to `test/jtx.h` to clarify its purpose:

```cpp
// src/test/jtx.h
// Convenience header for TEST FILES ONLY.
// Do NOT include this from other headers in test/jtx/.
// Instead, include only the specific headers you need.
```

## Expected Result After Changes

```
test.toplevel > test.jtx  (toplevel includes jtx - ALLOWED)
test.jtx > xrpl.*         (jtx includes xrpl libs - ALLOWED)
```

No cycle between test.jtx and test.toplevel.

## Verification Steps

1. Run levelization: `.github/scripts/levelization/generate.sh`
2. Verify no `Loop: test.jtx test.toplevel` in output
3. Build: `cd .build && ninja -j4`
4. Run tests: `./xrpld --unittest`

## Risk Assessment

| Risk                  | Likelihood | Impact | Mitigation                   |
| --------------------- | ---------- | ------ | ---------------------------- |
| Missing includes      | Medium     | Low    | Build will fail, easy to fix |
| Compile time increase | Low        | Low    | Minimal impact               |

## Estimated Effort

**Low effort (1-2 hours)** - Simple include replacements.

## Notes

This is a test-only cycle and doesn't affect production code. However, fixing it:

1. Improves build times (less header parsing)
2. Makes dependencies explicit
3. Follows include-what-you-use best practices
