#ifndef CLP_FFI_IR_STREAM_KV_IR_DESERIALIZER_IMPL_HPP
#define CLP_FFI_IR_STREAM_KV_IR_DESERIALIZER_IMPL_HPP

#include <memory>
#include <string>
#include <utility>

#include <ystdlib/error_handling/Result.hpp>

#include "../../ReaderInterface.hpp"
#include "../../time_types.hpp"
#include "../KeyValuePairLogEvent.hpp"
#include "../SchemaTree.hpp"
#include "decoding_methods.hpp"
#include "DeserializerImpl.hpp"
#include "IrUnitType.hpp"

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

    // Default move constructor and assignment operator
    KvIrDeserializerImpl(KvIrDeserializerImpl&&) = default;
    auto operator=(KvIrDeserializerImpl&&) -> KvIrDeserializerImpl& = default;

    // Destructor
    ~KvIrDeserializerImpl() override = default;

    // Methods implementing `DeserializerImpl`
    /**
     * The possible error codes:
     * - IrDeserializationErrorEnum::InvalidTag if the tag doesn't represent a valid IR unit.
     * - Forwards `deserialize_tag`'s return values on failure.
     */
    [[nodiscard]] auto get_next_ir_unit_type(ReaderInterface& reader)
            -> ystdlib::error_handling::Result<std::pair<IrUnitType, encoded_tag_t>> override;

    /**
     * The possible error codes:
     * - Forwards `clp::ffi::ir_stream::deserialize_ir_unit_kv_pair_log_event`'s return values
     *   on failure.
     */
    [[nodiscard]] auto deserialize_ir_unit_kv_pair_log_event(
            ReaderInterface& reader,
            encoded_tag_t tag,
            std::shared_ptr<SchemaTree> const& auto_gen_keys_schema_tree,
            std::shared_ptr<SchemaTree> const& user_gen_keys_schema_tree,
            UtcOffset utc_offset
    ) -> ystdlib::error_handling::Result<KeyValuePairLogEvent> override;

    /**
     * The possible error codes:
     * - Forwards `clp::ffi::ir_stream::deserialize_ir_unit_schema_tree_node_insertion`'s return
     *   values on failure.
     */
    [[nodiscard]] auto deserialize_ir_unit_schema_tree_node_insertion(
            ReaderInterface& reader,
            encoded_tag_t tag,
            std::string& key_name
    ) -> ystdlib::error_handling::Result<std::pair<bool, SchemaTree::NodeLocator>> override;
};
}  // namespace clp::ffi::ir_stream

#endif  // CLP_FFI_IR_STREAM_KV_IR_DESERIALIZER_IMPL_HPP
