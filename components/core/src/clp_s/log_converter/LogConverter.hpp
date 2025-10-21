#ifndef CLP_S_LOG_CONVERTER_LOGCONVERTER_HPP
#define CLP_S_LOG_CONVERTER_LOGCONVERTER_HPP

#include <cstddef>
#include <memory>
#include <optional>
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
     * @param reader A reader open to the start of the unstructured text input.
     * @param output_dir The output directory for generated KV-IR files.
     * @return A void result on success, or an error code indicating the failure:
     * - `std::errc::no_message` on any error from log-surgeon's parsing.
     * - Error codes forwarded from `LogSerializer::create()` on serializer creation failure.
     * - Error codes forwarded from `refill_buffer()` on buffer refill failure.
     * - Error codes forwarded from `LogSerializer::add_message()` on message serialization failure.
     */
    auto convert_file(
            clp_s::Path const& path,
            std::shared_ptr<clp::ReaderInterface>& reader,
            std::string_view output_dir
    ) -> ystdlib::error_handling::Result<void>;

private:
    // Constants
    static constexpr size_t cDefaultBufferSize{64ULL * 1024ULL};  // 64 KiB
    static constexpr size_t cMaxBufferSize{64ULL * 1024ULL * 1024ULL};  // 64 MiB

    // Methods
    /**
     * Refills the internal buffer by consuming bytes from a reader, growing the buffer if it is
     * already full.
     * @param reader
     * @return A result containing the number of new bytes consumed from `reader`, or
     * `std::errc::not_enough_memory` on faiulre.
     */
    auto refill_buffer(std::shared_ptr<clp::ReaderInterface>& reader)
            -> ystdlib::error_handling::Result<size_t>;

    /**
     * Compacts unconsumed content to the start of the buffer.
     */
    void compact_buffer();

    /**
     * Grows the buffer if it is full.
     * @return A void result on success, or `std::errc::result_out_of_range` on failure.
     */
    [[nodiscard]] auto grow_buffer_if_full() -> ystdlib::error_handling::Result<void>;

    ystdlib::containers::Array<char> m_buffer;
    size_t m_bytes_occupied{};
    size_t m_cur_offset{};
};
}  // namespace clp_s::log_converter
#endif  // CLP_S_LOG_CONVERTER_LOGCONVERTER_HPP
