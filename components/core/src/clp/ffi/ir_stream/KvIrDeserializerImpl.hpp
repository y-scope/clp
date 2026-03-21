#ifndef CLP_FFI_IR_STREAM_KV_IR_DESERIALIZER_IMPL_HPP
#define CLP_FFI_IR_STREAM_KV_IR_DESERIALIZER_IMPL_HPP

#include "DeserializerImpl.hpp"

namespace clp::ffi::ir_stream {
/**
 * IR deserializer implementation for key-value pair IR streams.
 */
class KvIrDeserializerImpl : public DeserializerImpl {
public:
    // Constructor
    KvIrDeserializerImpl() = default;

    // Delete copy constructor and assignment operator
    KvIrDeserializerImpl(KvIrDeserializerImpl const&) = delete;
    auto operator=(KvIrDeserializerImpl const&) -> KvIrDeserializerImpl& = delete;

    // Define default move constructor and assignment operator
    KvIrDeserializerImpl(KvIrDeserializerImpl&&) = default;
    auto operator=(KvIrDeserializerImpl&&) -> KvIrDeserializerImpl& = default;

    // Methods
    /**
     * Deserializes the next IR unit type from the given reader.
     * @param reader
     * @return A result containing a pair on success, or an error code indicating the failure:
     * - The pair:
     *   - The type of the deserialized IR unit.
     *   - The tag of the deserialized IR unit.
     * - The possible error codes:
     *   - IrDeserializationErrorEnum::InvalidTag if the tag doesn't represent a valid IR unit.
     *   - Forwards `deserialize_tag`'s return values on failure.
     */
    [[nodiscard]] auto get_next_ir_unit_type(ReaderInterface& reader)
            -> ystdlib::error_handling::Result<std::pair<IrUnitType, encoded_tag_t>> override;

    /**
     * Deserializes a key-value pair log event from the given reader.
     * @param reader
     * @param tag The already-deserialized tag for the IR unit.
     * @param auto_gen_keys_schema_tree
     * @param user_gen_keys_schema_tree
     * @param utc_offset
     * @return A result containing the deserialized log event on success, or an error code
     * indicating the failure:
     * - Forwards `clp::ffi::ir_stream::deserialize_ir_unit_kv_pair_log_event`'s return values
     * on failure.
     */
    [[nodiscard]] auto deserialize_ir_unit_kv_pair_log_event(
            ReaderInterface& reader,
            encoded_tag_t tag,
            std::shared_ptr<SchemaTree> const& auto_gen_keys_schema_tree,
            std::shared_ptr<SchemaTree> const& user_gen_keys_schema_tree,
            UtcOffset utc_offset
    ) -> ystdlib::error_handling::Result<KeyValuePairLogEvent> override;

    /**
     * Deserializes a schema tree node insertion from the given reader.
     * @param reader
     * @param tag The already-deserialized tag for the IR unit.
     * @param out_key_name Returns the inserted node's key name.
     * @return A result containing a pair on success, or an error code indicating the failure:
     * - The pair:
     *   - Whether the node is for the auto-generated keys schema tree.
     *   - The locator of the inserted schema tree node.
     * - The possible error codes:
     *   - Forwards `clp::ffi::ir_stream::deserialize_ir_unit_schema_tree_node_insertion`'s
     * return values on failure.
     */
    [[nodiscard]] auto deserialize_ir_unit_schema_tree_node_insertion(
            ReaderInterface& reader,
            encoded_tag_t tag,
            std::string& out_key_name
    ) -> ystdlib::error_handling::Result<std::pair<bool, SchemaTree::NodeLocator>> override;
};
}  // namespace clp::ffi::ir_stream

#endif  // CLP_FFI_KV_IR_DESERIALIZER_IMPL_HPP
