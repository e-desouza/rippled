# Cycle 2: xrpld.app ↔ xrpld.overlay

## ⚠️ IMPLEMENTATION STATUS: TIER 2 IN PROGRESS

**Progress:** overlay→app dependencies reduced from 29 to 7 (76% improvement)
**Cycle 4 (app→rpc):** Reduced from 15 to 2 (87% improvement)

### Implementation Phases Summary

| Phase | Description | Deps Before → After |
|-------|-------------|---------------------|
| Initial | Forward declarations, handler pattern setup | 35 → 29 |
| Handler extraction | Message handlers moved to app module | 29 → 17 |
| Tier 1 (COMPLETE) | Interface-based dependency inversion | 17 → 12 |
| Tier 2 (IN PROGRESS) | Medium risk interfaces | 12 → 7 |
| Tier 3 (PLANNED) | Higher complexity | 7 → 0 |

### Tier 1 Commits (Interface-Based Dependency Inversion)

| Commit | Description | Impact |
|--------|-------------|--------|
| `bbd065c7e7` | IFeeTrackOps + LoadFeeTrackAdapter | 17→16 |
| `f2e798ee40` | IHandshakeParams + HandshakeParamsAdapter | 16→14 |
| `246b19a463` | IPeerReservationStorage + PeerReservationStorageAdapter | 14→13 |
| `1a8e49ca30` | getServerPorts() to Application (app→rpc) | 3→2 |
| `a28f6142c7` | IOverlayOps + OverlayOpsHandler | 13→12 |

### Tier 2 Commits (Medium Risk Interfaces)

| Commit | Description | Impact |
|--------|-------------|--------|
| `9d9c892818` | Extend IOverlayOps (ServerCounts, Wallet) | 12→10 |
| `f10f47836b` | Wire IHashRouterOps into OverlayImpl | 10→9 |
| `e8b192e40d` | Create IValidatorOps interface | 9→8 |
| `3f98db1be8` | IOverlayProvider for lazy overlay access | 8→7 |

### Remaining 7 Dependencies (After Tier 2 Step 5)

| File | Include | Usage | Remaining Step |
|------|---------|-------|----------------|
| OverlayImpl.cpp | Application.h | config, journal, validators, manifests | 2.6.3 |
| OverlayImpl.cpp | ValidatorList.h | `listed()`, `getJson()`, `getAvailable()` | Complex |
| PeerImp.cpp | LedgerReplayMsgHandler.h | `processXxx()` methods | 2.6.1 |
| PeerImp.cpp | InboundLedgers.h | `gotLedgerData()` | 2.5.6 |
| PeerImp.cpp | InboundTransactions.h | `gotData()`, `getSet()` | 2.5.6 |
| PeerImp.cpp | LedgerMaster.h | 15 calls, 7 distinct methods | 2.6.2 |
| PeerImp.h | Application.h | `Application& app_` member | Complex |

### Tier 2 Remaining Steps

| Order | Step | Interface/Change | Impact |
|-------|------|-----------------|--------|
| 6 | 2.5.6 | Create LedgerDataMessageHandler | 2 deps |

**Expected result after Tier 2 Step 6:** 7 → 5 dependencies

### Tier 2 Cancelled Steps

| Step | Reason |
|------|--------|
| 2.5.4 | Forward declaring Application in PeerImp.h caused ConnectAttempt.cpp to need it directly, net increasing deps |

### Tier 3 Plan (Higher Complexity)

| Order | Step | Interface/Change | Impact |
|-------|------|-----------------|--------|
| 1 | 2.6.1 | Create ILedgerReplayHandler | 1 dep |
| 2 | 2.6.2 | Create ILedgerDataOps (LedgerMaster) | 1 dep |
| 3 | 2.6.3 | Remove final Application.h (OverlayImpl.cpp) | 1 dep |
| 4 | 2.6.4 | Remove ValidatorList.h (complex - many usages) | 1 dep |
| 5 | 2.6.5 | Forward declare Application in PeerImp.h | 1 dep |

**Expected result after Tier 3:** 5 → 0 dependencies (cycle fully broken)

### Lessons Learned from Tier 1

See `docs/cycles/TIER1_LESSONS_LEARNED.md` for detailed patterns, including:
- **Pattern A:** Interface in overlay, adapter in app
- **Pattern B:** Handler split (header in overlay, impl in app)
- **Pattern C:** Constructor injection via make_Overlay
- **Common pitfalls:** Forgetting jss.h, test file adapter includes, two-phase initialization

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
3. Build: `cd .build && ninja -j$(nproc)`
4. Run tests: `./xrpld --unittest=Overlay`

## Risk Assessment

| Risk                        | Likelihood | Impact | Mitigation           |
| --------------------------- | ---------- | ------ | -------------------- |
| Breaking peer communication | Medium     | High   | Extensive testing    |
| Performance regression      | Low        | Medium | Profile before/after |
| Missing interface methods   | Medium     | Medium | Thorough analysis    |

## Estimated Remaining Effort

**Medium effort (~2 weeks for Tier 2 + Tier 3)** - Interface patterns are now established, remaining work follows proven approach.

## Coordination with Cycle 5 (consensus↔overlay)

The handler and broadcaster changes in this cycle overlap with the `xrpld.consensus ↔ xrpld.overlay` cycle documented in `cycle-5-consensus-overlay.md`. Plan and implement the moves of `ValidationMessageHandler.cpp` and `OverlayBroadcasterImpl.cpp` together so that:

- Interfaces (`IOverlayBroadcaster`, validation handling hooks) remain in `xrpld.consensus`/`xrpld.overlay`.
- Concrete implementations and wiring live under `xrpld.app` (for example `app/overlay/handlers/` and `app/consensus/detail/`).
