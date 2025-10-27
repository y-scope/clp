#ifndef CLP_S_LOG_CONVERTER_LOGSERIALIZER_HPP
#define CLP_S_LOG_CONVERTER_LOGSERIALIZER_HPP

#include <cstddef>
#include <string_view>
#include <utility>

#include <ystdlib/error_handling/Result.hpp>

#include "../../clp/ffi/ir_stream/protocol_constants.hpp"
#include "../../clp/ffi/ir_stream/Serializer.hpp"
#include "../../clp/ir/types.hpp"
#include "../../clp/type_utils.hpp"
#include "../FileWriter.hpp"

namespace clp_s::log_converter {
/**
 * Utility class that generates KV-IR corresponding to a converted input file.
 */
class LogSerializer {
public:
    // Factory function
    /**
     * Creates an instance of `LogSerializer`.
     * @param output_dir The destination directory for generated KV-IR.
     * @param original_file_path The original path for the file being converted to KV-IR.
     * @return A result containing a `LogSerializer` on success, or an error code indicating the
     * failure:
     * - std::errc::no_such_file_or_directory if a `clp_s::FileWriter` fails to open an output file.
     * - Forwards `clp::ffi::ir_stream::Serializer<>::create()`'s return values.
     */
    [[nodiscard]] static auto
    create(std::string_view output_dir, std::string_view original_file_path)
            -> ystdlib::error_handling::Result<LogSerializer>;

    // Constructors
    // Delete copy constructor and assignment operator
    LogSerializer(LogSerializer const&) = delete;
    [[nodiscard]] auto operator=(LogSerializer const&) -> LogSerializer& = delete;

    // Default move constructor and assignment operator
    LogSerializer(LogSerializer&&) noexcept = default;
    [[nodiscard]] auto operator=(LogSerializer&&) -> LogSerializer& = default;

    // Destructor
    ~LogSerializer() = default;

    // Methods
    /**
     * Adds a message with a timestamp to the serialized output.
     *
     * The timestamp is serialized as a string so that the original timestamp format can be
     * preserved during clp-s ingestion.
     *
     * @param timestamp
     * @param message
     * @return A void result on success, or an error code indicating the failure:
     * - std::errc::invalid_argument if `clp::ffi::ir_stream::Serializer<>::serialize_msgpack_map`
     *   returns on failure.
     */
    [[nodiscard]] auto add_message(std::string_view timestamp, std::string_view message)
            -> ystdlib::error_handling::Result<void>;

    /**
     * Adds a message without a timestamp to the serialized output.
     * @param message
     * @return A void result on success, or an error code indicating the failure:
     * - std::errc::invalid_argument if `clp::ffi::ir_stream::Serializer<>::serialize_msgpack_map`
     *   returns on failure.
     */
    [[nodiscard]] auto add_message(std::string_view message)
            -> ystdlib::error_handling::Result<void>;

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
    static constexpr size_t cMaxIrBufSize{64ULL * 1024ULL};  // 64 KiB

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
        m_writer.write(
                clp::size_checked_pointer_cast<char const>(buffer.data()),
                buffer.size_bytes()
        );
        m_serializer.clear_ir_buf();
    }

    clp::ffi::ir_stream::Serializer<clp::ir::eight_byte_encoded_variable_t> m_serializer;
    clp_s::FileWriter m_writer;
};
}  // namespace clp_s::log_converter

#endif  // CLP_S_LOG_CONVERTER_LOGSERIALIZER_HPP
