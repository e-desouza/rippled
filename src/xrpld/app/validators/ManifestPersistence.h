#ifndef XRPLD_APP_VALIDATORS_MANIFESTPERSISTENCE_H_INCLUDED
#define XRPLD_APP_VALIDATORS_MANIFESTPERSISTENCE_H_INCLUDED

#include <xrpl/validators/Manifest.h>

#include <functional>
#include <string>
#include <vector>

namespace xrpl {

class DatabaseCon;

/** Populate manifest cache with manifests in database and config.

    @param cache ManifestCache to populate

    @param dbCon Database connection with dbTable

    @param dbTable Database table

    @param configManifest Base64 encoded manifest for local node's
        validator keys

    @param configRevocation Base64 encoded validator key revocation
        from the config

    @return true if successfully loaded, false otherwise

    @par Thread Safety

    May be called concurrently
*/
bool
loadManifests(
    ManifestCache& cache,
    DatabaseCon& dbCon,
    std::string const& dbTable,
    std::string const& configManifest,
    std::vector<std::string> const& configRevocation);

/** Populate manifest cache with manifests in database.

    @param cache ManifestCache to populate

    @param dbCon Database connection with dbTable

    @param dbTable Database table

    @par Thread Safety

    May be called concurrently
*/
void
loadManifests(
    ManifestCache& cache,
    DatabaseCon& dbCon,
    std::string const& dbTable);

/** Save cached manifests to database.

    @param cache ManifestCache to save from

    @param dbCon Database connection with `ValidatorManifests` table

    @param dbTable Database table name

    @param isTrusted Function that returns true if manifest is trusted

    @par Thread Safety

    May be called concurrently
*/
void
saveManifests(
    ManifestCache const& cache,
    DatabaseCon& dbCon,
    std::string const& dbTable,
    std::function<bool(PublicKey const&)> const& isTrusted);

}  // namespace xrpl

#endif
