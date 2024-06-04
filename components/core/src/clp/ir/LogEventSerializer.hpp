#ifndef CLP_IR_LOGEVENTDESERIALIZER_HPP
#define CLP_IR_LOGEVENTDESERIALIZER_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "../ErrorCode.hpp"
#include "../FileWriter.hpp"
#include "../streaming_compression/zstd/Compressor.hpp"
#include "../TraceableException.hpp"
#include "../type_utils.hpp"
#include "types.hpp"

namespace clp::ir {
/**
 * Class for serializing log events into a Zstandard compressed IR stream. The serializer first
 * buffers the serialized data into an internal buffer, and only flushes the buffered ir into the
 * on-disk file when `flush` or `close` is called.
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
        [[nodiscard]] auto what() const noexcept -> char const* override {
            return "ir::LogEventSerializer operation failed";
        }
    };

    // Constructors
    explicit LogEventSerializer() = default;

    // Delete copy constructor and assignment
    LogEventSerializer(LogEventSerializer const&) = delete;
    auto operator=(LogEventSerializer const&) -> LogEventSerializer& = delete;

    // Define default move constructor and assignment
    LogEventSerializer(LogEventSerializer&&) = default;
    auto operator=(LogEventSerializer&&) -> LogEventSerializer& = default;

    ~LogEventSerializer();

    /**
     * Creates a Zstandard compressed IR on the disk, and writes the preamble to the IR.
     * @param file_path
     */
    [[nodiscard]] auto open(std::string const& file_path) -> ErrorCode;

    /**
     * Flushes the buffered serialized data.
     * @throw FileWriter::OperationFailed on failure
     */
    auto flush() -> void;

    /**
     * Writes the EoF tag to the end of IR, flushes the data and closes the serializer
     * @throw FileWriter::OperationFailed on failure
     */
    auto close() -> void;

    [[nodiscard]] auto get_serialized_size() const -> size_t {
        return m_ir_buffer.size() + m_serialized_size;
    }

    /**
     * @return Number of serialized log events.
     */
    [[nodiscard]] auto get_num_log_events() const -> size_t { return m_num_log_events; }

    /**
     * Serializes a log event and writes it to the end of the internal buffer
     * @return Whether the log event is successfully serialized.
     */
    [[nodiscard]] auto
    serialize_log_event(std::string_view message, epoch_time_ms_t timestamp) -> ErrorCode;

private:
    // Method
    auto close_writer() -> void;

    // Constant
    // Note: In the current implementation, an encoded file could have multiple timestamp patterns
    // but IR metadata only supports a single pattern. CLP doesn't track files' time zone info
    // either. For now, the serializer uses a set of default values. The consumer of the IR should
    // decide what time pattern and time zone to use.
    static constexpr std::string_view cTimestampPattern{"%Y-%m-%d %H:%M:%S,%3"};
    static constexpr std::string_view cTimestampPatternSyntax{};
    static constexpr std::string_view cTimezoneID = {"UTC"};

    // Variables
    size_t m_num_log_events{0};
    size_t m_serialized_size{0};
    bool m_is_open{false};
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
