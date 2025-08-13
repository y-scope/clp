#ifndef CLP_GLOBAL_METADATA_DB_UTILS_HPP
#define CLP_GLOBAL_METADATA_DB_UTILS_HPP

#include <filesystem>
#include <memory>
#include <optional>

#include "GlobalMetadataDB.hpp"
#include "GlobalMetadataDBConfig.hpp"

namespace clp {
/**
 * Creates a GlobalMetadataDB instance based on the given configuration.
 *
 * @param optional_metadata_db_config
 * @param archives_dir
 * @return The created GlobalMetadataDB instance, or nullptr on failure.
 */
auto create_global_metadata_db(
        std::optional<GlobalMetadataDBConfig> const& optional_metadata_db_config,
        std::filesystem::path const& archives_dir
) -> std::unique_ptr<GlobalMetadataDB>;
}  // namespace clp

#endif  // CLP_GLOBAL_METADATA_DB_UTILS_HPP
