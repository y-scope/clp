#include "SegmentManager.hpp"

using std::string;

namespace clp::streaming_archive::reader {
void SegmentManager::open(string const& segment_dir_path) {
    // Cleanup in case caller forgot to call close before calling this function
    close();
    m_segment_dir_path = segment_dir_path;
}

void SegmentManager::close() {
    for (auto& id_segment_pair : m_id_to_open_segment) {
        id_segment_pair.second.close();
    }
    m_id_to_open_segment.clear();
    m_lru_ids_of_open_segments.clear();
}

ErrorCode SegmentManager::try_read(
        segment_id_t segment_id,
        uint64_t const decompressed_stream_pos,
        char* extraction_buf,
        uint64_t const extraction_len
) {
    static size_t const cMaxLRUSegments = 2;

    // Check that segment exists or insert it if not
    if (m_id_to_open_segment.count(segment_id) == 0) {
        // Insert and open segment
        ErrorCode error_code
                = m_id_to_open_segment[segment_id].try_open(m_segment_dir_path, segment_id);
        if (ErrorCode_Success != error_code) {
            m_id_to_open_segment.erase(segment_id);
            return error_code;
        }
        m_lru_ids_of_open_segments.push_back(segment_id);

        // Evict a segment if necessary
        if (m_lru_ids_of_open_segments.size() >= cMaxLRUSegments) {
            auto id_of_segment_to_evict = m_lru_ids_of_open_segments.front();
            m_lru_ids_of_open_segments.pop_front();
            m_id_to_open_segment.at(id_of_segment_to_evict).close();
            m_id_to_open_segment.erase(id_of_segment_to_evict);
        }
    }

    // Extract data from compressed segment
    auto& segment = m_id_to_open_segment.at(segment_id);
    return segment.try_read(decompressed_stream_pos, extraction_buf, extraction_len);
}
}  // namespace clp::streaming_archive::reader
