# Cycle 1: xrpld.app ↔ xrpld.consensus

## Current State

**Loop detected:** `xrpld.consensus > xrpld.app`

The consensus module depends on app (15 includes), and app depends on consensus (8 includes).

## Dependency Analysis

### consensus → app Dependencies (15 includes)

These are the problematic dependencies that create the cycle:

| File                                   | Includes                                                     | Purpose                   |
| -------------------------------------- | ------------------------------------------------------------ | ------------------------- |
| `IOperatingMode.h`                     | `app/misc/NetworkOPs.h`                                      | For `OperatingMode` enum  |
| `IValidationTracker.h`                 | `app/consensus/RCLValidations.h`                             | For `RCLValidations` type |
| `detail/LedgerProviderImpl.cpp`        | `app/ledger/LedgerMaster.h`, `app/main/Application.h`        | Implementation            |
| `detail/OverlayBroadcasterImpl.cpp`    | `app/main/Application.h`                                     | Implementation            |
| `detail/MessageRouterImpl.cpp`         | `app/main/Application.h`, `app/txqueue/HashRouter.h`         | Implementation            |
| `detail/ValidationTrackerImpl.cpp`     | `app/consensus/RCLValidations.h`, `app/main/Application.h`   | Implementation            |
| `detail/OperatingModeImpl.cpp`         | `app/main/Application.h`, `app/misc/NetworkOPs.h`            | Implementation            |
| `detail/TxSetManagerImpl.cpp`          | `app/ledger/InboundTransactions.h`, `app/main/Application.h` | Implementation            |
| `detail/ConsensusJobSchedulerImpl.cpp` | `app/main/Application.h`                                     | Implementation            |
| `detail/ConsensusTimeSourceImpl.cpp`   | `app/main/Application.h`                                     | Implementation            |

### app → consensus Dependencies (8 includes) - ALLOWED

These are the expected dependencies (app uses consensus):

| File                             | Includes                                                    | Purpose          |
| -------------------------------- | ----------------------------------------------------------- | ---------------- |
| `app/ledger/Ledger.cpp`          | `consensus/LedgerTiming.h`                                  | Timing constants |
| `app/consensus/RCLConsensus.cpp` | `consensus/LedgerTiming.h`                                  | Timing constants |
| `app/consensus/RCLCxPeerPos.h`   | `consensus/ConsensusProposal.h`                             | Proposal types   |
| `app/consensus/RCLConsensus.h`   | `consensus/Consensus.h`, `consensus/ConsensusAdaptorDeps.h` | Core consensus   |
| `app/consensus/RCLValidations.h` | `consensus/Validations.h`                                   | Validation types |
| `app/misc/NetworkOPs.cpp`        | `consensus/Consensus.h`, `consensus/ConsensusParms.h`       | Consensus params |

## Root Cause

The cycle exists because:

1. **Interface headers include app types**: `IOperatingMode.h` includes `NetworkOPs.h` for the `OperatingMode` enum, and `IValidationTracker.h` includes `RCLValidations.h`
2. **Implementation files include app headers**: All `*Impl.cpp` files include `Application.h` and various app components
3. **RCLValidatedLedger cannot be easily moved**: It depends on `app/ledger/Ledger.h`, so moving it to consensus would still create app dependencies

## Removal Strategy

### Option A: Move implementation files to app (Recommended)

Since the `*Impl.cpp` files all depend on app anyway, move them to app:

**Move to `src/xrpld/app/consensus/detail/`:**

- `LedgerProviderImpl.cpp`
- `OverlayBroadcasterImpl.cpp`
- `MessageRouterImpl.cpp`
- `ValidationTrackerImpl.cpp`
- `OperatingModeImpl.cpp`
- `TxSetManagerImpl.cpp`
- `ConsensusJobSchedulerImpl.cpp`
- `ConsensusTimeSourceImpl.cpp`

This way, the interface headers stay in consensus (without app dependencies), and implementations go in app.

### Option B: Forward declarations + RCLValidationsFwd.h

Create a forward declaration header:

```cpp
// src/xrpld/app/consensus/RCLValidationsFwd.h
#ifndef XRPL_APP_CONSENSUS_RCLVALIDATIONSFWD_H
#define XRPL_APP_CONSENSUS_RCLVALIDATIONSFWD_H

namespace xrpl {
class RCLValidatedLedger;
}

#endif
```

**Problem:** `IValidationTracker.h` uses `RCLValidatedLedger const&` parameters. Forward declarations work for references, but we need the complete type to call methods. This approach works IF the interface methods only take references.

### Step 1: Move OperatingMode enum to core module (shared by app and consensus)

**Current location:** `src/xrpld/app/misc/NetworkOPs.h`

**New location:** `src/xrpld/core/OperatingMode.h`

```cpp
// src/xrpld/core/OperatingMode.h
#ifndef RIPPLE_CORE_OPERATINGMODE_H_INCLUDED
#define RIPPLE_CORE_OPERATINGMODE_H_INCLUDED

namespace ripple {

enum class OperatingMode {
    DISCONNECTED = 0,  //!< not ready to process requests
    CONNECTED = 1,     //!< convinced we are talking to the network
    SYNCING = 2,       //!< fallen slightly behind
    TRACKING = 3,      //!< convinced we agree with the network
    FULL = 4           //!< we have the ledger and can even validate
};

} // namespace ripple
#endif
```

**Files to update:**

- `src/xrpld/app/misc/NetworkOPs.h` - Include new header, remove enum definition
- `src/xrpld/consensus/IOperatingMode.h` - Include `core/OperatingMode.h` instead

### Step 2: Create forward declaration header for RCLValidations

**New file:** `src/xrpld/consensus/RCLValidationsFwd.h`

```cpp
// Forward declarations for RCLValidations types
namespace ripple {
class RCLValidationsAdaptor;
using RCLValidations = Validations<RCLValidationsAdaptor>;
} // namespace ripple
```

**Update:** `src/xrpld/consensus/IValidationTracker.h` to use forward declaration

### Step 3: Move implementation files to app module

The `detail/*Impl.cpp` files are **implementations** that wrap app components. They should live in `app/consensus/detail/` not `consensus/detail/`.

**Move files:**

- `src/xrpld/consensus/detail/LedgerProviderImpl.cpp` → `src/xrpld/app/consensus/detail/`
- `src/xrpld/consensus/detail/OverlayBroadcasterImpl.cpp` → `src/xrpld/app/consensus/detail/`
- `src/xrpld/consensus/detail/MessageRouterImpl.cpp` → `src/xrpld/app/consensus/detail/`
- `src/xrpld/consensus/detail/ValidationTrackerImpl.cpp` → `src/xrpld/app/consensus/detail/`
- `src/xrpld/consensus/detail/OperatingModeImpl.cpp` → `src/xrpld/app/consensus/detail/`
- `src/xrpld/consensus/detail/TxSetManagerImpl.cpp` → `src/xrpld/app/consensus/detail/`
- `src/xrpld/consensus/detail/ConsensusJobSchedulerImpl.cpp` → `src/xrpld/app/consensus/detail/`
- `src/xrpld/consensus/detail/ConsensusTimeSourceImpl.cpp` → `src/xrpld/app/consensus/detail/`

**Keep in consensus module (interfaces only):**

- `src/xrpld/consensus/ILedgerProvider.h`
- `src/xrpld/consensus/IOverlayBroadcaster.h`
- `src/xrpld/consensus/IConsensusJobScheduler.h`
- `src/xrpld/consensus/ITxSetManager.h`
- `src/xrpld/consensus/IMessageRouter.h`
- `src/xrpld/consensus/IConsensusTimeSource.h`
- `src/xrpld/consensus/IValidationTracker.h`
- `src/xrpld/consensus/IOperatingMode.h`
- `src/xrpld/consensus/ConsensusAdaptorDeps.h`

## Expected Result After Changes

```
xrpld.app > xrpld.consensus  (app depends on consensus - ALLOWED)
xrpld.consensus > xrpl.*     (consensus depends on xrpl libs - ALLOWED)
```

No cycle between app and consensus.

## Test Impact

**Tests that include consensus headers:**

- `test/consensus/LedgerTrie_test.cpp`
- `test/consensus/Validations_test.cpp`
- `test/consensus/LedgerTiming_test.cpp`
- `test/consensus/Consensus_test.cpp`
- `test/csf/Proposal.h`, `test/csf/ledgers.h`, `test/csf/Peer.h`

**Tests using OperatingMode:**

- `test/csf/Peer.h` - Uses OperatingMode enum

**Required test runs after changes:**

```bash
./xrpld --unittest=Consensus
./xrpld --unittest=Validations
./xrpld --unittest=LedgerTrie
./xrpld --unittest=LedgerTiming
```

## Verification Steps

1. Run levelization: `.github/scripts/levelization/generate.sh`
2. Verify no `Loop: xrpld.app xrpld.consensus` in output
3. Build: `cd .build && ninja -j4`
4. Run tests (above)

## Risk Assessment

| Risk                    | Likelihood | Impact | Mitigation                                |
| ----------------------- | ---------- | ------ | ----------------------------------------- |
| Breaking existing code  | Low        | Medium | Incremental changes, test after each step |
| Missing include updates | Medium     | Low    | Grep for old paths, update all consumers  |
| Build order issues      | Low        | Medium | Verify with clean build                   |

## Rollback Strategy

```bash
git revert <commit-sha>
```

Each step should be a separate commit for easy rollback.
