#ifndef CLP_IR_LOGEVENTDESERIALIZER_HPP
#define CLP_IR_LOGEVENTDESERIALIZER_HPP

#include "../ErrorCode.hpp"
#include "../streaming_compression/zstd/Compressor.hpp"
#include "../TimestampPattern.hpp"
#include "../TraceableException.hpp"
#include "../type_utils.hpp"
#include "../WriterInterface.hpp"
#include "LogEvent.hpp"
#include "types.hpp"

namespace clp::ir {
/**
 * Class for Serializing log events into a Zstandard compressed IR stream. The serializer first
 * buffers the serialized data into an internal buffer, and flushes the buffer on demand.
 */
template <typename encoded_variable_t>
class LogEventSerializer {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        [[nodiscard]] char const* what() const noexcept override {
            return "ir::LogEventSerializer operation failed";
        }
    };

    // Constructors
    explicit LogEventSerializer() : m_log_event_ix{0}, m_serialized_size{0}, m_is_open{false} {}

    // Delete copy constructor and assignment
    LogEventSerializer(LogEventSerializer const&) = delete;
    auto operator=(LogEventSerializer const&) -> LogEventSerializer& = delete;

    // Define default move constructor and assignment
    LogEventSerializer(LogEventSerializer&&) = default;
    auto operator=(LogEventSerializer&&) -> LogEventSerializer& = default;

    ~LogEventSerializer();

    /**
     * Creates a Zstandard compressed eight bytes encoded IR stream and writes the preamble into it.
     * @param file_path
     */
    auto open(std::string const& file_path) -> ErrorCode;

    /**
     * Creates a Zstandard compressed four bytes encoded IR stream and writes the preamble into it.
     * @param file_path
     * @param epoch_time_ms_t
     * @return
     */
    auto open(std::string const& file_path, epoch_time_ms_t ref_timestamp) -> ErrorCode;

    /**
     * Flushes the serialized data in the internal buffer
     */
    auto flush() -> void;

    /**
     * Flushes the serialized data and writes the EoF tag to the IR
     */
    auto close() -> void;

    [[nodiscard]] auto get_serialized_size() const -> size_t {
        return m_ir_buffer.size() + m_serialized_size;
    }

    [[nodiscard]] auto get_log_event_ix() const -> size_t { return m_log_event_ix; }

    /**
     * Serializes a log event and store it into the internal buffer
     * @return True if the log event is serialized successfully, Otherwise false
     */
    [[nodiscard]] auto
    serialize_log_event(std::string_view message, epoch_time_ms_t timestamp) -> ErrorCode;

private:
    /**
     * Initializes the internal states
     */
    auto init_states() -> void;

    // Constant
    static constexpr std::string_view TIMESTAMP_PATTERN = "%Y-%m-%d %H:%M:%S,%3";
    static constexpr std::string_view TIMESTAMP_PATTERN_SYNTAX = "";
    static constexpr std::string_view TIME_ZONE_ID = "";

    // Variables
    size_t m_log_event_ix;
    size_t m_serialized_size;
    bool m_is_open;
    [[no_unique_address]] std::conditional_t<
            std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>,
            epoch_time_ms_t,
            EmptyType> m_prev_msg_timestamp{};
    std::vector<int8_t> m_ir_buffer;
    FileWriter m_writer;
    streaming_compression::zstd::Compressor m_zstd_compressor;
};
}  // namespace clp::ir

#endif  // CLP_IR_LOGEVENTDESERIALIZER_HPP
