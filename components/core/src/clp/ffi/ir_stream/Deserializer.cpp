#include "Deserializer.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <json/single_include/nlohmann/json.hpp>
#include <outcome/single-header/outcome.hpp>

#include "../../ErrorCode.hpp"
#include "../../ir/EncodedTextAst.hpp"
#include "../../ir/types.hpp"
#include "../../ReaderInterface.hpp"
#include "../../time_types.hpp"
#include "../../type_utils.hpp"
#include "../KeyValuePairLogEvent.hpp"
#include "../SchemaTree.hpp"
#include "../SchemaTreeNode.hpp"
#include "../Value.hpp"
#include "decoding_methods.hpp"
#include "protocol_constants.hpp"
#include "utils.hpp"

namespace clp::ffi::ir_stream {
namespace {
/**
 * A collection of schema tree leaf node IDs. It represents a the schema of a
 * `KeyValuePairLogEvent`.
 */
using Schema = std::vector<SchemaTreeNode::id_t>;

/**
 * Class to perform different actions depending on whether a transaction succeeds or fails. The
 * default state assumes the transaction fails.
 * @tparam SuccessHandler A cleanup lambda to call on success.
 * @tparam FailureHandler A cleanup lambda to call on failure.
 */
template <typename SuccessHandler, typename FailureHandler>
requires(std::is_invocable_v<SuccessHandler> && std::is_invocable_v<FailureHandler>)
class TransactionManager {
public:
    // Constructor
    TransactionManager(SuccessHandler success_handler, FailureHandler failure_handler)
            : m_success_handler{success_handler},
              m_failure_handler{failure_handler} {}

    // Delete copy/move constructor and assignment
    TransactionManager(TransactionManager const&) = delete;
    TransactionManager(TransactionManager&&) = delete;
    auto operator=(TransactionManager const&) -> TransactionManager& = delete;
    auto operator=(TransactionManager&&) -> TransactionManager& = delete;

    // Destructor
    ~TransactionManager() {
        if (m_success) {
            m_success_handler();
        } else {
            m_failure_handler();
        }
    }

    // Methods
    /**
     * Marks the transaction as successful.
     */
    auto mark_success() -> void { m_success = true; }

private:
    // Variables
    SuccessHandler m_success_handler;
    FailureHandler m_failure_handler;
    bool m_success{false};
};

/**
 * @param ir_error_code
 * @return Equivalent `std::errc` code indicating the same error type.
 */
[[nodiscard]] auto ir_error_code_to_errc(IRErrorCode ir_error_code) -> std::errc;

/**
 * @param tag
 * @return Whether the tag represents a schema tree node.
 */
[[nodiscard]] auto is_schema_tree_node_tag(encoded_tag_t tag) -> bool;

/**
 * @param tag
 * @return The corresponding schema tree node type on success.
 * @return std::nullopt if the tag doesn't match to any defined schema tree node type.
 */
[[nodiscard]] auto schema_tree_node_tag_to_type(encoded_tag_t tag
) -> std::optional<SchemaTreeNode::Type>;

/**
 * Deserializes the parent ID of a schema tree node.
 * @param reader
 * @param parent_id Returns the deserialized result.
 * @return IRErrorCode::IRErrorCode_Success on success.
 * @return IRErrorCode::IRErrorCode_Incomplete_IR if the stream is truncated.
 * @return IRErrorCode::IRErrorCode_Corrupted_IR if the next packet in the stream isn't a parent ID.
 * @return Same as `deserialize_tag` on any other failure.
 */
[[nodiscard]] auto deserialize_schema_tree_node_parent_id(
        ReaderInterface& reader,
        SchemaTreeNode::id_t& parent_id
) -> IRErrorCode;

/**
 * Deserializes the key name of a schema tree node.
 * @param reader
 * @param key_name Returns the deserialized key name.
 * @return IRErrorCode::IRErrorCode_Success on success.
 * @return Same as `deserialize_tag` or `deserialize_string` on failure.
 */
[[nodiscard]] auto deserialize_schema_tree_node_key_name(
        ReaderInterface& reader,
        std::string& key_name
) -> IRErrorCode;

/**
 * Deserializes an integer value packet.
 * @param reader
 * @param tag
 * @param val Returns the deserialized value.
 * @return IRErrorCode::IRErrorCode_Success on success.
 * @return IRErrorCode::IRErrorCode_Incomplete_IR if the stream is truncated.
 * @return IRErrorCode::IRErrorCode_Corrupted_IR if the given tag doesn't correspond to an integer
 * packet.
 */
[[nodiscard]] auto
deserialize_int_val(ReaderInterface& reader, encoded_tag_t tag, value_int_t& val) -> IRErrorCode;

/**
 * Deserializes a string packet.
 * @param reader
 * @param tag
 * @param deserialized_str Returns the deserialized string.
 * @return IRErrorCode::IRErrorCode_Success on success.
 * @return IRErrorCode::IRErrorCode_Incomplete_IR if the stream is truncated.
 * @return IRErrorCode::IRErrorCode_Corrupted_IR if the given tag doesn't correspond to a string
 * packet.
 */
[[nodiscard]] auto deserialize_string(
        ReaderInterface& reader,
        encoded_tag_t tag,
        std::string& deserialized_str
) -> IRErrorCode;

/**
 * Deserializes all UTC offset packets until a non-UTC offset packet tag is read.
 * @param reader
 * @param tag Takes the current tag as input and returns the last tag read.
 * @param utc_offset Returns the deserialized UTC offset.
 * @return IRErrorCode::IRErrorCode_Success on success.
 * @return Same as `deserialize_utc_offset_change` or `deserialize_tag` on failure.
 */
[[nodiscard]] auto deserialize_utc_offset_changes(
        ReaderInterface& reader,
        encoded_tag_t& tag,
        UtcOffset& utc_offset
) -> IRErrorCode;

/**
 * Deserializes all schema tree node packets and inserts them into the schema tree until a non-
 * schema tree node tag is read.
 * @param reader
 * @param tag Takes the current tag as input and returns the last tag read.
 * @param schema_tree Returns the schema tree with all new nodes inserted.
 * @return IRErrorCode::IRErrorCode_Success on success.
 * @return IRErrorCode::IRErrorCode_Corrupted_IR if the packet tag doesn't correspond to any known
 * schema node type or the node being deserialized already exists in the current in-memory schema
 * tree.
 * @return Same as `deserialize_schema_tree_node_parent_id`, `deserialize_string`, or
 * `deserialize_tag` on any other failure.
 */
[[nodiscard]] auto deserialize_schema_tree_nodes(
        ReaderInterface& reader,
        encoded_tag_t& tag,
        SchemaTree& schema_tree
) -> IRErrorCode;

/**
 * Deserializes the IDs of all keys in a log event.
 * @param reader
 * @param tag Takes the current tag as input and returns the last tag read.
 * @param schema Returns the deserialized schema.
 * @return IRErrorCode::IRErrorCode_Success on success.
 * @return IRErrorCode::IRErrorCode_Incomplete_IR if the stream is truncated.
 * @return Same as `deserialize_tag` on any other failure.
 */
[[nodiscard]] auto
deserialize_schema(ReaderInterface& reader, encoded_tag_t& tag, Schema& schema) -> IRErrorCode;

/**
 * Deserializes the next value and pushes the result into `node_id_value_pairs`.
 * @param reader
 * @param tag
 * @param node_id The node ID that corresponds to the value.
 * @param node_id_value_pairs Returns the ID-value pair constructed from the deserialized value.
 * @return IRErrorCode::IRErrorCode_Success on success.
 * @return IRErrorCode::IRErrorCode_Incomplete_IR if the stream is truncated.
 * @return IRErrorCode::IRErrorCode_Corrupted_IR if the tag doesn't correspond to any known value
 * type.
 * @return Same as `deserialize_encoded_text_ast_and_insert_to_node_id_value_pairs` on any other
 * failure.
 */
[[nodiscard]] auto deserialize_value_and_insert_to_node_id_value_pairs(
        ReaderInterface& reader,
        encoded_tag_t tag,
        SchemaTreeNode::id_t node_id,
        KeyValuePairLogEvent::NodeIdValuePairs& node_id_value_pairs
) -> IRErrorCode;

/**
 * Deserializes an encoded text AST and pushes the result into node_id_value_pairs.
 * @tparam encoded_variable_t
 * @param reader
 * @param node_id The node ID that corresponds to the value.
 * @param node_id_value_pairs Returns the ID-value pair constructed by the deserialized encoded text
 * AST.
 * @return IRErrorCode::IRErrorCode_Success on success.
 * @return Same as `deserialize_tag` or `deserialize_encoded_text_ast` on failure.
 */
template <typename encoded_variable_t>
requires(std::is_same_v<ir::four_byte_encoded_variable_t, encoded_variable_t>
         || std::is_same_v<ir::eight_byte_encoded_variable_t, encoded_variable_t>)
[[nodiscard]] auto deserialize_encoded_text_ast_and_insert_to_node_id_value_pairs(
        ReaderInterface& reader,
        SchemaTreeNode::id_t node_id,
        KeyValuePairLogEvent::NodeIdValuePairs& node_id_value_pairs
) -> IRErrorCode;

/**
 * Deserializes values and constructs ID-value pairs according to the given schema. The number of
 * values to deserialize is indicated by the size of the given schema.
 * @param reader
 * @param tag
 * @param schema The log event's schema.
 * @param node_id_value_pairs Returns the constructed ID-value pairs.
 * @return IRErrorCode::IRErrorCode_Success on success.
 * @return IRErrorCode::IRErrorCode_Corrupted_IR if a key is duplicated in the deserialized log
 * event.
 * @return Same as `deserialize_tag` or `deserialize_value_and_insert_to_node_id_value_pairs` on any
 * other failure.
 */
[[nodiscard]] auto deserialize_value_and_construct_node_id_value_pairs(
        ReaderInterface& reader,
        encoded_tag_t tag,
        Schema const& schema,
        KeyValuePairLogEvent::NodeIdValuePairs& node_id_value_pairs
) -> IRErrorCode;

auto ir_error_code_to_errc(IRErrorCode ir_error_code) -> std::errc {
    switch (ir_error_code) {
        case IRErrorCode_Incomplete_IR:
            return std::errc::result_out_of_range;
        case IRErrorCode_Corrupted_IR:
        case IRErrorCode_Decode_Error:
            return std::errc::protocol_error;
        case IRErrorCode_Eof:
            return std::errc::no_message_available;
        default:
            return std::errc::not_supported;
    }
}

auto is_schema_tree_node_tag(encoded_tag_t tag) -> bool {
    return (tag & cProtocol::Payload::SchemaTreeNodeMask) == cProtocol::Payload::SchemaTreeNodeMask;
}

auto schema_tree_node_tag_to_type(encoded_tag_t tag) -> std::optional<SchemaTreeNode::Type> {
    switch (tag) {
        case cProtocol::Payload::SchemaTreeNodeInt:
            return SchemaTreeNode::Type::Int;
        case cProtocol::Payload::SchemaTreeNodeFloat:
            return SchemaTreeNode::Type::Float;
        case cProtocol::Payload::SchemaTreeNodeBool:
            return SchemaTreeNode::Type::Bool;
        case cProtocol::Payload::SchemaTreeNodeStr:
            return SchemaTreeNode::Type::Str;
        case cProtocol::Payload::SchemaTreeNodeUnstructuredArray:
            return SchemaTreeNode::Type::UnstructuredArray;
        case cProtocol::Payload::SchemaTreeNodeObj:
            return SchemaTreeNode::Type::Obj;
        default:
            return std::nullopt;
    }
}

auto deserialize_schema_tree_node_parent_id(
        ReaderInterface& reader,
        SchemaTreeNode::id_t& parent_id
) -> IRErrorCode {
    encoded_tag_t tag{};
    if (auto const err{deserialize_tag(reader, tag)}; IRErrorCode::IRErrorCode_Success != err) {
        return err;
    }
    if (cProtocol::Payload::SchemaTreeNodeParentIdUByte == tag) {
        uint8_t deserialized_id{};
        if (false == deserialize_int(reader, deserialized_id)) {
            return IRErrorCode::IRErrorCode_Incomplete_IR;
        }
        parent_id = static_cast<SchemaTreeNode::id_t>(deserialized_id);
    } else if (cProtocol::Payload::SchemaTreeNodeParentIdUShort == tag) {
        uint16_t deserialized_id{};
        if (false == deserialize_int(reader, deserialized_id)) {
            return IRErrorCode::IRErrorCode_Incomplete_IR;
        }
        parent_id = static_cast<SchemaTreeNode::id_t>(deserialized_id);
    } else {
        return IRErrorCode::IRErrorCode_Corrupted_IR;
    }
    return IRErrorCode_Success;
}

auto deserialize_schema_tree_node_key_name(ReaderInterface& reader, std::string& key_name)
        -> IRErrorCode {
    encoded_tag_t str_packet_tag{};
    if (auto const err{deserialize_tag(reader, str_packet_tag)};
        IRErrorCode::IRErrorCode_Success != err)
    {
        return err;
    }
    if (auto const err{deserialize_string(reader, str_packet_tag, key_name)};
        IRErrorCode::IRErrorCode_Success != err)
    {
        return err;
    }
    return IRErrorCode::IRErrorCode_Success;
}

auto deserialize_int_val(ReaderInterface& reader, encoded_tag_t tag, value_int_t& val)
        -> IRErrorCode {
    if (cProtocol::Payload::ValueInt8 == tag) {
        int8_t deserialized_val{};
        if (false == deserialize_int(reader, deserialized_val)) {
            return IRErrorCode::IRErrorCode_Incomplete_IR;
        }
        // NOLINTNEXTLINE(bugprone-signed-char-misuse,cert-str34-c)
        val = deserialized_val;
    } else if (cProtocol::Payload::ValueInt16 == tag) {
        int16_t deserialized_val{};
        if (false == deserialize_int(reader, deserialized_val)) {
            return IRErrorCode::IRErrorCode_Incomplete_IR;
        }
        val = deserialized_val;
    } else if (cProtocol::Payload::ValueInt32 == tag) {
        int32_t deserialized_val{};
        if (false == deserialize_int(reader, deserialized_val)) {
            return IRErrorCode::IRErrorCode_Incomplete_IR;
        }
        val = deserialized_val;
    } else if (cProtocol::Payload::ValueInt64 == tag) {
        int64_t deserialized_val{};
        if (false == deserialize_int(reader, deserialized_val)) {
            return IRErrorCode::IRErrorCode_Incomplete_IR;
        }
        val = deserialized_val;
    } else {
        return IRErrorCode::IRErrorCode_Corrupted_IR;
    }
    return IRErrorCode::IRErrorCode_Success;
}

auto deserialize_string(ReaderInterface& reader, encoded_tag_t tag, std::string& deserialized_str)
        -> IRErrorCode {
    size_t str_length{};
    if (cProtocol::Payload::StrLenUByte == tag) {
        uint8_t length{};
        if (false == deserialize_int(reader, length)) {
            return IRErrorCode::IRErrorCode_Incomplete_IR;
        }
        str_length = static_cast<size_t>(length);
    } else if (cProtocol::Payload::StrLenUShort == tag) {
        uint16_t length{};
        if (false == deserialize_int(reader, length)) {
            return IRErrorCode::IRErrorCode_Incomplete_IR;
        }
        str_length = static_cast<size_t>(length);
    } else if (cProtocol::Payload::StrLenUInt == tag) {
        uint32_t length{};
        if (false == deserialize_int(reader, length)) {
            return IRErrorCode::IRErrorCode_Incomplete_IR;
        }
        str_length = static_cast<size_t>(length);
    } else {
        return IRErrorCode::IRErrorCode_Corrupted_IR;
    }
    if (clp::ErrorCode_Success != reader.try_read_string(str_length, deserialized_str)) {
        return IRErrorCode::IRErrorCode_Incomplete_IR;
    }
    return IRErrorCode::IRErrorCode_Success;
}

auto deserialize_utc_offset_changes(
        ReaderInterface& reader,
        encoded_tag_t& tag,
        UtcOffset& utc_offset
) -> IRErrorCode {
    while (cProtocol::Payload::UtcOffsetChange == tag) {
        if (auto const err{deserialize_utc_offset_change(reader, utc_offset)};
            IRErrorCode::IRErrorCode_Success != err)
        {
            return err;
        }
        if (auto const err{deserialize_tag(reader, tag)}; IRErrorCode::IRErrorCode_Success != err) {
            return err;
        }
    }
    return IRErrorCode::IRErrorCode_Success;
}

auto deserialize_schema_tree_nodes(
        ReaderInterface& reader,
        encoded_tag_t& tag,
        SchemaTree& schema_tree
) -> IRErrorCode {
    while (is_schema_tree_node_tag(tag)) {
        auto const type{schema_tree_node_tag_to_type(tag)};
        if (false == type.has_value()) {
            return IRErrorCode::IRErrorCode_Corrupted_IR;
        }

        SchemaTreeNode::id_t parent_id{};
        if (auto const err{deserialize_schema_tree_node_parent_id(reader, parent_id)};
            IRErrorCode_Success != err)
        {
            return err;
        }

        std::string key_name;
        if (auto const err{deserialize_schema_tree_node_key_name(reader, key_name)};
            IRErrorCode::IRErrorCode_Success != err)
        {
            return err;
        }

        // Insert the node to the schema tree
        SchemaTree::NodeLocator const locator{parent_id, key_name, type.value()};
        if (schema_tree.has_node(locator)) {
            return IRErrorCode::IRErrorCode_Corrupted_IR;
        }
        std::ignore = schema_tree.insert_node(locator);

        // Read the next tag
        if (auto const err{deserialize_tag(reader, tag)}; IRErrorCode::IRErrorCode_Success != err) {
            return err;
        }
    }
    return IRErrorCode::IRErrorCode_Success;
}

auto deserialize_schema(ReaderInterface& reader, encoded_tag_t& tag, Schema& schema)
        -> IRErrorCode {
    schema.clear();
    while (true) {
        if (cProtocol::Payload::KeyIdUByte == tag) {
            uint8_t id{};
            if (false == deserialize_int(reader, id)) {
                return IRErrorCode::IRErrorCode_Incomplete_IR;
            }
            schema.push_back(static_cast<SchemaTreeNode::id_t>(id));
        } else if (cProtocol::Payload::KeyIdUShort == tag) {
            uint16_t id{};
            if (false == deserialize_int(reader, id)) {
                return IRErrorCode::IRErrorCode_Incomplete_IR;
            }
            schema.push_back(static_cast<SchemaTreeNode::id_t>(id));
        } else {
            break;
        }

        if (auto const err{deserialize_tag(reader, tag)}; IRErrorCode::IRErrorCode_Success != err) {
            return err;
        }
    }

    return IRErrorCode::IRErrorCode_Success;
}

auto deserialize_value_and_insert_to_node_id_value_pairs(
        ReaderInterface& reader,
        encoded_tag_t tag,
        SchemaTreeNode::id_t node_id,
        KeyValuePairLogEvent::NodeIdValuePairs& node_id_value_pairs
) -> IRErrorCode {
    switch (tag) {
        case cProtocol::Payload::ValueInt8:
        case cProtocol::Payload::ValueInt16:
        case cProtocol::Payload::ValueInt32:
        case cProtocol::Payload::ValueInt64: {
            value_int_t value_int{};
            if (auto const err{deserialize_int_val(reader, tag, value_int)};
                IRErrorCode::IRErrorCode_Success != err)
            {
                return err;
            }
            node_id_value_pairs.emplace(node_id, Value{value_int});
            break;
        }
        case cProtocol::Payload::ValueFloat: {
            uint64_t val{};
            if (false == deserialize_int(reader, val)) {
                return IRErrorCode::IRErrorCode_Incomplete_IR;
            }
            node_id_value_pairs.emplace(node_id, Value{bit_cast<value_float_t>(val)});
            break;
        }
        case cProtocol::Payload::ValueTrue:
            node_id_value_pairs.emplace(node_id, Value{true});
            break;
        case cProtocol::Payload::ValueFalse:
            node_id_value_pairs.emplace(node_id, Value{false});
            break;
        case cProtocol::Payload::StrLenUByte:
        case cProtocol::Payload::StrLenUShort:
        case cProtocol::Payload::StrLenUInt: {
            std::string value_str;
            if (auto const err{deserialize_string(reader, tag, value_str)};
                IRErrorCode::IRErrorCode_Success != err)
            {
                return err;
            }
            node_id_value_pairs.emplace(node_id, Value{std::move(value_str)});
            break;
        }
        case cProtocol::Payload::ValueEightByteEncodingClpStr:
            if (auto const err{deserialize_encoded_text_ast_and_insert_to_node_id_value_pairs<
                        ir::eight_byte_encoded_variable_t>(reader, node_id, node_id_value_pairs)};
                IRErrorCode::IRErrorCode_Success != err)
            {
                return err;
            }
            break;
        case cProtocol::Payload::ValueFourByteEncodingClpStr:
            if (auto const err{deserialize_encoded_text_ast_and_insert_to_node_id_value_pairs<
                        ir::four_byte_encoded_variable_t>(reader, node_id, node_id_value_pairs)};
                IRErrorCode::IRErrorCode_Success != err)
            {
                return err;
            }
            break;
        case cProtocol::Payload::ValueNull:
            node_id_value_pairs.emplace(node_id, Value{});
            break;
        case cProtocol::Payload::ValueEmpty:
            node_id_value_pairs.emplace(node_id, std::nullopt);
            break;
        default:
            return IRErrorCode::IRErrorCode_Corrupted_IR;
    }
    return IRErrorCode::IRErrorCode_Success;
}

template <typename encoded_variable_t>
requires(std::is_same_v<ir::four_byte_encoded_variable_t, encoded_variable_t>
         || std::is_same_v<ir::eight_byte_encoded_variable_t, encoded_variable_t>)
[[nodiscard]] auto deserialize_encoded_text_ast_and_insert_to_node_id_value_pairs(
        ReaderInterface& reader,
        SchemaTreeNode::id_t node_id,
        KeyValuePairLogEvent::NodeIdValuePairs& node_id_value_pairs
) -> IRErrorCode {
    encoded_tag_t tag{};
    if (auto const err{deserialize_tag(reader, tag)}; IRErrorCode::IRErrorCode_Success != err) {
        return err;
    }

    std::string logtype;
    std::vector<encoded_variable_t> encoded_vars;
    std::vector<std::string> dict_vars;
    if (auto const err{deserialize_encoded_text_ast(reader, tag, logtype, encoded_vars, dict_vars)};
        IRErrorCode::IRErrorCode_Success != err)
    {
        return err;
    }

    node_id_value_pairs.emplace(
            node_id,
            Value{ir::EncodedTextAst<encoded_variable_t>{logtype, dict_vars, encoded_vars}}
    );
    return IRErrorCode::IRErrorCode_Success;
}

auto deserialize_value_and_construct_node_id_value_pairs(
        ReaderInterface& reader,
        encoded_tag_t tag,
        Schema const& schema,
        KeyValuePairLogEvent::NodeIdValuePairs& node_id_value_pairs
) -> IRErrorCode {
    node_id_value_pairs.clear();
    node_id_value_pairs.reserve(schema.size());
    for (auto const node_id : schema) {
        if (node_id_value_pairs.contains(node_id)) {
            // The key should be unique in a schema
            return IRErrorCode_Corrupted_IR;
        }

        if (auto const err{deserialize_value_and_insert_to_node_id_value_pairs(
                    reader,
                    tag,
                    node_id,
                    node_id_value_pairs
            )};
            IRErrorCode::IRErrorCode_Success != err)
        {
            return err;
        }

        if (schema.size() != node_id_value_pairs.size()) {
            if (auto const err{deserialize_tag(reader, tag)};
                IRErrorCode::IRErrorCode_Success != err)
            {
                return err;
            }
        }
    }
    return IRErrorCode::IRErrorCode_Success;
}
}  // namespace

auto Deserializer::create(ReaderInterface& reader
) -> OUTCOME_V2_NAMESPACE::std_result<Deserializer> {
    bool is_four_byte_encoded{};
    if (auto const err{get_encoding_type(reader, is_four_byte_encoded)};
        IRErrorCode::IRErrorCode_Success != err)
    {
        return ir_error_code_to_errc(err);
    }

    std::vector<int8_t> metadata;
    encoded_tag_t metadata_type{};
    if (auto const err{deserialize_preamble(reader, metadata_type, metadata)};
        IRErrorCode::IRErrorCode_Success != err)
    {
        return ir_error_code_to_errc(err);
    }

    if (cProtocol::Metadata::EncodingJson != metadata_type) {
        return std::errc::protocol_not_supported;
    }

    auto metadata_json = nlohmann::json::parse(metadata, nullptr, false);
    if (metadata_json.is_discarded()) {
        return std::errc::protocol_error;
    }
    auto const version_iter{metadata_json.find(cProtocol::Metadata::VersionKey)};
    if (metadata_json.end() == version_iter || false == version_iter->is_string()) {
        return std::errc::protocol_error;
    }
    auto const version = version_iter->get_ref<nlohmann::json::string_t&>();
    // TODO: Just before the KV-pair IR format is formally released, we should replace this
    // hard-coded version check with `ffi::ir_stream::validate_protocol_version`.
    if (std::string_view{static_cast<char const*>(cProtocol::Metadata::BetaVersionValue)}
        != version)
    {
        return std::errc::protocol_not_supported;
    }

    return Deserializer{};
}

auto Deserializer::deserialize_to_next_log_event(clp::ReaderInterface& reader
) -> OUTCOME_V2_NAMESPACE::std_result<KeyValuePairLogEvent> {
    auto const utc_offset_snapshot{m_utc_offset};
    m_schema_tree->take_snapshot();
    TransactionManager revert_manager{
            []() -> void {},
            [&]() -> void {
                m_utc_offset = utc_offset_snapshot;
                m_schema_tree->revert();
            }
    };

    encoded_tag_t tag{};
    if (auto const err{deserialize_tag(reader, tag)}; IRErrorCode::IRErrorCode_Success != err) {
        return ir_error_code_to_errc(err);
    }

    if (auto const err{deserialize_utc_offset_changes(reader, tag, m_utc_offset)};
        IRErrorCode::IRErrorCode_Success != err)
    {
        return ir_error_code_to_errc(err);
    }

    if (auto const err{deserialize_schema_tree_nodes(reader, tag, *m_schema_tree)};
        IRErrorCode::IRErrorCode_Success != err)
    {
        return ir_error_code_to_errc(err);
    }

    Schema schema;
    if (auto const err{deserialize_schema(reader, tag, schema)};
        IRErrorCode::IRErrorCode_Success != err)
    {
        return ir_error_code_to_errc(err);
    }

    KeyValuePairLogEvent::NodeIdValuePairs node_id_value_pairs;
    if (false == schema.empty()) {
        if (auto const err{deserialize_value_and_construct_node_id_value_pairs(
                    reader,
                    tag,
                    schema,
                    node_id_value_pairs
            )};
            IRErrorCode::IRErrorCode_Success != err)
        {
            return ir_error_code_to_errc(err);
        }
    } else {
        if (cProtocol::Payload::ValueEmpty != tag) {
            return ir_error_code_to_errc(IRErrorCode::IRErrorCode_Corrupted_IR);
        }
    }

    auto result{KeyValuePairLogEvent::create(
            m_schema_tree,
            std::move(node_id_value_pairs),
            m_utc_offset
    )};
    if (false == result.has_error()) {
        revert_manager.mark_success();
    }

    return std::move(result);
}
}  // namespace clp::ffi::ir_stream
