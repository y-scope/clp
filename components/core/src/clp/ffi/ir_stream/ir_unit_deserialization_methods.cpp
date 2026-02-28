#include "ir_unit_deserialization_methods.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

#include "../../ErrorCode.hpp"
#include "../../ir/types.hpp"
#include "../../ReaderInterface.hpp"
#include "../../time_types.hpp"
#include "../../type_utils.hpp"
#include "../KeyValuePairLogEvent.hpp"
#include "../SchemaTree.hpp"
#include "../Value.hpp"
#include "decoding_methods.hpp"
#include "IrDeserializationError.hpp"
#include "IrUnitType.hpp"
#include "protocol_constants.hpp"
#include "utils.hpp"

namespace clp::ffi::ir_stream {
namespace {
/**
 * A collection of schema tree leaf node IDs. It represents the schema of a `KeyValuePairLogEvent`.
 */
using Schema = std::vector<SchemaTree::Node::id_t>;

/**
 * @param tag
 * @return The corresponding schema tree node type on success.
 * @return std::nullopt if the tag doesn't match to any defined schema tree node type.
 */
[[nodiscard]] auto schema_tree_node_tag_to_type(encoded_tag_t tag)
        -> std::optional<SchemaTree::Node::Type>;

/**
 * Deserializes the parent ID of a schema tree node.
 * @param reader
 * @return A result containing a pair or an error code indicating the failure:
 * - The pair:
 *   - Whether the node ID is for an auto-generated node.
 *   - The decoded node ID.
 * - The possible error codes:
 *   - Forwards `deserialize_tag`'s return values.
 * @return Forwards `deserialize_and_decode_schema_tree_node_id`'s return values.
 */
[[nodiscard]] auto deserialize_schema_tree_node_parent_id(ReaderInterface& reader)
        -> ystdlib::error_handling::Result<std::pair<bool, SchemaTree::Node::id_t>>;

/**
 * Deserializes the key name of a schema tree node.
 * @param reader
 * @param key_name Returns the deserialized key name.
 * @return A void result on success.
 * @return Forwards `deserialize_tag`'s return values on failure.
 * @return Forwards `deserialize_string`'s return values on failure.
 */
[[nodiscard]] auto
deserialize_schema_tree_node_key_name(ReaderInterface& reader, std::string& key_name)
        -> ystdlib::error_handling::Result<void>;

/**
 * Deserializes an integer value packet.
 * @param reader
 * @param tag
 * @param val Returns the deserialized value.
 * @return A void result on success.
 * @return IrDeserializationErrorEnum::IncompleteStream if the stream is truncated.
 * @return IrDeserializationErrorEnum::InvalidTag if the given tag doesn't correspond to an integer
 * packet.
 */
[[nodiscard]] auto deserialize_int_val(ReaderInterface& reader, encoded_tag_t tag, value_int_t& val)
        -> ystdlib::error_handling::Result<void>;

/**
 * Deserializes a string packet.
 * @param reader
 * @param tag
 * @param deserialized_str Returns the deserialized string.
 * @return A void result on success.
 * @return IrDeserializationErrorEnum::IncompleteStream if the stream is truncated.
 * @return IrDeserializationErrorEnum::InvalidTag if the given tag doesn't correspond to a string
 * packet.
 */
[[nodiscard]] auto
deserialize_string(ReaderInterface& reader, encoded_tag_t tag, std::string& deserialized_str)
        -> ystdlib::error_handling::Result<void>;

/**
 * Deserializes the auto-generated node-ID-value pairs and the IDs of all user-generated keys in a
 * log event.
 * @param reader
 * @param tag Takes the current tag as input and returns the last tag read.
 * @return A result containing a pair or an error code indicating the failure:
 * - The pair:
 *   - The auto-generated node-ID-value pairs.
 *   - The IDs of all user-generated keys.
 * - The possible error codes:
 *   - Forwards `deserialize_tag`'s return values.
 *   - Forwards `deserialize_and_decode_schema_tree_node_id`'s return values.
 *   - IrDeserializationErrorEnum::InvalidKeyOrdering if the IR stream contains auto-generated key
 *     IDs *after* a user-generated key ID has been deserialized.
 */
[[nodiscard]] auto deserialize_auto_gen_node_id_value_pairs_and_user_gen_schema(
        ReaderInterface& reader,
        encoded_tag_t& tag
) -> ystdlib::error_handling::Result<std::pair<KeyValuePairLogEvent::NodeIdValuePairs, Schema>>;

/**
 * Deserializes the next value and pushes the result into `node_id_value_pairs`.
 * @param reader
 * @param tag
 * @param node_id The node ID that corresponds to the value.
 * @param node_id_value_pairs Returns the ID-value pair constructed from the deserialized value.
 * @return A void result on success.
 * @return IrDeserializationErrorEnum::IncompleteStream if the stream is truncated.
 * @return IrDeserializationErrorEnum::UnsupportedNodeType if the tag doesn't correspond to any
 * known value type.
 * @return Forwards `deserialize_encoded_text_ast_and_insert_to_node_id_value_pairs`'s return
 * values on any other failure.
 * @return Forwards `deserialize_int_val`'s return values on failure.
 * @return Forwards `deserialize_string`'s return values on failure.
 */
[[nodiscard]] auto deserialize_value_and_insert_to_node_id_value_pairs(
        ReaderInterface& reader,
        encoded_tag_t tag,
        SchemaTree::Node::id_t node_id,
        KeyValuePairLogEvent::NodeIdValuePairs& node_id_value_pairs
) -> ystdlib::error_handling::Result<void>;

/**
 * Deserializes an encoded text AST and pushes the result into node_id_value_pairs.
 * @tparam encoded_variable_t
 * @param reader
 * @param node_id The node ID that corresponds to the value.
 * @param node_id_value_pairs Returns the ID-value pair constructed by the deserialized encoded text
 * AST.
 * @return A void result on success.
 * @return Forwards `deserialize_tag`'s return values on failure.
 * @return Forwards `deserialize_encoded_text_ast`'s return values on failure.
 */
template <ir::EncodedVariableTypeReq encoded_variable_t>
[[nodiscard]] auto deserialize_encoded_text_ast_and_insert_to_node_id_value_pairs(
        ReaderInterface& reader,
        SchemaTree::Node::id_t node_id,
        KeyValuePairLogEvent::NodeIdValuePairs& node_id_value_pairs
) -> ystdlib::error_handling::Result<void>;

/**
 * Deserializes values and constructs ID-value pairs according to the given schema. The number of
 * values to deserialize is indicated by the size of the given schema.
 * @param reader
 * @param tag
 * @param schema The log event's schema.
 * @param node_id_value_pairs Returns the constructed ID-value pairs.
 * @return A void result on success.
 * @return IrDeserializationErrorEnum::DuplicateKey if a key is duplicated in the deserialized log
 * event.
 * @return Forwards `deserialize_tag`'s return values on failure.
 * @return Forwards `deserialize_value_and_insert_to_node_id_value_pairs`'s return values on
 * failure.
 */
[[nodiscard]] auto deserialize_value_and_construct_node_id_value_pairs(
        ReaderInterface& reader,
        encoded_tag_t tag,
        Schema const& schema,
        KeyValuePairLogEvent::NodeIdValuePairs& node_id_value_pairs
) -> ystdlib::error_handling::Result<void>;

/**
 * @param tag
 * @return Whether the given tag can be a valid leading tag of a log event IR unit.
 */
[[nodiscard]] auto is_log_event_ir_unit_tag(encoded_tag_t tag) -> bool;

/**
 * @param tag
 * @return Whether the given tag represents a valid encoded key ID.
 */
[[nodiscard]] auto is_encoded_key_id_tag(encoded_tag_t tag) -> bool;

auto schema_tree_node_tag_to_type(encoded_tag_t tag) -> std::optional<SchemaTree::Node::Type> {
    switch (tag) {
        case cProtocol::Payload::SchemaTreeNodeInt:
            return SchemaTree::Node::Type::Int;
        case cProtocol::Payload::SchemaTreeNodeFloat:
            return SchemaTree::Node::Type::Float;
        case cProtocol::Payload::SchemaTreeNodeBool:
            return SchemaTree::Node::Type::Bool;
        case cProtocol::Payload::SchemaTreeNodeStr:
            return SchemaTree::Node::Type::Str;
        case cProtocol::Payload::SchemaTreeNodeUnstructuredArray:
            return SchemaTree::Node::Type::UnstructuredArray;
        case cProtocol::Payload::SchemaTreeNodeObj:
            return SchemaTree::Node::Type::Obj;
        default:
            return std::nullopt;
    }
}

auto deserialize_schema_tree_node_parent_id(ReaderInterface& reader)
        -> ystdlib::error_handling::Result<std::pair<bool, SchemaTree::Node::id_t>> {
    encoded_tag_t tag{};
    if (auto const err{deserialize_tag(reader, tag)}; IRErrorCode_Success != err) {
        return to_ir_deserialization_error(err);
    }
    return deserialize_and_decode_schema_tree_node_id<
            cProtocol::Payload::EncodedSchemaTreeNodeParentIdByte,
            cProtocol::Payload::EncodedSchemaTreeNodeParentIdShort,
            cProtocol::Payload::EncodedSchemaTreeNodeParentIdInt
    >(tag, reader);
}

auto deserialize_schema_tree_node_key_name(ReaderInterface& reader, std::string& key_name)
        -> ystdlib::error_handling::Result<void> {
    encoded_tag_t str_packet_tag{};
    if (auto const err{deserialize_tag(reader, str_packet_tag)}; IRErrorCode_Success != err) {
        return to_ir_deserialization_error(err);
    }
    YSTDLIB_ERROR_HANDLING_TRYV(deserialize_string(reader, str_packet_tag, key_name));
    return ystdlib::error_handling::success();
}

auto deserialize_int_val(ReaderInterface& reader, encoded_tag_t tag, value_int_t& val)
        -> ystdlib::error_handling::Result<void> {
    if (cProtocol::Payload::ValueInt8 == tag) {
        int8_t deserialized_val{};
        if (false == deserialize_int(reader, deserialized_val)) {
            return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
        }
        val = deserialized_val;
    } else if (cProtocol::Payload::ValueInt16 == tag) {
        int16_t deserialized_val{};
        if (false == deserialize_int(reader, deserialized_val)) {
            return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
        }
        val = deserialized_val;
    } else if (cProtocol::Payload::ValueInt32 == tag) {
        int32_t deserialized_val{};
        if (false == deserialize_int(reader, deserialized_val)) {
            return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
        }
        val = deserialized_val;
    } else if (cProtocol::Payload::ValueInt64 == tag) {
        int64_t deserialized_val{};
        if (false == deserialize_int(reader, deserialized_val)) {
            return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
        }
        val = deserialized_val;
    } else {
        return IrDeserializationError{IrDeserializationErrorEnum::InvalidTag};
    }
    return ystdlib::error_handling::success();
}

auto deserialize_string(ReaderInterface& reader, encoded_tag_t tag, std::string& deserialized_str)
        -> ystdlib::error_handling::Result<void> {
    size_t str_length{};
    if (cProtocol::Payload::StrLenUByte == tag) {
        uint8_t length{};
        if (false == deserialize_int(reader, length)) {
            return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
        }
        str_length = static_cast<size_t>(length);
    } else if (cProtocol::Payload::StrLenUShort == tag) {
        uint16_t length{};
        if (false == deserialize_int(reader, length)) {
            return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
        }
        str_length = static_cast<size_t>(length);
    } else if (cProtocol::Payload::StrLenUInt == tag) {
        uint32_t length{};
        if (false == deserialize_int(reader, length)) {
            return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
        }
        str_length = static_cast<size_t>(length);
    } else {
        return IrDeserializationError{IrDeserializationErrorEnum::InvalidTag};
    }
    if (clp::ErrorCode_Success != reader.try_read_string(str_length, deserialized_str)) {
        return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
    }
    return ystdlib::error_handling::success();
}

auto deserialize_auto_gen_node_id_value_pairs_and_user_gen_schema(
        ReaderInterface& reader,
        encoded_tag_t& tag
) -> ystdlib::error_handling::Result<std::pair<KeyValuePairLogEvent::NodeIdValuePairs, Schema>> {
    KeyValuePairLogEvent::NodeIdValuePairs auto_gen_node_id_value_pairs;
    Schema user_gen_schema;

    // Deserialize pairs of auto-generated node IDs and values
    while (true) {
        if (false == is_encoded_key_id_tag(tag)) {
            break;
        }

        auto const schema_tree_node_id_result{deserialize_and_decode_schema_tree_node_id<
                cProtocol::Payload::EncodedSchemaTreeNodeIdByte,
                cProtocol::Payload::EncodedSchemaTreeNodeIdShort,
                cProtocol::Payload::EncodedSchemaTreeNodeIdInt
        >(tag, reader)};
        if (schema_tree_node_id_result.has_error()) {
            return schema_tree_node_id_result.error();
        }

        // Advance to the next tag. This is needed no matter whether the deserialized node ID is
        // auto-generated.
        if (auto const err{deserialize_tag(reader, tag)}; IRErrorCode_Success != err) {
            return to_ir_deserialization_error(err);
        }

        auto const [is_auto_generated, node_id]{schema_tree_node_id_result.value()};
        if (false == is_auto_generated) {
            // User-generated node ID has been deserialized, so save the node ID and terminate
            // auto-generated node-ID-value pair deserialization.
            user_gen_schema.push_back(node_id);
            break;
        }

        YSTDLIB_ERROR_HANDLING_TRYV(deserialize_value_and_insert_to_node_id_value_pairs(
                reader,
                tag,
                node_id,
                auto_gen_node_id_value_pairs
        ));
        if (auto const err{deserialize_tag(reader, tag)}; IRErrorCode_Success != err) {
            return to_ir_deserialization_error(err);
        }
    }

    // Deserialize any remaining user-generated node IDs
    while (is_encoded_key_id_tag(tag)) {
        auto const schema_tree_node_id_result{deserialize_and_decode_schema_tree_node_id<
                cProtocol::Payload::EncodedSchemaTreeNodeIdByte,
                cProtocol::Payload::EncodedSchemaTreeNodeIdShort,
                cProtocol::Payload::EncodedSchemaTreeNodeIdInt
        >(tag, reader)};
        if (schema_tree_node_id_result.has_error()) {
            return schema_tree_node_id_result.error();
        }
        auto const [is_auto_generated, node_id]{schema_tree_node_id_result.value()};
        if (is_auto_generated) {
            return IrDeserializationError{IrDeserializationErrorEnum::InvalidKeyOrdering};
        }
        user_gen_schema.push_back(node_id);

        if (auto const err{deserialize_tag(reader, tag)}; IRErrorCode_Success != err) {
            return to_ir_deserialization_error(err);
        }
    }

    return {std::move(auto_gen_node_id_value_pairs), std::move(user_gen_schema)};
}

auto deserialize_value_and_insert_to_node_id_value_pairs(
        ReaderInterface& reader,
        encoded_tag_t tag,
        SchemaTree::Node::id_t node_id,
        KeyValuePairLogEvent::NodeIdValuePairs& node_id_value_pairs
) -> ystdlib::error_handling::Result<void> {
    switch (tag) {
        case cProtocol::Payload::ValueInt8:
        case cProtocol::Payload::ValueInt16:
        case cProtocol::Payload::ValueInt32:
        case cProtocol::Payload::ValueInt64: {
            value_int_t value_int{};
            YSTDLIB_ERROR_HANDLING_TRYV(deserialize_int_val(reader, tag, value_int));
            node_id_value_pairs.emplace(node_id, Value{value_int});
            break;
        }
        case cProtocol::Payload::ValueFloat: {
            uint64_t val{};
            if (false == deserialize_int(reader, val)) {
                return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
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
            YSTDLIB_ERROR_HANDLING_TRYV(deserialize_string(reader, tag, value_str));
            node_id_value_pairs.emplace(node_id, Value{std::move(value_str)});
            break;
        }
        case cProtocol::Payload::ValueEightByteEncodingClpStr: {
            YSTDLIB_ERROR_HANDLING_TRYV(
                    deserialize_encoded_text_ast_and_insert_to_node_id_value_pairs<
                            ir::eight_byte_encoded_variable_t
                    >(reader, node_id, node_id_value_pairs)
            );
            break;
        }
        case cProtocol::Payload::ValueFourByteEncodingClpStr: {
            YSTDLIB_ERROR_HANDLING_TRYV(
                    deserialize_encoded_text_ast_and_insert_to_node_id_value_pairs<
                            ir::four_byte_encoded_variable_t
                    >(reader, node_id, node_id_value_pairs)
            );
            break;
        }
        case cProtocol::Payload::ValueNull:
            node_id_value_pairs.emplace(node_id, Value{});
            break;
        case cProtocol::Payload::ValueEmpty:
            node_id_value_pairs.emplace(node_id, std::nullopt);
            break;
        default:
            return IrDeserializationError{IrDeserializationErrorEnum::UnsupportedNodeType};
    }
    return ystdlib::error_handling::success();
}

template <ir::EncodedVariableTypeReq encoded_variable_t>
[[nodiscard]] auto deserialize_encoded_text_ast_and_insert_to_node_id_value_pairs(
        ReaderInterface& reader,
        SchemaTree::Node::id_t node_id,
        KeyValuePairLogEvent::NodeIdValuePairs& node_id_value_pairs
) -> ystdlib::error_handling::Result<void> {
    encoded_tag_t tag{};
    if (auto const err{deserialize_tag(reader, tag)}; IRErrorCode_Success != err) {
        return to_ir_deserialization_error(err);
    }

    auto encoded_text_ast_result{deserialize_encoded_text_ast<encoded_variable_t>(reader, tag)};
    if (encoded_text_ast_result.has_error()) {
        return to_ir_deserialization_error(encoded_text_ast_result.error());
    }

    node_id_value_pairs.emplace(node_id, Value{std::move(encoded_text_ast_result.value())});
    return ystdlib::error_handling::success();
}

auto deserialize_value_and_construct_node_id_value_pairs(
        ReaderInterface& reader,
        encoded_tag_t tag,
        Schema const& schema,
        KeyValuePairLogEvent::NodeIdValuePairs& node_id_value_pairs
) -> ystdlib::error_handling::Result<void> {
    node_id_value_pairs.clear();
    node_id_value_pairs.reserve(schema.size());
    for (auto const node_id : schema) {
        if (node_id_value_pairs.contains(node_id)) {
            // The key should be unique in a schema
            return IrDeserializationError{IrDeserializationErrorEnum::DuplicateKey};
        }

        YSTDLIB_ERROR_HANDLING_TRYV(deserialize_value_and_insert_to_node_id_value_pairs(
                reader,
                tag,
                node_id,
                node_id_value_pairs
        ));

        if (schema.size() != node_id_value_pairs.size()) {
            if (auto const err{deserialize_tag(reader, tag)}; IRErrorCode_Success != err) {
                return to_ir_deserialization_error(err);
            }
        }
    }
    return ystdlib::error_handling::success();
}

auto is_log_event_ir_unit_tag(encoded_tag_t tag) -> bool {
    if (cProtocol::Payload::ValueEmpty == tag) {
        // The log event is an empty object
        return true;
    }
    if (is_encoded_key_id_tag(tag)) {
        // If not empty, the log event must start with a tag byte indicating the key ID
        return true;
    }
    return false;
}

auto is_encoded_key_id_tag(encoded_tag_t tag) -> bool {
    // Ideally, we could check whether the tag is within the range of
    // [EncodedKeyIdByte, EncodedKeyIdInt], but we don't for two reasons:
    // - We optimize for streams that have few key IDs, meaning we can short circuit in the first
    //   branch below.
    // - Using a range check assumes all length indicators are defined continuously, in order, but
    //   we don't have static checks for this assumption.
    return cProtocol::Payload::EncodedSchemaTreeNodeIdByte == tag
           || cProtocol::Payload::EncodedSchemaTreeNodeIdShort == tag
           || cProtocol::Payload::EncodedSchemaTreeNodeIdInt == tag;
}
}  // namespace

auto get_ir_unit_type_from_tag(encoded_tag_t tag) -> std::optional<IrUnitType> {
    // First, we check the tags that have one-to-one IR unit mapping
    if (cProtocol::Eof == tag) {
        return IrUnitType::EndOfStream;
    }
    if (cProtocol::Payload::UtcOffsetChange == tag) {
        return IrUnitType::UtcOffsetChange;
    }

    // Then, check tags that may match any byte within a continuous range
    if ((tag & cProtocol::Payload::SchemaTreeNodeMask) == cProtocol::Payload::SchemaTreeNodeMask) {
        return IrUnitType::SchemaTreeNodeInsertion;
    }

    if (is_log_event_ir_unit_tag(tag)) {
        return IrUnitType::LogEvent;
    }

    return std::nullopt;
}

auto deserialize_ir_unit_schema_tree_node_insertion(
        ReaderInterface& reader,
        encoded_tag_t tag,
        std::string& key_name
) -> ystdlib::error_handling::Result<std::pair<bool, SchemaTree::NodeLocator>> {
    auto const type{schema_tree_node_tag_to_type(tag)};
    if (false == type.has_value()) {
        return IrDeserializationError{IrDeserializationErrorEnum::UnsupportedNodeType};
    }

    auto const parent_node_id_result{deserialize_schema_tree_node_parent_id(reader)};
    if (parent_node_id_result.has_error()) {
        return parent_node_id_result.error();
    }
    auto const [is_auto_generated, parent_id]{parent_node_id_result.value()};
    YSTDLIB_ERROR_HANDLING_TRYV(deserialize_schema_tree_node_key_name(reader, key_name));

    return {is_auto_generated, SchemaTree::NodeLocator{parent_id, key_name, type.value()}};
}

auto deserialize_ir_unit_utc_offset_change(ReaderInterface& reader)
        -> ystdlib::error_handling::Result<UtcOffset> {
    UtcOffset utc_offset{};
    if (auto const err{deserialize_utc_offset_change(reader, utc_offset)};
        IRErrorCode_Success != err)
    {
        return to_ir_deserialization_error(err);
    }
    return utc_offset;
}

auto deserialize_ir_unit_kv_pair_log_event(
        ReaderInterface& reader,
        encoded_tag_t tag,
        std::shared_ptr<SchemaTree> auto_gen_keys_schema_tree,
        std::shared_ptr<SchemaTree> user_gen_keys_schema_tree,
        UtcOffset utc_offset
) -> ystdlib::error_handling::Result<KeyValuePairLogEvent> {
    auto auto_gen_node_id_value_pairs_and_user_gen_schema_result{
            deserialize_auto_gen_node_id_value_pairs_and_user_gen_schema(reader, tag)
    };
    if (auto_gen_node_id_value_pairs_and_user_gen_schema_result.has_error()) {
        return auto_gen_node_id_value_pairs_and_user_gen_schema_result.error();
    }
    auto& [auto_gen_node_id_value_pairs,
           user_gen_schema]{auto_gen_node_id_value_pairs_and_user_gen_schema_result.value()};

    KeyValuePairLogEvent::NodeIdValuePairs user_gen_node_id_value_pairs;
    if (false == user_gen_schema.empty()) {
        YSTDLIB_ERROR_HANDLING_TRYV(deserialize_value_and_construct_node_id_value_pairs(
                reader,
                tag,
                user_gen_schema,
                user_gen_node_id_value_pairs
        ));
    } else {
        if (cProtocol::Payload::ValueEmpty != tag) {
            return IrDeserializationError{IrDeserializationErrorEnum::InvalidTag};
        }
    }

    return KeyValuePairLogEvent::create(
            std::move(auto_gen_keys_schema_tree),
            std::move(user_gen_keys_schema_tree),
            std::move(auto_gen_node_id_value_pairs),
            std::move(user_gen_node_id_value_pairs),
            utc_offset
    );
}
}  // namespace clp::ffi::ir_stream
