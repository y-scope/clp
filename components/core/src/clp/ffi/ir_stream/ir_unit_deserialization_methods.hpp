#ifndef CLP_FFI_IR_STREAM_IR_UNIT_DESERIALIZATION_METHODS_HPP
#define CLP_FFI_IR_STREAM_IR_UNIT_DESERIALIZATION_METHODS_HPP

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <ystdlib/error_handling/Result.hpp>

#include "../../ReaderInterface.hpp"
#include "../../time_types.hpp"
#include "../KeyValuePairLogEvent.hpp"
#include "../SchemaTree.hpp"
#include "decoding_methods.hpp"
#include "IrUnitType.hpp"

namespace clp::ffi::ir_stream {
/**
 * @param tag
 * @return The IR unit type as indicated by the given tag on success.
 * @return std::nullopt if the tag doesn't represent a valid IR unit.
 */
[[nodiscard]] auto get_ir_unit_type_from_tag(encoded_tag_t tag) -> std::optional<IrUnitType>;

/**
 * Deserializes a schema tree node insertion IR unit.
 * @param reader
 * @param tag
 * @param key_name Returns the key name of the deserialized new node. This should be the underlying
 * storage of the returned schema tree node locator.
 * @return A result containing a pair or an error code indicating the failure:
 * - The pair:
 *   - Whether the node is for auto-generated keys schema tree.
 *   - The locator of the inserted schema tree node.
 * - The possible error codes:
 *   - IrErrorCodeEnum::IncompleteStream if the IR stream is truncated.
 *   - IrErrorCodeEnum::CorruptedIR if the deserialized node type isn't supported.
 *   - Forwards `deserialize_schema_tree_node_key_name`'s return values.
 *   - Forwards `deserialize_schema_tree_node_parent_id`'s return values.
 */
[[nodiscard]] auto deserialize_ir_unit_schema_tree_node_insertion(
        ReaderInterface& reader,
        encoded_tag_t tag,
        std::string& key_name
) -> ystdlib::error_handling::Result<std::pair<bool, SchemaTree::NodeLocator>>;

/**
 * Deserializes a UTC offset change IR unit.
 * @param reader
 * @return A result containing the new UTC offset or an error code indicating the failure:
 * - IrErrorCodeEnum::IncompleteStream if the IR stream is truncated.
 * - Forwards `clp::ffi::ir_stream::deserialize_utc_offset_change`'s return values.
 */
[[nodiscard]] auto deserialize_ir_unit_utc_offset_change(ReaderInterface& reader)
        -> ystdlib::error_handling::Result<UtcOffset>;

/**
 * Deserializes a key-value pair log event IR unit.
 * @param reader
 * @param tag
 * @param auto_gen_keys_schema_tree Schema tree for auto-generated keys, used to construct the
 * KV-pair log event.
 * @param user_gen_keys_schema_tree Schema tree for user-generated keys, used to construct the
 * KV-pair log event.
 * @param utc_offset UTC offset used to construct the KV-pair log event.
 * @return A result containing the deserialized log event or an error code indicating the failure:
 * - IrErrorCodeEnum::IncompleteStream if the IR stream is truncated.
 * - IrErrorCodeEnum::CorruptedIR if the IR stream is corrupted or contains an unsupported
 *   metadata format or version.
 * - Forwards `deserialize_auto_gen_node_id_value_pairs_and_user_gen_schema`'s return values.
 * - Forwards `KeyValuePairLogEvent::create`'s return values if the intermediate deserialized result
 *   cannot construct a valid key-value pair log event.
 */
[[nodiscard]] auto deserialize_ir_unit_kv_pair_log_event(
        ReaderInterface& reader,
        encoded_tag_t tag,
        std::shared_ptr<SchemaTree> auto_gen_keys_schema_tree,
        std::shared_ptr<SchemaTree> user_gen_keys_schema_tree,
        UtcOffset utc_offset
) -> ystdlib::error_handling::Result<KeyValuePairLogEvent>;
}  // namespace clp::ffi::ir_stream

#endif  // CLP_FFI_IR_STREAM_IR_UNIT_DESERIALIZATION_METHODS_HPP
