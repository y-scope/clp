#ifndef CLP_S_JSONPARSER_HPP
#define CLP_S_JSONPARSER_HPP

#include <map>
#include <string>
#include <variant>
#include <vector>

#include <boost/uuid/random_generator.hpp>
#include <simdjson.h>

#include "../clp/GlobalMySQLMetadataDB.hpp"
#include "ArchiveWriter.hpp"
#include "DictionaryWriter.hpp"
#include "FileReader.hpp"
#include "FileWriter.hpp"
#include "ParsedMessage.hpp"
#include "Schema.hpp"
#include "SchemaMap.hpp"
#include "SchemaTree.hpp"
#include "SchemaWriter.hpp"
#include "TimestampDictionaryWriter.hpp"
#include "Utils.hpp"
#include "ZstdCompressor.hpp"

using namespace simdjson;

namespace clp_s {
struct JsonParserOption {
    std::vector<std::string> file_paths;
    std::string timestamp_key;
    std::string archives_dir;
    size_t target_encoded_size;
    size_t max_document_size;
    int compression_level;
    bool print_archive_stats;
    std::shared_ptr<clp::GlobalMySQLMetadataDB> metadata_db;
};

class JsonParser {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructor
    explicit JsonParser(JsonParserOption const& option);

    // Destructor
    ~JsonParser() = default;

    /**
     * Parses the JSON log messages and store the parsed data in the archive.
     * @return whether the JSON was parsed succesfully
     */
    [[nodiscard]] bool parse();

    /**
     * Writes the metadata and archive data to disk.
     */
    void store();

private:
    /**
     * Parses a JSON line
     * @param line the JSON line
     * @param parent_node_id the parent node id
     * @param key the key of the node
     */
    void parse_line(ondemand::value line, int32_t parent_node_id, std::string const& key);

    /**
     * Splits the archive if the size of the archive exceeds the maximum size
     */
    void split_archive();

    int m_num_messages;
    int m_compression_level;
    std::vector<std::string> m_file_paths;
    std::string m_archives_dir;
    std::string m_schema_tree_path;

    Schema m_current_schema;
    ParsedMessage m_current_parsed_message;

    std::string m_timestamp_key;
    std::vector<std::string> m_timestamp_column;

    boost::uuids::random_generator m_generator;
    std::unique_ptr<ArchiveWriter> m_archive_writer;
    size_t m_target_encoded_size;
    size_t m_max_document_size;
};
}  // namespace clp_s

#endif  // CLP_S_JSONPARSER_HPP
