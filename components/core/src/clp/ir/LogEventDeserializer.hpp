#ifndef CLP_IR_LOGEVENTDESERIALIZER_HPP
#define CLP_IR_LOGEVENTDESERIALIZER_HPP

#include <optional>

#include <ystdlib/error_handling/Result.hpp>

#include "../ffi/ir_stream/IrErrorCode.hpp"
#include "../ReaderInterface.hpp"
#include "../time_types.hpp"
#include "../TimestampPattern.hpp"
#include "../TraceableException.hpp"
#include "../type_utils.hpp"
#include "LogEvent.hpp"
#include "types.hpp"

namespace clp::ir {
/**
 * Class for deserializing IR log events from an IR stream.
 * @tparam encoded_variable_t Type of encoded variables in the stream
 */
template <typename encoded_variable_t>
class LogEventDeserializer {
public:
    // Factory functions
    /**
     * Creates a log event deserializer for the given stream
     * @param reader A reader for the IR stream
     * @return A result containing the deserializer or an error code indicating the failure:
     * - IrErrorCodeEnum::IncompleteStream if the IR stream is truncated
     * - IrErrorCodeEnum::CorruptedIR if the IR stream is corrupted
     * - IrErrorCodeEnum::UnsupportedFormat if the IR stream contains an unsupported metadata
     *   format or uses an unsupported version
     */
    static auto create(ReaderInterface& reader) -> ystdlib::error_handling::
            Result<LogEventDeserializer<encoded_variable_t>, ffi::ir_stream::IrErrorCode>;

    // Delete copy constructor and assignment
    LogEventDeserializer(LogEventDeserializer const&) = delete;
    auto operator=(LogEventDeserializer const&) -> LogEventDeserializer& = delete;

    // Define default move constructor and assignment
    LogEventDeserializer(LogEventDeserializer&&) = default;
    auto operator=(LogEventDeserializer&&) -> LogEventDeserializer& = default;

    ~LogEventDeserializer() = default;

    // Methods
    [[nodiscard]] auto get_timestamp_pattern() const -> TimestampPattern const& {
        return m_timestamp_pattern;
    }

    [[nodiscard]] auto get_current_utc_offset() const -> UtcOffset { return m_utc_offset; }

    /**
     * Deserializes a log event from the stream
     * @return A result containing the log event or an error code indicating the failure:
     * - IrErrorCodeEnum::EndOfStream on reaching the end of the IR stream
     * - IrErrorCodeEnum::IncompleteStream if the IR stream is truncated
     * - IrErrorCodeEnum::CorruptedIR if the IR stream is corrupted
     */
    [[nodiscard]] auto deserialize_log_event() -> ystdlib::error_handling::
            Result<LogEvent<encoded_variable_t>, ffi::ir_stream::IrErrorCode>;

private:
    // Constructors
    explicit LogEventDeserializer(ReaderInterface& reader) : m_reader{reader} {}

    LogEventDeserializer(ReaderInterface& reader, epoch_time_ms_t ref_timestamp)
            : m_reader{reader},
              m_prev_msg_timestamp{ref_timestamp} {}

    // Variables
    TimestampPattern m_timestamp_pattern{0, "%Y-%m-%dT%H:%M:%S.%3"};
    UtcOffset m_utc_offset{0};
    [[no_unique_address]] std::conditional_t<
            std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>,
            epoch_time_ms_t,
            EmptyType
    > m_prev_msg_timestamp{};
    ReaderInterface& m_reader;
};
}  // namespace clp::ir

#endif  // CLP_IR_LOGEVENTDESERIALIZER_HPP
