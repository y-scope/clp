#ifndef CLP_S_JSONPARSER_HPP
#define CLP_S_JSONPARSER_HPP

#include <map>
#include <string>
#include <variant>
#include <vector>

#include <boost/uuid/random_generator.hpp>
#include <simdjson.h>

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

    /**
     * @return the size of the input data before compression in bytes
     */
    [[nodiscard]] size_t get_uncompressed_size() { return m_uncompressed_size; }

    /**
     * @return the size of the compressed data in bytes
     */
    [[nodiscard]] size_t get_compressed_size() { return m_compressed_size; }

    [[nodiscard]] epochtime_t get_begin_timestamp() {
        return m_timestamp_dictionary->get_begin_timestamp();
    }

    [[nodiscard]] epochtime_t get_end_timestamp() {
        return m_timestamp_dictionary->get_end_timestamp();
    }

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
    std::shared_ptr<SchemaMap> m_schema_map;

    std::shared_ptr<SchemaTree> m_schema_tree;
    ParsedMessage m_current_parsed_message;
    std::shared_ptr<TimestampDictionaryWriter> m_timestamp_dictionary;

    std::string m_timestamp_key;
    std::vector<std::string> m_timestamp_column;

    boost::uuids::random_generator m_generator;
    std::unique_ptr<ArchiveWriter> m_archive_writer;
    size_t m_target_encoded_size;

    size_t m_uncompressed_size{0};
    size_t m_compressed_size{0};
};
}  // namespace clp_s

#endif  // CLP_S_JSONPARSER_HPP
