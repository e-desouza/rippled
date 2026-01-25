#ifndef XRPL_APP_TX_TRANSACTIONREGISTRY_H_INCLUDED
#define XRPL_APP_TX_TRANSACTIONREGISTRY_H_INCLUDED

#include <xrpld/app/tx/detail/ITransactionHandler.h>

#include <xrpl/protocol/TxFormats.h>

#include <memory>
#include <unordered_map>

namespace xrpl {

/**
 * Singleton registry for transaction handlers.
 *
 * Provides O(1) lookup of transaction handlers by TxType.
 * Handlers are registered at static initialization time via
 * the REGISTER_TRANSACTION macro.
 */
class TransactionRegistry
{
public:
    /**
     * Get the singleton instance.
     * Uses function-local static to avoid static initialization order issues.
     */
    static TransactionRegistry&
    instance();

    /**
     * Register a handler for a transaction type.
     * Called during static initialization.
     *
     * @param type The transaction type
     * @param handler The handler instance (ownership transferred)
     */
    void
    registerHandler(TxType type, std::unique_ptr<ITransactionHandler> handler);

    /**
     * Get the handler for a transaction type.
     *
     * @param type The transaction type
     * @return Pointer to handler, or nullptr if not found
     */
    ITransactionHandler const*
    getHandler(TxType type) const;

    /**
     * Check if a handler is registered for a transaction type.
     *
     * @param type The transaction type
     * @return true if handler exists
     */
    bool
    hasHandler(TxType type) const;

    /**
     * Get the number of registered handlers.
     * Useful for validation during startup.
     */
    std::size_t
    size() const;

    /**
     * Iterate over all registered handlers.
     *
     * @param func Callback function(TxType, ITransactionHandler const&)
     */
    template <typename F>
    void
    forEach(F&& func) const
    {
        for (auto const& [type, handler] : handlers_)
        {
            func(type, *handler);
        }
    }

private:
    TransactionRegistry() = default;
    TransactionRegistry(TransactionRegistry const&) = delete;
    TransactionRegistry&
    operator=(TransactionRegistry const&) = delete;

    std::unordered_map<TxType, std::unique_ptr<ITransactionHandler>> handlers_;
};

}  // namespace xrpl

#endif
