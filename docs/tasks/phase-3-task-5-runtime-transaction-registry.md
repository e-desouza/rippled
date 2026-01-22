# Phase 3 Task 5: Runtime Transaction Registry

## 1. Overview

### 1.1 Problem Statement

The current transaction system uses an X-macro pattern (`transactions.macro`) with 7 parameters
per transaction definition across 1,110 lines. This pattern creates several problems:

1. **Preprocessor Complexity**: Requires `#pragma push_macro`/`#pragma pop_macro` at each usage
2. **Giant Switch Statement**: `with_txn_type()` in applySteps.cpp generates a ~75-case switch
3. **Debugging Difficulty**: Preprocessor errors point to macro file, not actual source
4. **Testing Challenges**: Cannot mock transaction handlers for unit testing
5. **Extension Friction**: Adding transactions requires modifying central macro file
6. **Hidden Include Mechanism**: `TRANSACTION_INCLUDE` obscures header dependencies

### 1.2 Target Architecture

Replace the macro-generated switch with a runtime registry providing:
- **ITransactionHandler interface** for transaction processing phases
- **TransactionRegistry singleton** for handler lookup by TxType
- **REGISTER_TRANSACTION macro** for self-registration in transactor files
- **Preserved TxType enum** via simplified macro (backward compatibility)

### 1.3 Key Files

| File | Role |
|------|------|
| `include/xrpl/protocol/detail/transactions.macro` | Defines transaction types (X-macro) |
| `src/xrpld/app/tx/detail/applySteps.cpp` | Contains `with_txn_type()` switch statement |
| `include/xrpl/protocol/TxFormats.h` | TxType enum, TxFormats class |
| `src/libxrpl/protocol/TxFormats.cpp` | Builds format registry from macro |
| `src/xrpld/app/tx/detail/Transactor.h` | Base class for all transactors |

### 1.4 Estimated Effort

- **Development**: 3-4 weeks
- **Testing/Integration**: 1-2 weeks
- **Total**: 4-6 weeks

---

## 2. Deep Code Analysis

### 2.1 transactions.macro Structure

The macro defines 7 parameters per transaction:
```cpp
TRANSACTION(tag, value, name, delegable, amendments, privileges, fields)
// Example:
TRANSACTION(ttPAYMENT, 0, Payment,
    Delegation::delegable,
    uint256{},
    createAcct,
    ({
        {sfDestination, soeREQUIRED},
        {sfAmount, soeREQUIRED, soeMPTSupported},
        // ...
    }))
```

Current transaction count: ~75 types (values 0-102, with gaps for deprecated types).

The macro also conditionally includes transactor headers:
```cpp
#if TRANSACTION_INCLUDE
#   include <xrpld/app/tx/detail/Payment.h>
#endif
```

### 2.2 applySteps.cpp with_txn_type() Analysis

This function is the core dispatch mechanism (~85 lines):
```cpp
template <class F>
auto with_txn_type(Rules const& rules, TxType txnType, F&& f) {
    // Rules setup (lines 39-67)
    switch (txnType) {
        // Macro-generated cases:
        #define TRANSACTION(tag, value, name, ...) \
            case tag: return f.template operator()<name>();
        #include <xrpl/protocol/detail/transactions.macro>
        default:
            throw UnknownTxnType(txnType);
    }
}
```

Called from 4 invoke functions:
- `invoke_preflight()` - Calls `Transactor::invokePreflight<T>(ctx)`
- `invoke_preclaim()` - Calls `T::preclaim(ctx)` and check methods
- `invoke_calculateBaseFee()` - Calls `T::calculateBaseFee(view, tx)`
- `invoke_apply()` - Creates `T(ctx)` and calls `p()`

### 2.3 TxFormats.cpp Usage

Builds SOTemplate for each transaction type:
```cpp
#define TRANSACTION(tag, value, name, delegable, amendment, privileges, fields) \
    add(jss::name, tag, UNWRAP fields, commonFields);
#include <xrpl/protocol/detail/transactions.macro>
```

### 2.4 Transactor Base Class Pattern

All transactors inherit from `Transactor` and implement:
```cpp
class Payment : public Transactor {
public:
    static constexpr ConsequencesFactoryType ConsequencesFactory{Custom};
    explicit Payment(ApplyContext& ctx) : Transactor(ctx) {}

    static TxConsequences makeTxConsequences(PreflightContext const& ctx);
    static NotTEC preflight(PreflightContext const& ctx);
    static TER preclaim(PreclaimContext const& ctx);
    TER doApply() override;
};
```

Key observation: Static methods (`preflight`, `preclaim`, `calculateBaseFee`) require
template dispatch; `doApply()` is virtual. This shapes the handler interface design.

---

## 3. Design Considerations

### 3.1 Option A: Keep Macro + Runtime Validation (Minimal Change)

**Approach**: Retain macro-generated switch, add runtime registry for metadata only.

**Pros**: Minimal changes, no performance impact.
**Cons**: Doesn't solve extensibility, debugging, or testing problems.

**Verdict**: Not recommended - fails to address core issues.

### 3.2 Option B: Runtime Registry with CRTP (Recommended)

**Approach**: Use Curiously Recurring Template Pattern to wrap static methods into
virtual interface while maintaining compile-time type checking.

```cpp
class ITransactionHandler {
public:
    virtual ~ITransactionHandler() = default;
    virtual NotTEC preflight(PreflightContext const&) const = 0;
    virtual TER preclaim(PreclaimContext const&) const = 0;
    virtual ApplyResult doApply(ApplyContext&) const = 0;
    virtual XRPAmount calculateBaseFee(ReadView const&, STTx const&) const = 0;
    virtual TxConsequences makeTxConsequences(PreflightContext const&) const = 0;
    virtual Transactor::ConsequencesFactoryType consequencesFactory() const = 0;
};

template <typename T>
class TransactionHandlerAdapter : public ITransactionHandler {
public:
    NotTEC preflight(PreflightContext const& ctx) const override {
        return Transactor::invokePreflight<T>(ctx);
    }
    TER preclaim(PreclaimContext const& ctx) const override {
        return T::preclaim(ctx);
    }
    ApplyResult doApply(ApplyContext& ctx) const override {
        T transactor(ctx);
        return transactor();
    }
    // ... other methods
};
```

**Pros**:
- Clean interface, type-safe registration
- Easy to test via mock handlers
- No changes to existing transactor implementations
- O(1) lookup via unordered_map

**Cons**:
- Virtual dispatch overhead (~1-3 ns per call)
- Additional memory per handler type

**Verdict**: Recommended - best balance of maintainability and compatibility.

### 3.3 Option C: Full Virtual Interface

**Approach**: Make all transactor methods virtual, remove static methods entirely.

**Pros**: Simplest interface design.
**Cons**: Major refactoring of all transactors, breaks `invokePreflight` template.

**Verdict**: Not recommended - too invasive.

### 3.4 Performance Analysis

Transaction processing is I/O-bound (disk, network). Virtual dispatch overhead is:
- 1-3 nanoseconds per call
- 3-4 calls per transaction (preflight, preclaim, doApply, baseFee)
- ~12 ns total vs. 100μs+ for actual transaction processing

**Conclusion**: Performance impact is negligible (<0.01%).

### 3.5 Backward Compatibility

- **TxType enum**: Preserved, generated from simplified macro
- **Transactor classes**: No changes required to implementations
- **Wire protocol**: Unchanged (TxType values preserved)
- **API**: Unchanged (transaction names from jss::name)

---

## 4. Implementation Plan

### Step 1: Create ITransactionHandler Interface

**File**: `src/xrpld/app/tx/detail/ITransactionHandler.h` (new)

```cpp
#ifndef XRPL_APP_TX_ITRANSACTIONHANDLER_H_INCLUDED
#define XRPL_APP_TX_ITRANSACTIONHANDLER_H_INCLUDED

namespace xrpl {

class ITransactionHandler {
public:
    virtual ~ITransactionHandler() = default;

    // Phase 1: Preflight (before signature check)
    virtual NotTEC preflight(PreflightContext const& ctx) const = 0;

    // Phase 2: Preclaim (after signature verification)
    virtual TER preclaim(PreclaimContext const& ctx) const = 0;

    // Phase 3: Apply transaction to ledger
    virtual ApplyResult doApply(ApplyContext& ctx) const = 0;

    // Fee calculation
    virtual XRPAmount calculateBaseFee(ReadView const& view, STTx const& tx) const = 0;

    // Consequences factory
    virtual TxConsequences makeConsequences(PreflightContext const& ctx) const = 0;
    virtual Transactor::ConsequencesFactoryType consequencesFactory() const = 0;
};

}  // namespace xrpl
#endif
```

### Step 2: Create TransactionRegistry Singleton

**File**: `src/xrpld/app/tx/detail/TransactionRegistry.h` (new)

```cpp
#ifndef XRPL_APP_TX_TRANSACTIONREGISTRY_H_INCLUDED
#define XRPL_APP_TX_TRANSACTIONREGISTRY_H_INCLUDED

#include <unordered_map>
#include <memory>

namespace xrpl {

class TransactionRegistry {
public:
    static TransactionRegistry& instance();

    void registerHandler(TxType type, std::unique_ptr<ITransactionHandler> handler);
    ITransactionHandler const* getHandler(TxType type) const;
    bool hasHandler(TxType type) const;

    // Iteration for validation
    template <typename F>
    void forEach(F&& func) const;

private:
    TransactionRegistry() = default;
    std::unordered_map<TxType, std::unique_ptr<ITransactionHandler>> handlers_;
};

}  // namespace xrpl
#endif
```

**File**: `src/xrpld/app/tx/detail/TransactionRegistry.cpp` (new)

```cpp
#include "TransactionRegistry.h"

namespace xrpl {

TransactionRegistry& TransactionRegistry::instance() {
    static TransactionRegistry registry;
    return registry;
}

void TransactionRegistry::registerHandler(
    TxType type,
    std::unique_ptr<ITransactionHandler> handler)
{
    auto [it, inserted] = handlers_.emplace(type, std::move(handler));
    XRPL_ASSERT(inserted, "Duplicate transaction handler registration");
}

ITransactionHandler const* TransactionRegistry::getHandler(TxType type) const {
    auto it = handlers_.find(type);
    return it != handlers_.end() ? it->second.get() : nullptr;
}

bool TransactionRegistry::hasHandler(TxType type) const {
    return handlers_.count(type) > 0;
}

}  // namespace xrpl
```

### Step 3: Create CRTP Handler Adapter

**File**: `src/xrpld/app/tx/detail/TransactionHandlerAdapter.h` (new)

```cpp
template <typename T>
class TransactionHandlerAdapter : public ITransactionHandler {
public:
    NotTEC preflight(PreflightContext const& ctx) const override {
        return Transactor::invokePreflight<T>(ctx);
    }

    TER preclaim(PreclaimContext const& ctx) const override {
        // Replicate invoke_preclaim logic for type T
        auto const id = ctx.tx.getAccountID(sfAccount);
        if (id != beast::zero) {
            if (NotTEC const result = T::checkSeqProxy(ctx.view, ctx.tx, ctx.j))
                return result;
            if (NotTEC const result = T::checkPriorTxAndLastLedger(ctx))
                return result;
            if (NotTEC const result = T::checkPermission(ctx.view, ctx.tx))
                return result;
            if (NotTEC const result = T::checkSign(ctx))
                return result;
            if (TER const result = T::checkFee(ctx, calculateBaseFee(ctx.view, ctx.tx)))
                return result;
        }
        return T::preclaim(ctx);
    }

    ApplyResult doApply(ApplyContext& ctx) const override {
        T transactor(ctx);
        return transactor();
    }

    XRPAmount calculateBaseFee(ReadView const& view, STTx const& tx) const override {
        return T::calculateBaseFee(view, tx);
    }

    TxConsequences makeConsequences(PreflightContext const& ctx) const override {
        if constexpr (T::ConsequencesFactory == Transactor::Normal)
            return TxConsequences(ctx.tx);
        else if constexpr (T::ConsequencesFactory == Transactor::Blocker)
            return TxConsequences(ctx.tx, TxConsequences::blocker);
        else
            return T::makeTxConsequences(ctx);
    }

    Transactor::ConsequencesFactoryType consequencesFactory() const override {
        return T::ConsequencesFactory;
    }
};
```

### Step 4: Create Registration Macro

**File**: `src/xrpld/app/tx/detail/RegisterTransaction.h` (new)

```cpp
#define REGISTER_TRANSACTION(TxTypeName, TransactorClass) \
    namespace { \
        static bool registered_##TransactorClass = []() { \
            TransactionRegistry::instance().registerHandler( \
                TxTypeName, \
                std::make_unique<TransactionHandlerAdapter<TransactorClass>>()); \
            return true; \
        }(); \
    }
```

### Step 5: Update applySteps.cpp to Use Registry

Replace `with_txn_type()` switch with registry lookup:

```cpp
static std::pair<NotTEC, TxConsequences>
invoke_preflight(PreflightContext const& ctx) {
    auto const* handler = TransactionRegistry::instance()
        .getHandler(ctx.tx.getTxnType());
    if (!handler) {
        JLOG(ctx.j.fatal()) << "Unknown transaction type: " << ctx.tx.getTxnType();
        return {temUNKNOWN, TxConsequences{temUNKNOWN}};
    }

    auto const tec = handler->preflight(ctx);
    return {tec, isTesSuccess(tec) ? handler->makeConsequences(ctx)
                                   : TxConsequences{tec}};
}

static TER invoke_preclaim(PreclaimContext const& ctx) {
    auto const* handler = TransactionRegistry::instance()
        .getHandler(ctx.tx.getTxnType());
    if (!handler)
        return temUNKNOWN;
    return handler->preclaim(ctx);
}

static ApplyResult invoke_apply(ApplyContext& ctx) {
    auto const* handler = TransactionRegistry::instance()
        .getHandler(ctx.tx.getTxnType());
    if (!handler)
        return {temUNKNOWN, false};
    return handler->doApply(ctx);
}
```

### Step 6: Migrate Payment Transaction (Pilot)

**File**: `src/xrpld/app/tx/detail/Payment.cpp` (modify)

Add at end of file:
```cpp
#include <xrpld/app/tx/detail/RegisterTransaction.h>
REGISTER_TRANSACTION(ttPAYMENT, Payment)
```

Verify: Build, run tests, ensure Payment transactions work correctly.

### Step 7: Migrate Remaining Transactions

Migrate in groups by complexity:
1. **Simple transactions** (AccountSet, TrustSet, OfferCreate, etc.) - ~20 types
2. **NFT transactions** (NFTokenMint, NFTokenBurn, etc.) - 6 types
3. **AMM transactions** (AMMCreate, AMMDeposit, etc.) - 7 types
4. **Bridge transactions** (XChain*) - 8 types
5. **System transactions** (EnableAmendment, SetFee, UNLModify) - 3 types
6. **Remaining** - ~30 types

Each migration:
1. Add `REGISTER_TRANSACTION` macro to transactor .cpp file
2. Verify individual transaction tests pass
3. Commit as atomic change

### Step 8: Simplify transactions.macro

Reduce macro to only generate TxType enum and format fields:
```cpp
// Simplified: only tag, value, name, and fields needed
#define TRANSACTION(tag, value, name, fields) \
    /* Used by TxFormats.h for enum */ \
    /* Used by TxFormats.cpp for SOElement registration */
```

Remove `delegable`, `amendments`, `privileges` parameters - move to registry metadata.

### Step 9: Remove Old Macro Dispatch

Delete from applySteps.cpp:
- `with_txn_type()` template function
- All `#pragma push_macro`/`#pragma pop_macro` blocks
- `TRANSACTION_INCLUDE` header mechanism

---

## 5. Risk Assessment

### 5.1 Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Static initialization order | Medium | High | Use function-local static for registry singleton |
| Missing handler registration | Medium | High | Add runtime check in registry, fail-fast on missing |
| Performance regression | Low | Medium | Benchmark before/after; virtual dispatch is negligible |
| Memory corruption | Low | High | Use unique_ptr, no raw pointers |

### 5.2 Testing Gaps

1. **Registration order**: Ensure handlers registered before first use
2. **Edge cases**: Unknown TxType, null handler, duplicate registration
3. **Concurrency**: Registry is read-only after init; no locking needed
4. **Amendment interactions**: Test with various amendment combinations

### 5.3 Rollback Plan

1. Keep `transactions.macro` intact during migration
2. Gate new registry behind feature flag if needed
3. Can revert to macro-based dispatch by restoring `with_txn_type()`
4. Each transaction migration is atomic and reversible

---

## 6. Validation Criteria

### 6.1 Success Metrics

| Metric | Target |
|--------|--------|
| All existing transaction tests pass | 100% |
| No performance regression in transaction throughput | <1% overhead |
| Registry initialization before first transaction | Verified |
| All 75 transaction types registered | 100% |
| No compiler warnings | 0 warnings |

### 6.2 Performance Benchmarks

Run before and after migration:
```bash
# Transaction throughput test
./rippled --unittest=TransactionPerformance

# Individual transaction timing
./rippled --unittest=Payment_timing
./rippled --unittest=OfferCreate_timing
```

Expected overhead: <0.01% (virtual dispatch in I/O-bound code path).

### 6.3 Code Quality Metrics

- Lines of code in applySteps.cpp: Reduce from 461 to ~150
- Macro usage sites: Reduce from 4 patterns to 2 (enum + fields only)
- Cyclomatic complexity of `with_txn_type`: Eliminate entirely

### 6.4 Testing Requirements

1. **Unit tests**: Mock ITransactionHandler for isolated testing
2. **Integration tests**: All existing transaction tests must pass
3. **Regression tests**: No changes to transaction behavior
4. **Stress tests**: Registry lookup under high load

