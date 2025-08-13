#include "global_metadata_db_utils.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>

#include <spdlog/spdlog.h>

#include "GlobalMetadataDB.hpp"
#include "GlobalMetadataDBConfig.hpp"
#include "GlobalMySQLMetadataDB.hpp"
#include "GlobalSQLiteMetadataDB.hpp"
#include "streaming_archive/Constants.hpp"

namespace clp {
auto create_global_metadata_db(
        std::optional<GlobalMetadataDBConfig> const& optional_metadata_db_config,
        std::filesystem::path const& archives_dir
) -> std::unique_ptr<GlobalMetadataDB> {
    constexpr std::string_view cErrorMsgPrefix = "Failed to load global metadata DB config";

    if (false == optional_metadata_db_config.has_value()) {
        SPDLOG_ERROR("{}: Global metadata DB config not set.", cErrorMsgPrefix);
        return {};
    }
    auto const& metadata_db_config = optional_metadata_db_config.value();

    switch (metadata_db_config.get_metadata_db_type()) {
        case GlobalMetadataDBConfig::MetadataDBType::SQLite: {
            auto global_metadata_db_path = archives_dir / streaming_archive::cMetadataDBFileName;
            return std::make_unique<GlobalSQLiteMetadataDB>(global_metadata_db_path.string());
        }
        case GlobalMetadataDBConfig::MetadataDBType::MySQL: {
            auto const& metadata_db_username = metadata_db_config.get_metadata_db_username();
            auto const& metadata_db_password = metadata_db_config.get_metadata_db_password();
            if (false == metadata_db_username.has_value()
                || false == metadata_db_password.has_value())
            {
                SPDLOG_ERROR(
                        "{}: Global MySQL metadata DB credentials unexpectedly not set.",
                        cErrorMsgPrefix
                );
                return {};
            }
            return std::make_unique<GlobalMySQLMetadataDB>(
                    metadata_db_config.get_metadata_db_host(),
                    metadata_db_config.get_metadata_db_port(),
                    metadata_db_username.value(),
                    metadata_db_password.value(),
                    metadata_db_config.get_metadata_db_name(),
                    metadata_db_config.get_metadata_table_prefix()
            );
        }
        default:
            SPDLOG_ERROR(
                    "Unhandled metadata DB type: {}",
                    static_cast<int>(metadata_db_config.get_metadata_db_type())
            );
            return {};
    }
    return {};
}
}  // namespace clp
