#ifndef CLP_GLOBALMETADATADBCONFIG_HPP
#define CLP_GLOBALMETADATADBCONFIG_HPP

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
    GlobalMetadataDBConfig()
            : m_metadata_db_type(MetadataDBType::SQLite),
              m_metadata_db_host("localhost"),
              m_metadata_db_port(3306) {}

    // Methods
    void parse_config_file(std::string const& config_file_path);

    MetadataDBType get_metadata_db_type() const { return m_metadata_db_type; }

    std::string const& get_metadata_db_host() const { return m_metadata_db_host; }

    int get_metadata_db_port() const { return m_metadata_db_port; }

    std::string const& get_metadata_db_name() const { return m_metadata_db_name; }

    std::string const& get_metadata_db_username() const { return m_metadata_db_username; }

    std::string const& get_metadata_db_password() const { return m_metadata_db_password; }

    std::string const& get_metadata_table_prefix() const { return m_metadata_table_prefix; }

private:
    // Variables
    MetadataDBType m_metadata_db_type;

    std::string m_metadata_db_host;
    int m_metadata_db_port;
    std::string m_metadata_db_name;

    std::string m_metadata_db_username;
    std::string m_metadata_db_password;

    std::string m_metadata_table_prefix;
};
}  // namespace clp

#endif  // CLP_GLOBALMETADATADBCONFIG_HPP
