#include "KvIrDeserializerImpl.hpp"

#include "ir_unit_deserialization_methods.hpp"
#include "IrDeserializationError.hpp"

namespace clp::ffi::ir_stream {
auto KvIrDeserializerImpl::get_next_ir_unit_type(ReaderInterface& reader)
        -> ystdlib::error_handling::Result<std::pair<IrUnitType, encoded_tag_t>> {
    auto const tag{YSTDLIB_ERROR_HANDLING_TRYX(deserialize_tag(reader))};
    auto const optional_ir_unit_type{get_ir_unit_type_from_tag(tag)};
    if (false == optional_ir_unit_type.has_value()) {
        return IrDeserializationError{IrDeserializationErrorEnum::InvalidTag};
    }
    return std::pair{optional_ir_unit_type.value(), tag};
}

auto KvIrDeserializerImpl::deserialize_ir_unit_kv_pair_log_event(
        ReaderInterface& reader,
        encoded_tag_t tag,
        std::shared_ptr<SchemaTree> const& auto_gen_keys_schema_tree,
        std::shared_ptr<SchemaTree> const& user_gen_keys_schema_tree,
        UtcOffset utc_offset
) -> ystdlib::error_handling::Result<KeyValuePairLogEvent> {
    return ir_stream::deserialize_ir_unit_kv_pair_log_event(
            reader,
            tag,
            auto_gen_keys_schema_tree,
            user_gen_keys_schema_tree,
            utc_offset
    );
}

auto KvIrDeserializerImpl::deserialize_ir_unit_schema_tree_node_insertion(
        ReaderInterface& reader,
        encoded_tag_t tag,
        std::string& out_key_name
) -> ystdlib::error_handling::Result<std::pair<bool, SchemaTree::NodeLocator>> {
    return ir_stream::deserialize_ir_unit_schema_tree_node_insertion(reader, tag, out_key_name);
}
}  // namespace clp::ffi::ir_stream
