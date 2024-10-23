#ifndef CLP_FFI_IR_STREAM_IR_UNIT_DESERIALIZATION_METHODS_HPP
#define CLP_FFI_IR_STREAM_IR_UNIT_DESERIALIZATION_METHODS_HPP

#include <memory>
#include <optional>
#include <string>

#include <outcome/single-header/outcome.hpp>

#include "../../ReaderInterface.hpp"
#include "../../time_types.hpp"
#include "../KeyValuePairLogEvent.hpp"
#include "../SchemaTree.hpp"
#include "decoding_methods.hpp"
#include "IrUnitType.hpp"

namespace clp::ffi::ir_stream {
/**
 * @param tag
 * @return The IR unit type of indicated by the given tag on success.
 * @return std::nullopt if the tag doesn't represent a valid IR unit.
 */
[[nodiscard]] auto get_ir_unit_type_from_tag(encoded_tag_t tag) -> std::optional<IrUnitType>;

/**
 * Deserializes a schema tree node insertion IR unit.
 * @param reader
 * @param tag
 * @param key_name Returns the key name of the deserialized new node. This should be the underlying
 * storage of the returned schema tree node locator.
 * @return A result containing the locator of the inserted schema tree node or an error code
 * indicating the failure:
 * - std::errc::result_out_of_range if the IR stream is truncated.
 * - std::errc::protocol_error if the deserialized node type isn't supported.
 * - std::errc::protocol_not_supported if the IR stream contains auto-generated keys (TODO: Remove
 *   this once auto-generated keys are fully supported).
 * - Forwards `deserialize_schema_tree_node_key_name`'s return values.
 * - Forwards `deserialize_schema_tree_node_parent_id`'s return values.
 */
[[nodiscard]] auto deserialize_ir_unit_schema_tree_node_insertion(
        ReaderInterface& reader,
        encoded_tag_t tag,
        std::string& key_name
) -> OUTCOME_V2_NAMESPACE::std_result<SchemaTree::NodeLocator>;

/**
 * Deserializes a UTC offset change IR unit.
 * @param reader
 * @return A result containing the new UTC offset or an error code indicating the failure:
 * - std::errc::result_out_of_range if the IR stream is truncated.
 * - Forwards `clp::ffi::ir_stream::deserialize_utc_offset_change`'s return values.
 */
[[nodiscard]] auto deserialize_ir_unit_utc_offset_change(ReaderInterface& reader
) -> OUTCOME_V2_NAMESPACE::std_result<UtcOffset>;

/**
 * Deserializes a key-value pair log event IR unit.
 * @param reader
 * @param tag
 * @param schema_tree Schema tree used to construct the KV-pair log event.
 * @param utc_offset UTC offset used to construct the KV-pair log event.
 * @return A result containing the deserialized log event or an error code indicating the
 * failure:
 * - std::errc::result_out_of_range if the IR stream is truncated.
 * - std::errc::protocol_error if the IR stream is corrupted.
 * - std::errc::protocol_not_supported if the IR stream contains an unsupported metadata format
 *   or uses an unsupported version.
 * - Forwards `deserialize_schema`'s return values.
 * - Forwards `KeyValuePairLogEvent::create`'s return values if the intermediate deserialized result
 *   cannot construct a valid key-value pair log event.
 */
[[nodiscard]] auto deserialize_ir_unit_kv_pair_log_event(
        ReaderInterface& reader,
        encoded_tag_t tag,
        std::shared_ptr<SchemaTree> schema_tree,
        UtcOffset utc_offset
) -> OUTCOME_V2_NAMESPACE::std_result<KeyValuePairLogEvent>;
}  // namespace clp::ffi::ir_stream

#endif  // CLP_FFI_IR_STREAM_IR_UNIT_DESERIALIZATION_METHODS_HPP
