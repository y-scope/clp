#ifndef GLT_STREAMING_ARCHIVE_READER_SEGMENT_HPP
#define GLT_STREAMING_ARCHIVE_READER_SEGMENT_HPP

#include <memory>
#include <string>

#include <boost/iostreams/device/mapped_file.hpp>

#include "../../Defs.h"
#include "../../ErrorCode.hpp"
#include "../../streaming_compression/passthrough/Decompressor.hpp"
#include "../../streaming_compression/zstd/Decompressor.hpp"
#include "../Constants.hpp"

namespace glt::streaming_archive::reader {
/**
 * Class for reading segments. A segment is a container for multiple compressed buffers that
 * itself may be further compressed and stored on disk.
 */
class Segment {
public:
    // Constructor
    Segment() : m_segment_path({}) {}

    // Destructor
    ~Segment();

    /**
     * Opens a segment with the given ID from the given directory
     * @param segment_dir_path
     * @param segment_id
     * @return ErrorCode_Failure if unable to memory map the segment file
     * @return ErrorCode_Success on success
     */
    ErrorCode try_open(std::string const& segment_dir_path, segment_id_t segment_id);

    /**
     * Closes the segment
     */
    void close();

    /**
     * Reads content with the given offset and length into a buffer
     * @param decompressed_stream_pos Offset of the content in the segment
     * @param extraction_buf Buffer to store the content
     * @param extraction_len Length of the buffer
     * @return ErrorCode_Truncated if decompressed_stream_pos is outside of the segment
     * @return ErrorCode_Failure if decompression failed
     * @return ErrorCode_Success on success
     */
    ErrorCode
    try_read(uint64_t decompressed_stream_pos, char* extraction_buf, uint64_t extraction_len);

private:
    std::string m_segment_path;
    boost::iostreams::mapped_file_source m_memory_mapped_segment_file;

#if USE_PASSTHROUGH_COMPRESSION
    streaming_compression::passthrough::Decompressor m_decompressor;
#elif USE_ZSTD_COMPRESSION
    streaming_compression::zstd::Decompressor m_decompressor;
#else
    static_assert(false, "Unsupported compression mode.");
#endif
};
}  // namespace glt::streaming_archive::reader

#endif  // GLT_STREAMING_ARCHIVE_READER_SEGMENT_HPP
