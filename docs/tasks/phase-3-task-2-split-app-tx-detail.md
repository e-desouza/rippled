# Phase 3 Task 3.2: Split app/tx/detail into Submodules

## Overview

### Problem Statement

The `src/xrpld/app/tx/detail/` directory contains **68+ header files** and their corresponding implementation files for all transaction handlers. This monolithic structure creates several problems:

1. **Discoverability**: Finding a specific transaction handler requires scanning through a flat list of 68+ files
2. **Compilation dependencies**: Changes to shared infrastructure files (Transactor.h, ApplyContext.h) trigger recompilation of all handlers
3. **Conceptual grouping**: Related transactions (e.g., all AMM or NFT operations) are scattered among unrelated files
4. **Onboarding difficulty**: New developers struggle to understand which files are related to which feature
5. **Future modularity**: Cannot easily extract feature-specific transaction sets into separate modules

### Success Criteria

1. ✅ Transaction handlers organized into logical subdirectories by feature
2. ✅ Build compiles successfully at each incremental step
3. ✅ All transaction-related tests pass without modification
4. ✅ No regression in transaction processing performance
5. ✅ Include paths updated consistently across the codebase
6. ✅ `transactions.macro` updated to reference new header locations

---

## Deep Code Analysis

### Current Directory Structure

**Total files**: 68+ header files (.h) with corresponding implementation files (.cpp)

The `src/xrpld/app/tx/detail/` directory currently contains all transaction handlers in a flat structure:

#### Core Infrastructure Files (6 files - keep in detail/)
| File | Purpose |
|------|---------|
| `Transactor.h/cpp` | Base class for all transaction handlers |
| `ApplyContext.h/cpp` | Context passed to transaction application |
| `InvariantCheck.h/cpp` | Invariant checking for transaction processing |
| `apply.cpp` | Transaction application entry point |
| `applySteps.cpp` | Transaction dispatch via `with_txn_type` template |
| `BookTip.h/cpp` | Book tip tracking for offer processing |

#### AMM Transactions (7 files)
| File | Transaction Type |
|------|------------------|
| `AMMBid.h/cpp` | `ttAMM_BID` - Bid for AMM auction slot |
| `AMMClawback.h/cpp` | `ttAMM_CLAWBACK` - Clawback from AMM pool |
| `AMMCreate.h/cpp` | `ttAMM_CREATE` - Create AMM instance |
| `AMMDelete.h/cpp` | `ttAMM_DELETE` - Delete empty AMM |
| `AMMDeposit.h/cpp` | `ttAMM_DEPOSIT` - Deposit into AMM |
| `AMMVote.h/cpp` | `ttAMM_VOTE` - Vote on trading fee |
| `AMMWithdraw.h/cpp` | `ttAMM_WITHDRAW` - Withdraw from AMM |

#### NFT Transactions (7 files)
| File | Transaction Type |
|------|------------------|
| `NFTokenAcceptOffer.h/cpp` | `ttNFTOKEN_ACCEPT_OFFER` - Accept NFT offer |
| `NFTokenBurn.h/cpp` | `ttNFTOKEN_BURN` - Burn NFT |
| `NFTokenCancelOffer.h/cpp` | `ttNFTOKEN_CANCEL_OFFER` - Cancel NFT offer |
| `NFTokenCreateOffer.h/cpp` | `ttNFTOKEN_CREATE_OFFER` - Create NFT offer |
| `NFTokenMint.h/cpp` | `ttNFTOKEN_MINT` - Mint new NFT |
| `NFTokenModify.h/cpp` | `ttNFTOKEN_MODIFY` - Modify NFT metadata |
| `NFTokenUtils.h/cpp` | Shared utilities for NFT operations |

#### Vault Transactions (6 files)
| File | Transaction Type |
|------|------------------|
| `VaultClawback.h/cpp` | `ttVAULT_CLAWBACK` - Clawback from vault |
| `VaultCreate.h/cpp` | `ttVAULT_CREATE` - Create vault |
| `VaultDelete.h/cpp` | `ttVAULT_DELETE` - Delete vault |
| `VaultDeposit.h/cpp` | `ttVAULT_DEPOSIT` - Deposit into vault |
| `VaultSet.h/cpp` | `ttVAULT_SET` - Modify vault settings |
| `VaultWithdraw.h/cpp` | `ttVAULT_WITHDRAW` - Withdraw from vault |

#### Loan Transactions (9 files)
| File | Transaction Type |
|------|------------------|
| `LoanBrokerCoverClawback.h/cpp` | `ttLOAN_BROKER_COVER_CLAWBACK` |
| `LoanBrokerCoverDeposit.h/cpp` | `ttLOAN_BROKER_COVER_DEPOSIT` |
| `LoanBrokerCoverWithdraw.h/cpp` | `ttLOAN_BROKER_COVER_WITHDRAW` |
| `LoanBrokerDelete.h/cpp` | `ttLOAN_BROKER_DELETE` |
| `LoanBrokerSet.h/cpp` | `ttLOAN_BROKER_SET` |
| `LoanDelete.h/cpp` | `ttLOAN_DELETE` |
| `LoanManage.h/cpp` | `ttLOAN_MANAGE` |
| `LoanPay.h/cpp` | `ttLOAN_PAY` |
| `LoanSet.h/cpp` | `ttLOAN_SET` |

#### MPToken Transactions (4 files)
| File | Transaction Type |
|------|------------------|
| `MPTokenAuthorize.h/cpp` | `ttMPTOKEN_AUTHORIZE` |
| `MPTokenIssuanceCreate.h/cpp` | `ttMPTOKEN_ISSUANCE_CREATE` |
| `MPTokenIssuanceDestroy.h/cpp` | `ttMPTOKEN_ISSUANCE_DESTROY` |
| `MPTokenIssuanceSet.h/cpp` | `ttMPTOKEN_ISSUANCE_SET` |

#### XChain/Bridge Transactions (1 file, 8 classes)
| File | Transaction Types |
|------|-------------------|
| `XChainBridge.h/cpp` | Contains: `XChainCreateBridge`, `XChainModifyBridge`, `XChainClaim`, `XChainCommit`, `XChainCreateClaimID`, `XChainAddClaimAttestation`, `XChainAddAccountCreateAttestation`, `XChainAccountCreateCommit` |

#### Check Transactions (3 files)
| File | Transaction Type |
|------|------------------|
| `CancelCheck.h/cpp` | `ttCHECK_CANCEL` |
| `CashCheck.h/cpp` | `ttCHECK_CASH` |
| `CreateCheck.h/cpp` | `ttCHECK_CREATE` |

#### Escrow Transactions (1 file, 3 classes)
| File | Transaction Types |
|------|-------------------|
| `Escrow.h/cpp` | `EscrowCreate`, `EscrowFinish`, `EscrowCancel` |

#### PayChan Transactions (1 file, 3 classes)
| File | Transaction Types |
|------|-------------------|
| `PayChan.h/cpp` | `PaymentChannelCreate`, `PaymentChannelFund`, `PaymentChannelClaim` |

#### Offer/DEX Transactions (4 files)
| File | Transaction Type |
|------|------------------|
| `CancelOffer.h/cpp` | `ttOFFER_CANCEL` |
| `CreateOffer.h/cpp` | `ttOFFER_CREATE` |
| `Offer.h` | Offer helper class (header only) |
| `OfferStream.h/cpp` | Offer stream processing |

#### Credential Transactions (1 file, 3 classes)
| File | Transaction Types |
|------|-------------------|
| `Credentials.h/cpp` | `CredentialCreate`, `CredentialAccept`, `CredentialDelete` |

#### Oracle Transactions (2 files)
| File | Transaction Type |
|------|------------------|
| `DeleteOracle.h/cpp` | `ttORACLE_DELETE` |
| `SetOracle.h/cpp` | `ttORACLE_SET` |

#### PermissionedDomain Transactions (2 files)
| File | Transaction Type |
|------|------------------|
| `PermissionedDomainDelete.h/cpp` | `ttPERMISSIONED_DOMAIN_DELETE` |
| `PermissionedDomainSet.h/cpp` | `ttPERMISSIONED_DOMAIN_SET` |

#### Account/Core Transactions (remaining files)
| File | Transaction Type |
|------|------------------|
| `Batch.h/cpp` | `ttBATCH` - Batch transactions |
| `Change.h/cpp` | `ttAMENDMENT`, `ttFEE`, `ttUNL_MODIFY` - System transactions |
| `Clawback.h/cpp` | `ttCLAWBACK` - Token clawback |
| `CreateTicket.h/cpp` | `ttTICKET_CREATE` |
| `DelegateSet.h/cpp` | `ttDELEGATE_SET` |
| `DeleteAccount.h/cpp` | `ttACCOUNT_DELETE` |
| `DepositPreauth.h/cpp` | `ttDEPOSIT_PREAUTH` |
| `DID.h/cpp` | `ttDID_SET`, `ttDID_DELETE` |
| `LedgerStateFix.h/cpp` | `ttLEDGER_STATE_FIX` |
| `Payment.h/cpp` | `ttPAYMENT` |
| `SetAccount.h/cpp` | `ttACCOUNT_SET` |
| `SetRegularKey.h/cpp` | `ttREGULAR_KEY_SET` |
| `SetSignerList.h/cpp` | `ttSIGNER_LIST_SET` |
| `SetTrust.h/cpp` | `ttTRUST_SET` |
| `SignerEntries.h/cpp` | Signer entry utilities |
| `Taker.h` | Taker class for offer crossing (header only) |

### Transaction Handler Pattern

All transaction handlers inherit from `Transactor` and follow a consistent pattern:

```cpp
class PaymentHandler : public Transactor
{
public:
    // Factory type for TxConsequences (Normal, Blocker, or Custom)
    static constexpr ConsequencesFactoryType ConsequencesFactory{Custom};

    explicit PaymentHandler(ApplyContext& ctx) : Transactor(ctx) {}

    // Optional: Custom TxConsequences calculation
    static TxConsequences makeTxConsequences(PreflightContext const& ctx);

    // Optional: Check if required features/amendments are enabled
    static bool checkExtraFeatures(PreflightContext const& ctx);

    // Optional: Return allowed transaction flags mask
    static std::uint32_t getFlagsMask(PreflightContext const& ctx);

    // Required: Static validation (no ledger access)
    static NotTEC preflight(PreflightContext const& ctx);

    // Optional: Ledger-based validation
    static TER preclaim(PreclaimContext const& ctx);

    // Required: Apply the transaction
    TER doApply() override;
};
```

### Transaction Registration

Transactions are registered in `include/xrpl/protocol/detail/transactions.macro`:

```cpp
#if TRANSACTION_INCLUDE
#   include <xrpld/app/tx/detail/Payment.h>  // <-- Include path must be updated
#endif
TRANSACTION(ttPAYMENT, 0, Payment,
    Delegation::delegable,
    uint256{},
    createAcct,
    ({ /* fields */ }))
```

The `with_txn_type` function in `applySteps.cpp` uses template metaprogramming to dispatch transactions at compile time based on `TxType`.


---

## Design Considerations

### Option A: Create Subdirectories by Feature (RECOMMENDED)

Organize transaction handlers into subdirectories within `tx/detail/`:

```
src/xrpld/app/tx/detail/
├── Transactor.h/cpp          # Keep in place
├── ApplyContext.h/cpp        # Keep in place
├── InvariantCheck.h/cpp      # Keep in place
├── apply.cpp                 # Keep in place
├── applySteps.cpp            # Keep in place
├── BookTip.h/cpp             # Keep in place (used by offers)
├── amm/
│   ├── AMMBid.h/cpp
│   ├── AMMClawback.h/cpp
│   ├── AMMCreate.h/cpp
│   ├── AMMDelete.h/cpp
│   ├── AMMDeposit.h/cpp
│   ├── AMMVote.h/cpp
│   └── AMMWithdraw.h/cpp
├── nft/
│   ├── NFTokenAcceptOffer.h/cpp
│   ├── NFTokenBurn.h/cpp
│   ├── NFTokenCancelOffer.h/cpp
│   ├── NFTokenCreateOffer.h/cpp
│   ├── NFTokenMint.h/cpp
│   ├── NFTokenModify.h/cpp
│   └── NFTokenUtils.h/cpp
├── vault/
│   ├── VaultClawback.h/cpp
│   ├── VaultCreate.h/cpp
│   ├── VaultDelete.h/cpp
│   ├── VaultDeposit.h/cpp
│   ├── VaultSet.h/cpp
│   └── VaultWithdraw.h/cpp
├── loan/
│   ├── LoanBrokerCoverClawback.h/cpp
│   ├── LoanBrokerCoverDeposit.h/cpp
│   ├── LoanBrokerCoverWithdraw.h/cpp
│   ├── LoanBrokerDelete.h/cpp
│   ├── LoanBrokerSet.h/cpp
│   ├── LoanDelete.h/cpp
│   ├── LoanManage.h/cpp
│   ├── LoanPay.h/cpp
│   └── LoanSet.h/cpp
├── mptoken/
│   ├── MPTokenAuthorize.h/cpp
│   ├── MPTokenIssuanceCreate.h/cpp
│   ├── MPTokenIssuanceDestroy.h/cpp
│   └── MPTokenIssuanceSet.h/cpp
├── xchain/
│   └── XChainBridge.h/cpp
├── check/
│   ├── CancelCheck.h/cpp
│   ├── CashCheck.h/cpp
│   └── CreateCheck.h/cpp
├── escrow/
│   └── Escrow.h/cpp
├── paychan/
│   └── PayChan.h/cpp
├── offer/
│   ├── CancelOffer.h/cpp
│   ├── CreateOffer.h/cpp
│   ├── Offer.h
│   ├── OfferStream.h/cpp
│   └── Taker.h
├── credential/
│   └── Credentials.h/cpp
├── oracle/
│   ├── DeleteOracle.h/cpp
│   └── SetOracle.h/cpp
├── domain/
│   ├── PermissionedDomainDelete.h/cpp
│   └── PermissionedDomainSet.h/cpp
└── core/
    ├── Batch.h/cpp
    ├── Change.h/cpp
    ├── Clawback.h/cpp
    ├── CreateTicket.h/cpp
    ├── DelegateSet.h/cpp
    ├── DeleteAccount.h/cpp
    ├── DepositPreauth.h/cpp
    ├── DID.h/cpp
    ├── LedgerStateFix.h/cpp
    ├── Payment.h/cpp
    ├── SetAccount.h/cpp
    ├── SetRegularKey.h/cpp
    ├── SetSignerList.h/cpp
    ├── SetTrust.h/cpp
    └── SignerEntries.h/cpp
```

**Pros**:
- Maintains existing `detail/` convention
- Minimal disruption to build system
- Easy to find feature-specific handlers
- Groups related utilities with their transactions

**Cons**:
- Deeper directory nesting
- Longer include paths

### Option B: Promote to Separate Modules

Create top-level feature directories alongside `tx/`:

```
src/xrpld/app/
├── tx/
│   ├── detail/          # Only core infrastructure
│   └── ...
├── amm/                 # AMM transactions as separate module
├── nft/                 # NFT transactions as separate module
└── ...
```

**Pros**:
- Clearer module boundaries
- Could enable feature-level compilation units

**Cons**:
- Major restructuring required
- More CMake changes needed
- Deviates from current conventions

### Option C: Functional Grouping

Group by transaction category (payments, trading, governance):

```
src/xrpld/app/tx/detail/
├── payments/      # Payment, PayChan, Check, Escrow
├── trading/       # Offers, AMM
├── assets/        # NFT, MPToken
└── governance/    # Amendments, Fees, Signers
```

**Pros**:
- Conceptual grouping by function
- Fewer subdirectories

**Cons**:
- Less intuitive for feature-specific work
- Categories may overlap or be subjective

### Recommendation: Option A

**Option A is recommended** because:
1. **Follows existing patterns**: Keeps `detail/` convention
2. **Feature alignment**: Subdirectories match feature names (AMM, NFT, etc.)
3. **Minimal risk**: Incremental changes, easy to test
4. **Clear ownership**: Each subdirectory maps to a feature team/area
5. **Include path clarity**: `<xrpld/app/tx/detail/amm/AMMCreate.h>` is self-documenting

---

## Implementation Plan

### Step 1: Create Subdirectory Structure

Create the subdirectory structure without moving any files:

```bash
cd src/xrpld/app/tx/detail/
mkdir -p amm nft vault loan mptoken xchain check escrow paychan offer credential oracle domain core
```

**Verification**: Directory structure exists, build still works.

### Step 2: Move AMM Transactions (Most Self-Contained)

AMM transactions have minimal cross-dependencies and make a good starting point.

**Files to Move**:
```bash
git mv AMMBid.h AMMBid.cpp amm/
git mv AMMClawback.h AMMClawback.cpp amm/
git mv AMMCreate.h AMMCreate.cpp amm/
git mv AMMDelete.h AMMDelete.cpp amm/
git mv AMMDeposit.h AMMDeposit.cpp amm/
git mv AMMVote.h AMMVote.cpp amm/
git mv AMMWithdraw.h AMMWithdraw.cpp amm/
```

**Files to Update**:
1. `include/xrpl/protocol/detail/transactions.macro` - Update include paths:
   ```cpp
   #if TRANSACTION_INCLUDE
   #   include <xrpld/app/tx/detail/amm/AMMCreate.h>  // Updated path
   #endif
   ```

2. All files that include AMM headers (use `grep -r "AMMCreate.h" src/`)

3. `CMakeLists.txt` or build files if source files are listed explicitly

**Verification**: `cmake --build build --target rippled -j$(nproc)`

### Step 3: Move NFT Transactions

**Files to Move**:
```bash
git mv NFTokenAcceptOffer.h NFTokenAcceptOffer.cpp nft/
git mv NFTokenBurn.h NFTokenBurn.cpp nft/
git mv NFTokenCancelOffer.h NFTokenCancelOffer.cpp nft/
git mv NFTokenCreateOffer.h NFTokenCreateOffer.cpp nft/
git mv NFTokenMint.h NFTokenMint.cpp nft/
git mv NFTokenModify.h NFTokenModify.cpp nft/
git mv NFTokenUtils.h NFTokenUtils.cpp nft/
```

**Update**: `transactions.macro` and any files including NFT headers.

### Step 4: Move Vault Transactions

**Files to Move**:
```bash
git mv VaultClawback.h VaultClawback.cpp vault/
git mv VaultCreate.h VaultCreate.cpp vault/
git mv VaultDelete.h VaultDelete.cpp vault/
git mv VaultDeposit.h VaultDeposit.cpp vault/
git mv VaultSet.h VaultSet.cpp vault/
git mv VaultWithdraw.h VaultWithdraw.cpp vault/
```

### Step 5: Move Loan Transactions

**Files to Move**:
```bash
git mv LoanBrokerCoverClawback.h LoanBrokerCoverClawback.cpp loan/
git mv LoanBrokerCoverDeposit.h LoanBrokerCoverDeposit.cpp loan/
git mv LoanBrokerCoverWithdraw.h LoanBrokerCoverWithdraw.cpp loan/
git mv LoanBrokerDelete.h LoanBrokerDelete.cpp loan/
git mv LoanBrokerSet.h LoanBrokerSet.cpp loan/
git mv LoanDelete.h LoanDelete.cpp loan/
git mv LoanManage.h LoanManage.cpp loan/
git mv LoanPay.h LoanPay.cpp loan/
git mv LoanSet.h LoanSet.cpp loan/
```

### Step 6: Move MPToken Transactions

**Files to Move**:
```bash
git mv MPTokenAuthorize.h MPTokenAuthorize.cpp mptoken/
git mv MPTokenIssuanceCreate.h MPTokenIssuanceCreate.cpp mptoken/
git mv MPTokenIssuanceDestroy.h MPTokenIssuanceDestroy.cpp mptoken/
git mv MPTokenIssuanceSet.h MPTokenIssuanceSet.cpp mptoken/
```

### Step 7: Move XChain, Check, Escrow, PayChan

**XChain**:
```bash
git mv XChainBridge.h XChainBridge.cpp xchain/
```

**Check**:
```bash
git mv CancelCheck.h CancelCheck.cpp check/
git mv CashCheck.h CashCheck.cpp check/
git mv CreateCheck.h CreateCheck.cpp check/
```

**Escrow**:
```bash
git mv Escrow.h Escrow.cpp escrow/
```

**PayChan**:
```bash
git mv PayChan.h PayChan.cpp paychan/
```

### Step 8: Move Offer, Credential, Oracle, Domain

**Offer**:
```bash
git mv CancelOffer.h CancelOffer.cpp offer/
git mv CreateOffer.h CreateOffer.cpp offer/
git mv Offer.h offer/
git mv OfferStream.h OfferStream.cpp offer/
git mv Taker.h offer/
```

**Credential**:
```bash
git mv Credentials.h Credentials.cpp credential/
```

**Oracle**:
```bash
git mv DeleteOracle.h DeleteOracle.cpp oracle/
git mv SetOracle.h SetOracle.cpp oracle/
```

**Domain**:
```bash
git mv PermissionedDomainDelete.h PermissionedDomainDelete.cpp domain/
git mv PermissionedDomainSet.h PermissionedDomainSet.cpp domain/
```

### Step 9: Move Core Transactions

**Files to Move**:
```bash
git mv Batch.h Batch.cpp core/
git mv Change.h Change.cpp core/
git mv Clawback.h Clawback.cpp core/
git mv CreateTicket.h CreateTicket.cpp core/
git mv DelegateSet.h DelegateSet.cpp core/
git mv DeleteAccount.h DeleteAccount.cpp core/
git mv DepositPreauth.h DepositPreauth.cpp core/
git mv DID.h DID.cpp core/
git mv LedgerStateFix.h LedgerStateFix.cpp core/
git mv Payment.h Payment.cpp core/
git mv SetAccount.h SetAccount.cpp core/
git mv SetRegularKey.h SetRegularKey.cpp core/
git mv SetSignerList.h SetSignerList.cpp core/
git mv SetTrust.h SetTrust.cpp core/
git mv SignerEntries.h SignerEntries.cpp core/
```

### Step 10: Update All Include Paths

After all moves, systematically update include paths across the codebase:

```bash
# Find all files that include moved headers
grep -rl "xrpld/app/tx/detail/AMMCreate.h" src/ include/ | xargs sed -i 's|xrpld/app/tx/detail/AMMCreate.h|xrpld/app/tx/detail/amm/AMMCreate.h|g'

# Repeat for each moved file or use a script
```

**Critical File**: `include/xrpl/protocol/detail/transactions.macro` must have all includes updated.

### Step 11: Keep Infrastructure Files in Place

The following files remain in `tx/detail/` (not moved):

| File | Reason |
|------|--------|
| `Transactor.h/cpp` | Base class used by all handlers |
| `ApplyContext.h/cpp` | Context used by all handlers |
| `InvariantCheck.h/cpp` | Invariant checking infrastructure |
| `apply.cpp` | Transaction application entry |
| `applySteps.cpp` | Transaction dispatch logic |
| `BookTip.h/cpp` | Shared by offer-related handlers |

### Complete File Mapping Table

| Original Location | New Location |
|-------------------|--------------|
| `detail/AMMBid.h` | `detail/amm/AMMBid.h` |
| `detail/AMMClawback.h` | `detail/amm/AMMClawback.h` |
| `detail/AMMCreate.h` | `detail/amm/AMMCreate.h` |
| `detail/AMMDelete.h` | `detail/amm/AMMDelete.h` |
| `detail/AMMDeposit.h` | `detail/amm/AMMDeposit.h` |
| `detail/AMMVote.h` | `detail/amm/AMMVote.h` |
| `detail/AMMWithdraw.h` | `detail/amm/AMMWithdraw.h` |
| `detail/NFTokenAcceptOffer.h` | `detail/nft/NFTokenAcceptOffer.h` |
| `detail/NFTokenBurn.h` | `detail/nft/NFTokenBurn.h` |
| `detail/NFTokenCancelOffer.h` | `detail/nft/NFTokenCancelOffer.h` |
| `detail/NFTokenCreateOffer.h` | `detail/nft/NFTokenCreateOffer.h` |
| `detail/NFTokenMint.h` | `detail/nft/NFTokenMint.h` |
| `detail/NFTokenModify.h` | `detail/nft/NFTokenModify.h` |
| `detail/NFTokenUtils.h` | `detail/nft/NFTokenUtils.h` |
| `detail/VaultClawback.h` | `detail/vault/VaultClawback.h` |
| `detail/VaultCreate.h` | `detail/vault/VaultCreate.h` |
| `detail/VaultDelete.h` | `detail/vault/VaultDelete.h` |
| `detail/VaultDeposit.h` | `detail/vault/VaultDeposit.h` |
| `detail/VaultSet.h` | `detail/vault/VaultSet.h` |
| `detail/VaultWithdraw.h` | `detail/vault/VaultWithdraw.h` |
| `detail/LoanBrokerCoverClawback.h` | `detail/loan/LoanBrokerCoverClawback.h` |
| `detail/LoanBrokerCoverDeposit.h` | `detail/loan/LoanBrokerCoverDeposit.h` |
| `detail/LoanBrokerCoverWithdraw.h` | `detail/loan/LoanBrokerCoverWithdraw.h` |
| `detail/LoanBrokerDelete.h` | `detail/loan/LoanBrokerDelete.h` |
| `detail/LoanBrokerSet.h` | `detail/loan/LoanBrokerSet.h` |
| `detail/LoanDelete.h` | `detail/loan/LoanDelete.h` |
| `detail/LoanManage.h` | `detail/loan/LoanManage.h` |
| `detail/LoanPay.h` | `detail/loan/LoanPay.h` |
| `detail/LoanSet.h` | `detail/loan/LoanSet.h` |
| `detail/MPTokenAuthorize.h` | `detail/mptoken/MPTokenAuthorize.h` |
| `detail/MPTokenIssuanceCreate.h` | `detail/mptoken/MPTokenIssuanceCreate.h` |
| `detail/MPTokenIssuanceDestroy.h` | `detail/mptoken/MPTokenIssuanceDestroy.h` |
| `detail/MPTokenIssuanceSet.h` | `detail/mptoken/MPTokenIssuanceSet.h` |
| `detail/XChainBridge.h` | `detail/xchain/XChainBridge.h` |
| `detail/CancelCheck.h` | `detail/check/CancelCheck.h` |
| `detail/CashCheck.h` | `detail/check/CashCheck.h` |
| `detail/CreateCheck.h` | `detail/check/CreateCheck.h` |
| `detail/Escrow.h` | `detail/escrow/Escrow.h` |
| `detail/PayChan.h` | `detail/paychan/PayChan.h` |
| `detail/CancelOffer.h` | `detail/offer/CancelOffer.h` |
| `detail/CreateOffer.h` | `detail/offer/CreateOffer.h` |
| `detail/Offer.h` | `detail/offer/Offer.h` |
| `detail/OfferStream.h` | `detail/offer/OfferStream.h` |
| `detail/Taker.h` | `detail/offer/Taker.h` |
| `detail/Credentials.h` | `detail/credential/Credentials.h` |
| `detail/DeleteOracle.h` | `detail/oracle/DeleteOracle.h` |
| `detail/SetOracle.h` | `detail/oracle/SetOracle.h` |
| `detail/PermissionedDomainDelete.h` | `detail/domain/PermissionedDomainDelete.h` |
| `detail/PermissionedDomainSet.h` | `detail/domain/PermissionedDomainSet.h` |
| `detail/Batch.h` | `detail/core/Batch.h` |
| `detail/Change.h` | `detail/core/Change.h` |
| `detail/Clawback.h` | `detail/core/Clawback.h` |
| `detail/CreateTicket.h` | `detail/core/CreateTicket.h` |
| `detail/DelegateSet.h` | `detail/core/DelegateSet.h` |
| `detail/DeleteAccount.h` | `detail/core/DeleteAccount.h` |
| `detail/DepositPreauth.h` | `detail/core/DepositPreauth.h` |
| `detail/DID.h` | `detail/core/DID.h` |
| `detail/LedgerStateFix.h` | `detail/core/LedgerStateFix.h` |
| `detail/Payment.h` | `detail/core/Payment.h` |
| `detail/SetAccount.h` | `detail/core/SetAccount.h` |
| `detail/SetRegularKey.h` | `detail/core/SetRegularKey.h` |
| `detail/SetSignerList.h` | `detail/core/SetSignerList.h` |
| `detail/SetTrust.h` | `detail/core/SetTrust.h` |
| `detail/SignerEntries.h` | `detail/core/SignerEntries.h` |


---

## Risk Assessment

### High Risk: Many Files to Move and Update

**Risk**: With 68+ files to move and potentially hundreds of include statements to update, there's significant risk of missing updates or introducing errors.

**Mitigation**:
1. Move files in batches, verifying build after each batch
2. Use automated tools (`grep`, `sed`, or Python scripts) to update includes
3. Commit after each successful batch
4. Create a tracking checklist for each file group
5. Run full test suite after each batch

**Commands for verification**:
```bash
# Check for broken includes
cmake --build build --target rippled 2>&1 | grep -i "fatal error"

# Find files still using old paths
grep -r "xrpld/app/tx/detail/AMMCreate.h" src/ include/
```

### Medium Risk: Macro-Generated Transaction Registration

**Risk**: The `transactions.macro` file uses `TRANSACTION_INCLUDE` to conditionally include headers. If include paths aren't updated correctly, transaction types won't be recognized.

**Mitigation**:
1. Update `transactions.macro` immediately after moving each file group
2. Build and test transaction processing after each update
3. Verify all transaction types are registered:
   ```bash
   # Test a simple transaction
   ./build/rippled --unittest="Payment"
   ./build/rippled --unittest="AMM"
   ```

**Affected file**: `include/xrpl/protocol/detail/transactions.macro`

### Medium Risk: ApplyContext and Transactor Dependencies

**Risk**: `ApplyContext.h` and `Transactor.h` are included by all transaction handlers. Moving handlers to subdirectories may cause include path issues.

**Mitigation**:
1. Keep `ApplyContext.h` and `Transactor.h` in place (do not move)
2. Handlers in subdirectories will use:
   ```cpp
   #include <xrpld/app/tx/detail/Transactor.h>  // Unchanged path
   ```
3. Update relative includes within moved files to absolute paths if necessary

### Low Risk: Transaction Handler Self-Containment

**Risk**: Some handlers may have unexpected dependencies on other handlers.

**Mitigation**:
1. Transaction handlers are generally self-contained
2. Shared utilities (like `NFTokenUtils.h`) move with their feature group
3. Build verification will catch any missing dependencies
4. Cross-feature dependencies (if any) can use absolute include paths

### Low Risk: Test File Updates

**Risk**: Test files may include transaction handler headers directly.

**Mitigation**:
1. Search test files for affected includes:
   ```bash
   grep -r "xrpld/app/tx/detail/" src/test/
   ```
2. Update test includes as part of each batch
3. Run transaction-specific tests after each move

---

## Validation Criteria

### 1. All Transaction Handlers in Appropriate Subdirectories

**Verification**:
```bash
# Check that detail/ only contains infrastructure files
ls -la src/xrpld/app/tx/detail/*.h

# Expected: Only Transactor.h, ApplyContext.h, InvariantCheck.h, BookTip.h

# Check subdirectories exist with expected files
ls -la src/xrpld/app/tx/detail/amm/
ls -la src/xrpld/app/tx/detail/nft/
ls -la src/xrpld/app/tx/detail/vault/
ls -la src/xrpld/app/tx/detail/loan/
```

### 2. Build Compiles Successfully

**Verification**:
```bash
# Clean build
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target rippled -j$(nproc)

# Expected: Build succeeds with no errors
echo "Build verification: $([ $? -eq 0 ] && echo PASSED || echo FAILED)"
```

### 3. All Transaction Tests Pass

**Verification**:
```bash
# Run all unit tests
./build/rippled --unittest 2>&1 | tee test_results.txt

# Check specific transaction tests
./build/rippled --unittest="Payment"
./build/rippled --unittest="AMM"
./build/rippled --unittest="NFToken"
./build/rippled --unittest="Escrow"
./build/rippled --unittest="Check"
./build/rippled --unittest="PayChan"
./build/rippled --unittest="Offer"
./build/rippled --unittest="Trust"

# Verify no test failures
grep -E "(PASSED|FAILED)" test_results.txt | tail -20
```

### 4. No Regression in Transaction Processing

**Verification**:
1. Run integration tests that exercise transaction processing
2. Verify transaction execution times are within normal range
3. Check that all transaction types can be submitted and processed

### 5. Include Paths Updated Consistently

**Verification**:
```bash
# Check for any remaining old-style includes
grep -r "xrpld/app/tx/detail/AMMCreate.h" src/ include/
grep -r "xrpld/app/tx/detail/NFTokenMint.h" src/ include/
grep -r "xrpld/app/tx/detail/Payment.h" src/ include/

# Expected: No results (all should use new subdirectory paths)

# Verify transactions.macro uses new paths
grep "app/tx/detail/" include/xrpl/protocol/detail/transactions.macro | head -10
# Expected: All paths include subdirectory (e.g., detail/amm/, detail/nft/)
```

### 6. Directory Structure Verification

```bash
# Verify final directory structure
find src/xrpld/app/tx/detail/ -type d | sort

# Expected output:
# src/xrpld/app/tx/detail/
# src/xrpld/app/tx/detail/amm
# src/xrpld/app/tx/detail/check
# src/xrpld/app/tx/detail/core
# src/xrpld/app/tx/detail/credential
# src/xrpld/app/tx/detail/domain
# src/xrpld/app/tx/detail/escrow
# src/xrpld/app/tx/detail/loan
# src/xrpld/app/tx/detail/mptoken
# src/xrpld/app/tx/detail/nft
# src/xrpld/app/tx/detail/offer
# src/xrpld/app/tx/detail/oracle
# src/xrpld/app/tx/detail/paychan
# src/xrpld/app/tx/detail/vault
# src/xrpld/app/tx/detail/xchain
```

---

## Timeline Estimate

| Phase | Duration | Cumulative |
|-------|----------|------------|
| Step 1: Create directory structure | 0.5 days | 0.5 days |
| Step 2: Move AMM transactions | 0.5 days | 1 day |
| Step 3: Move NFT transactions | 0.5 days | 1.5 days |
| Step 4: Move Vault transactions | 0.5 days | 2 days |
| Step 5: Move Loan transactions | 0.5 days | 2.5 days |
| Step 6: Move MPToken transactions | 0.25 days | 2.75 days |
| Step 7: Move XChain, Check, Escrow, PayChan | 0.5 days | 3.25 days |
| Step 8: Move Offer, Credential, Oracle, Domain | 0.5 days | 3.75 days |
| Step 9: Move Core transactions | 0.5 days | 4.25 days |
| Step 10: Update remaining include paths | 0.5 days | 4.75 days |
| Step 11: Final verification and testing | 0.5 days | 5.25 days |

**Total Estimated Time**: 5-6 working days

---

## Related Tasks

- **Phase 3 Task 3.1**: Module boundary documentation
- **Phase 3 Task 3.3**: Split `app/ledger/detail` (similar pattern)
- **Phase 3 Task 3.4**: Split `protocol/detail` if applicable
- **Phase 4**: Build system integration with new module boundaries

---

## Appendix: Automation Script

Consider creating a Python script to automate the file moves and include updates:

```python
#!/usr/bin/env python3
"""Script to move transaction files and update includes."""

import os
import re
import subprocess
from pathlib import Path

DETAIL_DIR = Path("src/xrpld/app/tx/detail")
INCLUDE_DIR = Path("include/xrpl/protocol/detail")

FILE_MAPPINGS = {
    "amm": ["AMMBid", "AMMClawback", "AMMCreate", "AMMDelete",
            "AMMDeposit", "AMMVote", "AMMWithdraw"],
    "nft": ["NFTokenAcceptOffer", "NFTokenBurn", "NFTokenCancelOffer",
            "NFTokenCreateOffer", "NFTokenMint", "NFTokenModify", "NFTokenUtils"],
    # ... add remaining mappings
}

def move_files(subdir: str, files: list[str]):
    """Move files to subdirectory and update includes."""
    target_dir = DETAIL_DIR / subdir
    target_dir.mkdir(exist_ok=True)

    for filename in files:
        for ext in [".h", ".cpp"]:
            src = DETAIL_DIR / f"{filename}{ext}"
            if src.exists():
                subprocess.run(["git", "mv", str(src), str(target_dir / f"{filename}{ext}")])

def update_includes(old_path: str, new_path: str):
    """Update include statements across the codebase."""
    subprocess.run([
        "grep", "-rl", old_path, "src/", "include/"
    ], capture_output=True)
    # ... implement sed replacement

if __name__ == "__main__":
    for subdir, files in FILE_MAPPINGS.items():
        print(f"Processing {subdir}...")
        move_files(subdir, files)
    print("Done! Run build to verify.")
```

