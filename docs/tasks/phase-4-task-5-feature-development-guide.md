# Phase 4 Task 4.5: Feature Development Guide

## 1. Overview

### 1.1 Problem Statement

Adding new features to rippled (transactions, RPC handlers, amendments) requires understanding multiple interconnected modules and following specific patterns that are not formally documented. New developers face a steep learning curve, often resorting to cargo-cult coding from existing implementations without understanding the underlying architecture.

### 1.2 Success Criteria

- Developers can add a new transaction type by following the guide
- Developers can add a new RPC handler by following the guide
- Developers can add a new amendment by following the guide
- All examples in the guide compile and work correctly
- Reduced onboarding time for feature developers

### 1.3 Target Audience

- New rippled developers adding XRPL protocol features
- Core developers implementing new transaction types
- Integration developers creating new RPC endpoints

---

## 2. Deep Code Analysis

### 2.1 Transaction Registration System

Transactions are registered using a macro-based system in `include/xrpl/protocol/detail/transactions.macro`.

**TRANSACTION Macro Format:**
```cpp
TRANSACTION(tag, value, name, delegable, amendments, privileges, fields)
```

**Parameters:**
- `tag`: Transaction type enum (e.g., `ttPAYMENT`)
- `value`: Numeric transaction type identifier (must be unique)
- `name`: Transactor class name (e.g., `Payment`)
- `delegable`: Whether transaction can be delegated (`Delegation::delegable` or `Delegation::notDelegable`)
- `amendments`: Amendment feature hash (e.g., `featureAMM`, or `uint256{}` for no amendment)
- `privileges`: Privilege flags (e.g., `noPriv`, `createAcct`, `mustDeleteAcct`)
- `fields`: Transaction-specific fields with their requirements

**Example from codebase:**
```cpp
#if TRANSACTION_INCLUDE
#   include <xrpld/app/tx/detail/Payment.h>
#endif
TRANSACTION(ttPAYMENT, 0, Payment,
    Delegation::delegable,
    uint256{},
    createAcct,
    ({
    {sfDestination, soeREQUIRED},
    {sfAmount, soeREQUIRED, soeMPTSupported},
    {sfSendMax, soeOPTIONAL, soeMPTSupported},
    {sfPaths, soeDEFAULT},
    {sfInvoiceID, soeOPTIONAL},
    {sfDestinationTag, soeOPTIONAL},
    {sfDeliverMin, soeOPTIONAL, soeMPTSupported},
    {sfCredentialIDs, soeOPTIONAL},
    {sfDomainID, soeOPTIONAL},
}))
```

### 2.2 Transactor Class Pattern

All transaction handlers inherit from `Transactor` in `src/xrpld/app/tx/detail/Transactor.h`.

**Required Class Structure:**
```cpp
class MyTransaction : public Transactor
{
public:
    // ConsequencesFactory determines fee calculation behavior
    // Options: Normal, Blocker, Custom
    static constexpr ConsequencesFactoryType ConsequencesFactory{Normal};

    explicit MyTransaction(ApplyContext& ctx) : Transactor(ctx) {}

    // Called before signature verification - static checks only
    static NotTEC preflight(PreflightContext const& ctx);

    // Called after signature verification - ledger state checks
    static TER preclaim(PreclaimContext const& ctx);

    // Applies the transaction to the ledger
    TER doApply() override;
};
```

**Optional Methods:**
```cpp
// Gate transaction on additional amendments beyond transactions.macro
static bool checkExtraFeatures(PreflightContext const& ctx);

// Custom flag mask (default is tfUniversal)
static std::uint32_t getFlagsMask(PreflightContext const& ctx);

// For Custom ConsequencesFactory
static TxConsequences makeTxConsequences(PreflightContext const& ctx);

// Additional validation after signature verification
static NotTEC preflightSigValidated(PreflightContext const& ctx);
```

### 2.3 RPC Handler Registration

RPC handlers are registered in two ways:

**Method 1: Handler Array (Legacy)**
File: `src/xrpld/rpc/detail/Handler.cpp`

Handlers are added to `handlerArray` with the signature:
```cpp
Json::Value doFoo(RPC::JsonContext& context);
```

Declared in `src/xrpld/rpc/handlers/Handlers.h`:
```cpp
Json::Value doMyHandler(RPC::JsonContext&);
```

**Method 2: New-Style Handlers (Preferred)**
Create a class with static members:
```cpp
class MyHandler
{
public:
    static constexpr char const* name = "my_handler";
    static constexpr Role role = Role::USER;
    static constexpr Condition condition = NO_CONDITION;
    static constexpr unsigned minApiVer = apiMinimumSupportedVersion;
    static constexpr unsigned maxApiVer = apiMaximumValidVersion;
    // ... handler implementation
};
```

Register in `HandlerTable::HandlerTable()`:
```cpp
addHandler<MyHandler>();
```

### 2.4 Amendment System

Amendments are defined in `include/xrpl/protocol/detail/features.macro`.

**Macro Formats:**
```cpp
XRPL_FEATURE(name, supported, vote)  // For features (e.g., featureAMM)
XRPL_FIX(name, supported, vote)      // For fixes (e.g., fixAMMv1_1)
```

**Parameters:**
- `name`: Feature name (generates `feature{Name}` or `fix{Name}` variable)
- `supported`: `Supported::yes` or `Supported::no`
- `vote`: `VoteBehavior::DefaultYes`, `VoteBehavior::DefaultNo`, or `VoteBehavior::Obsolete`

**Amendment Lifecycle:**
1. Add with `Supported::no`, `VoteBehavior::DefaultNo`
2. Development complete → Change to `Supported::yes`
3. Ready for voting → Change to `VoteBehavior::DefaultYes`
4. Enabled for years → Move to retired section with `XRPL_RETIRE_FEATURE`

**Hash Generation:**
Amendment hashes are generated automatically via SHA-512 Half of the feature name:
```cpp
auto const f = sha512Half(Slice(name.data(), name.size()));
```

### 2.5 Test Patterns

Transaction tests use the `jtx` (JSON transaction) framework in `src/test/jtx/`.

**Basic Test Structure:**
```cpp
class MyTransaction_test : public beast::unit_test::suite
{
public:
    void testBasicOperation()
    {
        using namespace jtx;
        Env env(*this);

        // Create accounts
        Account alice{"alice"};
        Account bob{"bob"};
        env.fund(XRP(10000), alice, bob);
        env.close();

        // Submit transaction
        env(myTransaction(alice, bob, XRP(100)));
        env.close();

        // Verify results
        BEAST_EXPECT(env.balance(bob) == XRP(10100));
    }

    void run() override
    {
        testBasicOperation();
    }
};
BEAST_DEFINE_TESTSUITE(MyTransaction, app, ripple);
```

**RPC Test Pattern:**
```cpp
void testMyRPC()
{
    using namespace jtx;
    Env env(*this);

    auto const result = env.rpc("my_handler", "param1", "param2");
    BEAST_EXPECT(result[jss::status] == "success");
}
```

---

## 3. Design Considerations

### 3.1 Documentation Format Options

| Format | Pros | Cons |
|--------|------|------|
| Step-by-step tutorials | Easy to follow, practical | Can become outdated |
| Reference docs | Complete, authoritative | Harder to use for beginners |
| Code templates | Copy-paste ready | May miss context |
| Example-based | Concrete, testable | May not cover edge cases |

**Recommendation:** Hybrid approach with tutorials containing real code examples from existing transactions, supplemented by reference sections.

### 3.2 Code Examples Strategy

Use real transaction types as templates:
- Simple transaction: `CredentialCreate` - minimal fields, straightforward logic
- Medium complexity: `DepositPreauth` - amendment gating, optional methods
- Complex: `XChainCreateBridge` - multiple fields, cross-feature interaction

### 3.3 Key Architectural Insights

1. **Preflight vs Preclaim vs DoApply:**
   - `preflight`: No ledger access, validates transaction structure
   - `preclaim`: Read-only ledger access, validates account state
   - `doApply`: Read-write ledger access, applies changes

2. **Amendment Gating:**
   - Use `transactions.macro` amendment field for automatic gating
   - Use `checkExtraFeatures()` for additional amendment checks

3. **Error Handling:**
   - Return `NotTEC` from preflight (never applied)
   - Return `TER` from preclaim/doApply (may claim fee)

---

## 4. Implementation Plan

### Step 1: Create Documentation Structure
Create `docs/FEATURE_DEVELOPMENT.md` with the following sections.

### Step 2: Document "Adding a New Transaction" Workflow

**Content Outline:**
1. Choose a transaction type number (check `transactions.macro` for available)
2. Create header file in `src/xrpld/app/tx/detail/`
3. Create implementation file
4. Add entry to `transactions.macro`
5. Add protocol fields if needed (`include/xrpl/protocol/SField.h`)
6. Add error codes if needed (`include/xrpl/protocol/TER.h`)
7. Write unit tests in `src/test/app/`

### Step 3: Document "Adding a New RPC Handler" Workflow

**Content Outline:**
1. Create handler in `src/xrpld/rpc/handlers/`
2. Declare in `src/xrpld/rpc/handlers/Handlers.h`
3. Register in handler array or use new-style registration
4. Add JSON field names to `include/xrpl/protocol/jss.h` if needed
5. Write unit tests in `src/test/rpc/`

### Step 4: Document "Adding an Amendment" Workflow

**Content Outline:**
1. Add macro to `include/xrpl/protocol/detail/features.macro`
2. Use generated variable in code (`featureName` or `fixName`)
3. Check with `ctx.rules.enabled(featureName)`
4. Update amendment support status through development lifecycle

### Step 5: Include Complete Code Examples

Provide full, compilable examples for:
- Minimal transaction handler
- RPC handler with parameter validation
- Amendment-gated feature

### Step 6: Add Troubleshooting Section

Common issues and solutions:
- "Transaction returns temDISABLED" → Check amendment gating
- "RPC handler not found" → Verify registration
- "Compilation fails with undefined reference" → Check include guards

---

## 5. Content Outline for FEATURE_DEVELOPMENT.md

```markdown
# Feature Development Guide

## 1. Adding a New Transaction Type
- Overview and prerequisites
- Step-by-step with code examples
- Template files
- Testing requirements
- Checklist

## 2. Adding a New RPC Handler
- Handler structure patterns
- Registration methods
- JSON schema definition
- Error handling
- Testing patterns

## 3. Adding an Amendment
- Amendment hash generation
- Macro usage
- Voting mechanism
- Activation timeline
- Retirement process

## 4. Best Practices
- Error handling patterns
- Logging conventions (JLOG usage)
- Performance considerations
- Code review checklist

## 5. Common Pitfalls
- Thread safety in handlers
- Ledger state access patterns
- Transaction ordering issues
- Amendment dependency chains
- Fee calculation errors

## 6. Reference
- Transaction type enum values
- Error code reference
- Amendment status table
- API versioning guide
```

---

## 6. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Documentation becomes outdated | Medium | Medium | Link to actual code, use code examples |
| Incomplete coverage of edge cases | Medium | Low | Iterative improvement based on feedback |
| Pattern changes break examples | Low | High | Add CI tests for documentation examples |
| Developers skip documentation | Medium | Low | Integrate with onboarding process |

---

## 7. Validation Criteria

### 7.1 Transaction Development Validation
- [ ] New developer can add transaction following guide
- [ ] Transaction handler compiles without modification
- [ ] Tests pass for example transaction
- [ ] Guide covers amendment gating

### 7.2 RPC Handler Validation
- [ ] New developer can add RPC handler following guide
- [ ] Handler is discoverable via server_info
- [ ] Parameter validation works correctly
- [ ] Error responses match documentation

### 7.3 Amendment Validation
- [ ] Amendment hash generation is clear
- [ ] Lifecycle from development to retirement documented
- [ ] Voting behavior explained
- [ ] Code gating patterns covered

### 7.4 Overall Documentation Quality
- [ ] Examples are copy-paste ready
- [ ] All code compiles with current codebase
- [ ] Cross-references to actual source files
- [ ] Reviewed by at least 2 core developers

---

## 8. Key Files Reference

### Transaction System
- `include/xrpl/protocol/detail/transactions.macro` - Transaction registration
- `src/xrpld/app/tx/detail/Transactor.h` - Base transactor class
- `src/xrpld/app/tx/detail/*.cpp` - Transaction implementations
- `src/xrpld/app/tx/applySteps.h` - Apply step functions

### RPC System
- `src/xrpld/rpc/handlers/Handlers.h` - Handler declarations
- `src/xrpld/rpc/detail/Handler.cpp` - Handler registration
- `src/xrpld/rpc/Context.h` - JsonContext definition
- `src/xrpld/rpc/handlers/*.cpp` - Handler implementations

### Amendment System
- `include/xrpl/protocol/Feature.h` - Amendment documentation
- `include/xrpl/protocol/detail/features.macro` - Amendment definitions
- `src/libxrpl/protocol/Feature.cpp` - Amendment registration
- `src/xrpld/app/misc/AmendmentTable.h` - Amendment voting

### Testing
- `src/test/jtx/` - Test framework
- `src/test/app/` - Transaction tests
- `src/test/rpc/` - RPC handler tests

---

## 9. Dependencies

- Phase 1 documentation patterns (for consistency)
- Code review process understanding
- Build system knowledge (CMake)

---

## 10. Estimated Effort

| Task | Effort |
|------|--------|
| Create document structure | 2 hours |
| Transaction development guide | 8 hours |
| RPC handler guide | 4 hours |
| Amendment guide | 4 hours |
| Code examples | 6 hours |
| Review and iteration | 4 hours |
| **Total** | **28 hours**

