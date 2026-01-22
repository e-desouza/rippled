# Phase 4 Task 4.4: Architecture Guide

## Overview

### Problem Statement
The rippled codebase uses sophisticated architectural patterns including Dependency Injection and a levelization system that enforce clean module boundaries. These patterns are not well documented, creating a barrier for new contributors who need to understand:
- How the module tier system works
- Where to place new code
- How to follow dependency inversion patterns
- How the composition root (Application.cpp) wires everything together

### Success Criteria
- New contributors can understand the module hierarchy within 30 minutes
- Architecture patterns are clearly explained with code examples
- Diagrams illustrate complex module relationships
- Guidelines exist for adding new features (transactions, RPC handlers, etc.)

### Target Audience
- New contributors to rippled
- Developers adding new features
- Engineers performing architectural refactoring

---

## Deep Code Analysis

### Current Documentation Gaps

| Area | Current State | Gap |
|------|---------------|-----|
| Levelization | Script exists in `.github/scripts/levelization/` | No explanation of tier rationale |
| Dependency Injection | Patterns used throughout | No documented conventions |
| Module Responsibilities | Implicit in code structure | No explicit documentation |
| Adding New Features | Tribal knowledge | No written guide |

### Key Architectural Patterns Used

#### 1. Levelization System
**Location:** `.github/scripts/levelization/`

**Files:**
- `classifications.txt` - Defines module tiers
- `generate.sh` - Checks for violations
- `loops.txt` - Tracks allowed circular dependencies

**Tier Hierarchy (from classifications.txt):**
```
Tier 0 (External/Foundation): beast, basics, json, crypto
Tier 1 (Protocol):            protocol
Tier 2 (Core Infrastructure): core, resource, server, nodestore
Tier 3 (Data Structures):     shamap, net, ledger
Tier 4 (Application):         peerfinder, overlay, consensus, rpc, app
```

**Rule:** Lower tiers CANNOT depend on higher tiers. Higher tiers depend downward.

#### 2. Interface Segregation Pattern
**Example:** `LedgerDataProvider` (from Phase 2 refactoring)

```cpp
// Lower tier: interface definition (include/xrpl/ledger/)
class LedgerDataProvider {
public:
    virtual ~LedgerDataProvider() = default;
    virtual std::shared_ptr<Ledger const> getCurrentLedger() const = 0;
    virtual std::shared_ptr<Ledger const> getValidatedLedger() const = 0;
};

// Higher tier: implementation (src/xrpl/app/)
class LedgerMaster : public LedgerDataProvider { ... };
```

#### 3. Factory Function Pattern
**Examples in codebase:**
- `make_Overlay()` - Creates overlay network layer
- `make_LedgerMaster()` - Creates ledger management
- `make_NetworkOPs()` - Creates network operations

**Pattern:**
```cpp
// Declaration in header (lower tier)
std::unique_ptr<Overlay> make_Overlay(Application& app, ...);

// Implementation in source (same or higher tier)
std::unique_ptr<Overlay> make_Overlay(Application& app, ...) {
    return std::make_unique<OverlayImpl>(app, ...);
}
```

#### 4. Composition Root
**Location:** `src/xrpl/app/main/Application.cpp`

The `ApplicationImp` class wires together all major components:
- Creates concrete implementations via factory functions
- Injects dependencies through constructor parameters
- Manages component lifecycles

---

## Design Considerations

### Documentation Format Options

| Format | Pros | Cons | Recommendation |
|--------|------|------|----------------|
| Markdown | Easy to read, GitHub renders | No code integration | ✅ Primary |
| Doxygen | Integrates with code | Harder to discover | ✅ Supplementary |
| ADRs | Captures decisions | Too granular | ❌ Not needed |

### Recommended Approach
1. **Primary:** `docs/ARCHITECTURE.md` - High-level guide
2. **Supplementary:** Doxygen comments in key interfaces
3. **Inline:** Comments explaining pattern usage in complex code

---

## Implementation Plan

### Step 1: Create docs/ARCHITECTURE.md Skeleton
- Create basic structure with all section headers
- Add table of contents
- **Estimate:** 1 hour

### Step 2: Document Levelization System
- Explain tier rationale
- Document how to run levelization checks
- Show examples of valid/invalid dependencies
- **Estimate:** 2 hours

### Step 3: Document Dependency Injection Patterns
- Interface segregation with examples
- Factory function conventions
- Composition root walkthrough
- **Estimate:** 3 hours

### Step 4: Document Module Responsibilities
- Create table of modules with responsibilities
- Document key classes in each module
- **Estimate:** 2 hours

### Step 5: Add Architecture Diagrams
- Module dependency diagram (Mermaid)
- Component interaction diagram
- Request flow diagrams (RPC, P2P)
- **Estimate:** 3 hours

### Step 6: Add Contribution Guidelines
- How to add new transactions
- How to add new RPC handlers
- How to add new modules
- **Estimate:** 2 hours

**Total Estimate:** 13 hours

---

## Content Outline for ARCHITECTURE.md

```markdown
# rippled Architecture Guide

## Table of Contents

## 1. Module System
### 1.1 Tier Hierarchy
### 1.2 Levelization Enforcement
### 1.3 Running Levelization Checks

## 2. Dependency Inversion Patterns
### 2.1 Interface Definition Guidelines
### 2.2 Factory Function Patterns
### 2.3 Composition Root (Application.cpp)
### 2.4 Example: LedgerDataProvider

## 3. Module Responsibilities
### 3.1 core/ - Configuration, Database, Threading
### 3.2 protocol/ - XRPL Protocol Types
### 3.3 ledger/ - Ledger Data Structures
### 3.4 app/ - Business Logic
### 3.5 rpc/ - API Handlers
### 3.6 overlay/ - P2P Networking
### 3.7 consensus/ - Consensus Algorithm

## 4. Adding New Features
### 4.1 Where to Put New Code
### 4.2 Adding New Transactions
### 4.3 Adding New RPC Handlers
### 4.4 Adding New Ledger Objects

## 5. Testing Architecture
### 5.1 Unit Test Framework (jtx)
### 5.2 Integration Tests
### 5.3 Fuzz Testing
### 5.4 Test Data Builders

## Appendix A: Architecture Diagrams
## Appendix B: Glossary
```

---

## Risk Assessment

| Risk | Severity | Mitigation |
|------|----------|------------|
| Documentation becomes stale | Medium | Add doc review to PR checklist |
| Incomplete coverage | Low | Iterative improvement based on feedback |
| Incorrect information | Medium | Technical review by core maintainers |
| Over-documentation | Low | Focus on "why" not "what" |

---

## Validation Criteria

### Functional Validation
- [ ] New contributor can locate correct module for new feature
- [ ] Architecture patterns are demonstrated with real code examples
- [ ] Levelization rules are clearly explained
- [ ] Diagrams render correctly in GitHub

### Review Validation
- [ ] Core maintainer approves accuracy
- [ ] At least one new contributor validates usability
- [ ] All code examples compile/are syntactically correct

---

## Dependencies

- None (documentation-only task)

## Related Tasks

- Phase 2: LedgerDataProvider refactoring (provides DI example)
- Phase 3: Test framework documentation

## Notes

- Consider creating a "Architecture" label for PRs that affect architecture
- May want to add architecture checks to CI (beyond levelization)
- Could integrate with onboarding documentation for new team members

