# Cycle 7: test.jtx ↔ test.unit_test (Gemini Improved)

## Current State

**Loop detected:** `test.unit_test == test.jtx`

- `test.jtx` includes `test.unit_test` (1 include)
- `test.unit_test` includes `test.jtx` (1 include)

## Analysis of Proposed Solution vs. Gemini Solution

The original proposal suggests moving `SuiteJournal.h` from `unit_test` to `jtx`.
**Gemini Analysis:** This is directionally incorrect.

1.  **Nature of Modules:**
    - `test.unit_test` appears to be a **generic** unit testing framework (containing `SuiteJournal`, `FileDirGuard`, etc.) based on `beast::unit_test`.
    - `test.jtx` is the **Ledger/Transaction** test framework (containing `Env`, `Account`, `Ledger`, etc.).
    - Logically, `jtx` is a higher-level framework that _uses_ the `unit_test` primitives.

2.  **The Component in Question:**
    - `src/test/jtx/TestSuite.h` is a generic class inheriting from `beast::unit_test::suite`.
    - It provides generic assertion helpers (`expectEquals`, `expectException`).
    - It has **zero** dependencies on `jtx` specific types (like Ledger, Account, etc.).

3.  **Conclusion:**
    - `TestSuite.h` is misplaced in `jtx`. It belongs in the generic `unit_test` module.
    - Moving `SuiteJournal` to `jtx` would force the generic `unit_test` module to depend on the specific `jtx` module, verifying the architectural inversion.

## Improved Removal Strategy

### Step 1: Move TestSuite to test.unit_test

Move the generic suite base class to the unit test module.

```bash
git mv src/test/jtx/TestSuite.h src/test/unit_test/TestSuite.h
```

### Step 2: Update Includes in `test/unit_test/`

Update `src/test/unit_test/FileDirGuard.h`:

```cpp
// BEFORE
#include <test/jtx/TestSuite.h>

// AFTER
#include <test/unit_test/TestSuite.h>
```

### Step 3: Update Includes in `test/jtx/`

Update all files in `jtx` that use `TestSuite`. They will now include it from `unit_test`.

```bash
grep -l "test/jtx/TestSuite.h" src/test/jtx/*.h src/test/jtx/*.cpp
```

Common consumers (e.g., `Env_test.cpp`, `WSClient_test.cpp`) will need updating.

### Step 4: Verify Direction

**Resulting Dependency Graph:**

```
test.jtx > test.unit_test   (jtx uses TestSuite, SuiteJournal - ALLOWED)
test.unit_test > xrpl.beast (unit_test uses beast - ALLOWED)
```

This restores the correct architectural layering: `Specific (jtx) -> Generic (unit_test)`.

## Verification Steps

1.  Run levelization: `.github/scripts/levelization/generate.sh`
2.  Verify no `Loop: test.jtx test.unit_test`.
3.  Build: `cd .build && ninja -j4`.
4.  Run tests: `./xrpld --unittest`.

## Risk Assessment

| Risk                    | Likelihood | Impact | Mitigation           |
| :---------------------- | :--------- | :----- | :------------------- |
| Build breaks (includes) | Low        | Low    | Simple grep/sed fix. |

## Estimated Effort

**Very Low** - Single file move and include updates. Better architectural outcome.
