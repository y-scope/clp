#ifndef CLP_GLOBALMETADATADBCONFIG_HPP
#define CLP_GLOBALMETADATADBCONFIG_HPP

#include <cstdint>
#include <string>

#include <boost/program_options.hpp>

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
    };

    // Static functions
    /**
     * Adds database-related command-line options to the given options_description.
     * @param options_description
     */
    static auto add_command_line_options(
            boost::program_options::options_description& options_description
    ) -> void;

    // Constructors
    GlobalMetadataDBConfig() = default;

    // Methods
    /**
     * Initializes the configuration from parsed command-line options.
     * @param parsed_options
     */
    auto init_from_parsed_options(boost::program_options::variables_map const& parsed_options)
            -> void;

    /**
     * Reads database credentials from environment variables:
     * - CLP_DB_USER for database username
     * - CLP_DB_PASS for database password
     */
    auto read_credentials_from_env() -> void;

    /**
     * Validates that all required parameters are available and well-formed.
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
    static constexpr int cDefaultMetadataDbPort{3306};

    MetadataDBType m_metadata_db_type{MetadataDBType::SQLite};

    std::string m_metadata_db_host{"127.0.0.1"};
    int m_metadata_db_port{cDefaultMetadataDbPort};
    std::string m_metadata_db_name;

    std::string m_metadata_db_username;
    std::string m_metadata_db_password;

    std::string m_metadata_table_prefix;
};
}  // namespace clp

#endif  // CLP_GLOBALMETADATADBCONFIG_HPP
