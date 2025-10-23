#ifndef CLP_S_LOG_CONVERTER_LOGCONVERTER_HPP
#define CLP_S_LOG_CONVERTER_LOGCONVERTER_HPP

#include <cstddef>
#include <string_view>

#include <ystdlib/containers/Array.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include "../../clp/ReaderInterface.hpp"
#include "../InputConfig.hpp"

namespace clp_s::log_converter {
/**
 * Utility class that converts unstructured text logs into KV-IR streams.
 */
class LogConverter {
public:
    // Constructors
    LogConverter() : m_buffer(cDefaultBufferSize) {}

    // Methods
    /**
     * Converts a file into KV-IR and outputs the generated file to a given directory.
     * @param path The input path for the unstructured text file.
     * @param reader A reader positioned at the start of the input stream.
     * @param output_dir The output directory for generated KV-IR files.
     * @return A void result on success, or an error code indicating the failure:
     * - std::errc::no_message if `log_surgeon::BufferParser::parse_next_event` returns an error.
     * - Forwards `LogSerializer::create()`'s return values.
     * - Forwards `refill_buffer()`'s return values.
     * - Forwards `LogSerializer::add_message()`'s return values.
     */
    [[nodiscard]] auto
    convert_file(clp_s::Path const& path, clp::ReaderInterface* reader, std::string_view output_dir)
            -> ystdlib::error_handling::Result<void>;

private:
    // Constants
    static constexpr size_t cDefaultBufferSize{64ULL * 1024ULL};  // 64 KiB
    static constexpr size_t cMaxBufferSize{64ULL * 1024ULL * 1024ULL};  // 64 MiB

    // Methods
    /**
     * Refills the internal buffer by consuming bytes from a reader, growing the buffer if it is
     * already full.
     * @param reader
     * @return A result containing the number of new bytes consumed from `reader`, or an error code
     * indicating the failure:
     * - std::errc::not_enough_memory if `clp::ReaderInterface::try_read()` returns an error.
     * - Forwards `grow_buffer_if_full()`'s return values.
     */
    [[nodiscard]] auto refill_buffer(clp::ReaderInterface* reader)
            -> ystdlib::error_handling::Result<size_t>;

    /**
     * Compacts unconsumed content to the start of the buffer.
     */
    void compact_buffer();

    /**
     * Grows the buffer if it is full.
     * @return A void result on success, or an error code indicating the failure:
     * - std::errc::result_out_of_range if the grown buffer size exceeds the maximum allowed size.
     */
    [[nodiscard]] auto grow_buffer_if_full() -> ystdlib::error_handling::Result<void>;

    ystdlib::containers::Array<char> m_buffer;
    size_t m_num_bytes_buffered{};
    size_t m_parser_offset{};
};
}  // namespace clp_s::log_converter
#endif  // CLP_S_LOG_CONVERTER_LOGCONVERTER_HPP
