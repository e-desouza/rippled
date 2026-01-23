#ifndef XRPL_APP_MISC_OPERATINGMODEMANAGER_H_INCLUDED
#define XRPL_APP_MISC_OPERATINGMODEMANAGER_H_INCLUDED

#include <xrpld/app/misc/NetworkOPs.h>

#include <xrpl/beast/utility/Journal.h>
#include <xrpl/json/json_value.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>

namespace xrpl {

/**
 * Manages the operating mode state of a rippled node.
 *
 * This class encapsulates all state and logic related to the server's
 * operating mode (DISCONNECTED, CONNECTED, SYNCING, TRACKING, FULL)
 * and related blocked states (amendment blocked, UNL blocked).
 *
 * State accounting records two attributes for each possible server state:
 * 1) Amount of time spent in each state (in microseconds)
 * 2) Number of transitions to each state
 */
class OperatingModeManager
{
public:
    /**
     * Callback type for notifying of mode changes.
     * Called when the operating mode changes, after accounting is updated.
     */
    using ModeChangeCallback = std::function<void()>;

    /**
     * Callback type for getting validated ledger age.
     * Returns the age of the last validated ledger.
     */
    using GetValidatedLedgerAgeCallback =
        std::function<std::chrono::seconds()>;

    /**
     * Construct an OperatingModeManager.
     *
     * @param startMode Initial operating mode (typically DISCONNECTED or FULL)
     * @param journal Logger for mode transitions
     * @param onModeChange Callback invoked after mode changes
     * @param getValidatedLedgerAge Callback to get validated ledger age
     */
    OperatingModeManager(
        OperatingMode startMode,
        beast::Journal journal,
        ModeChangeCallback onModeChange,
        GetValidatedLedgerAgeCallback getValidatedLedgerAge);

    /** Destructor - must be defined in .cpp where StateAccounting is complete */
    ~OperatingModeManager();

    // Non-copyable, non-movable due to atomics and unique_ptr
    OperatingModeManager(OperatingModeManager const&) = delete;
    OperatingModeManager& operator=(OperatingModeManager const&) = delete;
    OperatingModeManager(OperatingModeManager&&) = delete;
    OperatingModeManager& operator=(OperatingModeManager&&) = delete;

    // --- Operating Mode Accessors ---

    /** Get the current operating mode. */
    OperatingMode
    getOperatingMode() const;

    /** Get a string representation of the operating mode. */
    std::string
    strOperatingMode(OperatingMode mode, bool admin) const;

    /** Get a string representation of the current operating mode. */
    std::string
    strOperatingMode(bool admin = false) const;

    /** Check if the server is in FULL mode and not needing a network ledger. */
    bool
    isFull() const;

    // --- Mode Setters ---

    /** Set the operating mode, applying validation/age checks. */
    void
    setMode(OperatingMode om);

    /** Set the operating mode directly without validation checks. */
    void
    setModeImpl(OperatingMode om);

    /** Set the server to standalone mode (immediately FULL). */
    void
    setStandAlone();

    // --- Blocked State ---

    /** Check if the server is blocked (amendment or UNL blocked). */
    bool
    isBlocked() const;

    /** Check if the server is blocked due to amendments. */
    bool
    isAmendmentBlocked() const;

    /** Set the amendment blocked state. */
    void
    setAmendmentBlocked();

    /** Check if the server has an amendment warning. */
    bool
    isAmendmentWarned() const;

    /** Set the amendment warning state. */
    void
    setAmendmentWarned();

    /** Clear the amendment warning state. */
    void
    clearAmendmentWarned();

    /** Check if the server is blocked due to UNL issues. */
    bool
    isUNLBlocked() const;

    /** Set the UNL blocked state. */
    void
    setUNLBlocked();

    /** Clear the UNL blocked state. */
    void
    clearUNLBlocked();

    // --- Network Ledger State ---

    /** Check if the server needs a network ledger. */
    bool
    isNeedNetworkLedger() const;

    /** Set the need network ledger flag. */
    void
    setNeedNetworkLedger();

    /** Clear the need network ledger flag. */
    void
    clearNeedNetworkLedger();

    // --- State Accounting ---

    /** Output state accounting data to JSON. */
    void
    stateAccounting(Json::Value& obj) const;

private:
    class StateAccounting;
    std::unique_ptr<StateAccounting> accounting_;

    beast::Journal journal_;
    ModeChangeCallback onModeChange_;
    GetValidatedLedgerAgeCallback getValidatedLedgerAge_;

    std::atomic<OperatingMode> mode_;
    std::atomic<bool> needNetworkLedger_{false};
    std::atomic<bool> amendmentBlocked_{false};
    std::atomic<bool> amendmentWarned_{false};
    std::atomic<bool> unlBlocked_{false};
};

}  // namespace xrpl

#endif

