#ifndef CLP_STREAMING_ARCHIVE_READER_SEGMENTMANAGER_HPP
#define CLP_STREAMING_ARCHIVE_READER_SEGMENTMANAGER_HPP

#include <cstddef>
#include <list>
#include <string>
#include <unordered_map>

#include "../../Defs.h"
#include "Segment.hpp"

namespace clp::streaming_archive::reader {
/**
 * This class handles segments in a given directory. This primarily consists of reading from
 * segments in a given directory.
 */
class SegmentManager {
public:
    // Methods
    /**
     * Opens the segment manager
     * @param segment_dir_path
     */
    void open(std::string const& segment_dir_path);

    /**
     * Closes the segment manager
     */
    void close();

    /**
     * Tries to read content with the given offset and length from a segment with the given ID
     * into a buffer
     * @param segment_id
     * @param decompressed_stream_pos
     * @param extraction_buf
     * @param extraction_len
     * @return Same as streaming_archive::reader::Segment::try_open
     * @return Same as streaming_archive::reader::Segment::try_read
     * @throw std::out_of_range if a segment ID cannot be found unexpectedly
     */
    ErrorCode try_read(
            segment_id_t segment_id,
            uint64_t const decompressed_stream_pos,
            char* extraction_buf,
            uint64_t const extraction_len
    );

private:
    std::string m_segment_dir_path;

    std::unordered_map<segment_id_t, Segment> m_id_to_open_segment;
    // List of open segment IDs in LRU order (LRU segment ID at front)
    std::list<segment_id_t> m_lru_ids_of_open_segments;
};
}  // namespace clp::streaming_archive::reader

#endif  // CLP_STREAMING_ARCHIVE_READER_SEGMENTMANAGER_HPP
