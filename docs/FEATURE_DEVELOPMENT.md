# Feature Development Guide

A practical guide for adding new features to rippled.

## Table of Contents

1. [Adding a New Transaction Type](#adding-a-new-transaction-type)
2. [Adding a New RPC Handler](#adding-a-new-rpc-handler)
3. [Adding an Amendment](#adding-an-amendment)
4. [Best Practices](#best-practices)
5. [Common Pitfalls](#common-pitfalls)
6. [Reference](#reference)

---

## Adding a New Transaction Type

### Overview

Adding a new transaction type involves five steps:

1. Choose an available transaction type number
2. Create the transactor header file
3. Create the transactor implementation
4. Register in transactions.macro
5. Write comprehensive tests

### Step 1: Choose a Transaction Type Number

Check `include/xrpl/protocol/detail/transactions.macro` for available numbers. Transaction type numbers are assigned sequentially, but gaps may exist. Choose the next available number that isn't already in use.

```cpp
// Example entries in transactions.macro:
// ttPAYMENT = 0
// ttESCROW_CREATE = 1
// ttESCROW_FINISH = 2
// ...
// ttDEPOSIT_PREAUTH = 19
```

### Step 2: Create the Transactor Header

Location: `src/xrpld/app/tx/detail/MyTransaction.h`

```cpp
#ifndef XRPL_TX_MY_TRANSACTION_H_INCLUDED
#define XRPL_TX_MY_TRANSACTION_H_INCLUDED

#include <xrpld/app/tx/detail/Transactor.h>

namespace xrpl {

class MyTransaction : public Transactor
{
public:
    // ConsequencesFactory determines fee calculation behavior:
    // - Normal: Standard fee calculation
    // - Blocker: Transaction blocks others (e.g., AccountDelete)
    // - Custom: Requires makeTxConsequences() implementation
    static constexpr ConsequencesFactoryType ConsequencesFactory{Normal};

    explicit MyTransaction(ApplyContext& ctx) : Transactor(ctx)
    {
    }

    // Optional: Override if transaction requires specific amendment(s)
    // beyond what's specified in transactions.macro
    static bool
    checkExtraFeatures(PreflightContext const& ctx);

    // Optional: Override if transaction uses any flags
    static std::uint32_t
    getFlagsMask(PreflightContext const& ctx);

    // Required: Stateless validation of transaction fields
    static NotTEC
    preflight(PreflightContext const& ctx);

    // Optional: Validate against current ledger state (no modifications)
    static TER
    preclaim(PreclaimContext const& ctx);

    // Required: Apply the transaction to the ledger
    TER
    doApply() override;
};

}  // namespace xrpl

#endif
```

### Step 3: Create the Implementation

Location: `src/xrpld/app/tx/detail/MyTransaction.cpp`

```cpp
#include <xrpld/app/tx/detail/MyTransaction.h>

#include <xrpl/basics/Log.h>
#include <xrpl/ledger/View.h>
#include <xrpl/protocol/Feature.h>
#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/TxFlags.h>

namespace xrpl {

bool
MyTransaction::checkExtraFeatures(PreflightContext const& ctx)
{
    // Return false if required feature is not enabled
    // This prevents the transaction from being processed
    if (!ctx.rules.enabled(featureMyFeature))
        return false;
    return true;
}

std::uint32_t
MyTransaction::getFlagsMask(PreflightContext const& ctx)
{
    // Return bitmask of valid flags for this transaction
    // tfUniversalMask is the default (no custom flags)
    return tfMyTransactionMask;
}

NotTEC
MyTransaction::preflight(PreflightContext const& ctx)
{
    // 1. Validate transaction fields (stateless checks)
    // 2. Check required fields are present
    // 3. Verify field values are within valid ranges

    if (!ctx.tx.isFieldPresent(sfRequiredField))
    {
        JLOG(ctx.j.trace()) << "Malformed transaction: Missing required field";
        return temMALFORMED;
    }

    // Validate field values
    auto const value = ctx.tx[sfSomeField];
    if (value <= 0)
    {
        JLOG(ctx.j.trace()) << "Malformed transaction: Invalid value";
        return temBAD_AMOUNT;
    }

    return tesSUCCESS;
}

TER
MyTransaction::preclaim(PreclaimContext const& ctx)
{
    // 1. Check ledger state (read-only)
    // 2. Verify objects exist in ledger
    // 3. Check for duplicates or conflicts

    AccountID const account(ctx.tx[sfAccount]);

    // Check if target object exists
    if (!ctx.view.exists(keylet::account(ctx.tx[sfDestination])))
        return tecNO_TARGET;

    // Check for duplicates
    if (ctx.view.exists(keylet::myObject(account, someKey)))
        return tecDUPLICATE;

    return tesSUCCESS;
}

TER
MyTransaction::doApply()
{
    // 1. Modify ledger state
    // 2. Create/delete ledger objects
    // 3. Update account balances and reserves

    auto const sleOwner = view().peek(keylet::account(account_));
    if (!sleOwner)
        return tefINTERNAL;

    // Check reserve requirements
    auto const reserve = view().fees().accountReserve(
        sleOwner->getFieldU32(sfOwnerCount) + 1);

    if (mPriorBalance < reserve)
        return tecINSUFFICIENT_RESERVE;

    // Create new ledger object
    auto const sleNew = std::make_shared<SLE>(keylet::myObject(account_, key));
    sleNew->setAccountID(sfAccount, account_);
    sleNew->setFieldU32(sfMyField, value);
    view().insert(sleNew);

    // Update owner count
    adjustOwnerCount(view(), sleOwner, 1, ctx_.journal);

    return tesSUCCESS;
}

}  // namespace xrpl
```

### Step 4: Register in transactions.macro

Add your transaction to `include/xrpl/protocol/detail/transactions.macro`:

```cpp
#if TRANSACTION_INCLUDE
#   include <xrpld/app/tx/detail/MyTransaction.h>
#endif
TRANSACTION(ttMY_TRANSACTION, 50, MyTransaction,
    Delegation::delegable,      // or Delegation::notDelegable
    featureMyFeature,           // Amendment ID (uint256{} if no amendment)
    noPriv,                     // Privilege flags (noPriv, createAcct, etc.)
    ({
    {sfDestination, soeREQUIRED},
    {sfAmount, soeREQUIRED},
    {sfOptionalField, soeOPTIONAL},
    {sfDefaultField, soeDEFAULT},  // Field with default value
}))
```

**Field Requirements:**
- `soeREQUIRED` - Field must be present
- `soeOPTIONAL` - Field may be present
- `soeDEFAULT` - Field has a default value

**Delegation:**
- `Delegation::delegable` - Can be delegated to another account
- `Delegation::notDelegable` - Cannot be delegated (e.g., AccountSet)

### Step 5: Add Tests

Location: `src/test/app/MyTransaction_test.cpp`

```cpp
#include <test/jtx.h>
#include <xrpl/protocol/Feature.h>
#include <xrpl/protocol/jss.h>

namespace xrpl {
namespace test {

class MyTransaction_test : public beast::unit_test::suite
{
    // Helper function for reserve calculations
    static XRPAmount
    reserve(jtx::Env& env, std::uint32_t count)
    {
        return env.current()->fees().accountReserve(count);
    }

    void
    testBasicFunctionality()
    {
        testcase("Basic Functionality");

        using namespace jtx;
        Account const alice{"alice"};
        Account const bob{"bob"};

        Env env(*this);
        env.fund(XRP(10000), alice, bob);
        env.close();

        // Test successful transaction
        env(my_tx(alice, bob, XRP(100)));
        env.close();

        // Verify ledger state changed
        BEAST_EXPECT(env.balance(bob) == XRP(10100));
    }

    void
    testErrorCases()
    {
        testcase("Error Cases");

        using namespace jtx;
        Account const alice{"alice"};

        Env env(*this);
        env.fund(XRP(10000), alice);
        env.close();

        // Test invalid destination
        env(my_tx(alice, Account{"nobody"}, XRP(100)),
            ter(tecNO_TARGET));

        // Test insufficient reserve
        Account const broke{"broke"};
        env.fund(reserve(env, 0), broke);
        env.close();
        env(my_tx(broke, alice, XRP(1)),
            ter(tecINSUFFICIENT_RESERVE));
    }

    void
    testWithAmendment()
    {
        testcase("With Amendment");

        using namespace jtx;
        Account const alice{"alice"};

        // Test without amendment
        {
            Env env(*this, supported_amendments() - featureMyFeature);
            env.fund(XRP(10000), alice);
            env.close();

            // Transaction should fail without amendment
            env(my_tx(alice, alice, XRP(100)),
                ter(temDISABLED));
        }

        // Test with amendment
        {
            Env env(*this);
            env.fund(XRP(10000), alice);
            env.close();

            env(my_tx(alice, alice, XRP(100)));
            env.close();
            BEAST_EXPECT(/* verify success */);
        }
    }

public:
    void
    run() override
    {
        testBasicFunctionality();
        testErrorCases();
        testWithAmendment();
    }
};

BEAST_DEFINE_TESTSUITE(MyTransaction, app, xrpl);

}  // namespace test
}  // namespace xrpl
```

---

## Adding a New RPC Handler

### Handler Structure

RPC handlers process JSON-RPC requests and return JSON responses.

Location: `src/xrpld/rpc/handlers/MyHandler.cpp`

```cpp
#include <xrpld/rpc/Context.h>
#include <xrpld/rpc/Role.h>
#include <xrpld/rpc/detail/RPCHelpers.h>

#include <xrpl/json/json_value.h>
#include <xrpl/protocol/jss.h>

namespace xrpl {

// Declare in Handlers.h as well
Json::Value
doMyHandler(RPC::JsonContext& context)
{
    auto const& params = context.params;
    Json::Value result(Json::objectValue);

    // 1. Validate required parameters
    if (!params.isMember(jss::account))
        return RPC::missing_field_error(jss::account);

    if (!params[jss::account].isString())
        return RPC::invalid_field_error(jss::account);

    // 2. Parse and validate account
    auto const accountID = parseBase58<AccountID>(
        params[jss::account].asString());
    if (!accountID)
        return RPC::make_error(rpcACT_MALFORMED);

    // 3. Get ledger if needed
    std::shared_ptr<ReadView const> ledger;
    auto lookupResult = RPC::lookupLedger(ledger, context);
    if (!ledger)
        return lookupResult;

    // 4. Check account exists
    if (!ledger->exists(keylet::account(*accountID)))
        return rpcError(rpcACT_NOT_FOUND);

    // 5. Process and return results
    result[jss::account] = params[jss::account];
    result[jss::status] = jss::success;

    // Access role-specific features
    if (context.role == Role::ADMIN)
    {
        result[jss::admin_data] = "sensitive info";
    }

    return result;
}

}  // namespace xrpl
```

### Declaration in Handlers.h

Add the function declaration to `src/xrpld/rpc/handlers/Handlers.h`:

```cpp
Json::Value
doMyHandler(RPC::JsonContext&);
```

### Registration in Handler.cpp

Register the handler in `src/xrpld/rpc/detail/Handler.cpp`:

```cpp
Handler const handlerArray[]{
    // ... existing handlers ...
    {"my_handler", byRef(&doMyHandler), Role::USER, NO_CONDITION},
    // ... more handlers ...
};
```

**Handler Parameters:**
- `name` - RPC method name (string)
- `method` - Handler function wrapped with `byRef()`
- `role` - Required role: `Role::USER`, `Role::ADMIN`, `Role::IDENTIFIED`, `Role::PROXY`
- `condition` - `NO_CONDITION`, `NEEDS_CURRENT_LEDGER`, `NEEDS_CLOSED_LEDGER`

### Testing RPC Handlers

Location: `src/test/rpc/MyHandler_test.cpp`

```cpp
#include <test/jtx.h>
#include <xrpl/protocol/jss.h>

namespace xrpl {
namespace test {

class MyHandler_test : public beast::unit_test::suite
{
    void
    testBasic()
    {
        testcase("Basic");

        using namespace jtx;
        Env env(*this);
        Account const alice{"alice"};
        env.fund(XRP(10000), alice);
        env.close();

        // Call RPC
        auto const result = env.rpc("my_handler",
            "{ \"account\": \"" + alice.human() + "\" }");

        BEAST_EXPECT(result[jss::status] == jss::success);
        BEAST_EXPECT(result.isMember(jss::account));
    }

    void
    testErrors()
    {
        testcase("Errors");

        using namespace jtx;
        Env env(*this);

        // Missing account
        auto const result1 = env.rpc("my_handler", "{}");
        BEAST_EXPECT(result1[jss::error] == "invalidParams");

        // Invalid account
        auto const result2 = env.rpc("my_handler",
            "{ \"account\": \"not_an_account\" }");
        BEAST_EXPECT(result2[jss::error] == "actMalformed");
    }

public:
    void
    run() override
    {
        testBasic();
        testErrors();
    }
};

BEAST_DEFINE_TESTSUITE(MyHandler, rpc, xrpl);

}  // namespace test
}  // namespace xrpl
```

---

## Adding an Amendment

Amendments control feature activation on the XRP Ledger network.

### Amendment Macros

Amendments are defined in `include/xrpl/protocol/detail/features.macro`:

```cpp
// New feature (not a bug fix)
XRPL_FEATURE(MyFeature, Supported::yes, VoteBehavior::DefaultNo)

// Bug fix
XRPL_FIX(MyBugFix, Supported::yes, VoteBehavior::DefaultNo)
```

**Parameters:**

1. **Name** - Amendment identifier (becomes `featureMyFeature` or `fixMyBugFix`)
2. **Supported** - Whether this build supports the feature:
   - `Supported::yes` - Feature is fully implemented
   - `Supported::no` - Feature is in development (cannot be voted on)
3. **VoteBehavior** - Default voting behavior:
   - `VoteBehavior::DefaultNo` - Validators don't vote by default
   - `VoteBehavior::DefaultYes` - Validators vote yes by default

### Amendment Lifecycle

```
Development → Ready → Voting → Active → Retired
```

| Phase | Supported | VoteBehavior | Description |
|-------|-----------|--------------|-------------|
| Development | `Supported::no` | `DefaultNo` | Feature under development |
| Ready | `Supported::yes` | `DefaultNo` | Feature complete, awaiting community review |
| Voting | `Supported::yes` | `DefaultYes` | Feature ready for network activation |
| Active | N/A | N/A | Feature enabled on network |
| Retired | N/A | N/A | Use `XRPL_RETIRE_FEATURE` / `XRPL_RETIRE_FIX` |

### Retiring Amendments

Once an amendment has been active for at least two years:

```cpp
// Remove from active section, add to retired section
XRPL_RETIRE_FEATURE(OldFeatureName)
XRPL_RETIRE_FIX(OldFixName)
```

### Gating Code on Amendments

Always check if an amendment is enabled before using gated functionality:

```cpp
// In preflight (stateless check)
bool
MyTransaction::checkExtraFeatures(PreflightContext const& ctx)
{
    if (!ctx.rules.enabled(featureMyFeature))
        return false;
    return true;
}

// In preclaim or doApply (with view access)
TER
MyTransaction::preclaim(PreclaimContext const& ctx)
{
    if (ctx.view.rules().enabled(featureMyFeature))
    {
        // New behavior
    }
    else
    {
        // Old behavior (or return temDISABLED)
    }
    return tesSUCCESS;
}

// In doApply
TER
MyTransaction::doApply()
{
    if (view().rules().enabled(featureMyFeature))
    {
        // New code path
    }
    return tesSUCCESS;
}
```

### Testing with Amendments

```cpp
void
testWithAndWithoutAmendment()
{
    using namespace jtx;

    // Without amendment
    {
        Env env(*this, supported_amendments() - featureMyFeature);
        // Test old behavior
    }

    // With amendment (default)
    {
        Env env(*this);  // All supported amendments enabled
        // Test new behavior
    }
}
```

---

## Best Practices

### Error Handling

Use appropriate TER (Transaction Engine Result) codes:

```cpp
// Malformed transaction (preflight)
return temMALFORMED;      // Generic malformed
return temBAD_AMOUNT;     // Invalid amount
return temBAD_FEE;        // Invalid fee
return temDISABLED;       // Feature disabled

// Transaction Engine Claim (preclaim/doApply)
return tecNO_TARGET;      // Target doesn't exist
return tecDUPLICATE;      // Object already exists
return tecNO_ENTRY;       // Object not found
return tecINSUFFICIENT_RESERVE;  // Not enough XRP

// Transaction Engine Failure (preclaim/doApply)
return tefINTERNAL;       // Internal error
```

### Logging with JLOG

Use appropriate log levels:

```cpp
// In preflight/preclaim (use ctx.j)
JLOG(ctx.j.trace()) << "Debug message";
JLOG(ctx.j.warn()) << "Warning: unusual condition";
JLOG(ctx.j.error()) << "Error: operation failed";

// In doApply (use j_ member)
JLOG(j_.trace()) << "Processing transaction for " << account_;
JLOG(j_.info()) << "Successfully created object";
```

### Reserve Calculations

Always check reserve requirements before creating objects:

```cpp
TER
MyTransaction::doApply()
{
    auto const sleOwner = view().peek(keylet::account(account_));
    std::uint32_t const ownerCount = sleOwner->getFieldU32(sfOwnerCount);

    // Calculate reserve for one more object
    auto const reserve = view().fees().accountReserve(ownerCount + 1);

    if (mPriorBalance < reserve)
        return tecINSUFFICIENT_RESERVE;

    // Create object and update owner count
    adjustOwnerCount(view(), sleOwner, 1, ctx_.journal);
    return tesSUCCESS;
}
```

### Performance Considerations

1. **Minimize ledger reads** - Cache SLE pointers when reading multiple times
2. **Batch writes** - Apply multiple modifications before returning
3. **Early validation** - Check cheap conditions before expensive ones
4. **Use keylets** - Efficient object lookups with `keylet::xxx()`

---

## Common Pitfalls

### Thread Safety Issues

```cpp
// WRONG: Storing ledger objects across async boundaries
std::shared_ptr<SLE> cached_sle;  // Don't cache between calls!

// RIGHT: Fetch fresh data each time
auto const sle = view().peek(keylet::account(account_));
```

### Ledger State Access Patterns

```cpp
// WRONG: Using read() then modifying
auto const sle = view().read(keylet::account(account_));
sle->setFieldU32(sfFlags, newFlags);  // Won't persist!

// RIGHT: Use peek() for modifications
auto const sle = view().peek(keylet::account(account_));
sle->setFieldU32(sfFlags, newFlags);  // Changes will be applied
```

### Amendment Dependency Chains

If your feature depends on another amendment:

```cpp
bool
MyTransaction::checkExtraFeatures(PreflightContext const& ctx)
{
    // Check all required amendments
    if (!ctx.rules.enabled(featureRequiredFeature))
        return false;
    if (!ctx.rules.enabled(featureMyFeature))
        return false;
    return true;
}
```

### Owner Count Mismatches

```cpp
// WRONG: Forgetting to update owner count
view().insert(newSLE);  // Object created but owner count unchanged!

// RIGHT: Always adjust owner count when creating/deleting objects
view().insert(newSLE);
adjustOwnerCount(view(), sleOwner, 1, ctx_.journal);  // +1 for creation

// When deleting
view().erase(existingSLE);
adjustOwnerCount(view(), sleOwner, -1, ctx_.journal);  // -1 for deletion
```

### Incorrect Field Access

```cpp
// WRONG: Using [] without checking field presence
auto const value = tx[sfOptionalField];  // May throw!

// RIGHT: Use optional access for optional fields
auto const value = tx[~sfOptionalField];  // Returns std::optional
if (value)
{
    // Use *value
}

// Or check presence first
if (tx.isFieldPresent(sfOptionalField))
{
    auto const value = tx[sfOptionalField];
}
```

---

## Reference

### Key Files by Feature Type

#### Transaction Development

| File | Purpose |
|------|---------|
| `include/xrpl/protocol/detail/transactions.macro` | Transaction type registration |
| `src/xrpld/app/tx/detail/Transactor.h` | Base transactor class |
| `src/xrpld/app/tx/detail/Transactor.cpp` | Base transactor implementation |
| `include/xrpl/protocol/TER.h` | Transaction result codes |
| `include/xrpl/protocol/TxFlags.h` | Transaction flags |
| `include/xrpl/protocol/Indexes.h` | Keylet definitions |

#### RPC Development

| File | Purpose |
|------|---------|
| `src/xrpld/rpc/handlers/Handlers.h` | Handler declarations |
| `src/xrpld/rpc/detail/Handler.cpp` | Handler registration |
| `src/xrpld/rpc/Context.h` | RPC context types |
| `include/xrpl/protocol/ErrorCodes.h` | RPC error codes |
| `include/xrpl/protocol/jss.h` | JSON field names |

#### Amendment Development

| File | Purpose |
|------|---------|
| `include/xrpl/protocol/detail/features.macro` | Amendment registration |
| `include/xrpl/protocol/Feature.h` | Feature declarations |
| `src/xrpl/protocol/Feature.cpp` | Feature implementation |

#### Testing

| File | Purpose |
|------|---------|
| `src/test/jtx.h` | Test framework main header |
| `src/test/jtx/Env.h` | Test environment |
| `src/test/jtx/Account.h` | Test accounts |
| `src/test/app/*.cpp` | Transaction tests |
| `src/test/rpc/*.cpp` | RPC tests |

### Useful Test Helpers

```cpp
// Create test environment
jtx::Env env(*this);

// Fund accounts
env.fund(XRP(10000), alice);
env.close();  // Close ledger

// Submit transaction
env(pay(alice, bob, XRP(100)));

// Check for specific result
env(pay(alice, bob, XRP(100)), ter(tecNO_TARGET));

// Get account balance
auto balance = env.balance(alice);
auto balance_usd = env.balance(alice, USD);

// Access ledger entry
auto const sle = env.le(alice);

// RPC call
auto result = env.rpc("account_info",
    "{ \"account\": \"" + alice.human() + "\" }");

// Test with specific amendments
Env env(*this, supported_amendments() - featureX);
```

### Transaction Result Code Categories

| Prefix | Category | Description |
|--------|----------|-------------|
| `tes` | Success | Transaction succeeded |
| `tec` | Claimed | Fee claimed, but transaction failed |
| `tef` | Failure | Transaction failed, no fee claimed |
| `tel` | Local | Local error (not submitted to network) |
| `tem` | Malformed | Malformed transaction |
| `ter` | Retry | Retry transaction later |

---

## Further Reading

- [XRP Ledger Developer Documentation](https://xrpl.org/docs.html)
- [Transaction Reference](https://xrpl.org/transaction-types.html)
- [Amendment Process](https://xrpl.org/amendments.html)
- [rippled Source Code](https://github.com/XRPLF/rippled)

