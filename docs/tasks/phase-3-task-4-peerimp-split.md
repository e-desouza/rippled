# Phase 3 Task 3.4: Split PeerImp Class into Focused Components

## Overview

### Problem Statement

The `PeerImp` class (`src/xrpld/overlay/detail/PeerImp.h/cpp`) has grown into a **monolithic 4500+ line class** that handles multiple distinct responsibilities:

- **Connection Management**: Stream, socket, strand, and SSL handling
- **Timer Management**: Activity monitoring, shutdown timeouts, ping/pong
- **Message I/O**: Reading, writing, compression, send queue management
- **Protocol Dispatch**: 21+ `onMessage()` handler overloads
- **Ledger/TxSet Tracking**: Recent ledgers, tx sets, tracking state
- **Shutdown State Machine**: Complex multi-stage graceful shutdown
- **Squelching & Load Management**: Resource charging, fee tracking

This creates several problems:

1. **High complexity**: 4500+ lines makes the class difficult to understand and maintain
2. **Mixed responsibilities**: Network I/O, protocol logic, and tracking state are interleaved
3. **Testing difficulty**: Cannot unit test message handlers independently from connection logic
4. **Thread safety complexity**: Multiple mutexes protect different subsets of members
5. **Change risk**: Modifications to one concern may inadvertently affect others

### Why Phase 3?

This task depends on Phase 2 work because:
1. **Overlay abstraction** (Phase 2 Task 2.2): PeerImp tightly couples to OverlayImpl
2. **Interface boundaries**: Cleaner interfaces make it easier to extract components
3. **Test infrastructure**: Phase 2 testing improvements enable component-level testing

### Success Criteria

1. ✅ PeerImp reduced to ~500 lines as a thin coordinator class
2. ✅ New focused components created with clear single responsibilities
3. ✅ All include paths updated throughout codebase
4. ✅ Thread safety maintained (strand serialization preserved)
5. ✅ Shutdown state machine functions correctly
6. ✅ Build compiles successfully
7. ✅ All tests pass
8. ✅ No performance regression in message handling

---

## Deep Code Analysis

### Current Class Structure (PeerImp.h: 909 lines, PeerImp.cpp: 3651 lines)

#### Base Classes & Inheritance

```cpp
class PeerImp : public Peer,
                public std::enable_shared_from_this<PeerImp>,
                public OverlayImpl::Child
```

- **Peer**: Public interface (125 lines) - defines virtual methods for peer operations
- **OverlayImpl::Child**: Integrates with overlay lifecycle management
- **enable_shared_from_this**: Required for async callback safety

#### Member Variable Groups

| Group | Members | Lines | Purpose |
|-------|---------|-------|---------|
| **Connection** | `stream_ptr_`, `socket_`, `stream_`, `strand_`, `timer_` | 116-131 | Network I/O primitives |
| **Identity** | `id_`, `publicKey_`, `name_`, `fingerprint_`, `prefix_`, `remote_address_` | 117-149 | Peer identification |
| **Protocol** | `protocol_`, `headers_`, `compressionEnabled_`, `request_`, `response_` | 142-244 | Protocol negotiation state |
| **Tracking** | `tracking_`, `trackingTime_`, `minLedger_`, `maxLedger_`, `closedLedgerHash_`, `previousLedgerHash_` | 144-157 | Ledger tracking state |
| **Buffers** | `read_buffer_`, `send_queue_`, `txQueue_` | 220-249 | I/O buffers and queues |
| **State Flags** | `shutdown_`, `shutdownStarted_`, `readPending_`, `writePending_` | 227-234 | Async operation state |
| **Metrics** | `metrics_.sent`, `metrics_.recv` | 258-289 | Traffic statistics |
| **Features** | `txReduceRelayEnabled_`, `ledgerReplayEnabled_`, `ledgerReplayMsgHandler_` | 251-254 | Feature toggles |

#### Message Handler Methods (21 handlers)

| Handler | Lines in .cpp | Category | Complexity |
|---------|---------------|----------|------------|
| `onMessage(TMManifests)` | 1147-1164 | Cluster | Low |
| `onMessage(TMPing)` | 1166-1200 | Cluster | Low |
| `onMessage(TMCluster)` | 1203-1273 | Cluster | Medium |
| `onMessage(TMEndpoints)` | 1275-1330 | PeerFinder | Medium |
| `onMessage(TMTransaction)` | 1332-1473 | Transactions | High |
| `onMessage(TMGetLedger)` | 1475-1562 | Ledger | High |
| `onMessage(TMLedgerData)` | 1669-1758 | Ledger | High |
| `onMessage(TMProposeSet)` | 1760-1879 | Consensus | High |
| `onMessage(TMStatusChange)` | 1881-2054 | Status | High |
| `onMessage(TMHaveTransactionSet)` | 2097-2121 | TxSet | Low |
| `onMessage(TMValidatorList)` | 2332-2358 | Validators | Medium |
| `onMessage(TMValidatorListCollection)` | 2360-2398 | Validators | Medium |
| `onMessage(TMValidation)` | 2400-2512 | Consensus | High |
| `onMessage(TMGetObjectByHash)` | 2514-2677 | Objects | High |
| `onMessage(TMHaveTransactions)` | 2679-2695 | Transactions | Low |
| `onMessage(TMTransactions)` | 2747-2769 | Transactions | Medium |
| `onMessage(TMSquelch)` | 2771-2813 | Squelch | Medium |
| `onMessage(TMProofPathRequest)` | 1564-1599 | Replay | Medium |
| `onMessage(TMProofPathResponse)` | 1601-1615 | Replay | Low |
| `onMessage(TMReplayDeltaRequest)` | 1617-1651 | Replay | Medium |
| `onMessage(TMReplayDeltaResponse)` | 1653-1667 | Replay | Low |

#### Shutdown State Machine (Lines 599-688)

```
Normal Operation → shutdown() → tryAsyncShutdown() → onShutdown() → close()
                      ↓              ↓                 ↓              ↓
                 Set shutdown_   SSL graceful      Timer cancel   Socket close
                 Cancel timer    shutdown start    & cleanup      & metrics
                 5s safety timer Set shutdownStarted_              update
```

Key coordination flags:
- `shutdown_`: Primary shutdown requested flag
- `shutdownStarted_`: SSL shutdown initiated
- `readPending_`: Read operation in flight
- `writePending_`: Write operation in flight

#### Internal Dependencies

| Component | Depends On |
|-----------|------------|
| `onReadMessage` | `invokeProtocolMessage()`, all message handlers |
| `onWriteMessage` | `send_queue_`, compression |
| `shutdown()` | Timer, socket, `tryAsyncShutdown()` |
| `tryAsyncShutdown()` | `shutdown_`, `readPending_`, `writePending_` |
| Message handlers | `app_`, `overlay_`, tracking state |

### Files Requiring Changes

| File | Lines | Changes Required |
|------|-------|------------------|
| `src/xrpld/overlay/detail/PeerImp.h` | 909 | Split into multiple headers |
| `src/xrpld/overlay/detail/PeerImp.cpp` | 3651 | Split into multiple sources |
| `src/xrpld/overlay/detail/OverlayImpl.h` | ~400 | Update PeerImp references |
| `src/xrpld/overlay/detail/OverlayImpl.cpp` | ~1200 | Update PeerImp usage |
| `src/xrpld/overlay/detail/ConnectAttempt.cpp` | ~620 | Update PeerImp construction |
| `src/xrpld/overlay/detail/ProtocolMessage.h` | 471 | May need handler interface |
| `src/xrpld/overlay/Peer.h` | 125 | Stable (interface) |

---

## Design Considerations

### Option A: Extract Helper Classes (Minimal Refactor)

Extract data-only structures and helper methods while keeping PeerImp as the main class.

**New Classes:**
- `PeerConnectionState` - Connection member variables only
- `PeerTrackingState` - Tracking member variables only
- `PeerMetrics` (already exists) - Traffic statistics

**Pros:**
- Minimal code changes
- No behavioral changes
- Low risk

**Cons:**
- Doesn't address fundamental complexity
- PeerImp still has 20+ message handlers
- Testing difficulty remains

### Option B: Component Extraction (RECOMMENDED)

Split PeerImp into distinct component classes, each owning a clear subset of functionality.

**New Classes:**

1. **PeerConnection** (~400 lines)
   - Owns: `stream_ptr_`, `socket_`, `stream_`, `strand_`, `timer_`
   - Methods: `connect()`, `shutdown()`, `tryAsyncShutdown()`, `onShutdown()`, `close()`
   - Handles all async I/O completion handlers

2. **PeerProtocol** (~300 lines)
   - Owns: `send_queue_`, `txQueue_`, `read_buffer_`, compression state
   - Methods: `send()`, `sendq()`, `onWriteMessage()`, `onReadMessage()`
   - Handles message framing, compression, queue management

3. **PeerMessageRouter** (~150 lines)
   - Dispatches parsed messages to appropriate handlers
   - Replaces current `invokeProtocolMessage()` call in `onReadMessage()`
   - Template-based dispatch with compile-time handler registration

4. **PeerMessageHandlers** (~1500 lines, can be further split)
   - All 21 `onMessage()` overloads
   - Grouped by category: Cluster, Transactions, Ledger, Consensus, Validators
   - Each handler receives context object with app references

5. **PeerTracker** (~200 lines)
   - Owns: `tracking_`, `trackingTime_`, `minLedger_`, `maxLedger_`, ledger hashes
   - Owns: `recentLedgers_`, `recentTxSets_`, `txReduceRelayEnabled_`
   - Methods: Ledger/TxSet tracking, squelch state

**Pros:**
- Clear separation of concerns
- Each component independently testable
- Reduced cognitive load per file
- Matches existing patterns in codebase

**Cons:**
- Moderate refactoring effort
- Need to carefully manage component interactions
- May require friend declarations or accessor methods

### Option C: Full Interface-Based Split

Define abstract interfaces for each component, with PeerImp holding interface pointers.

**Interfaces:**
- `IPeerConnection` - Connection lifecycle
- `IPeerMessageSender` - Message sending
- `IPeerMessageReceiver` - Message receiving
- `IPeerTracker` - Tracking state

**Pros:**
- Maximum testability with mock implementations
- Clean dependency injection
- Future-proof architecture

**Cons:**
- Significant overhead for interface indirection
- May impact performance for hot paths
- Over-engineering for internal implementation detail
- More files to maintain

### Recommendation: Option B with Phased Approach

**Rationale:**
1. Balances complexity reduction with implementation effort
2. Maintains performance (no virtual dispatch overhead in hot paths)
3. Components can be extracted incrementally
4. Each step maintains working build

**Key Design Decisions:**

1. **Strand ownership**: PeerConnection owns the strand; other components post to it
2. **Shared state**: Use `std::shared_ptr` where components need shared access
3. **Callback pattern**: Components use callbacks for cross-component communication
4. **Header organization**: One header per component in `overlay/detail/`

---

## Thread Safety Analysis

### Current Strand Usage Pattern

All PeerImp async operations serialize through a single strand:

```cpp
boost::asio::strand<boost::asio::executor> strand_;
```

**Current Pattern:**
```cpp
void PeerImp::send(std::shared_ptr<Message> const& m)
{
    // Not on strand - must post
    boost::asio::post(
        strand_,
        [self = shared_from_this(), m]() { self->sendq(m); });
}

void PeerImp::sendq(std::shared_ptr<Message> const& m)
{
    // On strand - direct access to send_queue_ safe
    send_queue_.push_back(m);
    // ...
}
```

### Component Thread Safety Design

**PeerConnection (strand owner):**
- Owns the strand
- All I/O completion handlers execute on strand
- Provides `post()` method for other components

**PeerProtocol:**
- All public methods post to strand
- Internal methods assume strand context
- Send queue accessed only on strand

**PeerMessageHandlers:**
- Handlers always called on strand (from `onReadMessage`)
- Can access shared state directly
- Outbound sends go through PeerProtocol

**PeerTracker:**
- Maintains its own mutex for `recentLedgers_` and `recentTxSets_`
- Read operations may be off-strand (for queries)
- Modifications always on strand

### Mutex Inventory

| Mutex | Current Location | New Owner | Access Pattern |
|-------|------------------|-----------|----------------|
| `recentLock_` | PeerImp | PeerTracker | Guards `recentLedgers_`, `recentTxSets_` |
| `nameMutex_` (if any) | PeerImp | PeerIdentity | Guards peer name updates |
| Implicit strand | PeerImp | PeerConnection | All async operations |

---

## Performance Implications

### Hot Paths

1. **Message receive path** (most critical):
   ```
   async_read → onReadMessage → invokeProtocolMessage → onMessage(X)
   ```
   - Must remain fast - avoid virtual dispatch in this path
   - Component extraction via composition, not inheritance

2. **Message send path**:
   ```
   send() → post to strand → sendq() → async_write
   ```
   - Single strand post is acceptable (already present)

3. **Tracking queries** (off hot path):
   ```
   hasLedger(hash) → lock → search recent ledgers
   ```
   - Already uses mutex, no change needed

### Performance Considerations

| Concern | Mitigation |
|---------|------------|
| Extra function calls | Inline accessors, compiler optimization |
| Pointer chasing | Keep components close in memory (single allocation) |
| Virtual dispatch | Avoid - use templates and composition |
| Cache locality | Group hot data together in each component |

### Benchmarking Requirements

- [ ] Message throughput: before/after split comparison
- [ ] Latency: message receive to handler completion
- [ ] Memory: per-peer memory footprint
- [ ] Connection setup: time to establish peer connection

---

## Backward Compatibility

### Public API (Peer.h)

The `Peer` interface in `src/xrpld/overlay/Peer.h` defines the public API:

```cpp
class Peer
{
public:
    virtual void send(std::shared_ptr<Message> const& m) = 0;
    virtual beast::IP::Endpoint getRemoteAddress() const = 0;
    virtual PublicKey const& getNodePublic() const = 0;
    // ... other virtual methods
};
```

**Compatibility guarantee**: All methods in `Peer` must remain stable.

### Internal API Changes

| Change | Impact |
|--------|--------|
| PeerImp member access | Internal only - OverlayImpl, ConnectAttempt |
| Message handler signatures | Internal only - ProtocolMessage.h |
| Construction pattern | Internal only - OverlayImpl::onHandoff |

### Wire Protocol

**No changes to wire protocol** - this is purely internal refactoring.

---

## Implementation Plan

### Prerequisites

- [ ] Phase 2 Task 2.2 (Overlay abstraction) complete
- [ ] Clean build on develop branch
- [ ] All existing tests passing

### Step 1: Create PeerTracker Component (Lowest Risk)

**Goal:** Extract ledger/txset tracking into standalone component.

**Files to create:**
- `src/xrpld/overlay/detail/PeerTracker.h` (~100 lines)
- `src/xrpld/overlay/detail/PeerTracker.cpp` (~100 lines)

**Members to move from PeerImp:**
```cpp
// Tracking state
Tracking tracking_ = Tracking::unknown;
clock_type::time_point trackingTime_;
std::uint32_t minLedger_ = 0;
std::uint32_t maxLedger_ = 0;
uint256 closedLedgerHash_;
uint256 previousLedgerHash_;

// Recent items with mutex
std::mutex recentLock_;
std::map<uint256, RecentsEntry> recentLedgers_;
std::map<uint256, RecentsEntry> recentTxSets_;

// Squelching
bool txReduceRelayEnabled_ = false;
bool ledgerReplayEnabled_ = false;
```

**Methods to move:**
```cpp
void ledgerRange(std::uint32_t& minSeq, std::uint32_t& maxSeq) const;
bool hasLedger(uint256 const& hash, std::uint32_t seq) const;
void ledgerRange(std::uint32_t& minSeq, std::uint32_t& maxSeq) const;
bool hasTxSet(uint256 const& hash) const;
void addTxSet(uint256 const& hash);
Tracking tracking() const;
void setTracking(Tracking t);
// ... tracking-related methods
```

**Validation:**
- [ ] PeerImp compiles with PeerTracker member
- [ ] All tracking tests pass
- [ ] Overlay unit tests pass

**Estimated time:** 2 hours

### Step 2: Create PeerMetrics Component (Already Exists)

**Goal:** Formalize and extend existing metrics structure.

The `Metrics` struct already exists in PeerImp:

```cpp
struct Metrics
{
    TrafficCount sent;
    TrafficCount recv;
};
Metrics metrics_;
```

**Enhancements:**
- Move to separate header
- Add helper methods for statistics
- Consider thread-safe counters (atomic)

**Estimated time:** 30 minutes

### Step 3: Create PeerConnection Component

**Goal:** Extract connection lifecycle management.

**Files to create:**
- `src/xrpld/overlay/detail/PeerConnection.h` (~200 lines)
- `src/xrpld/overlay/detail/PeerConnection.cpp` (~300 lines)

**Members to move from PeerImp:**
```cpp
// Connection primitives
std::unique_ptr<stream_type> stream_ptr_;
socket_type& socket_;
stream_type& stream_;
boost::asio::strand<boost::asio::executor> strand_;
waitable_timer timer_;

// Shutdown state
std::atomic_bool shutdown_{false};
bool shutdownStarted_ = false;
bool readPending_ = false;
bool writePending_ = false;
```

**Methods to move:**
```cpp
void shutdown();
void tryAsyncShutdown();
void onShutdown(error_code ec);
void close();
void fail(std::string const& reason);
void setTimer();
void cancelTimer();
// ... timer callbacks
```

**Key challenge:** Shutdown coordination with other components.

**Solution:** Use callback pattern:
```cpp
class PeerConnection {
    std::function<void()> onShutdownComplete_;
public:
    void setShutdownCallback(std::function<void()> cb);
};
```

**Validation:**
- [ ] Connection establishment works
- [ ] Graceful shutdown works
- [ ] Timer-based ping/pong works
- [ ] Error handling works

**Estimated time:** 4 hours

### Step 4: Create PeerProtocol Component

**Goal:** Extract message framing and queue management.

**Files to create:**
- `src/xrpld/overlay/detail/PeerProtocol.h` (~150 lines)
- `src/xrpld/overlay/detail/PeerProtocol.cpp` (~200 lines)

**Members to move from PeerImp:**
```cpp
// Buffers
boost::beast::multi_buffer read_buffer_;
std::list<SendItem> send_queue_;
std::list<std::shared_ptr<STTx const>> txQueue_;

// Protocol state
Compressed compressionEnabled_ = Compressed::Off;
bool vpReduceRelayEnabled_ = false;
```

**Methods to move:**
```cpp
void send(std::shared_ptr<Message> const& m);
void sendq(std::shared_ptr<Message> const& m);
void onWriteMessage(error_code ec, std::size_t bytes_transferred);
void onReadMessage(error_code ec, std::size_t bytes_transferred);
// ... queue management methods
```

**Interface with PeerConnection:**
```cpp
class PeerProtocol {
    PeerConnection& connection_;  // For strand access and I/O
public:
    PeerProtocol(PeerConnection& conn);
};
```

**Validation:**
- [ ] Message sending works
- [ ] Message receiving works
- [ ] Compression works
- [ ] Queue management works

**Estimated time:** 3 hours

### Step 5: Create PeerMessageRouter Component

**Goal:** Centralize message dispatch logic.

**Files to create:**
- `src/xrpld/overlay/detail/PeerMessageRouter.h` (~100 lines)
- `src/xrpld/overlay/detail/PeerMessageRouter.cpp` (~50 lines)

**Design:**
```cpp
class PeerMessageRouter {
    PeerMessageHandlers& handlers_;
public:
    void dispatch(
        std::shared_ptr<protocol::TMPing> const& m) { handlers_.onMessage(m); }
    void dispatch(
        std::shared_ptr<protocol::TMCluster> const& m) { handlers_.onMessage(m); }
    // ... template for each message type
};
```

**Integration with ProtocolMessage.h:**

The existing `invokeProtocolMessage()` in `ProtocolMessage.h` will call into the router:

```cpp
// Current pattern
invokeProtocolMessage(buffers, peer, ...);  // calls peer.onMessage(X)

// New pattern
invokeProtocolMessage(buffers, peer.router(), ...);  // calls router.dispatch(X)
```

**Validation:**
- [ ] All 21 message types dispatch correctly
- [ ] Error messages handled properly
- [ ] Unknown message types logged

**Estimated time:** 2 hours

### Step 6: Extract Message Handlers by Category

**Goal:** Split message handlers into logical groups.

**Files to create:**
- `src/xrpld/overlay/detail/handlers/ClusterHandlers.h/cpp` (~200 lines)
- `src/xrpld/overlay/detail/handlers/TransactionHandlers.h/cpp` (~400 lines)
- `src/xrpld/overlay/detail/handlers/LedgerHandlers.h/cpp` (~400 lines)
- `src/xrpld/overlay/detail/handlers/ConsensusHandlers.h/cpp` (~400 lines)
- `src/xrpld/overlay/detail/handlers/ValidatorHandlers.h/cpp` (~150 lines)
- `src/xrpld/overlay/detail/handlers/ReplayHandlers.h/cpp` (~150 lines)

**Handler distribution:**

| Category | Handlers | Lines |
|----------|----------|-------|
| Cluster | TMManifests, TMPing, TMCluster | ~130 |
| PeerFinder | TMEndpoints | ~60 |
| Transactions | TMTransaction, TMHaveTransactions, TMTransactions | ~200 |
| Ledger | TMGetLedger, TMLedgerData, TMHaveTransactionSet | ~250 |
| Consensus | TMProposeSet, TMStatusChange, TMValidation | ~370 |
| Validators | TMValidatorList, TMValidatorListCollection | ~70 |
| Objects | TMGetObjectByHash | ~170 |
| Replay | TMProofPath*, TMReplayDelta* | ~120 |
| Squelch | TMSquelch | ~50 |

**Handler Context Pattern:**
```cpp
struct HandlerContext {
    Application& app;
    OverlayImpl& overlay;
    PeerTracker& tracker;
    std::shared_ptr<PeerImp> peer;
    // ... other references needed by handlers
};

class TransactionHandlers {
public:
    void onMessage(HandlerContext& ctx, std::shared_ptr<protocol::TMTransaction> const& m);
    // ...
};
```

**Validation (per category):**
- [ ] Cluster tests pass
- [ ] Transaction relay tests pass
- [ ] Ledger fetch tests pass
- [ ] Consensus tests pass
- [ ] Validator list tests pass

**Estimated time:** 6 hours

### Step 7: Refactor PeerImp as Coordinator

**Goal:** Reduce PeerImp to thin coordinator owning components.

**Final PeerImp structure (~400 lines):**
```cpp
class PeerImp : public Peer,
                public std::enable_shared_from_this<PeerImp>,
                public OverlayImpl::Child
{
    // Identity (stays in PeerImp)
    id_t const id_;
    PublicKey const publicKey_;
    std::string name_;

    // Components
    PeerConnection connection_;
    PeerProtocol protocol_;
    PeerTracker tracker_;
    PeerMessageRouter router_;
    PeerMessageHandlers handlers_;

public:
    // Peer interface implementation - delegates to components
    void send(std::shared_ptr<Message> const& m) override
    {
        protocol_.send(m);
    }

    // ... other delegating methods
};
```

**Validation:**
- [ ] All Peer interface methods work
- [ ] Connection lifecycle works
- [ ] All message handlers work
- [ ] Shutdown works correctly

**Estimated time:** 4 hours

### Step 8: Update External Consumers

**Files to update:**

| File | Changes |
|------|---------|
| `OverlayImpl.cpp` | Update PeerImp construction, child management |
| `ConnectAttempt.cpp` | Update PeerImp creation |
| `ProtocolMessage.h` | Update handler dispatch |

**Estimated time:** 2 hours

### Step 9: Update CMakeLists.txt

**Add new source files:**
```cmake
target_sources(xrpld PRIVATE
    # PeerImp components
    src/xrpld/overlay/detail/PeerConnection.h
    src/xrpld/overlay/detail/PeerConnection.cpp
    src/xrpld/overlay/detail/PeerProtocol.h
    src/xrpld/overlay/detail/PeerProtocol.cpp
    src/xrpld/overlay/detail/PeerTracker.h
    src/xrpld/overlay/detail/PeerTracker.cpp
    src/xrpld/overlay/detail/PeerMessageRouter.h
    src/xrpld/overlay/detail/PeerMessageRouter.cpp

    # Message handlers
    src/xrpld/overlay/detail/handlers/ClusterHandlers.h
    src/xrpld/overlay/detail/handlers/ClusterHandlers.cpp
    src/xrpld/overlay/detail/handlers/TransactionHandlers.h
    src/xrpld/overlay/detail/handlers/TransactionHandlers.cpp
    src/xrpld/overlay/detail/handlers/LedgerHandlers.h
    src/xrpld/overlay/detail/handlers/LedgerHandlers.cpp
    src/xrpld/overlay/detail/handlers/ConsensusHandlers.h
    src/xrpld/overlay/detail/handlers/ConsensusHandlers.cpp
    src/xrpld/overlay/detail/handlers/ValidatorHandlers.h
    src/xrpld/overlay/detail/handlers/ValidatorHandlers.cpp
    src/xrpld/overlay/detail/handlers/ReplayHandlers.h
    src/xrpld/overlay/detail/handlers/ReplayHandlers.cpp
)
```

**Estimated time:** 30 minutes

---

## Complete File Mapping Table

### New Component Files (14 files)

| # | New File | Lines | Purpose |
|---|----------|-------|---------|
| 1 | `overlay/detail/PeerConnection.h` | ~200 | Connection lifecycle interface |
| 2 | `overlay/detail/PeerConnection.cpp` | ~300 | Connection implementation |
| 3 | `overlay/detail/PeerProtocol.h` | ~150 | Message I/O interface |
| 4 | `overlay/detail/PeerProtocol.cpp` | ~200 | Message I/O implementation |
| 5 | `overlay/detail/PeerTracker.h` | ~100 | Tracking state interface |
| 6 | `overlay/detail/PeerTracker.cpp` | ~100 | Tracking implementation |
| 7 | `overlay/detail/PeerMessageRouter.h` | ~100 | Message dispatch interface |
| 8 | `overlay/detail/PeerMessageRouter.cpp` | ~50 | Router implementation |
| 9 | `overlay/detail/handlers/ClusterHandlers.h` | ~50 | Cluster handler interface |
| 10 | `overlay/detail/handlers/ClusterHandlers.cpp` | ~150 | Cluster handlers |
| 11 | `overlay/detail/handlers/TransactionHandlers.h` | ~50 | Transaction handler interface |
| 12 | `overlay/detail/handlers/TransactionHandlers.cpp` | ~350 | Transaction handlers |
| 13 | `overlay/detail/handlers/LedgerHandlers.h` | ~50 | Ledger handler interface |
| 14 | `overlay/detail/handlers/LedgerHandlers.cpp` | ~350 | Ledger handlers |
| 15 | `overlay/detail/handlers/ConsensusHandlers.h` | ~50 | Consensus handler interface |
| 16 | `overlay/detail/handlers/ConsensusHandlers.cpp` | ~400 | Consensus handlers |
| 17 | `overlay/detail/handlers/ValidatorHandlers.h` | ~30 | Validator handler interface |
| 18 | `overlay/detail/handlers/ValidatorHandlers.cpp` | ~120 | Validator handlers |
| 19 | `overlay/detail/handlers/ReplayHandlers.h` | ~30 | Replay handler interface |
| 20 | `overlay/detail/handlers/ReplayHandlers.cpp` | ~120 | Replay handlers |

### Modified Files (7 files)

| # | File | Current Lines | New Lines | Changes |
|---|------|---------------|-----------|---------|
| 1 | `PeerImp.h` | 909 | ~300 | Remove moved members |
| 2 | `PeerImp.cpp` | 3651 | ~400 | Remove moved methods |
| 3 | `OverlayImpl.h` | ~400 | ~400 | Minor updates |
| 4 | `OverlayImpl.cpp` | ~1200 | ~1200 | Construction updates |
| 5 | `ConnectAttempt.cpp` | ~620 | ~620 | Construction updates |
| 6 | `ProtocolMessage.h` | 471 | ~480 | Router integration |
| 7 | `CMakeLists.txt` | varies | varies | Add new sources |

---

## Risk Assessment

### High Risk

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| **Shutdown race conditions** | Medium | High | Extensive testing, code review by domain expert |
| **Message loss during refactor** | Low | High | Comprehensive integration tests, traffic comparison |
| **Build breaks during migration** | Medium | Medium | Incremental steps, CI on each step |

### Medium Risk

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| **Performance regression** | Medium | Medium | Benchmark before/after, optimize hot paths |
| **Thread safety bugs** | Medium | High | Static analysis, thread sanitizer testing |
| **Merge conflicts** | Medium | Low | Coordinate with team, merge during quiet period |
| **Handler dispatch errors** | Low | High | Type-safe dispatch, compile-time verification |

### Low Risk

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| **IDE/tooling issues** | Low | Low | Communicate changes to team |
| **Documentation outdated** | Medium | Low | Update docs as part of PR |
| **Header guard conflicts** | Low | Low | Use `#pragma once` |

### Rollback Strategy

1. **Per-step rollback**: Each step is a separate commit, can revert individually
2. **Full rollback**: Revert all commits in the branch
3. **Feature flag** (optional): Add compile-time flag to use old vs new code

---

## Validation Criteria

### Build Validation

- [ ] `cmake --build build` completes without errors
- [ ] `cmake --build build --target rippled` succeeds
- [ ] No new compiler warnings introduced
- [ ] Static analysis passes (if configured)

### Unit Test Validation

- [ ] All existing overlay tests pass
- [ ] `Peer_test` passes
- [ ] `PeerFinder_test` passes
- [ ] `Overlay_test` passes

### Integration Test Validation

- [ ] Connection establishment: 100+ peer connections
- [ ] Message throughput: 10,000+ messages/second sustained
- [ ] Graceful shutdown: All connections close cleanly
- [ ] Error recovery: Handles network errors correctly

### Component-Level Testing (New)

- [ ] `PeerConnection_test`: Connection lifecycle, shutdown
- [ ] `PeerProtocol_test`: Send/receive, compression
- [ ] `PeerTracker_test`: Ledger/TxSet tracking
- [ ] `PeerMessageRouter_test`: Message dispatch
- [ ] Handler tests per category

### Performance Validation

- [ ] Message latency: No increase >5%
- [ ] Memory per peer: No increase >10%
- [ ] Connection setup time: No increase >10%
- [ ] CPU usage under load: No increase >5%

### Thread Safety Validation

- [ ] ThreadSanitizer: No race conditions detected
- [ ] Helgrind: No lock order issues
- [ ] Stress test: 24-hour sustained load

### Code Quality

- [ ] Each component <500 lines
- [ ] PeerImp.cpp <500 lines (down from 3651)
- [ ] No circular dependencies between components
- [ ] Levelization check passes

---

## Estimated Total Time

| Step | Time | Cumulative |
|------|------|------------|
| Step 1: PeerTracker | 2 hours | 2 hours |
| Step 2: PeerMetrics | 30 min | 2.5 hours |
| Step 3: PeerConnection | 4 hours | 6.5 hours |
| Step 4: PeerProtocol | 3 hours | 9.5 hours |
| Step 5: PeerMessageRouter | 2 hours | 11.5 hours |
| Step 6: Message Handlers | 6 hours | 17.5 hours |
| Step 7: PeerImp Coordinator | 4 hours | 21.5 hours |
| Step 8: External Consumers | 2 hours | 23.5 hours |
| Step 9: CMakeLists.txt | 30 min | 24 hours |
| Testing & Validation | 8 hours | 32 hours |
| **Total** | **~32 hours** | (~4 days) |

---

## Dependency Graph

```
                    ┌─────────────┐
                    │  PeerImp    │ (coordinator)
                    │  ~400 lines │
                    └──────┬──────┘
                           │ owns
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
┌───────────────┐  ┌───────────────┐  ┌───────────────┐
│PeerConnection │  │ PeerProtocol  │  │  PeerTracker  │
│   ~500 lines  │  │   ~350 lines  │  │   ~200 lines  │
└───────┬───────┘  └───────┬───────┘  └───────────────┘
        │                  │
        │ uses strand      │ uses connection
        └──────────────────┘
                │
                ▼
        ┌───────────────┐
        │PeerMessage    │
        │Router ~150    │
        └───────┬───────┘
                │ dispatches to
                ▼
        ┌───────────────┐
        │Message Handler│
        │  Categories   │
        │  ~1500 lines  │
        └───────────────┘
```

---

## Future Work (Out of Scope)

1. **Handler Plugins**: Allow runtime registration of message handlers
2. **Connection Pooling**: Reuse connections for multiple peers
3. **Protocol Versioning**: Version-specific handler dispatch
4. **Metrics Dashboard**: Real-time per-component metrics
5. **Mock Components**: Full mock implementations for testing

---

## Appendix A: Current PeerImp Method Index

### Connection Methods (→ PeerConnection)

| Method | Lines | Purpose |
|--------|-------|---------|
| `PeerImp()` constructor | 62-115 | Initialize connection |
| `~PeerImp()` | N/A | Cleanup |
| `run()` | 205-248 | Start async operations |
| `shutdown()` | 599-620 | Initiate shutdown |
| `tryAsyncShutdown()` | 622-657 | SSL shutdown |
| `onShutdown()` | 659-670 | Shutdown completion |
| `close()` | 672-688 | Final cleanup |
| `fail()` | 553-597 | Error handling |
| `setTimer()` | 692-720 | Activity timer |
| `cancelTimer()` | 722-730 | Cancel timer |
| `onTimer()` | 732-799 | Timer callback |

### Protocol Methods (→ PeerProtocol)

| Method | Lines | Purpose |
|--------|-------|---------|
| `send()` | 429-445 | Queue message |
| `sendq()` | 447-492 | Send queued message |
| `onWriteMessage()` | 494-551 | Write completion |
| `onReadMessage()` | 250-427 | Read completion |

### Tracking Methods (→ PeerTracker)

| Method | Lines | Purpose |
|--------|-------|---------|
| `ledgerRange()` | 801-810 | Get ledger range |
| `hasLedger()` | 812-835 | Check ledger availability |
| `hasTxSet()` | 837-855 | Check txset availability |
| `addTxSet()` | 857-870 | Add txset to recent |
| `tracking()` | Inline | Get tracking state |
| `setTracking()` | 872-895 | Update tracking |

### Message Handlers (→ PeerMessageHandlers)

See "Message Handler Methods" table in Deep Code Analysis section.

---

## Appendix B: Test Files to Update

| Test File | Changes Required |
|-----------|------------------|
| `src/test/overlay/Peer_test.cpp` | Update for component APIs |
| `src/test/overlay/PeerFinder_test.cpp` | Verify still works |
| `src/test/overlay/Overlay_test.cpp` | Update construction |
| `src/test/overlay/reduce_relay_test.cpp` | Update handler testing |
| `src/test/overlay/message_test.cpp` | Update message dispatch |

### New Test Files to Create

| Test File | Purpose |
|-----------|---------|
| `src/test/overlay/PeerConnection_test.cpp` | Connection lifecycle |
| `src/test/overlay/PeerProtocol_test.cpp` | Message I/O |
| `src/test/overlay/PeerTracker_test.cpp` | Tracking state |
| `src/test/overlay/PeerRouter_test.cpp` | Message dispatch |

