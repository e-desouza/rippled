#ifndef XRPLD_APP_LEDGER_LEDGERDATAPROVIDER_H_INCLUDED
#define XRPLD_APP_LEDGER_LEDGERDATAPROVIDER_H_INCLUDED

#include <xrpl/beast/utility/Journal.h>
#include <xrpl/protocol/LedgerHeader.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

namespace xrpl {

class Ledger;
class ReadView;

/**
 * Interface for read-only ledger data access.
 * Used by RPC handlers to access ledger information without
 * depending on the full LedgerMaster implementation.
 */
class LedgerDataProvider
{
public:
    virtual ~LedgerDataProvider() = default;

    // Core ledger access
    [[nodiscard]] virtual std::shared_ptr<Ledger const>
    getValidatedLedger() = 0;

    [[nodiscard]] virtual std::shared_ptr<Ledger const>
    getClosedLedger() = 0;

    [[nodiscard]] virtual std::shared_ptr<ReadView const>
    getCurrentLedger() = 0;

    [[nodiscard]] virtual LedgerIndex
    getCurrentLedgerIndex() = 0;

    // Ledger lookup
    [[nodiscard]] virtual std::shared_ptr<Ledger const>
    getLedgerBySeq(std::uint32_t seq) = 0;

    [[nodiscard]] virtual std::shared_ptr<Ledger const>
    getLedgerByHash(uint256 const& hash) = 0;

    [[nodiscard]] virtual uint256
    getHashBySeq(std::uint32_t seq) = 0;

    // Validation queries
    [[nodiscard]] virtual bool
    isValidated(ReadView const& ledger) = 0;

    [[nodiscard]] virtual std::chrono::seconds
    getValidatedLedgerAge() = 0;

    [[nodiscard]] virtual std::optional<NetClock::time_point>
    getCloseTimeBySeq(LedgerIndex seq) = 0;

    [[nodiscard]] virtual bool
    getValidatedRange(std::uint32_t& minSeq, std::uint32_t& maxSeq) = 0;

    // Additional methods needed by RPC
    [[nodiscard]] virtual bool
    haveLedger(std::uint32_t seq) = 0;

    [[nodiscard]] virtual LedgerIndex
    getValidLedgerIndex() = 0;

    // This is the last ledger we published to clients and can lag the validated
    // ledger
    [[nodiscard]] virtual std::shared_ptr<ReadView const>
    getPublishedLedger() = 0;

    // Transaction lookup
    // Iff a txn exists at the specified ledger and offset then return its txnid
    [[nodiscard]] virtual std::optional<uint256>
    txnIdFromIndex(std::uint32_t ledgerSeq, std::uint32_t txnIndex) = 0;
};

}  // namespace xrpl

#endif
