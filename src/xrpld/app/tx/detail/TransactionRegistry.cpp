#include <xrpld/app/tx/detail/TransactionRegistry.h>

#include <xrpl/beast/utility/instrumentation.h>

namespace xrpl {

TransactionRegistry&
TransactionRegistry::instance()
{
    static TransactionRegistry registry;
    return registry;
}

void
TransactionRegistry::registerHandler(
    TxType type,
    std::unique_ptr<ITransactionHandler> handler)
{
    auto [it, inserted] = handlers_.emplace(type, std::move(handler));
    // Duplicate registration is a programming error
    XRPL_ASSERT(
        inserted,
        "xrpl::TransactionRegistry::registerHandler : duplicate registration");
    (void)it;  // Suppress unused variable warning
}

ITransactionHandler const*
TransactionRegistry::getHandler(TxType type) const
{
    auto it = handlers_.find(type);
    return it != handlers_.end() ? it->second.get() : nullptr;
}

bool
TransactionRegistry::hasHandler(TxType type) const
{
    return handlers_.count(type) > 0;
}

std::size_t
TransactionRegistry::size() const
{
    return handlers_.size();
}

}  // namespace xrpl
