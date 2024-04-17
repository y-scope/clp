#ifndef GLT_STREAMING_ARCHIVE_WRITER_SEGMENT_HPP
#define GLT_STREAMING_ARCHIVE_WRITER_SEGMENT_HPP

#include <memory>
#include <string>

#include "../../Defs.h"
#include "../../ErrorCode.hpp"
#include "../../streaming_compression/passthrough/Compressor.hpp"
#include "../../streaming_compression/zstd/Compressor.hpp"
#include "../../TraceableException.hpp"
#include "../Constants.hpp"

namespace glt::streaming_archive::writer {
/**
 * Class for writing segments. A segment is a container for multiple compressed buffers that
 * itself may be further compressed and then stored on disk.
 */
class Segment {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "streaming_archive::writer::Segment operation failed";
        }
    };

    // Constructors
    Segment() : m_id(cInvalidSegmentId), m_offset(0) {}

    // Destructor
    ~Segment();

    // Methods
    /**
     * Creates a segment in the given directory
     * @param segments_dir_path
     * @param id
     * @param compression_level
     * @throw streaming_archive::writer::Segment::OperationFailed if segment wasn't closed
     * before this call
     */
    void open(std::string const& segments_dir_path, segment_id_t id, int compression_level);
    /**
     * Closes the segment
     * @throw streaming_archive::writer::Segment::OperationFailed if compression fails
     * @throw FileWriter::OperationFailed on open, write, or close failure
     */
    void close();

    /**
     * Appends the given buffer to the segment
     * @param buf Buffer to append
     * @param buf_len
     * @param offset Offset of the buffer in the segment
     * @throw streaming_archive::writer::Segment::OperationFailed if compression fails
     */
    void append(char const* buf, uint64_t buf_len, uint64_t& offset);

    segment_id_t get_id() const { return m_id; }

    bool is_open() const;
    /**
     * @return The amount of data (in bytes) appended (input) to the segment. Calling this after
     * the segment has been closed will return the final uncompressed size of the segment.
     */
    uint64_t get_uncompressed_size();
    /**
     * @return The on-disk size (in bytes) of the segment. Calling this after the segment has
     * been closed will return the final compressed size of the segment.
     */
    size_t get_compressed_size();

private:
    // Variables
    std::string m_segment_path;
    segment_id_t m_id;

    uint64_t m_offset;  // total input bytes processed
    uint64_t m_compressed_size;

    FileWriter m_file_writer;
#if USE_PASSTHROUGH_COMPRESSION
    streaming_compression::passthrough::Compressor m_compressor;
#elif USE_ZSTD_COMPRESSION
    streaming_compression::zstd::Compressor m_compressor;
#else
    static_assert(false, "Unsupported compression mode.");
#endif
};
}  // namespace glt::streaming_archive::writer

#endif  // GLT_STREAMING_ARCHIVE_WRITER_SEGMENT_HPP
