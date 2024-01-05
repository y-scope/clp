#ifndef CLP_S_JSONPARSER_HPP
#define CLP_S_JSONPARSER_HPP

#include <map>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include <boost/uuid/random_generator.hpp>

#include "ArchiveWriter.hpp"
#include "DictionaryWriter.hpp"
#include "FileReader.hpp"
#include "FileWriter.hpp"
#include "ParsedMessage.hpp"
#include "SchemaMap.hpp"
#include "SchemaTree.hpp"
#include "SchemaWriter.hpp"
#include "simdjson.h"
#include "TimestampDictionaryWriter.hpp"
#include "Utils.hpp"
#include "ZstdCompressor.hpp"

using namespace simdjson;

namespace clp_s {
struct JsonParserOption {
    std::vector<std::string> file_paths;
    std::vector<std::string> timestamp_column;
    std::string archive_dir;
    size_t max_encoding_size;
    int compression_level;
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
     */
    void parse();

    /**
     * Writes the metadata and archive data to disk.
     */
    void store();

    /**
     * Closes the archive and clean up.
     */
    void close();

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
    std::string m_archive_dir;
    std::string m_schema_tree_path;

    std::set<int32_t> m_current_schema;
    std::shared_ptr<SchemaMap> m_schema_map;

    std::shared_ptr<SchemaTree> m_schema_tree;
    ParsedMessage m_current_parsed_message;
    std::shared_ptr<TimestampDictionaryWriter> m_timestamp_dictionary;

    std::vector<std::string> m_timestamp_column;

    boost::uuids::random_generator m_generator;
    std::unique_ptr<ArchiveWriter> m_archive_writer;
    size_t m_max_encoding_size;
};
}  // namespace clp_s

#endif  // CLP_S_JSONPARSER_HPP
