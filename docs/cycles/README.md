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
| **Cycle 4** | app↔rpc | 24→15→3 | 3 headers | ⚠️ **87.5% reduced** |
| **Cycle 2** | app↔overlay | 35→29→4 | 4 cpp deps | ⚠️ **89% reduced** |

**Last Updated:** 2026-01-27

## Current State Summary

After extensive levelization work, only **2 cycles remain** (down from 8 in origin/develop):

```
Loop: xrpld.app xrpld.overlay
  xrpld.app > xrpld.overlay

Loop: xrpld.app xrpld.rpc
  xrpld.rpc > xrpld.app
```

### Remaining Dependencies

**Cycle 2 (app↔overlay):** 4 cpp-only dependencies
- `OverlayImpl.cpp` → `Application.h` (25 app_ usages)
- `PeerImp.cpp` → `Application.h`, `Ledger.h` (30+ app_ usages)
- `ConnectAttempt.cpp` → `Application.h` (1 usage: cluster check)

**Cycle 4 (app↔rpc):** 3 header dependencies
- `Context.h` → `LedgerDataProvider.h` (interface reference)
- `Handler.h` → `NetworkOPs.h` (isAmendmentBlocked/isUNLBlocked methods)
- `LedgerHandler.h` → `TxQ.h` (TxQ::TxDetails nested type)

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

### Cycle 2: app↔overlay ⚠️ 4 CPP DEPS REMAINING (89% reduced)

- **Status:** Reduced from 35 to 4 dependencies (89% improvement)
- **Changes Made:**
  - Created 10+ interfaces: `IFeeTrackOps`, `IHandshakeParams`, `IPeerReservationStorage`, `IOverlayOps`, `IHashRouterOps`, `IValidatorOps`, `ILedgerDataOps`, `ILedgerMasterOps`, `ILedgerReplayMsgHandler`, `IOverlayProvider`
  - Moved message handlers to `app/overlay/handlers/`
  - Forward declared Application in headers
  - Replaced ValidatorList.h with smaller Manifest.h
- **Remaining Work:** 4 cpp-only deps require major refactoring (see [cycle-2-app-overlay.md](cycle-2-app-overlay.md))
- **Doc:** [cycle-2-app-overlay.md](cycle-2-app-overlay.md)

### Cycle 3: app↔peerfinder ✅ REMOVED

- **Status:** Fully removed (previously completed)
- **Doc:** [cycle-3-app-peerfinder.md](cycle-3-app-peerfinder.md)

### Cycle 4: app↔rpc ⚠️ 3 HEADER DEPS REMAINING (87.5% reduced)

- **Status:** Reduced from 24 to 3 header dependencies (87.5% improvement)
- **Changes Made:**
  - Moved `CTID.h` to `include/xrpl/protocol/CTID.h`
  - Moved `LedgerDataProvider.h` to `app/ledger/`
  - Moved `InfoSub` to `include/xrpl/subscription/` (xrpl library)
  - Moved `Manifest` to `include/xrpl/validators/` (xrpl library)
  - Extracted `FailHard` enum to `core/FailHard.h`
  - Extracted `LedgerShortcut` enum to `core/LedgerShortcut.h`
  - Removed obsolete `GetCounts.h`
  - Moved `Version.h` constructor to `.cpp`
- **Remaining Work:** 3 header deps require interface extraction (see [cycle-4-app-rpc.md](cycle-4-app-rpc.md))
- **Doc:** [cycle-4-app-rpc.md](cycle-4-app-rpc.md)

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
