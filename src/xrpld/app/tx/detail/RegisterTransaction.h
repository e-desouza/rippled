#ifndef XRPL_APP_TX_REGISTERTRANSACTION_H_INCLUDED
#define XRPL_APP_TX_REGISTERTRANSACTION_H_INCLUDED

#include <xrpld/app/tx/detail/TransactionHandlerAdapter.h>
#include <xrpld/app/tx/detail/TransactionRegistry.h>

#include <memory>

namespace xrpl {

/**
 * Helper class for static registration of transaction handlers.
 *
 * This class is instantiated as a static variable in each transactor's
 * .cpp file, causing the handler to be registered during static
 * initialization before main() runs.
 */
template <typename T>
struct TransactionRegistrar
{
    TransactionRegistrar(TxType type)
    {
        TransactionRegistry::instance().registerHandler(
            type, std::make_unique<TransactionHandlerAdapter<T>>());
    }
};

}  // namespace xrpl

/**
 * Macro to register a transaction handler.
 *
 * Usage: Add at the end of each transactor's .cpp file:
 *   REGISTER_TRANSACTION(ttPAYMENT, Payment)
 *
 * This creates a static variable that registers the handler during
 * static initialization.
 *
 * @param TxTypeName The TxType enum value (e.g., ttPAYMENT)
 * @param TransactorClass The Transactor-derived class (e.g., Payment)
 */
#define REGISTER_TRANSACTION(TxTypeName, TransactorClass)      \
    namespace {                                                \
    static ::xrpl::TransactionRegistrar<TransactorClass> const \
        registrar_##TransactorClass{TxTypeName};               \
    }

#endif
