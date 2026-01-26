# Cycle Removal Documentation

This directory contains detailed plans for removing the 7 dependency cycles in the rippled codebase.

## Implementation Status

| Cycle | Description | Original Deps | Current Deps | Status |
|-------|-------------|---------------|--------------|--------|
| **Cycle 7** | test.jtx↔test.unit_test | 2 | 0 | ✅ **REMOVED** |
| **Cycle 6** | test.jtx↔test.toplevel | 10 | 0 | ✅ **REMOVED** |
| **Cycle 5** | consensus↔overlay | 3 | 0 | ✅ **REMOVED** |
| **Cycle 3** | app↔peerfinder | 1 | 0 | ✅ **REMOVED** |
| **Cycle 1** | app↔consensus | 15 | 0 | ✅ **REMOVED** |
| **Cycle 4** | app↔rpc | 24 | 15 | ⚠️ **PARTIAL** (37.5% reduced) |
| **Cycle 2** | app↔overlay | 35 | 29 | ⚠️ **PARTIAL** (17% reduced) |

**Last Updated:** 2026-01-26

## Recommended Execution Order

Based on risk/effort analysis, here is the recommended order for removing cycles:

| Priority | Cycle                             | Risk     | Effort    | Status |
| -------- | --------------------------------- | -------- | --------- | ------ |
| 1        | Cycle 7: test.jtx↔test.unit_test | Very Low | Very Low  | ✅ Done |
| 2        | Cycle 6: test.jtx↔test.toplevel  | Very Low | Low       | ✅ Done |
| 3        | Cycle 3: app↔peerfinder          | Low      | Low       | ✅ Done |
| 4        | Cycle 5: consensus↔overlay       | Medium   | Medium    | ✅ Done |
| 5        | Cycle 4: app↔rpc                 | Medium   | Medium    | ⚠️ Partial |
| 6        | Cycle 1: app↔consensus           | Medium   | High      | ✅ Done |
| 7        | Cycle 2: app↔overlay             | High     | Very High | ⚠️ Partial |

## Quick Summary

### Cycle 1: app↔consensus ✅ REMOVED

- **Status:** Fully removed in commit `dfb17af6ae`
- **Changes Made:**
  - Created `src/xrpld/core/OperatingMode.h` with OperatingMode enum
  - Created `src/xrpld/consensus/RCLValidationsFwd.h` for forward declarations
  - Moved 7 *Impl.cpp files from `consensus/detail/` to `app/consensus/detail/`
- **Doc:** [cycle-1-app-consensus.md](cycle-1-app-consensus.md)

### Cycle 2: app↔overlay ⚠️ PARTIAL (29 deps remaining)

- **Status:** Reduced from 35 to 29 dependencies (17% improvement)
- **Changes Made:**
  - Forward declared Application in `OverlayImpl.h` and `Handshake.h`
  - Added missing includes for header self-containment
- **Remaining Work:** 29 deps in .cpp files (PeerImp.cpp, OverlayImpl.cpp) accessing app subsystems. Requires creating overlay dependency interfaces and moving message handlers to app module.
- **Doc:** [cycle-2-app-overlay.md](cycle-2-app-overlay.md)
  - See the **Refined Implementation Plan** section in that doc for a phased interface-based refactor.

### Cycle 3: app↔peerfinder ✅ REMOVED

- **Status:** Fully removed (previously completed)
- **Doc:** [cycle-3-app-peerfinder.md](cycle-3-app-peerfinder.md)

### Cycle 4: app↔rpc ⚠️ PARTIAL (15 deps remaining)

- **Status:** Reduced from 24 to 15 dependencies (37.5% improvement)
- **Changes Made:**
  - Moved `CTID.h` to `include/xrpl/protocol/CTID.h`
  - Moved `LedgerDataProvider.h` to `app/ledger/`
  - Moved `InfoSub.h` and `InfoSub.cpp` to `app/misc/`
  - Extracted `LedgerShortcut` enum to `core/LedgerShortcut.h`
- **Remaining Work:** 15 deps are deeply coupled (DeliveredAmount uses RPC::Context, GRPCServer needs 4 RPC headers). Requires major refactoring.
- **Doc:** [cycle-4-app-rpc.md](cycle-4-app-rpc.md)
  - See the **Refined Implementation Plan** section in that doc for details on JSON helper extraction, `LedgerToJson` decoupling, server wiring, and `Main`/`RPCCall` changes.

### Cycle 5: consensus↔overlay ✅ REMOVED

- **Status:** Fully removed (previously completed)
- **Doc:** [cycle-5-consensus-overlay.md](cycle-5-consensus-overlay.md)

### Cycle 6: test.jtx↔test.toplevel ✅ REMOVED

- **Status:** Fully removed (previously completed)
- **Doc:** [cycle-6-test-jtx-toplevel.md](cycle-6-test-jtx-toplevel.md)

### Cycle 7: test.jtx↔test.unit_test ✅ REMOVED

- **Status:** Fully removed (previously completed)
- **Doc:** [cycle-7-test-jtx-unit_test.md](cycle-7-test-jtx-unit_test.md)

## Verification

After each cycle is broken, run:

```bash
# Check levelization
.github/scripts/levelization/generate.sh
cat .github/scripts/levelization/results/loops.txt

# Build
cd .build && ninja -j4

# Test
./xrpld --unittest
```

## Rollback Strategy

Each cycle removal should be done in separate commits. If issues arise:

```bash
git revert <commit-sha>
```

## Re-analysis Passes Completed

This documentation was created after 5 re-analysis passes:

1. **Pass 1: Dependency Verification** - Verified all include dependencies are correct
2. **Pass 2: Interface Completeness** - Verified interfaces cover required functionality
3. **Pass 3: Build Order Validation** - Verified changes maintain valid level order
4. **Pass 4: Test Impact Assessment** - Identified affected test files
5. **Pass 5: Risk and Rollback** - Assessed risks and verified rollback strategies
