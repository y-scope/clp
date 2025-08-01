#include "GlobalMetadataDBConfig.hpp"

#include <cstdlib>
#include <stdexcept>

#include <fmt/core.h>

using std::invalid_argument;
using std::string;

namespace clp {
// Constants
constexpr int cMinPort{1};
constexpr int cMaxPort{65'535};

auto operator>>(std::istream& in, GlobalMetadataDBConfig::MetadataDBType& metadata_db_type)
        -> std::istream& {
    string db_type_string;
    in >> db_type_string;

    if ("sqlite" == db_type_string) {
        metadata_db_type = GlobalMetadataDBConfig::MetadataDBType::SQLite;
    } else if ("mysql" == db_type_string) {
        metadata_db_type = GlobalMetadataDBConfig::MetadataDBType::MySQL;
    } else {
        throw invalid_argument("Unknown database type: " + db_type_string);
    }

    return in;
}

auto GlobalMetadataDBConfig::add_command_line_options(
        boost::program_options::options_description& options_description
) -> void {
    options_description.add_options()
            ("db-type",
             boost::program_options::value<MetadataDBType>(&m_metadata_db_type)->default_value(cDefaultMetadataDbType, cDefaultMetadataDbTypeName.data()),
             "Database type [sqlite | mysql]")
            ("db-host",
             boost::program_options::value<string>(&m_metadata_db_host)->default_value(cDefaultMetadataDbHost.data()),
             "[db-type=mysql] Database host")
            ("db-port",
             boost::program_options::value<int>(&m_metadata_db_port)->default_value(cDefaultMetadataDbPort),
             "[db-type=mysql] Database port")
            ("db-name",
             boost::program_options::value<string>(&m_metadata_db_name)->default_value(cDefaultMetadataDbName.data()),
             "[db-type=mysql] Database name")
            ("db-table-prefix",
             boost::program_options::value<string>(&m_metadata_table_prefix)->default_value(cDefaultMetadataTablePrefix.data()),
             "[db-type=mysql] Database table prefix");
}

auto GlobalMetadataDBConfig::read_credentials_from_env() -> void {
    auto const* db_username{std::getenv("CLP_DB_USER")};
    if (nullptr != db_username) {
        m_metadata_db_username = db_username;
    }

    auto const* db_password{std::getenv("CLP_DB_PASS")};
    if (nullptr != db_password) {
        m_metadata_db_password = db_password;
    }
}

auto GlobalMetadataDBConfig::validate() const -> void {
    if (m_metadata_db_type == MetadataDBType::MySQL) {
        if (m_metadata_db_host.empty()) {
            throw invalid_argument("Database '--db-host' is empty.");
        }

        if (cMinPort > m_metadata_db_port || cMaxPort < m_metadata_db_port) {
            throw invalid_argument(
                    fmt::format(
                            "Database '--db-port' is out of range [{}, {}]: {}",
                            cMinPort,
                            cMaxPort,
                            m_metadata_db_port
                    )
            );
        }

        if (m_metadata_db_name.empty()) {
            throw invalid_argument("Database '--db-name' is empty.");
        }

        if (m_metadata_table_prefix.empty()) {
            throw invalid_argument("Database '--db-table_prefix' is empty.");
        }

        if (m_metadata_db_username.empty()) {
            throw invalid_argument("Database 'CLP_DB_USER' not specified or empty.");
        }

        if (m_metadata_db_password.empty()) {
            throw invalid_argument("Database 'CLP_DB_PASS' not specified or empty.");
        }
    }
}
}  // namespace clp
