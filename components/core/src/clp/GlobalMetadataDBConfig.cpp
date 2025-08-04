#include "GlobalMetadataDBConfig.hpp"

#include <cstdlib>
#include <stdexcept>

#include <fmt/core.h>

#include "type_utils.hpp"
using std::invalid_argument;
using std::string;

namespace {
// Constants
constexpr int cMinPort{1};
constexpr int cMaxPort{65'535};
}  // namespace

namespace clp {
auto operator>>(std::istream& in, GlobalMetadataDBConfig::MetadataDBType& metadata_db_type)
        -> std::istream& {
    string db_type_string;
    in >> db_type_string;

    for (size_t i = 0; i < GlobalMetadataDBConfig::cMetadataDBTypeNames.size(); ++i) {
        if (GlobalMetadataDBConfig::cMetadataDBTypeNames[i] == db_type_string) {
            metadata_db_type = static_cast<GlobalMetadataDBConfig::MetadataDBType>(i);
            return in;
        }
    }

    throw invalid_argument(fmt::format("Unknown database type: {}", db_type_string));
}

auto GlobalMetadataDBConfig::add_command_line_options(
        boost::program_options::options_description& options_description
) -> void {
    constexpr std::string_view cMetadataDbTypeMysqlOptDescPrefix{"[db-type=mysql]"};

    options_description.add_options()
        ("db-type",
         boost::program_options::value<MetadataDBType>(&m_metadata_db_type)
         ->default_value(cDefaultMetadataDbType,
                         cMetadataDBTypeNames[enum_to_underlying_type(cDefaultMetadataDbType)].
                         data()),
         fmt::format("Database type [{} | {}]",
                        cMetadataDBTypeNames
                            [enum_to_underlying_type(MetadataDBType::SQLite)],
                        cMetadataDBTypeNames[enum_to_underlying_type(MetadataDBType::MySQL)]).c_str()
             )
        ("db-host",
         boost::program_options::value<string>(&m_metadata_db_host)->default_value(
             cDefaultMetadataDbHost.data()),
             fmt::format(
                 "{} Database host",
                 cMetadataDbTypeMysqlOptDescPrefix
             ).c_str())
        ("db-port",
         boost::program_options::value<int>(&m_metadata_db_port)->default_value(
             cDefaultMetadataDbPort),
                fmt::format(
                    "{} Database port (default: {})",
                    cMetadataDbTypeMysqlOptDescPrefix,
                    cDefaultMetadataDbPort
                ).c_str())
        ("db-name",
         boost::program_options::value<string>(&m_metadata_db_name)->default_value(
             cDefaultMetadataDbName.data()),
             fmt::format(
                 "{} Database name (default: {})",
                 cMetadataDbTypeMysqlOptDescPrefix,
                 cDefaultMetadataDbName.data()
             ).c_str())
        ("db-table-prefix",
         boost::program_options::value<string>(&m_metadata_table_prefix)->default_value(
             cDefaultMetadataTablePrefix.data()),
                fmt::format(
                    "{} Database table prefix (default: {})",
                    cMetadataDbTypeMysqlOptDescPrefix,
                    cDefaultMetadataTablePrefix.data()
                ).c_str());
}

auto GlobalMetadataDBConfig::read_credentials_from_env_if_needed() -> void {
    if (MetadataDBType::SQLite == m_metadata_db_type) {
        // SQLite does not require credentials.
        return;
    }

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
    if (m_metadata_db_type == MetadataDBType::SQLite) {
        // SQLite does not require extra parameters.
        if (false == m_metadata_db_host.empty() || cDefaultMetadataDbPort != m_metadata_db_port
            || false == m_metadata_db_name.empty() || false == m_metadata_table_prefix.empty()
            || false == m_metadata_db_username.empty() || false == m_metadata_db_password.empty())
        {
            throw invalid_argument(
                    "MySQL-specific parameters were provided when '--db-type' is 'SQLite'. "
                    "Please remove them or set '--db-type=mysql'."
            );
        }
        return;
    }

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
}  // namespace clp
