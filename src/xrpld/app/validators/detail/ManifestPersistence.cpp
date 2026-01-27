#include <xrpld/app/rdb/Wallet.h>
#include <xrpld/app/validators/ManifestPersistence.h>
#include <xrpld/core/DatabaseCon.h>

#include <xrpl/basics/Log.h>
#include <xrpl/basics/base64.h>

#include <boost/algorithm/string/trim.hpp>

#include <numeric>

namespace xrpl {

void
loadManifests(
    ManifestCache& cache,
    DatabaseCon& dbCon,
    std::string const& dbTable)
{
    auto db = dbCon.checkoutDb();
    auto j = cache.journal();
    xrpl::getManifests(*db, dbTable, cache, j);
}

bool
loadManifests(
    ManifestCache& cache,
    DatabaseCon& dbCon,
    std::string const& dbTable,
    std::string const& configManifest,
    std::vector<std::string> const& configRevocation)
{
    auto j = cache.journal();

    loadManifests(cache, dbCon, dbTable);

    if (!configManifest.empty())
    {
        auto mo = deserializeManifest(base64_decode(configManifest));
        if (!mo)
        {
            JLOG(j.error()) << "Malformed validator_token in config";
            return false;
        }

        if (mo->revoked())
        {
            JLOG(j.warn()) << "Configured manifest revokes public key";
        }

        if (cache.applyManifest(std::move(*mo)) == ManifestDisposition::invalid)
        {
            JLOG(j.error()) << "Manifest in config was rejected";
            return false;
        }
    }

    if (!configRevocation.empty())
    {
        std::string revocationStr;
        revocationStr.reserve(std::accumulate(
            configRevocation.cbegin(),
            configRevocation.cend(),
            std::size_t(0),
            [](std::size_t init, std::string const& s) {
                return init + s.size();
            }));

        for (auto const& line : configRevocation)
            revocationStr += boost::algorithm::trim_copy(line);

        auto mo = deserializeManifest(base64_decode(revocationStr));

        if (!mo || !mo->revoked() ||
            cache.applyManifest(std::move(*mo)) == ManifestDisposition::invalid)
        {
            JLOG(j.error()) << "Invalid validator key revocation in config";
            return false;
        }
    }

    return true;
}

void
saveManifests(
    ManifestCache const& cache,
    DatabaseCon& dbCon,
    std::string const& dbTable,
    std::function<bool(PublicKey const&)> const& isTrusted)
{
    auto j = cache.journal();
    auto db = dbCon.checkoutDb();

    cache.with_map([&](auto const& map) {
        xrpl::saveManifests(*db, dbTable, isTrusted, map, j);
    });
}

}  // namespace xrpl
