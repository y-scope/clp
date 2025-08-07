#ifndef CLP_GLOBALMETADATADBCONFIG_HPP
#define CLP_GLOBALMETADATADBCONFIG_HPP

#include <cstdint>
#include <string>

#include <boost/program_options.hpp>

#include "type_utils.hpp"

namespace clp {
/**
 * Class encapsulating the global metadata database's configuration details
 */
class GlobalMetadataDBConfig {
public:
    // Types
    enum class MetadataDBType : uint8_t {
        SQLite = 0,
        MySQL,
        MetadataDBTypeLength
    };

    // Constants
    // NOTE: The strings in the array must match the order of the enums values in `MetadataDBType`.
    static constexpr std::
            array<std::string_view, enum_to_underlying_type(MetadataDBType::MetadataDBTypeLength)>
                    cMetadataDBTypeNames{"sqlite", "mysql"};

    /**
     * Overloads operator >> to read MetadataDBType from an input stream.
     * @param in
     * @param metadata_db_type
     * @return std::istream& reference to the input stream
     */
    friend auto operator>>(std::istream& in, MetadataDBType& metadata_db_type) -> std::istream&;

    // Constructors
    GlobalMetadataDBConfig() = default;

    // Methods
    /**
     * Adds database-related command-line options to the given options_description.
     * @param options_description
     */
    auto add_command_line_options(boost::program_options::options_description& options_description)
            -> void;

    /**
     * Reads database credentials from environment variables if required by the database type.
     * - CLP_DB_USER for database username
     * - CLP_DB_PASS for database password
     */
    auto read_credentials_from_env_if_needed() -> void;

    /**
     * Validates that all required parameters are available and well-formed.
     * @throw std::invalid_argument if validation fails
     */
    auto validate() const -> void;

    [[nodiscard]] auto get_metadata_db_type() const -> MetadataDBType { return m_metadata_db_type; }

    [[nodiscard]] auto get_metadata_db_host() const -> std::string const& {
        return m_metadata_db_host;
    }

    [[nodiscard]] auto get_metadata_db_port() const -> int { return m_metadata_db_port; }

    [[nodiscard]] auto get_metadata_db_name() const -> std::string const& {
        return m_metadata_db_name;
    }

    [[nodiscard]] auto get_metadata_db_username() const -> std::string const& {
        return m_metadata_db_username;
    }

    [[nodiscard]] auto get_metadata_db_password() const -> std::string const& {
        return m_metadata_db_password;
    }

    [[nodiscard]] auto get_metadata_table_prefix() const -> std::string const& {
        return m_metadata_table_prefix;
    }

private:
    // Variables
    static constexpr MetadataDBType cDefaultMetadataDbType{MetadataDBType::SQLite};
    static constexpr std::string_view cDefaultMetadataDbHost{"127.0.0.1"};
    static constexpr int cDefaultMetadataDbPort{3306};
    static constexpr std::string_view cDefaultMetadataDbName{"clp-db"};
    static constexpr std::string_view cDefaultMetadataTablePrefix{"clp_"};

    MetadataDBType m_metadata_db_type{cDefaultMetadataDbType};

    std::string m_metadata_db_host{cDefaultMetadataDbHost};
    int m_metadata_db_port{cDefaultMetadataDbPort};
    std::string m_metadata_db_name{cDefaultMetadataDbName};
    std::string m_metadata_table_prefix{cDefaultMetadataTablePrefix};

    std::string m_metadata_db_username;
    std::string m_metadata_db_password;
};
}  // namespace clp

#endif  // CLP_GLOBALMETADATADBCONFIG_HPP
