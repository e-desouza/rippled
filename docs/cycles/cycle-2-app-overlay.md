# Cycle 2: xrpld.app ↔ xrpld.overlay

## ⚠️ IMPLEMENTATION STATUS: PARTIAL

**Progress:** overlay→app dependencies reduced from 35 to 27 (23% improvement)

### Changes Made:

1. **Commit `97ce488155`:** Forward declare Application in overlay headers
   - Replaced `#include <xrpld/app/main/Application.h>` with forward declaration in `OverlayImpl.h`
   - Replaced `#include <xrpld/app/main/Application.h>` with forward declaration in `Handshake.h`
   - Added missing includes to `Handshake.h` for header self-containment (`base_uint.h`, `IPAddress.h`, `PublicKey.h`)
   - Kept Application.h include in `PeerImp.h` (required for inline template constructor)

2. **Commit `6a77611371`:** Extract HashRouterFlags to shared library
   - Created `include/xrpl/basics/HashRouterFlags.h` with flags enum and operators
   - Updated `PeerImp.h` to include shared header instead of `app/txqueue/HashRouter.h`
   - Reduces overlay→app by 1 (29→28)

3. **Commit `62d7deb388`:** Forward declare Application in PeerSet.h
   - Replaced `#include <xrpld/app/main/Application.h>` with forward declaration
   - Reduces overlay→app by 1 (28→27)

### Remaining Dependencies (27):

The remaining dependencies are deeply integrated:

| File | Includes | Why Difficult |
|------|----------|---------------|
| `PeerImp.h` | 3 app headers | RCLCxPeerPos.h, LedgerReplayMsgHandler.h, Application.h |
| `Handshake.cpp` | `app/ledger/LedgerMaster.h`, `app/main/Application.h` | Implementation needs ledger access |
| `PeerReservationTable.cpp` | `app/rdb/RelationalDatabase.h`, `app/rdb/Wallet.h` | Database access |
| `PeerSet.cpp` | `app/main/Application.h` | Implementation |
| `PeerImp.cpp` | 11 app headers | Message handling, validations, ledger sync |
| `OverlayImpl.cpp` | 8 app headers | Network ops, validators, database |

### Blockers Identified:

1. **LedgerReplayMsgHandler cannot use unique_ptr** - The inline template constructor in
   `PeerImp.h` uses `std::make_unique<LedgerReplayMsgHandler>(...)`, which requires the
   complete type at instantiation. Moving to `unique_ptr` fails with "allocation of incomplete type".

2. **RCLCxPeerPos passed by value** - The `checkPropose()` method takes `RCLCxPeerPos` by value,
   requiring the full type definition in the header.

3. **Application reference deeply embedded** - Multiple overlay classes store `Application&` and
   call methods throughout. Full decoupling requires 6-7 interfaces with 50+ methods total.

### Remaining Work Required:

1. **Create overlay dependency interfaces:**
   - `IOverlayLedgerProvider` - Interface for ledger access
   - `IOverlayTxRouter` - Interface for transaction routing
   - `IOverlayValidationHandler` - Interface for validation handling

2. **Move message handlers to app module:**
   - Move validation handling from PeerImp.cpp to app/overlay/
   - Move transaction handling to app module

3. **Inject dependencies via constructor:**
   - Pass interfaces to Overlay/PeerImp instead of Application reference
   - This allows overlay to depend on interfaces, not app types

### Refined Implementation Plan (2026-01-26)

1. **Classify overlay→app usage by responsibility**
   - For each of `PeerSet`, `PeerImp`, `OverlayImpl`, `Handshake`, and `PeerReservationTable`, record which app subsystems they touch (ledger, txs, validators, DB).
   - Use this to decide which operations belong in ledger ops, tx ops, validation ops, or reservation/DB ops interfaces.

2. **Introduce fine-grained overlay dependency interfaces**
   - Define `IOverlayLedgerOps`, `IOverlayTxOps`, `IOverlayValidationOps`, and `IOverlayReservationOps` under `overlay/`, depending only on xrpl library types (no `Application`).
   - Keep method sets minimal and cohesive, matching the classification from step 1.

3. **Move message handlers to app module**
   - Move validation and transaction message handling code (e.g., from `PeerImp.cpp`) into `app/overlay/handlers/`.
   - Have overlay call these handlers via the new interfaces instead of directly using app types.

4. **Introduce an `OverlayDeps` aggregate and inject it**
   - Create an `OverlayDeps` struct that groups the interfaces overlay needs (ledger, tx, validation, reservation/DB).
   - Change constructors of `PeerImp`, `PeerSet`, `OverlayImpl`, and related classes to take `OverlayDeps` (or references to the individual interfaces) instead of `Application&`.

5. **Implement app-side adapters and wire them in `Application`**
   - In the app module, implement the overlay interfaces by delegating to existing subsystems such as `LedgerMaster`, `NetworkOPs`, validators, and the relational database.
   - Construct concrete implementations during `Application` startup and pass them to overlay when building `OverlayImpl`.

6. **Clean up includes and re-run levelization**
   - Remove direct includes of app headers from overlay sources where calls now go through the interfaces.
   - Re-run the levelization tool to confirm that the `xrpld.app → xrpld.overlay` cycle is removed.

---

## Original State (Before Fixes)

**Loop detected:** `xrpld.overlay > xrpld.app`

The overlay module had 35 includes from app, and app had 23 includes from overlay.

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
namespace xrpl {

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

} // namespace xrpl
```

> Note (2026-01-26): The actual implementation will use interfaces named
> `IOverlayLedgerOps`, `IOverlayTxOps`, `IOverlayValidationOps`, and
> `IOverlayReservationOps`, grouped via an `OverlayDeps` aggregate, as
> described in the Refined Implementation Plan above.

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
Move them to `app/overlay/handlers/` where they can freely include app headers, and coordinate this move with Cycle 5 (consensus↔overlay) so validation/broadcasting responsibilities end up in app.

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

## Coordination with Cycle 5 (consensus↔overlay)

The handler and broadcaster changes in this cycle overlap with the `xrpld.consensus ↔ xrpld.overlay` cycle documented in `cycle-5-consensus-overlay.md`. Plan and implement the moves of `ValidationMessageHandler.cpp` and `OverlayBroadcasterImpl.cpp` together so that:

- Interfaces (`IOverlayBroadcaster`, validation handling hooks) remain in `xrpld.consensus`/`xrpld.overlay`.
- Concrete implementations and wiring live under `xrpld.app` (for example `app/overlay/handlers/` and `app/consensus/detail/`).
