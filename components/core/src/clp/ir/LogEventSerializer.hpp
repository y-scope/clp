#ifndef CLP_IR_LOGEVENTSERIALIZER_HPP
#define CLP_IR_LOGEVENTSERIALIZER_HPP

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
 * Class for serializing log events into a Zstandard-compressed IR stream. The serializer first
 * buffers the serialized data into an internal buffer, and only flushes the buffered IR to disk
 * when `flush` or `close` is called.
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
            return "clp::ir::LogEventSerializer operation failed";
        }
    };

    // Constructors
    LogEventSerializer() = default;

    // Delete copy constructor and assignment
    LogEventSerializer(LogEventSerializer const&) = delete;
    auto operator=(LogEventSerializer const&) -> LogEventSerializer& = delete;

    // Define default move constructor and assignment
    LogEventSerializer(LogEventSerializer&&) = default;
    auto operator=(LogEventSerializer&&) -> LogEventSerializer& = default;

    ~LogEventSerializer();

    /**
     * Creates a Zstandard-compressed IR file on disk, and writes the IR file's preamble.
     * @param file_path
     * @return true on success, false if serializing the preamble fails
     * @throw FileWriter::OperationFailed if the FileWriter fails to open the file specified by
     * file_path
     * @throw streaming_compression::zstd::Compressor if the Zstandard compressor couldn't be opened
     * @throw ir::LogEventSerializer::OperationFailed if an IR file is already open
     */
    [[nodiscard]] auto open(std::string const& file_path) -> bool;

    /**
     * Flushes any buffered data.
     * @throw ir::LogEventSerializer::OperationFailed if no IR file is open
     */
    auto flush() -> void;

    /**
     * Serializes the EoF tag, flushes the buffer, and closes the current IR stream.
     * @throw ir::LogEventSerializer::OperationFailed if no IR file is open
     */
    auto close() -> void;

    /**
     * @return Size of serialized data in bytes
     */
    [[nodiscard]] auto get_serialized_size() const -> size_t { return m_serialized_size; }

    /**
     * @return Number of serialized log events.
     */
    [[nodiscard]] auto get_num_log_events() const -> size_t { return m_num_log_events; }

    /**
     * Serializes the given log event.
     * @return Whether the log event was successfully serialized.
     */
    [[nodiscard]] auto serialize_log_event(epoch_time_ms_t timestamp, std::string_view message)
            -> bool;

private:
    // Constants
    // NOTE: IR files currently store the log's timestamp pattern and timezone ID. However:
    // - files in CLP archives don't currently support encoding time zones;
    // - IR files don't support multiple timestamp patterns whereas files in CLP archives do;
    // - No consumers of IR files currently use these fields.
    // Accordingly, we store a default timestamp pattern and timezone ID in the IR file's metadata,
    // but it is really up to consumers of the IR file to use an appropriate timestamp pattern and
    // timezone when rendering log events.
    static constexpr std::string_view cTimestampPattern{"%Y-%m-%d %H:%M:%S,%3"};
    static constexpr std::string_view cTimestampPatternSyntax{};
    static constexpr std::string_view cTimezoneID{"UTC"};

    // Methods
    /**
     * Closes the member compressor and file writer in the proper order.
     */
    auto close_writer() -> void;

    // Variables
    size_t m_num_log_events{0};
    size_t m_serialized_size{0};  // Bytes

    [[no_unique_address]] std::conditional_t<
            std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>,
            epoch_time_ms_t,
            EmptyType> m_prev_event_timestamp{};

    std::vector<int8_t> m_ir_buf;
    FileWriter m_writer;
    streaming_compression::zstd::Compressor m_zstd_compressor;

    bool m_is_open{false};
};
}  // namespace clp::ir

#endif  // CLP_IR_LOGEVENTSERIALIZER_HPP
