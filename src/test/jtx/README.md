# JTx Testing Framework

The `jtx` (JSON Transaction) testing framework provides a fluent, expressive API for writing unit tests that exercise XRP Ledger transaction processing. It enables developers to create accounts, submit transactions, and verify ledger state changes with minimal boilerplate.

## Core Components

### Env (Environment)

`Env` is the central class that simulates a standalone XRP Ledger node. It manages:

- Account creation and funding
- Transaction submission
- Ledger advancement
- State queries and verification

```cpp
#include <test/jtx/Env.h>

class MyTest : public beast::unit_test::suite {
    void testExample() {
        using namespace jtx;

        // Create environment with all supported amendments
        Env env(*this);

        // Or with specific features enabled/disabled
        Env env2(*this, testable_amendments() - featureSomeFeature);
    }
};
```

### Account

`Account` represents a cryptographic identity with a name, public/secret keypair, and AccountID:

```cpp
Account alice("alice");                           // secp256k1 by default
Account bob("bob", KeyType::ed25519);            // ed25519 key
auto const gw = Account("gateway");              // Gateway account
auto const USD = gw["USD"];                      // IOU issued by gateway
```

### JTx (JSON Transaction)

`JTx` wraps a JSON transaction with execution context (expected results, auto-fill settings, signing requirements). Usually created implicitly via `env()` or `env.jt()`.

## Common Testing Patterns

### Creating and Funding Accounts

```cpp
Env env(*this);
auto const alice = Account("alice");
auto const bob = Account("bob");

// Fund accounts with XRP (10,000 XRP each)
env.fund(XRP(10000), alice, bob);

// Fund without setting defaultRipple flag
env.fund(XRP(10000), noripple(alice));
```

### Submitting Transactions

Use the `env()` operator to submit transactions with optional modifiers (funclets):

```cpp
// Simple payment
env(pay(alice, bob, XRP(100)));

// Payment with expected error
env(pay(alice, bob, XRP(1000000)), ter(tecUNFUNDED_PAYMENT));

// Payment with explicit fee and sequence
env(pay(alice, bob, XRP(50)), fee(drops(15)), seq(5));

// IOU payment through trust line
env(trust(alice, USD(1000)));              // Create trust line
env(pay(gw, alice, USD(100)));             // Send IOU
```

### Verifying State with require()

```cpp
// Check balances
env.require(balance(alice, XRP(9900)));
env.require(balance(alice, USD(100)));

// Check account flags
env.require(flags(alice, asfDefaultRipple));
env.require(nflags(alice, asfDisableMaster));

// Check owner counts
env.require(owners(alice, 2));             // Total owned objects
env.require(lines(alice, 1));              // Trust lines only

// Inline requirements in transaction
env(pay(gw, alice, USD(50)), require(balance(alice, USD(150))));
```

### Working with Ledgers

```cpp
env.close();                               // Close ledger, advance time by 5s
env.close(std::chrono::seconds(10));       // Close with custom time delta

auto const current = env.current();        // Current open ledger
auto const closed = env.closed();          // Last closed ledger
```

## Transaction Helpers

| Helper                              | Description                  |
| ----------------------------------- | ---------------------------- |
| `pay(from, to, amount)`             | Create a Payment transaction |
| `trust(account, amount)`            | Create/modify a trust line   |
| `offer(account, pays, gets)`        | Create an offer              |
| `offer_cancel(account, seq)`        | Cancel an offer              |
| `fset(account, flag)`               | Set account flag             |
| `fclear(account, flag)`             | Clear account flag           |
| `noop(account)`                     | AccountSet with no changes   |
| `regkey(account, key)`              | Set regular key              |
| `signers(account, quorum, signers)` | Set up multisig              |

## Funclets (Transaction Modifiers)

Funclets modify transaction properties before submission:

| Funclet                    | Description                     |
| -------------------------- | ------------------------------- |
| `ter(code)`                | Set expected transaction result |
| `fee(amount)`              | Set transaction fee             |
| `seq(n)`                   | Set sequence number             |
| `sig(account)`             | Sign with specific account      |
| `msig(accounts...)`        | Multi-sign transaction          |
| `memo(data, format, type)` | Add memo                        |
| `require(conditions...)`   | Add post-conditions             |

## Amounts

```cpp
XRP(100)                    // 100 XRP
drops(1000000)              // 1 XRP in drops
gw["USD"](100)              // 100 USD from gateway
any(USD(100))               // Any issuer acceptable
```

## Writing a New Test

```cpp
#include <test/jtx/Env.h>
#include <xrpl/beast/unit_test.h>
#include <xrpl/protocol/Feature.h>

namespace xrpl {
namespace test {

class MyFeature_test : public beast::unit_test::suite {
    void testBasicFunctionality() {
        testcase("Basic functionality");
        using namespace jtx;

        Env env(*this);
        auto const alice = Account("alice");
        auto const bob = Account("bob");

        env.fund(XRP(10000), alice, bob);
        env.close();

        // Test your feature
        env(pay(alice, bob, XRP(100)));
        env.require(balance(bob, XRP(10100)));

        // Test error case
        env(pay(alice, bob, XRP(1000000)), ter(tecUNFUNDED_PAYMENT));
    }

    void testWithSpecificAmendments() {
        testcase("With specific amendments");
        using namespace jtx;

        // Test with feature disabled
        Env env(*this, testable_amendments() - featureMyNewFeature);
        // ... test legacy behavior
    }

    void run() override {
        testBasicFunctionality();
        testWithSpecificAmendments();
    }
};

BEAST_DEFINE_TESTSUITE(MyFeature, app, ripple);

}  // namespace test
}  // namespace xrpl
```
