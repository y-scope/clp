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
#include "CommandLineArguments.hpp"
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
struct KafkaOption {
    std::string topic;
    int32_t partition{};
    int64_t offset{};
    size_t num_messages{};
    std::string kafka_config_file;
};

struct JsonParserOption {
    CommandLineArguments::InputSourceType input_source{
            CommandLineArguments::InputSourceType::Filesystem
    };
    std::vector<std::string> file_paths;
    std::string timestamp_key;
    std::string archives_dir;
    size_t target_encoded_size;
    size_t max_document_size;
    int compression_level;
    bool print_archive_stats;
    bool structurize_arrays;
    std::shared_ptr<clp::GlobalMySQLMetadataDB> metadata_db;
    KafkaOption kafka_option;
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
     * @throw simdjson::simdjson_error when encountering invalid fields while parsing line
     */
    void parse_line(ondemand::value line, int32_t parent_node_id, std::string const& key);

    /**
     * Parses an array within a JSON line
     * @param line the JSON array
     * @param parent_node_id the parent node id
     */
    void parse_array(ondemand::array line, int32_t parent_node_id);

    /**
     * Parses an object within an array in a JSON line
     * @param line the JSON object
     * @param parent_node_id the parent node id
     */
    void parse_obj_in_array(ondemand::object line, int32_t parent_node_id);

    /**
     * Splits the archive if the size of the archive exceeds the maximum size
     */
    void split_archive();

    /**
     * Parses input from kafka according to the configuration specified in `m_kafka_option`.
     * @throws KafkaReader::OperationFailed
     * @return true on success, false otherwise
     */
    bool parse_from_kafka();

    std::vector<std::string> m_file_paths;

    Schema m_current_schema;
    ParsedMessage m_current_parsed_message;

    std::string m_timestamp_key;
    std::vector<std::string> m_timestamp_column;

    boost::uuids::random_generator m_generator;
    std::unique_ptr<ArchiveWriter> m_archive_writer;
    ArchiveWriterOption m_archive_options{};
    size_t m_target_encoded_size;
    size_t m_max_document_size;
    bool m_structurize_arrays{false};

    CommandLineArguments::InputSourceType m_input_source{
            CommandLineArguments::InputSourceType::Filesystem
    };
    KafkaOption m_kafka_option;
};
}  // namespace clp_s

#endif  // CLP_S_JSONPARSER_HPP
