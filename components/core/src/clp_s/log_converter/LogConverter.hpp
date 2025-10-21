#ifndef CLP_S_LOG_CONVERTER_LOGCONVERTER_HPP
#define CLP_S_LOG_CONVERTER_LOGCONVERTER_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>

#include <ystdlib/containers/Array.hpp>

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
     * @return Whether conversion was successful.
     */
    auto convert_file(
            clp_s::Path const& path,
            std::shared_ptr<clp::ReaderInterface>& reader,
            std::string_view output_dir
    ) -> bool;

private:
    // Constants
    static constexpr size_t cDefaultBufferSize{64ULL * 1024ULL};  // 64 KiB
    static constexpr size_t cMaxBufferSize{64ULL * 1024ULL * 1024ULL};  // 64 MiB

    // Methods
    /**
     * Refills the internal buffer by consuming bytes from a reader, growing the buffer if it is
     * already full.
     * @param reader
     * @return The number of new bytes consumed from `reader`, or `std::nullopt` on failure.
     */
    auto refill_buffer(std::shared_ptr<clp::ReaderInterface>& reader) -> std::optional<size_t>;

    ystdlib::containers::Array<char> m_buffer;
    size_t m_bytes_occupied{};
    size_t m_cur_offset{};
};
}  // namespace clp_s::log_converter
#endif  // CLP_S_LOG_CONVERTER_LOGCONVERTER_HPP
