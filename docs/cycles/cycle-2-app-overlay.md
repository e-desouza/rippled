# Cycle 2: xrpld.app ↔ xrpld.overlay

## Current State

**Loop detected:** `xrpld.overlay > xrpld.app`

The overlay module has 35 includes from app, and app has 23 includes from overlay.

## Dependency Analysis

### overlay → app Dependencies (35 includes) - PROBLEMATIC

| File                                           | Includes                                                           | Purpose                |
| ---------------------------------------------- | ------------------------------------------------------------------ | ---------------------- |
| `detail/PeerImp.h`                             | `app/consensus/RCLCxPeerPos.h`                                     | Peer position handling |
| `detail/PeerImp.h`                             | `app/ledger/detail/LedgerReplayMsgHandler.h`                       | Ledger replay          |
| `detail/PeerImp.h`                             | `app/txqueue/HashRouter.h`                                         | Transaction routing    |
| `detail/PeerReservationTable.cpp`              | `app/rdb/RelationalDatabase.h`, `app/rdb/Wallet.h`                 | Database access        |
| `detail/Handshake.h`                           | `app/main/Application.h`                                           | App reference          |
| `detail/Handshake.cpp`                         | `app/ledger/LedgerMaster.h`, `app/main/Application.h`              | Ledger access          |
| `detail/OverlayImpl.h`                         | `app/main/Application.h`                                           | App reference          |
| `detail/OverlayImpl.cpp`                       | `app/misc/NetworkOPs.h`, `app/misc/ServerCounts.h`                 | Network ops            |
| `detail/OverlayImpl.cpp`                       | `app/rdb/RelationalDatabase.h`, `app/rdb/Wallet.h`                 | Database               |
| `detail/OverlayImpl.cpp`                       | `app/txqueue/HashRouter.h`                                         | Hash routing           |
| `detail/OverlayImpl.cpp`                       | `app/validators/ValidatorList.h`, `app/validators/ValidatorSite.h` | Validators             |
| `detail/PeerSet.cpp`                           | `app/main/Application.h`                                           | App reference          |
| `detail/PeerImp.cpp`                           | `app/consensus/RCLValidations.h`                                   | Validations            |
| `detail/PeerImp.cpp`                           | `app/ledger/InboundLedgers.h`, `app/ledger/InboundTransactions.h`  | Ledger sync            |
| `detail/PeerImp.cpp`                           | `app/ledger/LedgerMaster.h`, `app/ledger/TransactionMaster.h`      | Ledger/tx access       |
| `detail/PeerImp.cpp`                           | `app/misc/LoadFeeTrack.h`, `app/misc/NetworkOPs.h`                 | Fees, network          |
| `detail/handlers/ValidationMessageHandler.cpp` | Multiple app includes                                              | Validation handling    |

### app → overlay Dependencies (23 includes) - ALLOWED

These are expected (app uses overlay for networking):

| File                                      | Includes                                                           | Purpose            |
| ----------------------------------------- | ------------------------------------------------------------------ | ------------------ |
| `app/rdb/Wallet.h`                        | `overlay/PeerReservationTable.h`                                   | Peer reservations  |
| `app/validators/ValidatorList.h`          | `overlay/Message.h`                                                | Message types      |
| `app/validators/detail/ValidatorList.cpp` | `overlay/Overlay.h`                                                | Overlay access     |
| `app/main/Application.h`                  | `overlay/PeerReservationTable.h`                                   | Peer reservations  |
| `app/main/Application.cpp`                | `overlay/Cluster.h`, `overlay/PeerSet.h`, `overlay/make_Overlay.h` | Overlay creation   |
| `app/ledger/*.h/cpp`                      | `overlay/PeerSet.h`, `overlay/Peer.h`, `overlay/Overlay.h`         | Peer communication |
| `app/consensus/RCLConsensus.cpp`          | `overlay/Overlay.h`, `overlay/predicates.h`                        | Broadcasting       |
| `app/misc/NetworkOPs.cpp`                 | `overlay/Cluster.h`, `overlay/Overlay.h`, `overlay/predicates.h`   | Network ops        |

## Root Cause

The overlay module needs to:

1. Access Application to get subsystems (LedgerMaster, NetworkOPs, etc.)
2. Handle consensus-related messages (RCLCxPeerPos, RCLValidations)
3. Access database for peer reservations
4. Access validators for message validation

## Removal Strategy

### Step 1: Create Overlay Dependency Interfaces

Create interfaces in `overlay/` that define what overlay needs from app:

**New file:** `src/xrpld/overlay/IOverlayDeps.h`

```cpp
namespace ripple {

// Interface for ledger access from overlay
class IOverlayLedgerProvider {
public:
    virtual ~IOverlayLedgerProvider() = default;
    virtual std::shared_ptr<Ledger const> getValidatedLedger() = 0;
    virtual std::shared_ptr<Ledger const> getClosedLedger() = 0;
    virtual LedgerIndex getValidLedgerIndex() = 0;
};

// Interface for transaction routing from overlay
class IOverlayTxRouter {
public:
    virtual ~IOverlayTxRouter() = default;
    virtual bool shouldRelay(uint256 const& hash) = 0;
    virtual void setFlags(uint256 const& hash, int flags) = 0;
};

// Interface for validation handling from overlay
class IOverlayValidationHandler {
public:
    virtual ~IOverlayValidationHandler() = default;
    virtual void onValidation(std::shared_ptr<STValidation> const& val) = 0;
};

} // namespace ripple
```

### Step 2: Remove Application.h includes from overlay headers

**Problem:** `detail/OverlayImpl.h` and `detail/Handshake.h` include `Application.h`

**Solution:** Use forward declarations and pass interfaces via constructor

```cpp
// detail/OverlayImpl.h - BEFORE
#include <xrpld/app/main/Application.h>

// detail/OverlayImpl.h - AFTER
namespace ripple { class Application; }  // Forward declaration
```

### Step 3: Move message handlers to app module

The `detail/handlers/` directory contains message handlers that need app access.
Move them to `app/overlay/handlers/` where they can freely include app headers.

**Move:**

- `overlay/detail/handlers/ValidationMessageHandler.cpp` → `app/overlay/handlers/`

### Step 4: Create adapter implementations in app

Create implementations of the overlay interfaces in `app/overlay/`:

```cpp
// src/xrpld/app/overlay/OverlayLedgerProviderImpl.h
class OverlayLedgerProviderImpl : public IOverlayLedgerProvider {
    LedgerMaster& ledgerMaster_;
public:
    explicit OverlayLedgerProviderImpl(LedgerMaster& lm) : ledgerMaster_(lm) {}
    // ... implement interface methods
};
```

## Expected Result After Changes

```
xrpld.app > xrpld.overlay  (app depends on overlay - ALLOWED)
xrpld.overlay > xrpl.*     (overlay depends on xrpl libs - ALLOWED)
```

No cycle between app and overlay.

## Verification Steps

1. Run levelization: `.github/scripts/levelization/generate.sh`
2. Verify no `Loop: xrpld.app xrpld.overlay` in output
3. Build: `cd .build && ninja -j4`
4. Run tests: `./xrpld --unittest=Overlay`

## Risk Assessment

| Risk                        | Likelihood | Impact | Mitigation           |
| --------------------------- | ---------- | ------ | -------------------- |
| Breaking peer communication | Medium     | High   | Extensive testing    |
| Performance regression      | Low        | Medium | Profile before/after |
| Missing interface methods   | Medium     | Medium | Thorough analysis    |

## Estimated Effort

**High effort (2-3 weeks)** - This is the largest cycle with 35 dependencies to break.
