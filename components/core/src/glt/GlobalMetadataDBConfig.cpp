#include "GlobalMetadataDBConfig.hpp"

#include <fmt/core.h>
#include <yaml-cpp/include/yaml-cpp/yaml.h>

using std::exception;
using std::invalid_argument;
using std::string;

static exception get_yaml_missing_key_exception(string const& key_name) {
    throw invalid_argument(fmt::format("Missing key '{}'", key_name));
}

static exception
get_yaml_unconvertable_value_exception(string const& key_name, string const& destination_type) {
    throw invalid_argument(
            fmt::format("'{}' could not be converted to type '{}'", key_name, destination_type)
    );
}

namespace glt {
void GlobalMetadataDBConfig::parse_config_file(string const& config_file_path) {
    YAML::Node config = YAML::LoadFile(config_file_path);

    if (!config["type"]) {
        throw get_yaml_missing_key_exception("type");
    }

    auto db_type_string = config["type"].as<string>();
    if ("sqlite" == db_type_string) {
        m_metadata_db_type = MetadataDBType::SQLite;
    } else if ("mysql" == db_type_string) {
        m_metadata_db_type = MetadataDBType::MySQL;

        if (!config["host"]) {
            throw get_yaml_missing_key_exception("host");
        }
        try {
            m_metadata_db_host = config["host"].as<string>();
        } catch (YAML::BadConversion& e) {
            throw get_yaml_unconvertable_value_exception("host", "string");
        }
        if (m_metadata_db_host.empty()) {
            throw invalid_argument("Database 'host' not specified or empty.");
        }

        if (!config["port"]) {
            throw get_yaml_missing_key_exception("port");
        }
        try {
            m_metadata_db_port = config["port"].as<int>();
        } catch (YAML::BadConversion& e) {
            throw get_yaml_unconvertable_value_exception("port", "int");
        }
        if (m_metadata_db_port < 0) {
            throw invalid_argument("Database 'port' cannot be negative.");
        }

        if (!config["name"]) {
            throw get_yaml_missing_key_exception("name");
        }
        try {
            m_metadata_db_name = config["name"].as<string>();
        } catch (YAML::BadConversion& e) {
            throw get_yaml_unconvertable_value_exception("name", "string");
        }
        if (m_metadata_db_name.empty()) {
            throw invalid_argument("Database 'name' not specified or empty.");
        }

        if (!config["username"]) {
            throw get_yaml_missing_key_exception("username");
        }
        try {
            m_metadata_db_username = config["username"].as<string>();
        } catch (YAML::BadConversion& e) {
            throw get_yaml_unconvertable_value_exception("username", "string");
        }
        if (m_metadata_db_username.empty()) {
            throw invalid_argument("Database 'username' not specified or empty.");
        }

        if (!config["password"]) {
            throw get_yaml_missing_key_exception("password");
        }
        try {
            m_metadata_db_password = config["password"].as<string>();
        } catch (YAML::BadConversion& e) {
            throw get_yaml_unconvertable_value_exception("password", "string");
        }
        if (m_metadata_db_password.empty()) {
            throw invalid_argument("Database 'password' not specified or empty.");
        }

        if (!config["table_prefix"]) {
            throw get_yaml_missing_key_exception("table_prefix");
        }
        try {
            m_metadata_table_prefix = config["table_prefix"].as<string>();
        } catch (YAML::BadConversion& e) {
            throw get_yaml_unconvertable_value_exception("table_prefix", "string");
        }
        if (m_metadata_table_prefix.empty()) {
            throw invalid_argument("Database 'table_prefix' not specified or empty.");
        }
    } else {
        throw invalid_argument("Unknown type");
    }
}
}  // namespace glt
