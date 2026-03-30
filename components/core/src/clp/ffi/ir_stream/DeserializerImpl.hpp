#ifndef CLP_FFI_IR_STREAM_DESERIALIZER_IMPL_HPP
#define CLP_FFI_IR_STREAM_DESERIALIZER_IMPL_HPP

#include <memory>
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
 * A base class for implementing deserializers for different IR stream formats.
 */
class DeserializerImpl {
public:
    // Destructor
    virtual ~DeserializerImpl() = default;

    // Methods
    /**
     * Deserializes the next IR unit type from the given reader.
     * @param reader
     * @return A result containing a pair on success, or an error code indicating the failure:
     * - The pair:
     *   - The type of the deserialized IR unit.
     *   - The tag of the deserialized IR unit.
     * - Derived classes define the possible error codes.
     */
    [[nodiscard]] virtual auto get_next_ir_unit_type(ReaderInterface& reader)
            -> ystdlib::error_handling::Result<std::pair<IrUnitType, encoded_tag_t>>
            = 0;

    /**
     * Deserializes a KV pair log event from the given reader.
     * @param reader
     * @param tag
     * @param auto_gen_keys_schema_tree
     * @param user_gen_keys_schema_tree
     * @param utc_offset
     * @return A result containing the deserialized KV pair log event on success, or an error code
     * indicating the failure defined by derived classes.
     */
    [[nodiscard]] virtual auto deserialize_ir_unit_kv_pair_log_event(
            ReaderInterface& reader,
            encoded_tag_t tag,
            std::shared_ptr<SchemaTree> const& auto_gen_keys_schema_tree,
            std::shared_ptr<SchemaTree> const& user_gen_keys_schema_tree,
            UtcOffset utc_offset
    ) -> ystdlib::error_handling::Result<KeyValuePairLogEvent>
            = 0;

    /**
     * Deserializes a schema tree node insertion from the given reader.
     * @param reader
     * @param tag
     * @param out_key_name Returns the inserted node's key name.
     * @return A result containing a pair on success, or an error code indicating the failure:
     * - The pair:
     *   - Whether the node is for auto-generated keys schema tree.
     *   - The locator of the inserted schema tree node.
     * - Derived classes define the possible error codes.
     */
    [[nodiscard]] virtual auto deserialize_ir_unit_schema_tree_node_insertion(
            ReaderInterface& reader,
            encoded_tag_t tag,
            std::string& out_key_name
    ) -> ystdlib::error_handling::Result<std::pair<bool, SchemaTree::NodeLocator>>
            = 0;

    /**
     * Deserializes a UTC offset change packet.
     * @param reader
     * @return A result containing the deserialized UTC offset on success, or an error code
     * indicating the failure:
     * - IrDeserializationErrorEnum::IncompleteStream if reader doesn't contain enough data to
     *   deserialize.
     */
    [[nodiscard]] auto deserialize_utc_offset_change(ReaderInterface& reader)
            -> ystdlib::error_handling::Result<UtcOffset>;
};
}  // namespace clp::ffi::ir_stream

#endif  // CLP_FFI_IR_STREAM_DESERIALIZER_IMPL_HPP
