# Cycle 3: xrpld.app ↔ xrpld.peerfinder

## Current State

**Loop detected:** `xrpld.peerfinder ~= xrpld.app`

The `~=` indicates a weak/approximate cycle. Peerfinder has 1 include from app, and app has 2 includes from peerfinder.

## Dependency Analysis

### peerfinder → app Dependencies (1 include) - PROBLEMATIC

| File                 | Includes               | Purpose                              |
| -------------------- | ---------------------- | ------------------------------------ |
| `detail/StoreSqdb.h` | `app/rdb/PeerFinder.h` | Database operations for peer storage |

### app → peerfinder Dependencies (2 includes) - ALLOWED

| File                   | Includes             | Purpose         |
| ---------------------- | -------------------- | --------------- |
| `app/rdb/State.h`      | `peerfinder/Store.h` | Store interface |
| `app/rdb/PeerFinder.h` | `peerfinder/Store.h` | Store interface |

## Root Cause

The cycle exists because:

1. `peerfinder/detail/StoreSqdb.h` includes `app/rdb/PeerFinder.h` for database operations
2. `app/rdb/PeerFinder.h` includes `peerfinder/Store.h` for the Store interface

This creates a circular dependency:

```
peerfinder/detail/StoreSqdb.h → app/rdb/PeerFinder.h → peerfinder/Store.h
```

## Removal Strategy

### Step 1: Analyze the dependency

Let's understand what `StoreSqdb.h` needs from `app/rdb/PeerFinder.h`:

```cpp
// Current: peerfinder/detail/StoreSqdb.h
#include <xrpld/app/rdb/PeerFinder.h>  // For database functions
```

The `app/rdb/PeerFinder.h` provides database helper functions for peer storage.

### Step 2: Move database functions to peerfinder module

**Option A: Move functions to peerfinder**

Move the peer-finder-specific database functions from `app/rdb/PeerFinder.h` to `peerfinder/detail/PeerFinderDb.h`:

```cpp
// NEW: src/xrpld/peerfinder/detail/PeerFinderDb.h
#ifndef RIPPLE_PEERFINDER_PEER_FINDER_DB_H_INCLUDED
#define RIPPLE_PEERFINDER_PEER_FINDER_DB_H_INCLUDED

#include <xrpld/peerfinder/Store.h>
#include <soci/soci.h>

namespace ripple {
namespace PeerFinder {

// Database operations for peer storage
void savePeers(soci::session& session, std::vector<PeerInfo> const& peers);
std::vector<PeerInfo> loadPeers(soci::session& session);
void deletePeer(soci::session& session, beast::IP::Endpoint const& endpoint);

} // namespace PeerFinder
} // namespace ripple

#endif
```

**Option B: Use dependency injection**

Pass a database interface to `StoreSqdb` instead of including the header:

```cpp
// NEW: src/xrpld/peerfinder/IPeerFinderStore.h
class IPeerFinderStore {
public:
    virtual ~IPeerFinderStore() = default;
    virtual void save(std::vector<PeerInfo> const& peers) = 0;
    virtual std::vector<PeerInfo> load() = 0;
};
```

### Step 3: Update StoreSqdb.h

**After changes:**

```cpp
// peerfinder/detail/StoreSqdb.h - AFTER
#include <xrpld/peerfinder/detail/PeerFinderDb.h>  // Local include, no app dependency
// OR
#include <xrpld/peerfinder/IPeerFinderStore.h>     // Interface only
```

### Step 4: Update app/rdb/PeerFinder.h

If we move functions to peerfinder, update `app/rdb/PeerFinder.h` to:

1. Include the new peerfinder header
2. Re-export or delegate to the peerfinder functions

```cpp
// app/rdb/PeerFinder.h - AFTER
#include <xrpld/peerfinder/detail/PeerFinderDb.h>

// Re-export for backward compatibility
using PeerFinder::savePeers;
using PeerFinder::loadPeers;
```

## Expected Result After Changes

```
xrpld.app > xrpld.peerfinder  (app depends on peerfinder - ALLOWED)
xrpld.peerfinder > xrpl.*     (peerfinder depends on xrpl libs - ALLOWED)
```

No cycle between app and peerfinder.

## Verification Steps

1. Run levelization: `.github/scripts/levelization/generate.sh`
2. Verify no `Loop: xrpld.app xrpld.peerfinder` in output
3. Build: `cd .build && ninja -j4`
4. Run tests: `./xrpld --unittest=PeerFinder`

## Risk Assessment

| Risk                     | Likelihood | Impact | Mitigation              |
| ------------------------ | ---------- | ------ | ----------------------- |
| Breaking peer discovery  | Low        | High   | Test peer connections   |
| Database schema issues   | Low        | Medium | Verify schema unchanged |
| Missing function exports | Low        | Low    | Grep for usages         |

## Estimated Effort

**Low effort (1-2 days)** - Only 1 dependency to break.

## Recommended Approach

**Option A (Move functions)** is recommended because:

1. Simpler than creating new interfaces
2. Functions are already peer-finder-specific
3. Minimal code changes required
4. No new abstractions needed
