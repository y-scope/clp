#ifndef CLP_FFI_IR_STREAM_SERIALIZER_HPP
#define CLP_FFI_IR_STREAM_SERIALIZER_HPP

#include <cstdint>
#include <span>
#include <vector>

#include <boost-outcome/include/boost/outcome/std_result.hpp>

#include "../../time_types.hpp"
#include "../SchemaTree.hpp"

namespace clp::ffi::ir_stream {
/**
 * A template class for serializing log events into CLP IR format. This class maintains all the
 * necessary internal data structure to track serialization states. It also provides APIs to
 * serialize log events into IR format, as well as APIs to access the serialized IR bytes.
 * Notice that this class is designed to provide serialization functionalities only. The upper-level
 * caller should be responsible for writing the serialized bytes into IO streams properly. In
 * addition, it doesn't provide a call to terminate the IR stream. The upper-level caller should
 * decide when to terminate the stream by append `clp::ffi::ir_stream::cProtocol::Eof` at the end of
 * the stream.
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
     * Creates an IR serializer and serialize the preamble at the beginning of the stream.
     * @return A result containing the serializer or an error code indicating the failure:
     * - std::errc::protocol_error if failed to serialize the preamble.
     */
    [[nodiscard]] static auto create(
    ) -> BOOST_OUTCOME_V2_NAMESPACE::std_result<Serializer<encoded_variable_t>>;

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
     * @return A view to the underlying IR buffer which contains the serialized IR bytes.
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
     * Changes the UTC offset and serializes the UTC offset change packet into the underlying IR
     * buffer if the given UTC offset is different with the current UTC offset.
     * @param utc_offset
     */
    auto change_utc_offset(UtcOffset utc_offset) -> void;

private:
    // Constructors
    Serializer() = default;

    UtcOffset m_curr_utc_offset{0};
    Buffer m_ir_buf;
    SchemaTree m_schema_tree;

    Buffer m_schema_tree_node_buf;
    Buffer m_key_group_buf;
    Buffer m_value_group_buf;
};
}  // namespace clp::ffi::ir_stream

#endif
