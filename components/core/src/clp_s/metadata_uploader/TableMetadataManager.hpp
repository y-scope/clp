#ifndef CLP_S_METADATA_UPLOADER_TABLEMETADATAMANAGER_HPP
#define CLP_S_METADATA_UPLOADER_TABLEMETADATAMANAGER_HPP

#include "../../clp/GlobalMetadataDBConfig.hpp"
#include "../ArchiveReader.hpp"
#include "MySQLTableMetadataDB.hpp"

namespace clp_s::metadata_uploader {
/**
 * Class to manage the metadata for a table
 */
class TableMetadataManager {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    enum class OutputType : uint8_t {
        Database
    };

    // Constructors
    TableMetadataManager(
            std::optional<clp::GlobalMetadataDBConfig> const& db_config = std::nullopt
    );

    // Destructor
    ~TableMetadataManager();

    // Methods
    /**
     * Updates the metadata for a given archive
     * @param archive_dir used as the table name
     * @param archive_id
     */
    void update_metadata(std::string const& archive_dir, std::string const& archive_id);

private:
    /**
     * Traverses the schema tree and returns a list of path names and their types
     * @param schema_tree
     * @return a list of path names and their types
     */
    std::vector<std::pair<std::string, clp_s::NodeType>> traverse_schema_tree(
            std::shared_ptr<SchemaTree> const& schema_tree
    );

    std::shared_ptr<MySQLTableMetadataDB> m_table_metadata_db;
    OutputType m_output_type;
};
}  // namespace clp_s::metadata_uploader
#endif  // CLP_S_METADATA_UPLOADER_TABLEMETADATAMANAGER_HPP
