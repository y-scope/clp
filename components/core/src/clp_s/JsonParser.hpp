#ifndef CLP_S_JSONPARSER_HPP
#define CLP_S_JSONPARSER_HPP

#include <cstdlib>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <boost/uuid/random_generator.hpp>

#include "../clp/BufferReader.hpp"
#include "../clp/ffi/ir_stream/Deserializer.hpp"
#include "../clp/ffi/KeyValuePairLogEvent.hpp"
#include "../clp/ffi/SchemaTree.hpp"
#include "../clp/ffi/SchemaTreeNode.hpp"
#include "../clp/ffi/Value.hpp"
#include "../clp/GlobalMySQLMetadataDB.hpp"
#include "../clp/type_utils.hpp"
#include "ArchiveWriter.hpp"
#include "ParsedMessage.hpp"
#include "Schema.hpp"
#include "SchemaTree.hpp"

using clp::BufferReader;
using clp::ffi::ir_stream::Deserializer;
using clp::ffi::KeyValuePairLogEvent;
using clp::size_checked_pointer_cast;

namespace clp_s {
struct JsonParserOption {
    std::vector<std::string> file_paths;
    std::string timestamp_key;
    std::string archives_dir;
    size_t target_encoded_size;
    size_t max_document_size;
    int compression_level;
    bool print_archive_stats;
    bool structurize_arrays;
    std::shared_ptr<clp::GlobalMySQLMetadataDB> metadata_db;
};

struct JsonToIrParserOption {
    std::vector<std::string> file_paths;
    std::string irs_dir;
    size_t max_document_size;
    size_t max_ir_buffer_size;
    int compression_level;
    int encoding;
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

    JsonParser(JsonToIRParserOption const& option);

    // Destructor
    ~JsonParser() = default;

    /**
     * Parses the JSON log messages and store the parsed data in the archive.
     * @return whether the JSON was parsed succesfully
     */
    [[nodiscard]] bool parse();

    /**
     * Parses the Key Value IR Stream and stores the data in the archive.
     * @return whether the IR Stream was parsed successfully
     */
    [[nodiscard]] auto parse_from_ir() -> bool;

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
     * Determines the archive node type based on the IR node type and value.
     * @param ir_node_type schema node type from the IR stream
     * @param node_has_value Boolean that says whether or not the node has value.
     * @param node_value The IR schema node value if the node has value
     * @return The clp-s archive Node Type that should be used for the archive node
     */
    static auto get_archive_node_type(
            clp::ffi::SchemaTreeNode::Type ir_node_type,
            bool node_has_value,
            std::optional<clp::ffi::Value> const& node_value
    ) -> NodeType;

    /**
     * Get archive node id for ir node
     * @param ir_node_to_archive_node_unordered_map cache of node id conversions between
     * deserializer schema tree nodes and archive schema tree nodes
     * @param ir_node_id ID of the IR node
     * @param archive_node_type Type of the archive node
     * @param ir_tree The IR schema tree
     */
    auto get_archive_node_id(
            std::unordered_map<int32_t, std::vector<std::pair<NodeType, int32_t>>>&
                    ir_node_to_archive_node_unordered_map,
            int32_t ir_node_id,
            NodeType archive_node_type,
            clp::ffi::SchemaTree const& ir_tree
    ) -> int;

    /**
     * Parses a Key Value Log Event
     * @param kv the key value log event
     * @param ir_node_to_archive_node_unordered_map cache of node id conversions between
     * deserializer schema tree nodes and archive schema tree nodes
     */
    void parse_kv_log_event(
            KeyValuePairLogEvent const& kv,
            std::unordered_map<int32_t, std::vector<std::pair<NodeType, int32_t>>>&
                    ir_node_to_archive_node_unordered_map
    );

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

    int m_num_messages;
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
};
}  // namespace clp_s

#endif  // CLP_S_JSONPARSER_HPP
