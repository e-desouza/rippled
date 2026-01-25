#ifndef XRPL_APP_RDB_PEERFINDER_H_INCLUDED
#define XRPL_APP_RDB_PEERFINDER_H_INCLUDED

// This header re-exports the PeerFinder database functions from peerfinder
// for backward compatibility. The actual declarations are now in peerfinder
// to break the app ↔ peerfinder dependency cycle.

#include <xrpld/peerfinder/detail/PeerFinderDb.h>

#endif
