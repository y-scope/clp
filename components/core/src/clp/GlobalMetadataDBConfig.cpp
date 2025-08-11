#include "GlobalMetadataDBConfig.hpp"

#include <cstdlib>
#include <istream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <fmt/core.h>

#include "GlobalMySQLMetadataDB.hpp"
#include "type_utils.hpp"

using std::invalid_argument;
using std::string;

namespace {
// Constants
constexpr clp::GlobalMetadataDBConfig::MetadataDBType cDefaultMetadataDbType{
        clp::GlobalMetadataDBConfig::MetadataDBType::SQLite
};
constexpr std::string_view cDefaultMetadataDbHost{"127.0.0.1"};
constexpr int cDefaultMetadataDbPort{3306};
constexpr std::string_view cDefaultMetadataDbName{"clp-db"};
constexpr std::string_view cDefaultMetadataTablePrefix{"clp_"};
constexpr int cMinPort{1};
constexpr int cMaxPort{65'535};
}  // namespace

namespace clp {
auto operator>>(std::istream& in, GlobalMetadataDBConfig::MetadataDBType& metadata_db_type)
        -> std::istream& {
    string db_type_string;
    in >> db_type_string;

    for (size_t i = 0; i < GlobalMetadataDBConfig::cMetadataDBTypeNames.size(); ++i) {
        if (GlobalMetadataDBConfig::cMetadataDBTypeNames.at(i) == db_type_string) {
            metadata_db_type = static_cast<GlobalMetadataDBConfig::MetadataDBType>(i);
            return in;
        }
    }

    throw invalid_argument(fmt::format("Unknown database type: {}", db_type_string));
}

GlobalMetadataDBConfig::GlobalMetadataDBConfig(
        boost::program_options::options_description& options_description
) {
    std::string_view const cMetadataDbTypeMysqlOptDescPrefix{fmt::format(
            "(--db-type={} only)",
            cMetadataDBTypeNames[enum_to_underlying_type(MetadataDBType::MySQL)]
    )};

    // clang-format off
    options_description.add_options()
        (
            "db-type",
            boost::program_options::value<MetadataDBType>(&m_metadata_db_type)
                ->default_value(
                    cDefaultMetadataDbType,
                    string(
                        cMetadataDBTypeNames[
                            enum_to_underlying_type(cDefaultMetadataDbType)
                        ]
                    )
                ),
            fmt::format(
                "Database type [{} | {}]",
                cMetadataDBTypeNames[enum_to_underlying_type(MetadataDBType::SQLite)],
                cMetadataDBTypeNames[enum_to_underlying_type(MetadataDBType::MySQL)]
            ).c_str()
        )
        (
            "db-host",
            boost::program_options::value<string>(&m_metadata_db_host)
                ->default_value(string(cDefaultMetadataDbHost)),
            fmt::format(
                "{} Database host",
                cMetadataDbTypeMysqlOptDescPrefix
            ).c_str()
        )
        (
            "db-port",
            boost::program_options::value<int>(&m_metadata_db_port)
                ->default_value(cDefaultMetadataDbPort),
            fmt::format(
                "{} Database port",
                cMetadataDbTypeMysqlOptDescPrefix
            ).c_str()
        )
        (
            "db-name",
            boost::program_options::value<string>(&m_metadata_db_name)
                ->default_value(string(cDefaultMetadataDbName)),
            fmt::format(
                "{} Database name",
                cMetadataDbTypeMysqlOptDescPrefix
            ).c_str()
        )
        (
            "db-table-prefix",
            boost::program_options::value<string>(&m_metadata_table_prefix)
                ->default_value(string(cDefaultMetadataTablePrefix)),
            fmt::format(
                "{} Database table prefix",
                cMetadataDbTypeMysqlOptDescPrefix
            ).c_str()
        );
    // clang-format on
}

auto GlobalMetadataDBConfig::read_credentials_from_env_if_needed() -> void {
    if (MetadataDBType::SQLite == m_metadata_db_type) {
        // SQLite doesn't require extra parameters.
        return;
    }

    // Silence the check since this class won't be used in a multithreaded context.
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    if (auto const* db_username{std::getenv("CLP_DB_USER")}; nullptr != db_username) {
        m_metadata_db_username.emplace(db_username);
    }

    // Silence the check since this class won't be used in a multithreaded context.
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    if (auto const* db_password{std::getenv("CLP_DB_PASS")}; nullptr != db_password) {
        m_metadata_db_password.emplace(db_password);
    }
}

auto GlobalMetadataDBConfig::validate() const -> void {
    if (m_metadata_db_type == MetadataDBType::SQLite) {
        // SQLite doesn't require extra parameters.
        if (cDefaultMetadataDbHost != m_metadata_db_host
            || cDefaultMetadataDbPort != m_metadata_db_port
            || cDefaultMetadataDbName != m_metadata_db_name
            || cDefaultMetadataTablePrefix != m_metadata_table_prefix
            || m_metadata_db_username.has_value() || m_metadata_db_password.has_value())
        {
            throw invalid_argument(
                    fmt::format(
                            "MySQL-specific parameters cannot be used with --db-type={}."
                            " Please remove them or set '--db-type={}'.",
                            cMetadataDBTypeNames[enum_to_underlying_type(m_metadata_db_type)],
                            cMetadataDBTypeNames[enum_to_underlying_type(MetadataDBType::MySQL)]
                    )
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

    if (false == m_metadata_db_username.has_value()) {
        throw invalid_argument("Environment variable 'CLP_DB_USER' not set.");
    }

    if (false == m_metadata_db_password.has_value()) {
        throw invalid_argument("Environment variable 'CLP_DB_PASS' not set.");
    }
}
}  // namespace clp
