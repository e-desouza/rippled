# Feature Development Guide

A practical guide for adding new features to rippled.

## Getting Started

### Prerequisites

- **C++20** compiler (GCC 13+ or Clang 16+)
- **CMake** 3.25+ with Ninja generator
- **Conan** 2.x package manager

### Building the Project

```bash
cd .build
cmake /path/to/rippled -G Ninja -DCMAKE_BUILD_TYPE=Debug -Dxrpld=ON
distcc-pump ninja -j20   # See .augment-guidelines for distcc setup
```

### Running Tests

```bash
./rippled --unittest                    # All tests
./rippled --unittest=MyFeature          # Specific suite
./rippled --unittest=MyFeature --unittest-log  # Verbose
```

---

## Adding a New Transaction Type

### Step 1: Define in transactions.macro

Edit `include/xrpl/protocol/detail/transactions.macro`:

```cpp
#if TRANSACTION_INCLUDE
#   include <xrpld/app/tx/detail/MyTransaction.h>
#endif
TRANSACTION(ttMY_TRANSACTION, 99, MyTransaction,
    Delegation::notDelegable,
    uint256{},      // Amendment hash (or featureMyFeature)
    none,           // Privileges
    ({
    {sfDestination, soeREQUIRED},
    {sfAmount, soeREQUIRED},
}))
```

### Step 2: Implement the Transactor

Create `src/xrpld/app/tx/detail/MyTransaction.h`:

```cpp
#include <xrpld/app/tx/detail/Transactor.h>

namespace xrpl {
class MyTransaction : public Transactor {
public:
    static constexpr ConsequencesFactoryType ConsequencesFactory{Normal};
    explicit MyTransaction(ApplyContext& ctx) : Transactor(ctx) {}

    static NotTEC preflight(PreflightContext const& ctx);
    static TER preclaim(PreclaimContext const& ctx);
    TER doApply() override;
};
}  // namespace xrpl
```

Create `src/xrpld/app/tx/detail/MyTransaction.cpp`:

```cpp
NotTEC MyTransaction::preflight(PreflightContext const& ctx) {
    if (auto ret = preflight1(ctx); !isTesSuccess(ret)) return ret;
    if (ctx.tx[sfAmount] <= beast::zero) return temBAD_AMOUNT;
    return preflight2(ctx);
}

TER MyTransaction::preclaim(PreclaimContext const& ctx) {
    if (!ctx.view.exists(keylet::account(ctx.tx[sfDestination])))
        return tecNO_TARGET;
    return tesSUCCESS;
}

TER MyTransaction::doApply() {
    auto sle = view().peek(keylet::account(account_));
    if (!sle) return tefINTERNAL;
    // Modify ledger state...
    return tesSUCCESS;
}
```

---

## Adding a New RPC Method

### Step 1: Create the Handler

Create `src/xrpld/rpc/handlers/MyHandler.cpp`:

```cpp
#include <xrpld/rpc/Context.h>
#include <xrpl/protocol/jss.h>

namespace xrpl {

Json::Value doMyHandler(RPC::JsonContext& context) {
    auto& params = context.params;
    if (!params.isMember(jss::account))
        return RPC::missing_field_error(jss::account);

    std::shared_ptr<ReadView const> ledger;
    auto result = RPC::lookupLedger(ledger, context);
    if (!ledger) return result;

    result[jss::status] = jss::success;
    return result;
}

}  // namespace xrpl
```

### Step 2: Declare and Register

Add to `src/xrpld/rpc/handlers/Handlers.h`:

```cpp
Json::Value doMyHandler(RPC::JsonContext&);
```

Add to `handlerArray` in `src/xrpld/rpc/detail/Handler.cpp`:

```cpp
{"my_handler", byRef(&doMyHandler), Role::USER, NO_CONDITION},
```

---

## Adding a New Ledger Object Type

### Step 1: Define in ledger_entries.macro

Edit `include/xrpl/protocol/detail/ledger_entries.macro`:

```cpp
LEDGER_ENTRY(ltMY_OBJECT, 0x0088, MyObject, myObject, ({
    {sfAccount,           soeREQUIRED},
    {sfOwnerNode,         soeREQUIRED},
    {sfPreviousTxnID,     soeREQUIRED},
    {sfPreviousTxnLgrSeq, soeREQUIRED},
    {sfMyField,           soeOPTIONAL},
}))
```

### Step 2: Add Keylet Function

Add to `include/xrpl/protocol/Indexes.h`:

```cpp
Keylet myObject(AccountID const& owner, uint256 const& key) noexcept;
```

Implement in `src/libxrpl/protocol/Indexes.cpp`:

```cpp
Keylet myObject(AccountID const& owner, uint256 const& key) noexcept {
    return {ltMY_OBJECT, indexHash(LedgerNameSpace::MY_OBJECT, owner, key)};
}
```

---

## Testing Guidelines

See `src/test/jtx/README.md` for complete jtx framework documentation.

### Basic Test Structure

```cpp
#include <test/jtx.h>

class MyFeature_test : public beast::unit_test::suite {
    void testBasic() {
        testcase("Basic");
        using namespace jtx;

        Env env(*this);
        Account alice("alice"), bob("bob");
        env.fund(XRP(10000), alice, bob);
        env.close();

        env(pay(alice, bob, XRP(100)));
        env.require(balance(bob, XRP(10100)));
    }

    void testErrors() {
        testcase("Errors");
        using namespace jtx;
        Env env(*this);
        env(pay(Account("alice"), Account("bob"), XRP(100)), ter(tecNO_DST));
    }

    void run() override { testBasic(); testErrors(); }
};

BEAST_DEFINE_TESTSUITE(MyFeature, app, xrpl);
```

### Running Specific Tests

```bash
./rippled --unittest=MyFeature
./rippled --unittest=MyFeature.Basic   # Specific testcase
```

---

## Code Style

### C++20 Features Used

- Concepts and constraints
- Designated initializers
- `std::span`, `std::ranges`
- `constexpr` improvements

### Naming Conventions

- **Classes**: `PascalCase` (e.g., `MyTransaction`)
- **Functions**: `camelCase` (e.g., `doApply`)
- **Constants**: `camelCase` or `kPascalCase`
- **Member variables**: `camelCase_` with trailing underscore

### clang-format

Use pre-commit hooks:

```bash
pre-commit run clang-format --files src/xrpld/app/tx/detail/MyTransaction.cpp
```

---

## Common Patterns

### Error Handling (TER Codes)

| Prefix | Use Case                          |
| ------ | --------------------------------- |
| `tem`  | Malformed transaction (preflight) |
| `tef`  | Failure, fee not claimed          |
| `ter`  | Retry later                       |
| `tec`  | Claimed fee, but failed           |
| `tes`  | Success                           |

### Logging (beast::Journal)

```cpp
JLOG(ctx.j.trace()) << "Debug: " << value;
JLOG(ctx.j.warn()) << "Warning condition";
JLOG(j_.error()) << "Error in doApply";
```

### Configuration

Access config in RPC handlers:

```cpp
auto const& cfg = context.app.config();
```

---

## Further Reading

- [jtx Testing Framework](../src/test/jtx/README.md)
- [XRP Ledger Docs](https://xrpl.org/docs.html)
- [Amendment Process](https://xrpl.org/amendments.html)
