#include "CLPFile.hpp"

namespace streaming_archive::writer {

    void CLPFile::open_derived () {
        m_timestamps = std::make_unique<PageAllocatedVector<encoded_variable_t>>();
        m_logtypes = std::make_unique<PageAllocatedVector<logtype_dictionary_id_t>>();
        m_variables = std::make_unique<PageAllocatedVector<encoded_variable_t>>();
    }

    void CLPFile::set_segment_metadata (segment_id_t segment_id,
                                        uint64_t segment_timestamps_uncompressed_pos,
                                        uint64_t segment_logtypes_uncompressed_pos,
                                        uint64_t segment_variables_uncompressed_pos) {
        m_segment_id = segment_id;
        m_segment_timestamps_pos = segment_timestamps_uncompressed_pos;
        m_segment_logtypes_pos = segment_logtypes_uncompressed_pos;
        m_segment_variables_pos = segment_variables_uncompressed_pos;
    }

    void CLPFile::append_to_segment (const LogTypeDictionaryWriter& logtype_dict,
                                     Segment& segment) {
        if (m_is_open) {
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }

        // Append files to segment
        uint64_t segment_timestamps_uncompressed_pos;
        segment.append(reinterpret_cast<const char*>(m_timestamps->data()),
                       m_timestamps->size_in_bytes(), segment_timestamps_uncompressed_pos);
        uint64_t segment_logtypes_uncompressed_pos;
        segment.append(reinterpret_cast<const char*>(m_logtypes->data()),
                       m_logtypes->size_in_bytes(), segment_logtypes_uncompressed_pos);
        uint64_t segment_variables_uncompressed_pos;
        segment.append(reinterpret_cast<const char*>(m_variables->data()),
                       m_variables->size_in_bytes(), segment_variables_uncompressed_pos);
        set_segment_metadata(segment.get_id(), segment_timestamps_uncompressed_pos,
                             segment_logtypes_uncompressed_pos,
                             segment_variables_uncompressed_pos);

        // clear in-memory columns
        m_timestamps.reset(nullptr);
        m_logtypes.reset(nullptr);
        m_variables.reset(nullptr);
    }

    void CLPFile::write_encoded_msg (epochtime_t timestamp, logtype_dictionary_id_t logtype_id,
                                     const std::vector<encoded_variable_t>& encoded_vars,
                                     const std::vector<variable_dictionary_id_t>& var_ids,
                                     size_t num_uncompressed_bytes) {

        m_timestamps->push_back(timestamp);
        m_logtypes->push_back(logtype_id);
        m_variables->push_back_all(encoded_vars);

        // Update metadata
        ++m_num_messages;
        m_num_variables += encoded_vars.size();

        if (timestamp < m_begin_ts) {
            m_begin_ts = timestamp;
        }
        if (timestamp > m_end_ts) {
            m_end_ts = timestamp;
        }

        m_num_uncompressed_bytes += num_uncompressed_bytes;
    }
}