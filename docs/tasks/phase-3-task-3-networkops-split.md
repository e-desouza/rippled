# Phase 3 Task 3.3: Split NetworkOPs God Object

## Overview

### Problem Statement

`NetworkOPs` is a **god object** with ~4900 lines handling fundamentally unrelated concerns:
- Network state and operating mode management
- Transaction submission and batch processing
- Consensus coordination and lifecycle
- Pub/Sub subscription management (~25 different subscription types)
- Server information and monitoring
- Book/offer page queries
- Owner info queries

This monolithic design creates:
1. **Violation of Single Responsibility**: One class handles 6+ unrelated domains
2. **Testing complexity**: Cannot unit test concerns in isolation
3. **Thread safety complexity**: Multiple locks protecting unrelated state (`mSubLock`, `mMutex`, `validationsMutex_`)
4. **Compilation coupling**: Any change to any feature requires recompiling 4900 lines
5. **Hidden dependencies**: RCLConsensus, LedgerMaster, Application all tightly coupled
6. **Difficulty adding features**: Understanding impact of changes requires understanding entire class

### Current Interface Analysis

**File**: `src/xrpld/app/misc/NetworkOPs.h` (281 lines)

```cpp
class NetworkOPs : public InfoSub::Source {
public:
    // Network State (~8 methods)
    virtual OperatingMode getOperatingMode() const = 0;
    virtual std::string strOperatingMode(...) const = 0;
    virtual bool isFull() = 0;
    virtual bool isBlocked() = 0;
    virtual bool isAmendmentBlocked() = 0;
    virtual bool isUNLBlocked() = 0;
    virtual void setMode(OperatingMode om) = 0;
    virtual bool isNeedNetworkLedger() = 0;
    
    // Transaction Processing (~5 methods)
    virtual void submitTransaction(std::shared_ptr<STTx const> const&) = 0;
    virtual void processTransaction(...) = 0;
    virtual void processTransactionSet(CanonicalTXSet const& set) = 0;
    
    // Consensus (~8 methods)
    virtual bool beginConsensus(uint256 const& netLCL, ...) = 0;
    virtual void endConsensus(...) = 0;
    virtual bool processTrustedProposal(RCLCxPeerPos peerPos) = 0;
    virtual bool recvValidation(std::shared_ptr<STValidation> const& val, ...) = 0;
    virtual void mapComplete(std::shared_ptr<SHAMap> const& map, bool fromAcquire) = 0;
    virtual void consensusViewChange() = 0;
    
    // Pub/Sub (~3 methods in base, ~25 in InfoSub::Source)
    virtual void pubLedger(std::shared_ptr<ReadView const> const&) = 0;
    virtual void pubProposedTransaction(...) = 0;
    virtual void pubValidation(std::shared_ptr<STValidation> const&) = 0;
    
    // Info/Query (~6 methods)
    virtual Json::Value getConsensusInfo() = 0;
    virtual Json::Value getServerInfo(bool human, bool admin, bool counters) = 0;
    virtual Json::Value getLedgerFetchInfo() = 0;
    virtual Json::Value getOwnerInfo(...) = 0;
    virtual void getBookPage(...) = 0;
    
    // Ledger Operations (~5 methods)
    virtual std::uint32_t acceptLedger(...) = 0;
    virtual void updateLocalTx(ReadView const& newValidLedger) = 0;
    virtual std::size_t getLocalTxCount() = 0;
    virtual void reportFeeChange() = 0;
    virtual void clearLedgerFetch() = 0;
    
    // Timer/Lifecycle (~5 methods)
    virtual void stop() = 0;
    virtual void setStandAlone() = 0;
    virtual void setStateTimer() = 0;
    virtual void setNeedNetworkLedger() = 0;
    virtual void clearNeedNetworkLedger() = 0;
};
```

### Implementation Analysis

**File**: `src/xrpld/app/misc/NetworkOPs.cpp` (4894 lines)

Key internal classes:
- `TransactionStatus` (lines 75-97): Transaction with batch state
- `StateAccounting` (lines 124-181): State transition tracking
- `ServerFeeSummary` (lines 184-205): Fee summary for subscriptions
- `SubAccountHistoryIndex` (lines 674-697): Account history streaming state
- `SubAccountHistoryInfo/Weak` (lines 698-707): Account history subscription info

Key member variables:
```cpp
class NetworkOPsImp {
    Application& app_;
    std::unique_ptr<LocalTxs> m_localTX;
    std::atomic<OperatingMode> mMode;
    std::atomic<bool> needNetworkLedger_, amendmentBlocked_, amendmentWarned_, unlBlocked_;
    
    // Timers
    boost::asio::steady_timer heartbeatTimer_, clusterTimer_, accountHistoryTxTimer_;
    
    // Consensus
    RCLConsensus mConsensus;
    ConsensusPhase mLastConsensusPhase;
    
    // Subscriptions (protected by mSubLock)
    std::recursive_mutex mSubLock;
    SubInfoMapType mSubAccount, mSubRTAccount;
    subRpcMapType mRpcSubMap;
    SubAccountHistoryMapType mSubAccountHistory;
    std::array<SubMapType, SubTypes::sLastEntry> mStreamMaps;
    
    // Transaction batching (protected by mMutex)
    std::mutex mMutex;
    std::condition_variable mCond;
    DispatchState mDispatchState;
    std::vector<TransactionStatus> mTransactions;
    
    // Validation deduplication
    std::mutex validationsMutex_;
    std::set<uint256> pendingValidations_;
    
    LedgerMaster& m_ledgerMaster;
    JobQueue& m_job_queue;
    StateAccounting accounting_;
};
```

### Dependencies and Consumers

**External Consumers of NetworkOPs**:

| Consumer | Methods Used |
|----------|--------------|
| `Application::getOPs()` | Primary access point |
| `RPC::Context.netOps` | All RPC handlers |
| `RCLConsensus::Adaptor` | `getOPs().consensusViewChange()`, `getOPs().reportFeeChange()`, `getOPs().isBlocked()`, `getOPs().isFull()`, `getOPs().setMode()`, `getOPs().endConsensus()` |
| `SHAMapStoreImp` | `getOPs()` stored as `netOPs_` |
| `OverlayImpl` | `getOPs().getServerInfo()` |
| `ServerInfo.cpp` | `context.netOps.getServerInfo()` |
| `ServerState.cpp` | `context.netOps.getServerInfo()` |
| `ConsensusInfo.cpp` | `context.netOps.getConsensusInfo()` |
| `FetchInfo.cpp` | `context.netOps.getLedgerFetchInfo()`, `clearLedgerFetch()` |
| `Submit.cpp` | `context.netOps.processTransaction()` |
| `GetCounts.cpp` | `app.getOPs().getLocalTxCount()` |
| `Handler.h` | `app.getOPs().isAmendmentBlocked()`, `isUNLBlocked()` |

**Internal Dependencies of NetworkOPsImp**:

| Dependency | Purpose |
|------------|---------|
| `RCLConsensus mConsensus` | Consensus engine member |
| `LedgerMaster& m_ledgerMaster` | Ledger state access |
| `LocalTxs m_localTX` | Local transaction tracking |
| `Application& app_` | Access to all subsystems |
| `JobQueue& m_job_queue` | Background job scheduling |
| `boost::asio::steady_timer` | Heartbeat, cluster, history timers |

### Target Architecture

Split into 5 focused interfaces with clear responsibilities:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              NetworkOPs (Facade)                             │
│    Maintains backward compatibility, delegates to focused components         │
└───────────────┬─────────────┬─────────────┬─────────────┬─────────────┬─────┘
                │             │             │             │             │
                ▼             ▼             ▼             ▼             ▼
        ┌───────────┐ ┌─────────────┐ ┌───────────┐ ┌──────────┐ ┌──────────┐
        │INetwork   │ │ITransaction │ │IConsensus │ │IPubSub   │ │INetwork  │
        │State      │ │Processor    │ │Coordinator│ │Manager   │ │Info      │
        └───────────┘ └─────────────┘ └───────────┘ └──────────┘ └──────────┘
             │             │              │             │             │
             ▼             ▼              ▼             ▼             ▼
        ┌───────────┐ ┌─────────────┐ ┌───────────┐ ┌──────────┐ ┌──────────┐
        │NetworkStat│ │Transaction  │ │Consensus  │ │PubSub    │ │Network   │
        │eImpl      │ │ProcessorImpl│ │Coordinator│ │ManagerImp│ │InfoImpl  │
        │           │ │             │ │Impl       │ │          │ │          │
        │~150 lines │ │~500 lines   │ │~800 lines │ │~1500 line│ │~500 lines│
        └───────────┘ └─────────────┘ └───────────┘ └──────────┘ └──────────┘
```

### Estimated Effort

| Component | Estimated Lines | Complexity | Dependencies |
|-----------|-----------------|------------|--------------|
| INetworkState | 50 interface + 150 impl | Low | Minimal |
| ITransactionProcessor | 30 interface + 500 impl | Medium | LocalTxs, TxQ, HashRouter |
| IConsensusCoordinator | 40 interface + 800 impl | High | RCLConsensus, LedgerMaster |
| IPubSubManager | 80 interface + 1500 impl | Medium | InfoSub, OrderBookDB |
| INetworkInfo | 30 interface + 500 impl | Low | Various for getServerInfo |
| NetworkOPs Facade | 100 lines | Low | All above |
| **Total** | ~3700 lines | Medium-High | 3-4 weeks |

---

## Deep Code Analysis

### Files Requiring Changes

#### Core Files to Modify

| File | Lines | Change Type | Description |
|------|-------|-------------|-------------|
| `src/xrpld/app/misc/NetworkOPs.h` | 281 | Major refactor | Split into 5 interfaces, keep facade |
| `src/xrpld/app/misc/NetworkOPs.cpp` | 4894 | Major refactor | Extract code into 5 implementation classes |
| `src/xrpld/app/main/Application.h` | ~300 | Add methods | Add getters for new interfaces |
| `src/xrpld/app/main/Application.cpp` | ~2500 | Add members | Instantiate new components |
| `src/xrpld/rpc/Context.h` | ~100 | Update | Add references to new interfaces |

#### New Files to Create

| File | Est. Lines | Purpose |
|------|------------|---------|
| `src/xrpld/app/misc/INetworkState.h` | 50 | Network state interface |
| `src/xrpld/app/misc/NetworkStateImpl.h` | 30 | Implementation header |
| `src/xrpld/app/misc/NetworkStateImpl.cpp` | 150 | Implementation |
| `src/xrpld/app/misc/ITransactionProcessor.h` | 40 | Transaction processing interface |
| `src/xrpld/app/misc/TransactionProcessorImpl.h` | 50 | Implementation header |
| `src/xrpld/app/misc/TransactionProcessorImpl.cpp` | 500 | Implementation |
| `src/xrpld/app/misc/IConsensusCoordinator.h` | 50 | Consensus coordination interface |
| `src/xrpld/app/misc/ConsensusCoordinatorImpl.h` | 60 | Implementation header |
| `src/xrpld/app/misc/ConsensusCoordinatorImpl.cpp` | 800 | Implementation |
| `src/xrpld/app/misc/IPubSubManager.h` | 100 | Pub/Sub interface (extends InfoSub::Source) |
| `src/xrpld/app/misc/PubSubManagerImpl.h` | 100 | Implementation header |
| `src/xrpld/app/misc/PubSubManagerImpl.cpp` | 1500 | Implementation |
| `src/xrpld/app/misc/INetworkInfo.h` | 40 | Server info interface |
| `src/xrpld/app/misc/NetworkInfoImpl.h` | 40 | Implementation header |
| `src/xrpld/app/misc/NetworkInfoImpl.cpp` | 500 | Implementation |

#### Consumer Files Requiring Updates

| File | Change Type | Reason |
|------|-------------|--------|
| `src/xrpld/app/consensus/RCLConsensus.cpp` | Update calls | Uses `getOPs()` for state/consensus |
| `src/xrpld/overlay/detail/OverlayImpl.cpp` | Update calls | Uses `getOPs().getServerInfo()` |
| `src/xrpld/app/misc/SHAMapStoreImp.cpp` | Update calls | Stores `netOPs_` pointer |
| `src/xrpld/rpc/handlers/ServerInfo.cpp` | Update calls | Uses `netOps.getServerInfo()` |
| `src/xrpld/rpc/handlers/ServerState.cpp` | Update calls | Uses `netOps.getServerInfo()` |
| `src/xrpld/rpc/handlers/ConsensusInfo.cpp` | Update calls | Uses `netOps.getConsensusInfo()` |
| `src/xrpld/rpc/handlers/FetchInfo.cpp` | Update calls | Uses `netOps.getLedgerFetchInfo()` |
| `src/xrpld/rpc/handlers/Submit.cpp` | Update calls | Uses `netOps.processTransaction()` |
| `src/xrpld/rpc/handlers/SubmitMultiSigned.cpp` | Update calls | Uses `netOps.processTransaction()` |
| `src/xrpld/rpc/handlers/GetCounts.cpp` | Update calls | Uses `getOPs().getLocalTxCount()` |
| `src/xrpld/rpc/detail/Handler.h` | Update calls | Uses `getOPs().isAmendmentBlocked()` |
| `src/xrpld/app/main/GRPCServer.cpp` | Update calls | Creates context with `getOPs()` |

### All Classes/Interfaces Affected

#### Classes to Create

```cpp
// INetworkState - Operating mode and status
class INetworkState {
public:
    virtual ~INetworkState() = default;
    virtual OperatingMode getOperatingMode() const = 0;
    virtual std::string strOperatingMode(OperatingMode mode, bool admin) const = 0;
    virtual std::string strOperatingMode() const = 0;
    virtual bool isFull() = 0;
    virtual bool isBlocked() = 0;
    virtual bool isAmendmentBlocked() = 0;
    virtual bool isUNLBlocked() = 0;
    virtual void setMode(OperatingMode om) = 0;
    virtual void setAmendmentBlocked() = 0;
    virtual void setAmendmentWarned() = 0;
    virtual void clearAmendmentWarned() = 0;
    virtual bool isAmendmentWarned() = 0;
    virtual void setUNLBlocked() = 0;
    virtual void clearUNLBlocked() = 0;
    virtual bool isNeedNetworkLedger() = 0;
    virtual void setNeedNetworkLedger() = 0;
    virtual void clearNeedNetworkLedger() = 0;
    virtual void setStandAlone() = 0;
};

// ITransactionProcessor - Transaction submission and batching
class ITransactionProcessor {
public:
    virtual ~ITransactionProcessor() = default;
    virtual void submitTransaction(std::shared_ptr<STTx const> const&) = 0;
    virtual void processTransaction(
        std::shared_ptr<Transaction>& transaction,
        bool bUnlimited,
        bool bLocal,
        FailHard failType) = 0;
    virtual void processTransactionSet(CanonicalTXSet const& set) = 0;
    virtual void updateLocalTx(ReadView const& newValidLedger) = 0;
    virtual std::size_t getLocalTxCount() = 0;
};

// IConsensusCoordinator - Consensus lifecycle management
class IConsensusCoordinator {
public:
    virtual ~IConsensusCoordinator() = default;
    virtual bool beginConsensus(
        uint256 const& networkClosed,
        std::unique_ptr<std::stringstream> const& clog) = 0;
    virtual void endConsensus(bool correctLCL) = 0;
    virtual bool processTrustedProposal(RCLCxPeerPos peerPos) = 0;
    virtual bool recvValidation(
        std::shared_ptr<STValidation> const& val,
        std::string const& source) = 0;
    virtual void mapComplete(
        std::shared_ptr<SHAMap> const& map,
        bool fromAcquire) = 0;
    virtual void consensusViewChange() = 0;
    virtual void gotTXSet(
        std::shared_ptr<SHAMap> const& map,
        bool fromAcquire) = 0;
    virtual std::uint32_t acceptLedger(
        std::optional<std::chrono::milliseconds> consensusDelay,
        std::unique_ptr<std::stringstream> const& clog) = 0;
    virtual void reportFeeChange() = 0;
    virtual Json::Value getConsensusInfo() = 0;
    virtual bool checkLastClosedLedger(
        Overlay::PeerSequence const& peerList,
        uint256& networkClosed) = 0;
};

// IPubSubManager - Subscription management (extends InfoSub::Source)
class IPubSubManager : public InfoSub::Source {
public:
    virtual ~IPubSubManager() = default;
    // Publishing methods
    virtual void pubLedger(std::shared_ptr<ReadView const> const&) = 0;
    virtual void pubProposedTransaction(
        std::shared_ptr<ReadView const> const& ledger,
        std::shared_ptr<STTx const> const& transaction,
        TER result) = 0;
    virtual void pubValidation(std::shared_ptr<STValidation> const&) = 0;
    // Inherited from InfoSub::Source: subAccount, unsubAccount, subBook, etc.
};

// INetworkInfo - Server information and queries
class INetworkInfo {
public:
    virtual ~INetworkInfo() = default;
    virtual Json::Value getServerInfo(bool human, bool admin, bool counters) = 0;
    virtual Json::Value getLedgerFetchInfo() = 0;
    virtual void clearLedgerFetch() = 0;
    virtual Json::Value getOwnerInfo(
        std::shared_ptr<ReadView const> lpLedger,
        AccountID const& account) = 0;
    virtual void getBookPage(
        std::shared_ptr<ReadView const>& lpLedger,
        Book const& book,
        AccountID const& uTakerID,
        bool const bProof,
        unsigned int iLimit,
        Json::Value const& jvMarker,
        Json::Value& jvResult) = 0;
};
```

#### Classes to Modify

```cpp
// NetworkOPs becomes a facade that delegates to focused interfaces
class NetworkOPs : public InfoSub::Source {
public:
    // Keep existing interface for backward compatibility
    // Delegate to appropriate component

    // Access to individual components for new code
    virtual INetworkState& networkState() = 0;
    virtual ITransactionProcessor& transactionProcessor() = 0;
    virtual IConsensusCoordinator& consensusCoordinator() = 0;
    virtual IPubSubManager& pubSubManager() = 0;
    virtual INetworkInfo& networkInfo() = 0;
};

// Application.h - Add accessors
class Application {
    // Existing
    virtual NetworkOPs& getOPs() = 0;

    // New focused accessors (optional, can use getOPs().component())
    virtual INetworkState& getNetworkState() = 0;
    virtual ITransactionProcessor& getTransactionProcessor() = 0;
    virtual IConsensusCoordinator& getConsensusCoordinator() = 0;
    virtual IPubSubManager& getPubSubManager() = 0;
    virtual INetworkInfo& getNetworkInfo() = 0;
};
```

### Function Signatures Being Modified

#### Methods Moving to INetworkState

```cpp
// From NetworkOPs interface (lines 65-90 in NetworkOPs.h)
virtual OperatingMode getOperatingMode() const = 0;
virtual std::string strOperatingMode(OperatingMode mode, bool admin) const = 0;
virtual std::string strOperatingMode() const = 0;
virtual void setMode(OperatingMode om) = 0;
virtual bool isFull() = 0;
virtual bool isBlocked() = 0;
virtual bool isAmendmentBlocked() = 0;
virtual bool isUNLBlocked() = 0;
virtual void setAmendmentBlocked() = 0;
virtual void setUNLBlocked() = 0;
virtual bool isNeedNetworkLedger() = 0;
virtual void setNeedNetworkLedger() = 0;
virtual void clearNeedNetworkLedger() = 0;
virtual void setStandAlone() = 0;
```

#### Methods Moving to ITransactionProcessor

```cpp
// From NetworkOPs interface (lines 115-135 in NetworkOPs.h)
virtual void submitTransaction(std::shared_ptr<STTx const> const&) = 0;
virtual void processTransaction(
    std::shared_ptr<Transaction>& transaction,
    bool bUnlimited,
    bool bLocal,
    FailHard failType) = 0;
virtual void processTransactionSet(CanonicalTXSet const& set) = 0;
virtual void updateLocalTx(ReadView const& newValidLedger) = 0;
virtual std::size_t getLocalTxCount() = 0;
```

#### Methods Moving to IConsensusCoordinator

```cpp
// From NetworkOPs interface (lines 170-220 in NetworkOPs.h)
virtual bool beginConsensus(
    uint256 const& networkClosed,
    std::unique_ptr<std::stringstream> const& clog) = 0;
virtual void endConsensus(bool correctLCL) = 0;
virtual bool processTrustedProposal(RCLCxPeerPos peerPos) = 0;
virtual bool recvValidation(
    std::shared_ptr<STValidation> const& val,
    std::string const& source) = 0;
virtual void mapComplete(
    std::shared_ptr<SHAMap> const& map,
    bool fromAcquire) = 0;
virtual void consensusViewChange() = 0;
virtual std::uint32_t acceptLedger(
    std::optional<std::chrono::milliseconds> consensusDelay,
    std::unique_ptr<std::stringstream> const& clog) = 0;
virtual void reportFeeChange() = 0;
virtual Json::Value getConsensusInfo() = 0;
```

#### Methods Moving to IPubSubManager

```cpp
// From NetworkOPs interface (lines 225-265 in NetworkOPs.h)
virtual void pubLedger(std::shared_ptr<ReadView const> const&) = 0;
virtual void pubProposedTransaction(
    std::shared_ptr<ReadView const> const& ledger,
    std::shared_ptr<STTx const> const& transaction,
    TER result) = 0;
virtual void pubValidation(std::shared_ptr<STValidation> const&) = 0;

// Inherited from InfoSub::Source (InfoSub.h lines 85-243)
virtual void subAccount(...) = 0;          // ~15 subscription methods
virtual void unsubAccount(...) = 0;
virtual void subBook(...) = 0;
virtual void unsubBook(...) = 0;
virtual void subLedger(...) = 0;
virtual void subServer(...) = 0;
virtual void subTransactions(...) = 0;
virtual void subValidations(...) = 0;
// ... etc
```

#### Methods Moving to INetworkInfo

```cpp
// From NetworkOPs interface
virtual Json::Value getServerInfo(bool human, bool admin, bool counters) = 0;
virtual Json::Value getLedgerFetchInfo() = 0;
virtual void clearLedgerFetch() = 0;
virtual Json::Value getOwnerInfo(
    std::shared_ptr<ReadView const> lpLedger,
    AccountID const& account) = 0;
virtual void getBookPage(
    std::shared_ptr<ReadView const>& lpLedger,
    Book const& book,
    AccountID const& uTakerID,
    bool const bProof,
    unsigned int iLimit,
    Json::Value const& jvMarker,
    Json::Value& jvResult) = 0;
```

### Code Flow Diagrams

#### Transaction Processing Flow (Current)

```
RPC Handler (Submit.cpp)
    │
    ▼
NetworkOPs::processTransaction()  [NetworkOPs.cpp:1200-1500]
    │
    ├─► HashRouter::setFlags()    [Pre-processing]
    │
    ├─► Queue transaction         [mMutex protected]
    │   └─► mTransactions.push_back(TransactionStatus)
    │
    ├─► Schedule batch job        [If not already scheduled]
    │   └─► mDispatchState = scheduled
    │
    └─► transactionBatch()        [Job thread]
        │
        ├─► apply(lock)           [Apply transactions]
        │   └─► OpenLedger::accept()
        │
        └─► Publish results       [pubProposedTransaction]
```

#### Consensus Flow (Current)

```
Timer/Heartbeat
    │
    ▼
NetworkOPsImp::processHeartbeat()  [NetworkOPs.cpp:900-1100]
    │
    ├─► checkLastClosedLedger()
    │   └─► Validation::getPreferredLCL()
    │
    ├─► beginConsensus()          [NetworkOPs.cpp:2030-2100]
    │   └─► mConsensus.startRound()
    │
    └─► setStateTimer()           [Schedule next heartbeat]

RCLConsensus (external events)
    │
    ▼
NetworkOPsImp::endConsensus()
    │
    ├─► acceptLedger()            [Close and accept ledger]
    │   ├─► OpenLedger::accept()
    │   ├─► LedgerMaster::storeLedger()
    │   └─► pubLedger()
    │
    └─► setMode(FULL)             [Update operating mode]
```

#### Pub/Sub Flow (Current)

```
Event Source (ledger close, transaction, validation)
    │
    ▼
NetworkOPsImp::pub*() methods     [NetworkOPs.cpp:3800-4400]
    │
    ├─► Lock mSubLock             [Recursive mutex]
    │
    ├─► Iterate subscribers       [mStreamMaps, mSubAccount, etc.]
    │   │
    │   └─► For each InfoSub
    │       └─► InfoSub::send()   [Queue JSON message]
    │
    └─► Unlock mSubLock
```

### Internal Classes Analysis

#### TransactionStatus (lines 75-97)

```cpp
class TransactionStatus {
    std::shared_ptr<Transaction> transaction_;
    bool applied_ = false;
    bool broadcast_ = true;
    bool failHard_ = false;
    bool queued_ = false;
    TER result_ = temUNKNOWN;
public:
    // Used for batch processing in transactionBatch()
};
```
**Destination**: Move to `TransactionProcessorImpl` as private nested class.

#### StateAccounting (lines 124-181)

```cpp
class StateAccounting {
    struct Counters {
        std::uint32_t transitions = 0;
        std::chrono::microseconds dur = std::chrono::microseconds(0);
    };
    std::array<Counters, 5> counters_;  // One per OperatingMode
    OperatingMode mode_ = OperatingMode::DISCONNECTED;
    std::chrono::system_clock::time_point start_;
public:
    void mode(OperatingMode om);
    Json::Value json() const;
};
```
**Destination**: Move to `NetworkStateImpl` as private nested class.

#### ServerFeeSummary (lines 184-205)

```cpp
struct ServerFeeSummary {
    XRPAmount loadFee;
    TxQ::Metrics txQMetrics;
    std::uint64_t baseFee;
    std::uint32_t loadFactor;
};
```
**Destination**: Move to `PubSubManagerImpl` for subscription publishing.

#### SubAccountHistoryIndex (lines 674-697)

```cpp
struct SubAccountHistoryIndex {
    AccountID account;
    LedgerIndex minLedger = std::numeric_limits<LedgerIndex>::min();
    LedgerIndex maxLedger = std::numeric_limits<LedgerIndex>::max();
    std::int32_t txnsRemaining = std::numeric_limits<std::int32_t>::max();
};
```
**Destination**: Move to `PubSubManagerImpl` for account history streaming.

---

## Design Considerations

### Alternative 1: Full Interface Extraction (Recommended)

**Description**: Extract 5 focused interfaces as new abstract classes. NetworkOPs becomes a thin facade delegating to implementations. All implementations are created and owned by Application.

**Pros**:
- Clear separation of concerns
- Each component can be tested in isolation
- Gradual migration path (facade maintains compatibility)
- Future-proof for further refactoring
- Thread safety can be optimized per component

**Cons**:
- Significant refactoring effort (~3-4 weeks)
- Risk of subtle behavior changes during extraction
- Increased number of files to maintain
- May require touching many consumer files

**Thread Safety**:
- `NetworkStateImpl`: Simple atomic operations, no mutex needed
- `TransactionProcessorImpl`: Own `mMutex` for batch processing
- `ConsensusCoordinatorImpl`: Own `mConsensus` with internal locking
- `PubSubManagerImpl`: Own `mSubLock` (recursive_mutex)
- `NetworkInfoImpl`: Read-only queries, minimal locking

### Alternative 2: Composition Within NetworkOPsImp

**Description**: Keep NetworkOPsImp but refactor internally using composition. Create helper classes for each concern that are instantiated as members.

**Pros**:
- Less invasive change
- No API changes required
- Faster implementation (~2 weeks)
- All existing tests remain valid

**Cons**:
- Still one monolithic class from outside
- Helper classes tightly coupled to NetworkOPsImp
- Limited testing improvement
- Does not address compilation coupling
- Not suitable for dependency injection

### Alternative 3: Mixin/CRTP Pattern

**Description**: Use template mixins to compose NetworkOPsImp from multiple base classes, each providing a subset of functionality.

**Pros**:
- Zero runtime overhead
- Strong compile-time type checking
- No virtual dispatch cost

**Cons**:
- Complex template metaprogramming
- Difficult to debug
- Hard to understand for new developers
- Does not allow runtime substitution for testing

### Recommendation

**Alternative 1 (Full Interface Extraction)** is recommended because:
1. It provides the cleanest long-term architecture
2. Enables proper unit testing of each component
3. Aligns with phase 2 interface work
4. The facade pattern minimizes breaking changes
5. Well-understood pattern in C++ codebases

### Thread Safety Requirements

| Component | Current Locking | Proposed Locking |
|-----------|-----------------|------------------|
| NetworkStateImpl | `std::atomic` for flags | Keep `std::atomic` |
| TransactionProcessorImpl | `mMutex` + `mCond` | Keep same pattern |
| ConsensusCoordinatorImpl | Internal to RCLConsensus | Delegate to RCLConsensus |
| PubSubManagerImpl | `mSubLock` (recursive_mutex) | Keep recursive_mutex |
| NetworkInfoImpl | Various app_ calls | Use existing app locks |

**Cross-Component Synchronization**:
- Consensus notifies PubSub via events (not direct calls)
- TransactionProcessor notifies PubSub via callbacks
- NetworkState changes are atomic, observable by all

### Performance Implications

**No Expected Performance Regression**:
1. Interface dispatch: One virtual call per method (already exists)
2. Facade delegation: Trivial inline forwards
3. Lock granularity: Same or better than current

**Potential Improvements**:
1. Smaller compilation units = faster incremental builds
2. More focused hot paths in CPU cache
3. Opportunity for lock-free patterns in NetworkStateImpl

### Backward Compatibility

**Full Backward Compatibility Maintained**:
1. `Application::getOPs()` returns existing `NetworkOPs&`
2. All existing method signatures preserved in facade
3. `RPC::Context.netOps` unchanged
4. Consumer code can use old patterns indefinitely

**Deprecation Path** (Future):
1. Mark old `NetworkOPs` methods as `[[deprecated]]`
2. Add compiler warnings to guide migration
3. Eventually remove facade after all consumers migrate

### Error Handling Strategy

**Current Pattern** (Preserve):
- Exceptions for programming errors (`Throw<>`, `LogicError`)
- Return values for operational errors (`TER`, `bool`, `std::optional`)
- Logging via `beast::Journal`

**Component-Specific Handling**:
- `INetworkState`: No errors expected (state transitions always valid)
- `ITransactionProcessor`: Return `TER` codes, exceptions for logic errors
- `IConsensusCoordinator`: Return `bool` success, log warnings
- `IPubSubManager`: Silent failures for disconnected subscribers
- `INetworkInfo`: Return `Json::Value` with error fields

---

## Implementation Plan

### Phase 1: Create New Interfaces (Week 1)

#### Step 1.1: Create INetworkState Interface

**File**: `src/xrpld/app/misc/INetworkState.h`

```cpp
#ifndef RIPPLE_APP_MISC_INETWORKSTATE_H_INCLUDED
#define RIPPLE_APP_MISC_INETWORKSTATE_H_INCLUDED

#include <xrpld/app/misc/NetworkOPs.h>  // For OperatingMode

namespace ripple {

class INetworkState {
public:
    virtual ~INetworkState() = default;

    virtual OperatingMode getOperatingMode() const = 0;
    virtual std::string strOperatingMode(OperatingMode, bool admin) const = 0;
    virtual std::string strOperatingMode() const = 0;
    virtual void setMode(OperatingMode om) = 0;
    virtual bool isFull() = 0;
    virtual bool isBlocked() = 0;
    virtual bool isAmendmentBlocked() = 0;
    virtual bool isUNLBlocked() = 0;
    virtual void setAmendmentBlocked() = 0;
    virtual void setUNLBlocked() = 0;
    virtual void clearUNLBlocked() = 0;
    virtual bool isAmendmentWarned() = 0;
    virtual void setAmendmentWarned() = 0;
    virtual void clearAmendmentWarned() = 0;
    virtual bool isNeedNetworkLedger() = 0;
    virtual void setNeedNetworkLedger() = 0;
    virtual void clearNeedNetworkLedger() = 0;
    virtual void setStandAlone() = 0;
};

}  // namespace ripple

#endif
```

**Validation**: Compiles with `cmake --build build --target xrpld`

#### Step 1.2: Create ITransactionProcessor Interface

**File**: `src/xrpld/app/misc/ITransactionProcessor.h`

```cpp
#ifndef RIPPLE_APP_MISC_ITRANSACTIONPROCESSOR_H_INCLUDED
#define RIPPLE_APP_MISC_ITRANSACTIONPROCESSOR_H_INCLUDED

#include <xrpld/app/misc/NetworkOPs.h>  // For FailHard
#include <xrpld/app/misc/CanonicalTXSet.h>
#include <xrpld/app/misc/Transaction.h>

namespace ripple {

class ITransactionProcessor {
public:
    virtual ~ITransactionProcessor() = default;

    virtual void submitTransaction(std::shared_ptr<STTx const> const&) = 0;
    virtual void processTransaction(
        std::shared_ptr<Transaction>& transaction,
        bool bUnlimited,
        bool bLocal,
        FailHard failType) = 0;
    virtual void processTransactionSet(CanonicalTXSet const& set) = 0;
    virtual void updateLocalTx(ReadView const& newValidLedger) = 0;
    virtual std::size_t getLocalTxCount() = 0;
};

}  // namespace ripple

#endif
```

**Validation**: Compiles with `cmake --build build --target xrpld`

#### Step 1.3: Create IConsensusCoordinator Interface

**File**: `src/xrpld/app/misc/IConsensusCoordinator.h`

Lines: ~60

**Validation**: Compiles with `cmake --build build --target xrpld`

#### Step 1.4: Create IPubSubManager Interface

**File**: `src/xrpld/app/misc/IPubSubManager.h`

Lines: ~100 (extends InfoSub::Source)

**Validation**: Compiles with `cmake --build build --target xrpld`

#### Step 1.5: Create INetworkInfo Interface

**File**: `src/xrpld/app/misc/INetworkInfo.h`

Lines: ~50

**Validation**: Compiles with `cmake --build build --target xrpld`

### Phase 2: Create Implementations (Week 2)

#### Step 2.1: Create NetworkStateImpl

**Files**:
- `src/xrpld/app/misc/NetworkStateImpl.h` (~40 lines)
- `src/xrpld/app/misc/NetworkStateImpl.cpp` (~150 lines)

**Extract from NetworkOPs.cpp**:
- Lines 1812-1866: `isBlocked()`, `isAmendmentBlocked()`, `setAmendmentBlocked()`, etc.
- Lines 2147-2175: `setMode()`, `getOperatingMode()`
- State variables: `mMode`, `needNetworkLedger_`, `amendmentBlocked_`, etc.
- Inner class: `StateAccounting`

**Validation**: Unit tests for state transitions

#### Step 2.2: Create TransactionProcessorImpl

**Files**:
- `src/xrpld/app/misc/TransactionProcessorImpl.h` (~60 lines)
- `src/xrpld/app/misc/TransactionProcessorImpl.cpp` (~500 lines)

**Extract from NetworkOPs.cpp**:
- Lines 1200-1500: `processTransaction()`, preprocessing
- Lines 1500-1650: `transactionBatch()`, `apply()`
- Lines 2195-2400: Ledger acceptance transaction handling
- Inner class: `TransactionStatus`
- Variables: `mMutex`, `mCond`, `mDispatchState`, `mTransactions`, `m_localTX`

**Validation**: Transaction processing tests

#### Step 2.3: Create ConsensusCoordinatorImpl

**Files**:
- `src/xrpld/app/misc/ConsensusCoordinatorImpl.h` (~80 lines)
- `src/xrpld/app/misc/ConsensusCoordinatorImpl.cpp` (~800 lines)

**Extract from NetworkOPs.cpp**:
- Lines 1868-2100: `checkLastClosedLedger()`, `switchLastClosedLedger()`, `beginConsensus()`
- Lines 2100-2195: `endConsensus()`, `acceptLedger()`
- Lines 2195-2400: Consensus result handling
- Variables: `mConsensus`, `mLastConsensusPhase`

**Validation**: Consensus flow tests

#### Step 2.4: Create PubSubManagerImpl

**Files**:
- `src/xrpld/app/misc/PubSubManagerImpl.h` (~120 lines)
- `src/xrpld/app/misc/PubSubManagerImpl.cpp` (~1500 lines)

**Extract from NetworkOPs.cpp**:
- Lines 3000-3800: `subAccount()`, `unsubAccount()`, etc.
- Lines 3800-4400: `pubLedger()`, `pubProposedTransaction()`, etc.
- Lines 4400-4894: Account history streaming
- Inner classes: `ServerFeeSummary`, `SubAccountHistoryIndex`, etc.
- Variables: `mSubLock`, all subscription maps

**Validation**: Subscription tests

#### Step 2.5: Create NetworkInfoImpl

**Files**:
- `src/xrpld/app/misc/NetworkInfoImpl.h` (~50 lines)
- `src/xrpld/app/misc/NetworkInfoImpl.cpp` (~500 lines)

**Extract from NetworkOPs.cpp**:
- Lines 2400-3000: `getServerInfo()`, `getConsensusInfo()`
- Lines 1700-1810: `getOwnerInfo()`
- Lines 4647-4894: `getBookPage()`

**Validation**: RPC info tests

### Phase 3: Create Facade and Wire Components (Week 3)

#### Step 3.1: Modify NetworkOPs Interface

**File**: `src/xrpld/app/misc/NetworkOPs.h`

Add component accessor methods:
```cpp
virtual INetworkState& networkState() = 0;
virtual ITransactionProcessor& transactionProcessor() = 0;
virtual IConsensusCoordinator& consensusCoordinator() = 0;
virtual IPubSubManager& pubSubManager() = 0;
virtual INetworkInfo& networkInfo() = 0;
```

**Validation**: Compiles, all tests pass

#### Step 3.2: Modify NetworkOPsImp to Delegate

**File**: `src/xrpld/app/misc/NetworkOPs.cpp`

1. Add component members:
```cpp
std::unique_ptr<NetworkStateImpl> networkState_;
std::unique_ptr<TransactionProcessorImpl> transactionProcessor_;
std::unique_ptr<ConsensusCoordinatorImpl> consensusCoordinator_;
std::unique_ptr<PubSubManagerImpl> pubSubManager_;
std::unique_ptr<NetworkInfoImpl> networkInfo_;
```

2. Delegate existing methods to components
3. Remove extracted code (now in component impls)

**Validation**: Full test suite passes

#### Step 3.3: Update Application

**Files**:
- `src/xrpld/app/main/Application.h`
- `src/xrpld/app/main/Application.cpp`

Add optional direct accessors (for new code):
```cpp
INetworkState& getNetworkState() { return getOPs().networkState(); }
// etc.
```

**Validation**: Application starts correctly

### Phase 4: Update Consumers (Week 4)

#### Step 4.1: Update RPC Context

**File**: `src/xrpld/rpc/Context.h`

Optionally add focused references for new RPC handlers:
```cpp
INetworkInfo& networkInfo;  // Optional, can use netOps.networkInfo()
```

#### Step 4.2: Update RCLConsensus::Adaptor

**File**: `src/xrpld/app/consensus/RCLConsensus.cpp`

No changes required - still uses `app_.getOPs().method()` pattern.
Future optimization: Use `app_.getConsensusCoordinator()` directly.

#### Step 4.3: Update Tests

**Files**:
- `src/test/app/NetworkOPs_test.cpp`
- Any other tests using NetworkOPs

Ensure all existing tests pass with new structure.

#### Step 4.4: Update CMakeLists.txt

**File**: `src/CMakeLists.txt` or appropriate build file

Add new source files:
```cmake
src/xrpld/app/misc/NetworkStateImpl.cpp
src/xrpld/app/misc/TransactionProcessorImpl.cpp
src/xrpld/app/misc/ConsensusCoordinatorImpl.cpp
src/xrpld/app/misc/PubSubManagerImpl.cpp
src/xrpld/app/misc/NetworkInfoImpl.cpp
```

---

## Risk Assessment

### Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Subtle behavior changes during extraction | Medium | High | Extensive testing, extract incrementally |
| Thread safety bugs in refactored code | Medium | Critical | Keep same locking patterns, code review |
| Missing method in interface | Low | Low | Compiler will catch, easy to add |
| Performance regression | Low | Medium | Profile before/after, benchmark critical paths |
| Build system issues with new files | Low | Low | Test on all platforms early |
| Merge conflicts with concurrent work | Medium | Medium | Coordinate with team, rebase frequently |

### Testing Gaps

| Area | Current Coverage | Risk | Recommendation |
|------|------------------|------|----------------|
| Transaction batching | Medium | High | Add stress tests for concurrent submission |
| Consensus coordinator | High | Low | Existing consensus tests sufficient |
| Pub/Sub manager | Low | Medium | Add subscription lifecycle tests |
| State transitions | Medium | Low | Add state machine tests |
| Cross-component interactions | Low | High | Add integration tests for event flows |

### Rollback Strategy

1. **Before merge**: All changes in feature branch, main untouched
2. **During review**: Incremental PRs, each independently revertable
3. **After merge**: Git revert of merge commit restores previous state
4. **Production**: Feature flags not needed (internal refactoring only)

**Rollback steps**:
```bash
# If issues found after merge
git revert -m 1 <merge-commit-sha>
git push origin develop
```

---

## Validation Criteria

### Success Metrics

| Metric | Target | Measurement |
|--------|--------|-------------|
| All existing tests pass | 100% | CI pipeline |
| No new test failures | 0 | CI pipeline |
| Build time change | < 5% increase | Measure full rebuild |
| Incremental build improvement | > 10% faster | Measure single-file change rebuild |
| Code coverage maintained | >= current | Coverage report |
| NetworkOPs.cpp line count | < 500 lines | wc -l |
| Largest component | < 1500 lines | wc -l |

### Levelization Verification

```bash
# Verify no new dependency cycles
./.github/scripts/levelization/levelization.sh

# Expected: No cycles involving new files
# INetworkState.h should be leaf-level (no app dependencies)
# Implementations depend on interfaces, not each other
```

### Required Test Coverage

#### Unit Tests

1. **NetworkStateImpl tests**:
   - State transitions (DISCONNECTED → CONNECTED → SYNCING → TRACKING → FULL)
   - Amendment/UNL blocked states
   - Standalone mode

2. **TransactionProcessorImpl tests**:
   - Single transaction processing
   - Batch processing
   - Concurrent submission
   - FailHard behavior

3. **ConsensusCoordinatorImpl tests**:
   - Begin/end consensus cycle
   - Ledger switching
   - Validation handling

4. **PubSubManagerImpl tests**:
   - Subscribe/unsubscribe lifecycle
   - Publishing to multiple subscribers
   - Subscriber disconnection handling

5. **NetworkInfoImpl tests**:
   - getServerInfo with various flags
   - getBookPage pagination

#### Integration Tests

1. **Full consensus round with pub/sub**
2. **Transaction submission through full pipeline**
3. **State transitions during network events**

### Build Verification

```bash
# Full build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Run all tests
cd build && ctest --output-on-failure

# Levelization check
./.github/scripts/levelization/levelization.sh
```

### Manual Verification Checklist

- [ ] Server starts successfully
- [ ] RPC server_info returns valid data
- [ ] Transactions can be submitted
- [ ] Consensus proceeds normally
- [ ] WebSocket subscriptions work
- [ ] Admin commands function
- [ ] Logging shows no unexpected warnings
- [ ] Memory usage stable over time

---

## Appendix A: Method Assignment Summary

| Method | Current Location | Target Interface |
|--------|------------------|------------------|
| `getOperatingMode()` | NetworkOPs:65 | INetworkState |
| `strOperatingMode()` | NetworkOPs:67-70 | INetworkState |
| `isFull()` | NetworkOPs:72 | INetworkState |
| `isBlocked()` | NetworkOPs:73 | INetworkState |
| `isAmendmentBlocked()` | NetworkOPs:74 | INetworkState |
| `isUNLBlocked()` | NetworkOPs:75 | INetworkState |
| `setMode()` | NetworkOPs:77 | INetworkState |
| `setAmendmentBlocked()` | NetworkOPs:78 | INetworkState |
| `setUNLBlocked()` | NetworkOPs:79 | INetworkState |
| `isNeedNetworkLedger()` | NetworkOPs:82 | INetworkState |
| `setNeedNetworkLedger()` | NetworkOPs:83 | INetworkState |
| `clearNeedNetworkLedger()` | NetworkOPs:84 | INetworkState |
| `setStandAlone()` | NetworkOPs:86 | INetworkState |
| `submitTransaction()` | NetworkOPs:115 | ITransactionProcessor |
| `processTransaction()` | NetworkOPs:120 | ITransactionProcessor |
| `processTransactionSet()` | NetworkOPs:132 | ITransactionProcessor |
| `updateLocalTx()` | NetworkOPs:137 | ITransactionProcessor |
| `getLocalTxCount()` | NetworkOPs:138 | ITransactionProcessor |
| `beginConsensus()` | NetworkOPs:170 | IConsensusCoordinator |
| `endConsensus()` | NetworkOPs:175 | IConsensusCoordinator |
| `processTrustedProposal()` | NetworkOPs:180 | IConsensusCoordinator |
| `recvValidation()` | NetworkOPs:185 | IConsensusCoordinator |
| `mapComplete()` | NetworkOPs:190 | IConsensusCoordinator |
| `consensusViewChange()` | NetworkOPs:195 | IConsensusCoordinator |
| `acceptLedger()` | NetworkOPs:200 | IConsensusCoordinator |
| `reportFeeChange()` | NetworkOPs:205 | IConsensusCoordinator |
| `getConsensusInfo()` | NetworkOPs:207 | IConsensusCoordinator |
| `pubLedger()` | NetworkOPs:225 | IPubSubManager |
| `pubProposedTransaction()` | NetworkOPs:230 | IPubSubManager |
| `pubValidation()` | NetworkOPs:235 | IPubSubManager |
| `sub*/unsub*()` | InfoSub::Source | IPubSubManager |
| `getServerInfo()` | NetworkOPs:250 | INetworkInfo |
| `getLedgerFetchInfo()` | NetworkOPs:255 | INetworkInfo |
| `clearLedgerFetch()` | NetworkOPs:256 | INetworkInfo |
| `getOwnerInfo()` | NetworkOPs:260 | INetworkInfo |
| `getBookPage()` | NetworkOPs:265 | INetworkInfo |

---

## Appendix B: Member Variable Assignment

| Variable | Current Scope | Target Component |
|----------|---------------|------------------|
| `mMode` | NetworkOPsImp | NetworkStateImpl |
| `needNetworkLedger_` | NetworkOPsImp | NetworkStateImpl |
| `amendmentBlocked_` | NetworkOPsImp | NetworkStateImpl |
| `amendmentWarned_` | NetworkOPsImp | NetworkStateImpl |
| `unlBlocked_` | NetworkOPsImp | NetworkStateImpl |
| `standalone_` | NetworkOPsImp | NetworkStateImpl |
| `accounting_` | NetworkOPsImp | NetworkStateImpl |
| `m_localTX` | NetworkOPsImp | TransactionProcessorImpl |
| `mMutex` | NetworkOPsImp | TransactionProcessorImpl |
| `mCond` | NetworkOPsImp | TransactionProcessorImpl |
| `mDispatchState` | NetworkOPsImp | TransactionProcessorImpl |
| `mTransactions` | NetworkOPsImp | TransactionProcessorImpl |
| `mConsensus` | NetworkOPsImp | ConsensusCoordinatorImpl |
| `mLastConsensusPhase` | NetworkOPsImp | ConsensusCoordinatorImpl |
| `validationsMutex_` | NetworkOPsImp | ConsensusCoordinatorImpl |
| `pendingValidations_` | NetworkOPsImp | ConsensusCoordinatorImpl |
| `mSubLock` | NetworkOPsImp | PubSubManagerImpl |
| `mSubAccount` | NetworkOPsImp | PubSubManagerImpl |
| `mSubRTAccount` | NetworkOPsImp | PubSubManagerImpl |
| `mRpcSubMap` | NetworkOPsImp | PubSubManagerImpl |
| `mSubAccountHistory` | NetworkOPsImp | PubSubManagerImpl |
| `mStreamMaps` | NetworkOPsImp | PubSubManagerImpl |
| `heartbeatTimer_` | NetworkOPsImp | NetworkOPsFacade (timer owner) |
| `clusterTimer_` | NetworkOPsImp | NetworkOPsFacade (timer owner) |
| `accountHistoryTxTimer_` | NetworkOPsImp | PubSubManagerImpl |

