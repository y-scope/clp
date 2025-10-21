#ifndef CLP_S_LOG_CONVERTER_LOGSERIALIZER_HPP
#define CLP_S_LOG_CONVERTER_LOGSERIALIZER_HPP

#include <cstddef>
#include <optional>
#include <string_view>

#include "../../clp/ffi/ir_stream/protocol_constants.hpp"
#include "../../clp/ffi/ir_stream/Serializer.hpp"
#include "../../clp/ir/types.hpp"
#include "../FileWriter.hpp"

namespace clp_s::log_converter {
/**
 * Utility class that generates KV-IR corresponding to a converted input file.
 */
class LogSerializer {
public:
    // Constructors
    // Disable copy constructor/assignment operator
    LogSerializer(LogSerializer const&) = delete;
    auto operator=(LogSerializer const&) -> LogSerializer& = delete;

    // Define default move constructor/assignment operator
    LogSerializer(LogSerializer&&) = default;
    auto operator=(LogSerializer&&) -> LogSerializer& = default;

    // Destructor
    ~LogSerializer() = default;

    // Methods
    /**
     * Creates an instance of LogSerializer.
     * @param output_dir The destination directory for generated KV-IR.
     * @param original_file_path The original path for the file being converted to KV-IR.
     * @return An instance of LogSerializer on success, or std::nullopt on failure.
     */
    static auto create(std::string_view output_dir, std::string_view original_file_path)
            -> std::optional<LogSerializer>;

    /**
     * Adds a message with a timestamp to the serialized output.
     * @param timestamp
     * @param message
     * @return Whether adding the message was successful.
     */
    auto add_message(std::string_view timestamp, std::string_view message) -> bool;

    /**
     * Adds a message without a timestamp to the serialized output.
     * @param message
     * @return Whether adding the message was successful.
     */
    auto add_message(std::string_view message) -> bool;

    /**
     * Closes and flushes the serialized output.
     */
    void close() {
        flush_buffer();
        m_writer.write_numeric_value(clp::ffi::ir_stream::cProtocol::Eof);
        m_writer.close();
    }

private:
    // Constants
    static constexpr std::string_view cOriginalFileMetadataKey{"original_file"};
    static constexpr std::string_view cTimestampKey{"timestamp"};
    static constexpr std::string_view cMessageKey{"message"};
    static constexpr size_t cMaxIrBufSize{64 * 1024};  // 64 KiB

    // Constructors
    explicit LogSerializer(
            clp::ffi::ir_stream::Serializer<clp::ir::eight_byte_encoded_variable_t>&& serializer,
            clp_s::FileWriter&& writer
    )
            : m_serializer{std::move(serializer)},
              m_writer{std::move(writer)} {}

    // Methods
    /**
     * Flushes the buffer from the serializer to the output file.
     */
    void flush_buffer() {
        auto const buffer{m_serializer.get_ir_buf_view()};
        m_writer.write(reinterpret_cast<char const*>(buffer.data()), buffer.size_bytes());
        m_serializer.clear_ir_buf();
    }

    clp::ffi::ir_stream::Serializer<clp::ir::eight_byte_encoded_variable_t> m_serializer;
    clp_s::FileWriter m_writer;
};
}  // namespace clp_s::log_converter

#endif  // CLP_S_LOG_CONVERTER_LOGSERIALIZER_HPP
