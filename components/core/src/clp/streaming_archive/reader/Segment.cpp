#include "Segment.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <climits>

#include <boost/filesystem.hpp>
#include <fmt/format.h>

#include "../../ErrorCode.hpp"
#include "../../FileReader.hpp"
#include "../../spdlog_with_specializations.hpp"
#include "../../TraceableException.hpp"

using std::make_unique;
using std::string;
using std::to_string;
using std::unique_ptr;

namespace clp::streaming_archive::reader {
Segment::~Segment() {
    // If user forgot to explicitly close the file for some reason, close it again (doesn't
    // hurt)
    close();
}

ErrorCode Segment::try_open(string const& segment_dir_path, segment_id_t segment_id) {
    // Construct segment path
    string segment_path = segment_dir_path;
    segment_path += std::to_string(segment_id);

    if (segment_path == m_segment_path) {
        // Do nothing if segment file path is the same because it is already memory mapped
        // If we want to re-open the same file, we need to close it first
        return ErrorCode_Success;
    }

    // Sanity check: previously used memory mapped file should be closed before opening a new one
    if (m_memory_mapped_segment_file.has_value()) {
        SPDLOG_WARN(
                "streaming_archive::reader::Segment: Previous segment should be closed before "
                "opening new one: {}",
                segment_path.c_str()
        );
        m_memory_mapped_segment_file.reset();
    }

    // Create read-only memory mapped file
    try {
        m_memory_mapped_segment_file.emplace(segment_path);
    } catch (TraceableException const& ex) {
        auto const error_code{ex.get_error_code()};
        auto const formatted_error{
                ErrorCode_errno == error_code
                        ? fmt::format("errno={}", errno)
                        : fmt::format("error_code={}, message={}", error_code, ex.what())
        };
        SPDLOG_ERROR(
                "streaming_archive::reader:Segment: Unable to memory map the compressed "
                "segment with path: {}. Error: {}",
                segment_path.c_str(),
                formatted_error
        );
        return ErrorCode_Failure;
    }

    auto const view{m_memory_mapped_segment_file.value().get_view()};
    m_decompressor.open(view.data(), view.size());

    m_segment_path = segment_path;
    return ErrorCode_Success;
}

void Segment::close() {
    if (!m_segment_path.empty()) {
        m_decompressor.close();
        m_memory_mapped_segment_file.reset();
        m_segment_path.clear();
    }
}

ErrorCode
Segment::try_read(uint64_t decompressed_stream_pos, char* extraction_buf, uint64_t extraction_len) {
    // We always assume the passed in buffer is already pre-allocated, but we check anyway as a
    // precaution
    if (nullptr == extraction_buf) {
        SPDLOG_ERROR("streaming_archive::reader::Segment: Extraction buffer not allocated "
                     "during decompression");
        return ErrorCode_BadParam;
    }
    return m_decompressor.get_decompressed_stream_region(
            decompressed_stream_pos,
            extraction_buf,
            extraction_len
    );
}
}  // namespace clp::streaming_archive::reader
