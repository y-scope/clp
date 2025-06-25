#ifndef CLP_GLOBALMETADATADBCONFIG_HPP
#define CLP_GLOBALMETADATADBCONFIG_HPP

#include <cstdint>
#include <string>

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

    // Constructors
    GlobalMetadataDBConfig() = default;

    // Methods
    void parse_config_file(std::string const& config_file_path);

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

    std::string m_metadata_db_host{"localhost"};
    int m_metadata_db_port{cDefaultMetadataDbPort};
    std::string m_metadata_db_name;

    std::string m_metadata_db_username;
    std::string m_metadata_db_password;

    std::string m_metadata_table_prefix;
};
}  // namespace clp

#endif  // CLP_GLOBALMETADATADBCONFIG_HPP
