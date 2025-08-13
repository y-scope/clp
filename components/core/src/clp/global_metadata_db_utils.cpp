#include "global_metadata_db_utils.hpp"

#include <spdlog/spdlog.h>

#include "ErrorCode.hpp"
#include "GlobalMetadataDBConfig.hpp"
#include "GlobalMySQLMetadataDB.hpp"
#include "GlobalSQLiteMetadataDB.hpp"
#include "TraceableException.hpp"

namespace clp {
auto create_global_metadata_db(
        std::optional<GlobalMetadataDBConfig> const& optional_metadata_db_config,
        std::filesystem::path const& archives_dir
) -> std::unique_ptr<GlobalMetadataDB> {
    constexpr std::string_view cErrorMsgPrefix = "Failed to load global metadata DB config: ";
    try {
        auto const& metadata_db_config = optional_metadata_db_config.value();
        switch (metadata_db_config.get_metadata_db_type()) {
            case GlobalMetadataDBConfig::MetadataDBType::SQLite: {
                auto global_metadata_db_path
                        = archives_dir / streaming_archive::cMetadataDBFileName;
                return std::make_unique<GlobalSQLiteMetadataDB>(global_metadata_db_path.string());
            }
            case GlobalMetadataDBConfig::MetadataDBType::MySQL: {
                return std::make_unique<GlobalMySQLMetadataDB>(
                        metadata_db_config.get_metadata_db_host(),
                        metadata_db_config.get_metadata_db_port(),
                        metadata_db_config.get_metadata_db_username().value(),
                        metadata_db_config.get_metadata_db_password().value(),
                        metadata_db_config.get_metadata_db_name(),
                        metadata_db_config.get_metadata_table_prefix()
                );
            }
            default:
                SPDLOG_ERROR(
                        "Unhandled metadata DB type: {}",
                        static_cast<int>(metadata_db_config.get_metadata_db_type())
                );
                return nullptr;
        }
    } catch (TraceableException const& e) {
        auto error_code = e.get_error_code();
        if (ErrorCode_errno == error_code) {
            SPDLOG_ERROR(
                    "{}: {}:{} {}, errno={}",
                    cErrorMsgPrefix,
                    e.get_filename(),
                    e.get_line_number(),
                    e.what(),
                    errno
            );
        } else {
            SPDLOG_ERROR(
                    "{}: {}:{} {}, error_code={}",
                    cErrorMsgPrefix,
                    e.get_filename(),
                    e.get_line_number(),
                    e.what(),
                    error_code
            );
        }
    } catch (std::exception const& e) {
        SPDLOG_ERROR("{}: {}", cErrorMsgPrefix, e.what());
    }
    return nullptr;
}
}  // namespace clp
