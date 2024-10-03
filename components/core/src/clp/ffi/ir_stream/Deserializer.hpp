#ifndef CLP_FFI_IR_STREAM_DESERIALIZER_HPP
#define CLP_FFI_IR_STREAM_DESERIALIZER_HPP

#include <memory>

#include <outcome/single-header/outcome.hpp>

#include "../../ReaderInterface.hpp"
#include "../../time_types.hpp"
#include "../KeyValuePairLogEvent.hpp"
#include "../SchemaTree.hpp"

namespace clp::ffi::ir_stream {
/**
 * A deserializer for log events from a CLP kv-pair IR stream. The class ensures any internal state
 * remains consistent even when a deserialization failure occurs (i.e., it's transactional).
 *
 * NOTE: This class is designed only to provide deserialization functionalities. Callers are
 * responsible for maintaining a `ReaderInterface` to input IR bytes from an I/O stream.
 */
class Deserializer {
public:
    // Factory function
    /**
     * Creates a deserializer by reading the stream's preamble from the given reader.
     * @param reader
     * @return A result containing the deserializer or an error code indicating the failure:
     * - std::errc::result_out_of_range if the IR stream is truncated
     * - std::errc::protocol_error if the IR stream is corrupted
     * - std::errc::protocol_not_supported if the IR stream contains an unsupported metadata format
     *   or uses an unsupported version
     */
    [[nodiscard]] static auto create(ReaderInterface& reader
    ) -> OUTCOME_V2_NAMESPACE::std_result<Deserializer>;

    // Delete copy constructor and assignment
    Deserializer(Deserializer const&) = delete;
    auto operator=(Deserializer const&) -> Deserializer& = delete;

    // Define default move constructor and assignment
    Deserializer(Deserializer&&) = default;
    auto operator=(Deserializer&&) -> Deserializer& = default;

    // Destructor
    ~Deserializer() = default;

    // Methods
    /**
     * Deserializes the stream from the given reader up to and including the next log event IR unit.
     * @param reader
     * @return A result containing the deserialized log event or an error code indicating the
     * failure:
     * - std::errc::no_message_available if the IR stream has been fully consumed.
     * - std::errc::result_out_of_range if the IR stream is truncated.
     * - std::errc::protocol_error if the IR stream is corrupted.
     * - std::errc::protocol_not_supported if the IR stream contains an unsupported metadata format
     *   or uses an unsupported version.
     * - Forwards `KeyValuePairLogEvent::create`'s return values if the intermediate deserialized
     *   result cannot construct a valid key-value pair log event.
     */
    [[nodiscard]] auto deserialize_to_next_log_event(ReaderInterface& reader
    ) -> OUTCOME_V2_NAMESPACE::std_result<KeyValuePairLogEvent>;

private:
    // Constructor
    Deserializer() = default;

    // Variables
    std::shared_ptr<SchemaTree> m_schema_tree{std::make_shared<SchemaTree>()};
    UtcOffset m_utc_offset{0};
};
}  // namespace clp::ffi::ir_stream

#endif  // CLP_FFI_IR_STREAM_DESERIALIZER_HPP
