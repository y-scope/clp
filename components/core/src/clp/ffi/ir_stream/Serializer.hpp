#ifndef CLP_FFI_IR_STREAM_SERIALIZER_HPP
#define CLP_FFI_IR_STREAM_SERIALIZER_HPP

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <msgpack.hpp>
#include <nlohmann/json.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include "../../time_types.hpp"
#include "../SchemaTree.hpp"
#include "IrSerializationError.hpp"

namespace clp::ffi::ir_stream {
/**
 * Class for serializing log events into the kv-pair IR format.
 *
 * This class:
 * - maintains all necessary internal data structures to track serialization state;
 * - provides APIs to serialize log events into the IR format; and
 * - provides APIs to access the serialized IR bytes.
 *
 * NOTE:
 * - This class is designed only to provide serialization functionalities. Callers are responsible
 *   for writing the serialized bytes into I/O streams.
 * - This class doesn't provide an API to terminate the IR stream. Callers should
 *   terminate the stream by flushing this class' IR buffer to the I/O stream and then writing
 *   `clp::ffi::ir_stream::cProtocol::Eof` to the I/O stream.
 * @tparam encoded_variable_t Type of encoded variables in the serialized IR stream.
 */
template <typename encoded_variable_t>
class Serializer {
public:
    // Types
    using Buffer = std::vector<int8_t>;
    using BufferView = std::span<int8_t const>;

    // Factory functions
    /**
     * Creates an IR serializer and serializes the stream's preamble.
     * @param optional_user_defined_metadata Stream-level user-defined metadata, given as a JSON
     * object.
     * @return A result containing the serializer or an error code indicating the failure:
     * - IrSerializationErrorEnum::MetadataSerializationFailure if the stream's metadata couldn't
     *   be serialized.
     * - IrSerializationErrorEnum::UnsupportedUserDefinedMetadata if the given user-defined
     *   metadata is not a JSON object.
     */
    [[nodiscard]] static auto create(
            std::optional<nlohmann::json> optional_user_defined_metadata = std::nullopt
    ) -> ystdlib::error_handling::Result<Serializer<encoded_variable_t>>;

    // Disable copy constructor/assignment operator
    Serializer(Serializer const&) = delete;
    auto operator=(Serializer const&) -> Serializer& = delete;

    // Define default move constructor/assignment operator
    Serializer(Serializer&&) = default;
    auto operator=(Serializer&&) -> Serializer& = default;

    // Destructor
    ~Serializer() = default;

    // Methods
    /**
     * @return A view of the underlying IR buffer which contains the serialized IR bytes.
     */
    [[nodiscard]] auto get_ir_buf_view() const -> BufferView {
        return {m_ir_buf.data(), m_ir_buf.size()};
    }

    /**
     * Clears the underlying IR buffer.
     */
    auto clear_ir_buf() -> void { m_ir_buf.clear(); }

    /**
     * @return The current UTC offset.
     */
    [[nodiscard]] auto get_curr_utc_offset() const -> UtcOffset { return m_curr_utc_offset; }

    /**
     * Changes the UTC offset and serializes a UTC offset change packet, if the given UTC offset is
     * different than the current UTC offset.
     * @param utc_offset
     */
    auto change_utc_offset(UtcOffset utc_offset) -> void;

    /**
     * Serializes the given msgpack maps as a key-value pair log event.
     * @param auto_gen_kv_pairs_map
     * @param user_gen_kv_pairs_map
     * @return A void result on success, or an error code indicating the failure:
     * - Forwards `serialize_schema_tree_node`'s return values on failure.
     * - Forwards `serialize_msgpack_map_using_dfs`'s return values on failure.
     */
    [[nodiscard]] auto serialize_msgpack_map(
            msgpack::object_map const& auto_gen_kv_pairs_map,
            msgpack::object_map const& user_gen_kv_pairs_map
    ) -> ystdlib::error_handling::Result<void>;

private:
    // Constructors
    Serializer() = default;

    // Methods
    /**
     * Serializes a schema tree node identified by the given locator into `m_schema_tree_node_buf`.
     * @tparam is_auto_generated_node
     * @param locator
     * @return A void result on success, or an error code indicating the failure:
     * - IrSerializationErrorEnum::UnknownSchemaTreeNodeType if the node type is unsupported.
     * - IrSerializationErrorEnum::SchemaTreeNodeSerializationFailure if the key name couldn't be
     *   serialized.
     * - Forwards `encode_and_serialize_schema_tree_node_id`'s return value on failure.
     */
    template <bool is_auto_generated_node>
    [[nodiscard]] auto serialize_schema_tree_node(SchemaTree::NodeLocator const& locator)
            -> ystdlib::error_handling::Result<void>;

    UtcOffset m_curr_utc_offset{0};
    Buffer m_ir_buf;
    SchemaTree m_auto_gen_keys_schema_tree;
    SchemaTree m_user_gen_keys_schema_tree;

    std::string m_logtype_buf;
    Buffer m_schema_tree_node_buf;
    Buffer m_sequential_serialization_buf;
    Buffer m_user_gen_val_group_buf;
};
}  // namespace clp::ffi::ir_stream

#endif
