#include "GlobalMetadataDBConfig.hpp"

#include <cstdlib>
#include <stdexcept>

#include <fmt/core.h>

using std::exception;
using std::invalid_argument;
using std::string;

namespace clp {
auto GlobalMetadataDBConfig::add_command_line_options(
        boost::program_options::options_description& options_description
) -> void {
    options_description.add_options()
            ("db-type",
             boost::program_options::value<string>()->default_value("sqlite"),
             "Database type [sqlite | mysql]")
            ("db-host",
             boost::program_options::value<string>()->default_value("127.0.0.1"),
             "[db-type=mysql] Database host")
            ("db-port",
             boost::program_options::value<int>()->default_value(cDefaultMetadataDbPort),
             "[db-type=mysql] Database port")
            ("db-name",
             boost::program_options::value<string>()->default_value("clp-db"),
             "[db-type=mysql] Database name")
            ("db-table-prefix",
             boost::program_options::value<string>()->default_value("clp_"),
             "[db-type=mysql] Database table prefix");
}

auto GlobalMetadataDBConfig::init_from_parsed_options(
        boost::program_options::variables_map const& parsed_options
) -> void {
    if (auto const db_type_string{parsed_options["db-type"].as<string>()};
        "sqlite" == db_type_string)
    {
        m_metadata_db_type = MetadataDBType::SQLite;
    } else if ("mysql" == db_type_string) {
        m_metadata_db_type = MetadataDBType::MySQL;
    } else {
        throw invalid_argument("Unknown database type: " + db_type_string);
    }

    if (m_metadata_db_type == MetadataDBType::MySQL) {
        m_metadata_db_host = parsed_options["db-host"].as<string>();
        m_metadata_db_port = parsed_options["db-port"].as<int>();
        m_metadata_db_name = parsed_options["db-name"].as<string>();
        m_metadata_table_prefix = parsed_options["db-table-prefix"].as<string>();
    }
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

        if (0 > m_metadata_db_port || 65535 < m_metadata_db_port) {
            throw invalid_argument("Database '--db-port' is out of range [0, 65535]: "
                                   + std::to_string(m_metadata_db_port));
        }

        if (m_metadata_db_name.empty()) {
            throw invalid_argument("Database '--db--name' is empty.");
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
