# Cycle Removal Documentation

This directory contains detailed plans for removing the 7 remaining dependency cycles in the rippled codebase.

## Recommended Execution Order

Based on risk/effort analysis, here is the recommended order for removing cycles:

| Priority | Cycle                             | Risk     | Effort    | Why First?                    |
| -------- | --------------------------------- | -------- | --------- | ----------------------------- |
| 1        | Cycle 7: test.jtx↔test.unit_test | Very Low | Very Low  | Quick win, builds confidence  |
| 2        | Cycle 6: test.jtx↔test.toplevel  | Very Low | Low       | Quick win, test-only          |
| 3        | Cycle 3: app↔peerfinder          | Low      | Low       | Only 1 dependency to break    |
| 4        | Cycle 5: consensus↔overlay       | Medium   | Medium    | Unblocks Cycle 1 and 2        |
| 5        | Cycle 4: app↔rpc                 | Medium   | Medium    | Important for modularity      |
| 6        | Cycle 1: app↔consensus           | Medium   | High      | Core architecture improvement |
| 7        | Cycle 2: app↔overlay             | High     | Very High | Largest, do last              |

## Quick Summary

### Cycle 1: app↔consensus (15 deps to break)

- **Root Cause:** Interface headers include app types (OperatingMode enum, RCLValidations)
- **Solution:** Move OperatingMode to `xrpld.core` (shared core header) and move consensus impl files to app
- **Doc:** [cycle-1-app-consensus.md](cycle-1-app-consensus.md)

### Cycle 2: app↔overlay (35 deps to break)

- **Root Cause:** Overlay deeply integrated with app for message handling
- **Solution:** Create overlay dependency interfaces, move handlers to app
- **Doc:** [cycle-2-app-overlay.md](cycle-2-app-overlay.md)

### Cycle 3: app↔peerfinder (1 dep to break)

- **Root Cause:** StoreSqdb.h includes app/rdb/PeerFinder.h
- **Solution:** Move database functions to peerfinder module
- **Doc:** [cycle-3-app-peerfinder.md](cycle-3-app-peerfinder.md)

### Cycle 4: app↔rpc (24 deps to break)

- **Root Cause:** App directly includes RPC types (InfoSub, Context, JSON helpers) and JSON utilities live under rpc but are used by app and lower-level code
- **Solution:** Keep InfoSub in rpc and route app through app-level pub/sub interfaces; move JSON utilities to xrpl/json or xrpl/protocol
- **Doc:** [cycle-4-app-rpc.md](cycle-4-app-rpc.md)

### Cycle 5: consensus↔overlay (3 deps to break)

- **Root Cause:** IOverlayBroadcaster includes Peer.h
- **Solution:** Use forward declarations, define PeerId type alias
- **Doc:** [cycle-5-consensus-overlay.md](cycle-5-consensus-overlay.md)

### Cycle 6: test.jtx↔test.toplevel (10 deps to break)

- **Root Cause:** Files in test/jtx include test/jtx.h convenience header
- **Solution:** Replace convenience includes with specific includes
- **Doc:** [cycle-6-test-jtx-toplevel.md](cycle-6-test-jtx-toplevel.md)

### Cycle 7: test.jtx↔test.unit_test (2 deps to break)

- **Root Cause:** Env.h includes SuiteJournal.h from unit_test
- **Solution:** Move SuiteJournal.h to jtx
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
