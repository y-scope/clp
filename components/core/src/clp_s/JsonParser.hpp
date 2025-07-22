#ifndef CLP_S_JSONPARSER_HPP
#define CLP_S_JSONPARSER_HPP

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <boost/uuid/random_generator.hpp>
#include <simdjson.h>

#include "../clp/ffi/KeyValuePairLogEvent.hpp"
#include "../clp/ffi/SchemaTree.hpp"
#include "../clp/ffi/Value.hpp"
#include "../clp/ReaderInterface.hpp"
#include "ArchiveWriter.hpp"
#include "DictionaryWriter.hpp"
#include "FileReader.hpp"
#include "FileWriter.hpp"
#include "InputConfig.hpp"
#include "ParsedMessage.hpp"
#include "Schema.hpp"
#include "SchemaTree.hpp"
#include "SchemaWriter.hpp"
#include "TimestampDictionaryWriter.hpp"
#include "Utils.hpp"
#include "ZstdCompressor.hpp"

using namespace simdjson;

namespace clp_s {
struct JsonParserOption {
    std::vector<Path> input_paths;
    FileType input_file_type{FileType::Json};
    std::string timestamp_key;
    std::string archives_dir;
    size_t target_encoded_size{};
    size_t max_document_size{};
    size_t min_table_size{};
    int compression_level{};
    bool print_archive_stats{};
    bool structurize_arrays{};
    bool record_log_order{true};
    bool single_file_archive{false};
    NetworkAuthOption network_auth{};
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
     * Parses the Key Value IR Stream and stores the data in the archive.
     * @return whether the IR Stream was parsed successfully
     */
    [[nodiscard]] auto parse_from_ir() -> bool;

    /**
     * Writes the metadata and archive data to disk.
     * @return Statistics for every archive that was written without encountering an error.
     */
    [[nodiscard]] auto store() -> std::vector<ArchiveStats>;

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
     * @return The NodeType that should be used for the archive node
     */
    static auto get_archive_node_type(
            clp::ffi::SchemaTree const& tree,
            std::pair<clp::ffi::SchemaTree::Node::id_t, std::optional<clp::ffi::Value>> const&
                    kv_pair
    ) -> NodeType;

    /**
     * Adjusts the node type used to represent an IR node in an archive based on whether it is a
     * timestamp as well as its original type.
     * @param node_type
     * @param matches_timestamp
     * @return The NodeType that should be used to represent the original IR node in the archive.
     */
    static auto adjust_archive_node_type_for_timestamp(NodeType node_type, bool matches_timestamp)
            -> NodeType;

    /**
     * Adds new schema node to archive and adds translation for IR node ID and NodeType to mapping
     * @param ir_node_id ID of the IR node
     * @param ir_node_to_add IR Schema Node that is being translated to archive
     * @param archive_node_type Type of the archive node
     * @param parent_node_id ID of the parent of the IR node
     * @param matches_timestamp
     * @tparam autogen whether this node is being added in the auto-generated subtree or not
     * @return The ID of the node added to the archive's Schema Tree
     */
    template <bool autogen>
    auto add_node_to_archive_and_translations(
            uint32_t ir_node_id,
            clp::ffi::SchemaTree::Node const& ir_node_to_add,
            NodeType archive_node_type,
            int32_t parent_node_id,
            bool matches_timestamp
    ) -> int32_t;

    /**
     * Gets the archive node ID for an IR node.
     * @param ir_node_id ID of the IR node
     * @param archive_node_type Type of the archive node
     * @param ir_tree The IR schema tree
     * @tparam autogen whether this node is being added in the auto-generated subtree or not
     * @return The ID of the corresponding node in the archive's schema tree and a flag indicating
     * whether the field should be treated as a timestamp.
     */
    template <bool autogen>
    auto get_archive_node_id_and_check_timestamp(
            uint32_t ir_node_id,
            NodeType archive_node_type,
            clp::ffi::SchemaTree const& ir_tree
    ) -> std::pair<int32_t, bool>;

    /**
     * Parses a subtree (user-gen or auto-gen) of a Key Value Log Event.
     * @param kv_pairs
     * @param tree
     * @tparam autogen whether this node is being added in the auto-generated subtree or not
     */
    template <bool autogen>
    void parse_kv_log_event_subtree(
            clp::ffi::KeyValuePairLogEvent::NodeIdValuePairs const& kv_pairs,
            clp::ffi::SchemaTree const& tree
    );

    /**
     * Parses a Key Value Log Event.
     * @param kv the Key Value Log Event
     */
    void parse_kv_log_event(clp::ffi::KeyValuePairLogEvent const& kv);

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
     * Adds an internal field to the MPT and get its Id.
     *
     * Note: this method should be called before parsing a record so that internal fields come first
     * in each table. This isn't strictly necessary, but it is a nice convention.
     */
    int32_t add_metadata_field(std::string_view const field_name, NodeType type);

    /**
     * Checks if a reader interface is a clp::NetworkReader that has encountered a CURL error and
     * logs relevant CURL error information if a CURL error has occurred.
     * @param path
     * @param reader
     * @return true if the provided ReaderInterface has experienced a CURL error and false otherwise
     */
    static bool
    check_and_log_curl_error(Path const& path, std::shared_ptr<clp::ReaderInterface> reader);

    int m_num_messages;
    std::vector<Path> m_input_paths;
    NetworkAuthOption m_network_auth{};

    Schema m_current_schema;
    ParsedMessage m_current_parsed_message;

    std::string m_timestamp_key;
    std::vector<std::string> m_timestamp_column;
    std::string m_timestamp_namespace;

    boost::uuids::random_generator m_generator;
    std::unique_ptr<ArchiveWriter> m_archive_writer;
    ArchiveWriterOption m_archive_options{};
    size_t m_target_encoded_size;
    size_t m_max_document_size;
    bool m_structurize_arrays{false};
    bool m_record_log_order{true};

    absl::flat_hash_map<std::pair<uint32_t, NodeType>, std::pair<int32_t, bool>>
            m_ir_node_to_archive_node_id_mapping;
    absl::flat_hash_map<std::pair<uint32_t, NodeType>, std::pair<int32_t, bool>>
            m_autogen_ir_node_to_archive_node_id_mapping;

    std::vector<ArchiveStats> m_archive_stats;
};
}  // namespace clp_s

#endif  // CLP_S_JSONPARSER_HPP
