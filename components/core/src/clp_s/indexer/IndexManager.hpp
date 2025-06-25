#ifndef CLP_S_INDEXER_INDEXMANAGER_HPP
#define CLP_S_INDEXER_INDEXMANAGER_HPP

#include <functional>
#include <memory>
#include <optional>

#include "../../clp/GlobalMetadataDBConfig.hpp"
#include "../ArchiveReader.hpp"
#include "../TimestampDictionaryReader.hpp"
#include "MySQLIndexStorage.hpp"

namespace clp_s::indexer {
/**
 * This class updates field names (e.g., JSON full paths) and data types for a specified archive.
 * It traverses the schema tree from root to leaf, concatenating escaped key names using the
 * delimiter `.` at each level.
 *
 * Currently, only leaf nodes with primitive types are indexed. Object nodes and subtrees of
 * structured arrays are not indexed. The results are stored in a database to enable efficient
 * querying.
 *
 * Multiple archives related to the same topic can form a table that can be queried using a SQL
 * query engine. When indexing, a table name must be specified. This table is then used by the SQL
 * engine to resolve column metadata.
 */
class IndexManager {
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
    IndexManager(
            std::optional<clp::GlobalMetadataDBConfig> const& db_config = std::nullopt,
            bool should_create_table = false
    );

    // Destructor
    ~IndexManager();

    // Methods
    /**
     * Updates the metadata for a given archive
     * @param dataset_name
     * @param archive_path
     */
    void update_metadata(std::string const& dataset_name, Path const& archive_path);

private:
    /**
     * Escapes a key name
     * @param key_name
     * @return the escaped key name
     */
    static std::string escape_key_name(std::string_view const key_name);

    /**
     * Traverses the schema tree and updates the metadata
     * @param schema_tree
     * @param timestamp_dict
     */
    void traverse_schema_tree_and_update_metadata(
            std::shared_ptr<SchemaTree> const& schema_tree,
            std::shared_ptr<TimestampDictionaryReader> const& timestamp_dict
    );

    OutputType m_output_type{OutputType::Database};
    std::shared_ptr<MySQLIndexStorage> m_mysql_index_storage;
    bool m_should_create_table{false};
    std::function<void(std::string&, NodeType)> m_field_update_callback;
};
}  // namespace clp_s::indexer
#endif  // CLP_S_INDEXER_INDEXMANAGER_HPP
