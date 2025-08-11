#ifndef CLP_GLOBALMETADATADBCONFIG_HPP
#define CLP_GLOBALMETADATADBCONFIG_HPP

#include <cstdint>
#include <optional>
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
     *
     * @param in
     * @param metadata_db_type
     * @return The input stream.
     */
    friend auto operator>>(std::istream& in, MetadataDBType& metadata_db_type) -> std::istream&;

    // Constructors
    /**
     * Adds database-related command-line options to the given options_description.
     *
     * @param options_description
     */
    explicit GlobalMetadataDBConfig(
            boost::program_options::options_description& options_description
    );

    // Methods
    /**
     * Reads database credentials from the following environment variables, if required by the
     * database type:
     *
     * - CLP_DB_USER for the database username
     * - CLP_DB_PASS for the database password
     */
    auto read_credentials_from_env_if_needed() -> void;

    /**
     * Validates that all required parameters are available and well-formed.
     *
     * @throw std::invalid_argument if validation fails.
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

    [[nodiscard]] auto get_metadata_db_username() const -> std::optional<std::string> const& {
        return m_metadata_db_username;
    }

    [[nodiscard]] auto get_metadata_db_password() const -> std::optional<std::string> const& {
        return m_metadata_db_password;
    }

    [[nodiscard]] auto get_metadata_table_prefix() const -> std::string const& {
        return m_metadata_table_prefix;
    }

private:
    // Variables
    MetadataDBType m_metadata_db_type{};

    std::string m_metadata_db_host;
    int m_metadata_db_port{};
    std::string m_metadata_db_name;
    std::string m_metadata_table_prefix;

    std::optional<std::string> m_metadata_db_username;
    std::optional<std::string> m_metadata_db_password;
};
}  // namespace clp

#endif  // CLP_GLOBALMETADATADBCONFIG_HPP
