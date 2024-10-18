#ifndef CLP_FFI_IR_STREAM_SERIALIZER_HPP
#define CLP_FFI_IR_STREAM_SERIALIZER_HPP

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include <msgpack.hpp>
#include <outcome/single-header/outcome.hpp>

#include "../../time_types.hpp"
#include "../SchemaTree.hpp"

namespace clp::ffi::ir_stream {
/**
 * A work-in-progress class for serializing log events into the kv-pair IR format.
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
     * @return A result containing the serializer or an error code indicating the failure:
     * - std::errc::protocol_error on failure to serialize the preamble.
     */
    [[nodiscard]] static auto create(
    ) -> OUTCOME_V2_NAMESPACE::std_result<Serializer<encoded_variable_t>>;

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
     * Serializes the given msgpack map as a key-value pair log event.
     * @param msgpack_map
     * @return Whether serialization succeeded.
     */
    [[nodiscard]] auto serialize_msgpack_map(msgpack::object_map const& msgpack_map) -> bool;

private:
    // Constructors
    Serializer() = default;

    // Methods
    /**
     * Serializes a schema tree node identified by the given locator into `m_schema_tree_node_buf`.
     * @param locator
     * @return Whether serialization succeeded.
     */
    [[nodiscard]] auto serialize_schema_tree_node(SchemaTree::NodeLocator const& locator) -> bool;

    /**
     * Serializes the given key ID into `m_key_group_buf`.
     * @param id
     * @return Forwards `encode_and_serialize_schema_tree_node_id`'s return values.
     */
    [[nodiscard]] auto serialize_key(SchemaTree::Node::id_t id) -> bool;

    /**
     * Serializes the given MessagePack value into `m_value_group_buf`.
     * @param val
     * @param schema_tree_node_type The type of the schema tree node that corresponds to `val`.
     * @return Whether serialization succeeded.
     */
    [[nodiscard]] auto
    serialize_val(msgpack::object const& val, SchemaTree::Node::Type schema_tree_node_type) -> bool;

    UtcOffset m_curr_utc_offset{0};
    Buffer m_ir_buf;
    SchemaTree m_schema_tree;

    std::string m_logtype_buf;
    Buffer m_schema_tree_node_buf;
    Buffer m_key_group_buf;
    Buffer m_value_group_buf;
};
}  // namespace clp::ffi::ir_stream

#endif
