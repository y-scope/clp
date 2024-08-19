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
 * A deserializer for log events from a CLP kv-pair IR stream.
 *
 * This class:
 * - maintains all the necessary internal data structure to track deserialization state;
 * - provide APIs to deserialize log events from an IR stream.
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
     * Deserializes the stream from the given reader up to and including the next log event.
     * @param reader
     * @return A result containing the deserialized log event or an error code indicating the
     * failure:
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
