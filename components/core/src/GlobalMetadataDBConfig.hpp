#ifndef GLOBALMETADATADBCONFIG_HPP
#define GLOBALMETADATADBCONFIG_HPP

// C++ standard libraries
#include <string>

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
    GlobalMetadataDBConfig () : m_metadata_db_type(MetadataDBType::SQLite), m_metadata_db_host("localhost"), m_metadata_db_port(3306) {}

    // Methods
    void parse_config_file (const std::string& config_file_path);

    MetadataDBType get_metadata_db_type () const { return m_metadata_db_type; }

    const std::string& get_metadata_db_host () const { return m_metadata_db_host; }
    int get_metadata_db_port () const { return m_metadata_db_port; }
    const std::string& get_metadata_db_name () const { return m_metadata_db_name; }

    const std::string& get_metadata_db_username () const { return m_metadata_db_username; }
    const std::string& get_metadata_db_password () const { return m_metadata_db_password; }

    const std::string& get_metadata_table_prefix () const { return m_metadata_table_prefix; }

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

#endif // GLOBALMETADATADBCONFIG_HPP
