#include "Segment.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <climits>

#include <boost/filesystem.hpp>

#include "../../FileReader.hpp"
#include "../../spdlog_with_specializations.hpp"

using std::make_unique;
using std::string;
using std::to_string;
using std::unique_ptr;

namespace glt::streaming_archive::reader {
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

    // Get the size of the compressed segment file
    boost::system::error_code boost_error_code;
    size_t segment_file_size = boost::filesystem::file_size(segment_path, boost_error_code);
    if (boost_error_code) {
        SPDLOG_ERROR(
                "streaming_archive::reader::Segment: Unable to obtain file size for segment: "
                "{}",
                segment_path.c_str()
        );
        SPDLOG_ERROR("streaming_archive::reader::Segment: {}", boost_error_code.message().c_str());
        return ErrorCode_Failure;
    }

    // Sanity check: previously used memory mapped file should be closed before opening a new
    // one
    if (m_memory_mapped_segment_file.is_open()) {
        SPDLOG_WARN(
                "streaming_archive::reader::Segment: Previous segment should be closed before "
                "opening new one: {}",
                segment_path.c_str()
        );
        m_memory_mapped_segment_file.close();
    }
    // Create read only memory mapped file
    boost::iostreams::mapped_file_params memory_map_params;
    memory_map_params.path = segment_path;
    memory_map_params.flags = boost::iostreams::mapped_file::readonly;
    memory_map_params.length = segment_file_size;
    // Try to map it to the same memory location as the previous memory mapped file
    memory_map_params.hint = m_memory_mapped_segment_file.data();
    m_memory_mapped_segment_file.open(memory_map_params);
    if (!m_memory_mapped_segment_file.is_open()) {
        SPDLOG_ERROR(
                "streaming_archive::reader:Segment: Unable to memory map the compressed "
                "segment with path: {}",
                segment_path.c_str()
        );
        return ErrorCode_Failure;
    }

    m_decompressor.open(m_memory_mapped_segment_file.data(), segment_file_size);

    m_segment_path = segment_path;
    return ErrorCode_Success;
}

void Segment::close() {
    if (!m_segment_path.empty()) {
        m_decompressor.close();
        m_memory_mapped_segment_file.close();
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
}  // namespace glt::streaming_archive::reader
