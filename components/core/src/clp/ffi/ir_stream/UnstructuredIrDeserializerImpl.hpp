#ifndef CLP_FFI_IR_STREAM_UNSTRUCTURED_IR_DESERIALIZER_IMPL_HPP
#define CLP_FFI_IR_STREAM_UNSTRUCTURED_IR_DESERIALIZER_IMPL_HPP

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <nlohmann/json_fwd.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include "../../ir/types.hpp"
#include "../../ReaderInterface.hpp"
#include "../../time_types.hpp"
#include "../../type_utils.hpp"
#include "../KeyValuePairLogEvent.hpp"
#include "../SchemaTree.hpp"
#include "decoding_methods.hpp"
#include "DeserializerImpl.hpp"
#include "IrUnitType.hpp"

namespace clp::ffi::ir_stream {
/**
 * Deserializer implementation for backward-compatible unstructured IR streams.
 *
 * Simulates KV-IR deserialization behavior so `Deserializer` can reuse the same flow across
 * formats. It emits synthetic schema tree node insertion IR units for `message` and `timestamp`
 * before reading stream tags and converts unstructured log events into `KeyValuePairLogEvent`
 * values.
 * @tparam encoded_variable_t
 */
template <ir::EncodedVariableTypeReq encoded_variable_t>
class UnstructuredIrDeserializerImpl : public DeserializerImpl {
public:
    // Factory function
    /**
     * @param metadata
     * @return A result containing the deserializer on success, or an error code
     * indicating the failure:
     * - IrDeserializationErrorEnum::InvalidReferenceTimestampMetadata if the reference timestamp is
     *   missing or not a string (four-byte only).
     * - IrDeserializationErrorEnum::InvalidReferenceTimestampValue if the reference timestamp
     *   string is not an integer (four-byte only).
     */
    [[nodiscard]] static auto create(nlohmann::json const& metadata)
            -> ystdlib::error_handling::Result<std::unique_ptr<UnstructuredIrDeserializerImpl>>;

    // Delete copy constructor and assignment operator
    UnstructuredIrDeserializerImpl(UnstructuredIrDeserializerImpl const&) = delete;
    auto operator=(UnstructuredIrDeserializerImpl const&)
            -> UnstructuredIrDeserializerImpl& = delete;

    // Default move constructor and assignment operator
    UnstructuredIrDeserializerImpl(UnstructuredIrDeserializerImpl&&) = default;
    auto operator=(UnstructuredIrDeserializerImpl&&) -> UnstructuredIrDeserializerImpl& = default;

    // Destructor
    ~UnstructuredIrDeserializerImpl() override = default;

    // Methods implementing `DeserializerImpl`
    /**
     * The possible error codes:
     * - Forwards `deserialize_tag`'s return value on failure.
     */
    [[nodiscard]] auto get_next_ir_unit_type(ReaderInterface& reader)
            -> ystdlib::error_handling::Result<std::pair<IrUnitType, encoded_tag_t>> override;

    /**
     * The possible error codes:
     * - IrDeserializationErrorEnum::MissingRequiredSchemaNodes if required schema nodes are
     *   missing.
     * - Forwards `deserialize_encoded_text_ast`'s return value on failure.
     * - Forwards `deserialize_tag`'s return value on failure.
     * - Forwards `deserialize_timestamp`'s return value on failure.
     * - Forwards `resolve_required_node_ids`'s return value on failure.
     */
    [[nodiscard]] auto deserialize_ir_unit_kv_pair_log_event(
            ReaderInterface& reader,
            encoded_tag_t tag,
            std::shared_ptr<SchemaTree> const& auto_gen_keys_schema_tree,
            std::shared_ptr<SchemaTree> const& user_gen_keys_schema_tree,
            UtcOffset utc_offset
    ) -> ystdlib::error_handling::Result<KeyValuePairLogEvent> override;

    /**
     * Emits one synthetic schema tree node insertion IR unit.
     * NOTE: Pops from pending insertions. Does not read from the reader.
     */
    [[nodiscard]] auto deserialize_ir_unit_schema_tree_node_insertion(
            ReaderInterface& reader,
            encoded_tag_t tag,
            std::string& key_name_buffer
    ) -> ystdlib::error_handling::Result<std::pair<bool, SchemaTree::NodeLocator>> override;

private:
    // Constructors
    explicit UnstructuredIrDeserializerImpl(
            std::vector<std::pair<bool, SchemaTree::NodeLocator>> initial_insertions
    )
    requires std::is_same_v<encoded_variable_t, ir::eight_byte_encoded_variable_t>
            : m_pending_schema_insertions{std::move(initial_insertions)} {}

    UnstructuredIrDeserializerImpl(
            std::vector<std::pair<bool, SchemaTree::NodeLocator>> initial_insertions,
            ir::epoch_time_ms_t reference_timestamp
    )
    requires std::is_same_v<encoded_variable_t, ir::four_byte_encoded_variable_t>
            : m_pending_schema_insertions{std::move(initial_insertions)},
              m_previous_timestamp{reference_timestamp} {}

    // Methods
    /**
     * Resolves the node IDs for `message` and `timestamp` in the schema tree. If the node IDs are
     * already resolved, returns them immediately.
     *
     * NOTE: `message` is resolved from the user-generated schema tree, while `timestamp` is
     * resolved from the auto-generated schema tree.
     *
     * @param auto_gen_keys_schema_tree
     * @param user_gen_keys_schema_tree
     * @return A result containing a pair on success, or an error code indicating the failure:
     * - The pair:
     *   - `message` node ID
     *   - `timestamp` node ID
     * - IrDeserializationErrorEnum::MissingRequiredSchemaNodes if either required schema node is
     *   missing.
     */
    [[nodiscard]] auto resolve_required_node_ids(
            std::shared_ptr<SchemaTree> const& auto_gen_keys_schema_tree,
            std::shared_ptr<SchemaTree> const& user_gen_keys_schema_tree
    ) -> ystdlib::error_handling::Result<std::pair<SchemaTree::Node::id_t, SchemaTree::Node::id_t>>;

    // Variables
    std::vector<std::pair<bool, SchemaTree::NodeLocator>> m_pending_schema_insertions;
    std::optional<SchemaTree::Node::id_t> m_optional_message_node_id;
    std::optional<SchemaTree::Node::id_t> m_optional_timestamp_node_id;
    [[no_unique_address]] std::conditional_t<
            std::is_same_v<encoded_variable_t, ir::four_byte_encoded_variable_t>,
            ir::epoch_time_ms_t,
            EmptyType
    > m_previous_timestamp{};
};
}  // namespace clp::ffi::ir_stream

#endif  // CLP_FFI_IR_STREAM_UNSTRUCTURED_IR_DESERIALIZER_IMPL_HPP
