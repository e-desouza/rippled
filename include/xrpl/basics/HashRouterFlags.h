#ifndef XRPL_BASICS_HASHROUTERFLAGS_H_INCLUDED
#define XRPL_BASICS_HASHROUTERFLAGS_H_INCLUDED

#include <cstdint>
#include <type_traits>

namespace xrpl {

/** Flags used by the HashRouter for tracking message state.

    These flags are used to track the state of hashed items (transactions,
    validations, etc.) as they are routed through the peer-to-peer overlay.
*/
enum class HashRouterFlags : std::uint16_t {
    // Public flags
    UNDEFINED = 0x00,
    BAD = 0x02,  // Temporarily bad
    SAVED = 0x04,
    HELD = 0x08,     // Held by LedgerMaster after potential processing failure
    TRUSTED = 0x10,  // Comes from a trusted source

    // Private flags (used internally in apply.cpp)
    // Do not attempt to read, set, or reuse.
    PRIVATE1 = 0x0100,
    PRIVATE2 = 0x0200,
    PRIVATE3 = 0x0400,
    PRIVATE4 = 0x0800,
    PRIVATE5 = 0x1000,
    PRIVATE6 = 0x2000
};

constexpr HashRouterFlags
operator|(HashRouterFlags lhs, HashRouterFlags rhs)
{
    return static_cast<HashRouterFlags>(
        static_cast<std::underlying_type_t<HashRouterFlags>>(lhs) |
        static_cast<std::underlying_type_t<HashRouterFlags>>(rhs));
}

constexpr HashRouterFlags&
operator|=(HashRouterFlags& lhs, HashRouterFlags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

constexpr HashRouterFlags
operator&(HashRouterFlags lhs, HashRouterFlags rhs)
{
    return static_cast<HashRouterFlags>(
        static_cast<std::underlying_type_t<HashRouterFlags>>(lhs) &
        static_cast<std::underlying_type_t<HashRouterFlags>>(rhs));
}

constexpr HashRouterFlags&
operator&=(HashRouterFlags& lhs, HashRouterFlags rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

constexpr bool
any(HashRouterFlags flags)
{
    return static_cast<std::underlying_type_t<HashRouterFlags>>(flags) != 0;
}

}  // namespace xrpl

#endif

