#ifndef CLP_FFI_IR_STREAM_DESERIALIZERIMPL_HPP
#define CLP_FFI_IR_STREAM_DESERIALIZERIMPL_HPP

#include <memory>
#include <string>

#include <outcome/single-header/outcome.hpp>

#include "../../ReaderInterface.hpp"
#include "../../time_types.hpp"
#include "../KeyValuePairLogEvent.hpp"
#include "../SchemaTree.hpp"
#include "decoding_methods.hpp"

namespace clp::ffi::ir_stream {
/**
 * Deserializes schema tree node insertion IR unit.
 * @param reader
 * @param tag
 * @param key_name Returns the key name of the deserialized new node. This should be the underlying
 * storage of the returned schema tree node locator.
 * @return A result containing the locator of the inserted schema tree node or an error code
 * indicating the failure:
 * - std::errc::result_out_of_range if the IR stream is truncated.
 * - std::errc::protocol_error if the deserialized node type isn't supported.
 * - Same as `deserialize_schema_tree_node_parent_id` or `deserialize_schema_tree_node_key_name`.
 */
[[nodiscard]] auto deserialize_ir_unit_schema_tree_node_insertion(
        ReaderInterface& reader,
        encoded_tag_t tag,
        std::string& key_name
) -> OUTCOME_V2_NAMESPACE::std_result<SchemaTree::NodeLocator>;

/**
 * Deserializes UTC offset change IR unit.
 * @param reader
 * @return A result containing the new UTC offset or an error code indicating the failure:
 * - std::errc::result_out_of_range if the IR stream is truncated.
 * - Same as `clp::ffi::ir_stream::deserialize_utc_offset_change`.
 */
[[nodiscard]] auto deserialize_ir_unit_utc_offset_change(ReaderInterface& reader
) -> OUTCOME_V2_NAMESPACE::std_result<UtcOffset>;

/**
 * Deserializes Key-value pair log event IR unit.
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
 * - Same as `KeyValuePairLogEvent::create` if the intermediate deserialized result cannot
 *   construct a valid key-value pair log event.
 */
[[nodiscard]] auto deserialize_ir_unit_kv_pair_log_event(
        ReaderInterface& reader,
        encoded_tag_t tag,
        std::shared_ptr<SchemaTree> schema_tree,
        UtcOffset utc_offset
) -> OUTCOME_V2_NAMESPACE::std_result<KeyValuePairLogEvent>;
}  // namespace clp::ffi::ir_stream

#endif  // CLP_FFI_IR_STREAM_DESERIALIZERIMPL_HPP
