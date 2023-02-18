#include "GLTFile.hpp"

namespace streaming_archive::writer {
    void GLTFile::open_derived () {
        m_logtypes = std::make_unique<PageAllocatedVector<logtype_dictionary_id_t>>();
        m_offset = std::make_unique<PageAllocatedVector<size_t>>();
    }

    void GLTFile::append_to_segment (const LogTypeDictionaryWriter& logtype_dict,
                                     CompressedStreamOnDisk& segment) {
        if (m_is_open) {
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }

        // Append files to segment
        uint64_t segment_logtypes_uncompressed_pos;
        segment.append(reinterpret_cast<const char*>(m_logtypes->data()),
                       m_logtypes->size_in_bytes(), segment_logtypes_uncompressed_pos);
        uint64_t segment_offset_uncompressed_pos;
        segment.append(reinterpret_cast<const char*>(m_offset->data()), m_offset->size_in_bytes(),
                       segment_offset_uncompressed_pos);
        set_segment_metadata(segment.get_id(), segment_logtypes_uncompressed_pos,
                             segment_offset_uncompressed_pos);

        // Mark file as written out and clear in-memory columns and clear the in-memory data (except metadata)
        m_is_written_out = true;
        m_logtypes.reset(nullptr);
        m_offset.reset(nullptr);
        m_logtype_id_occurance.clear();
        m_uncompressed_file_size = 0;
    }

    void GLTFile::write_encoded_msg (epochtime_t timestamp, logtype_dictionary_id_t logtype_id,
                                     size_t vars_offset,
                                     const std::vector<variable_dictionary_id_t>& var_ids,
                                     size_t num_uncompressed_bytes, size_t num_vars) {
        m_logtypes->push_back(logtype_id);
        if (m_logtype_id_occurance.count(logtype_id) == 0) {
            m_logtype_id_occurance.insert(logtype_id);
            m_offset->push_back(vars_offset);
        } else {
            m_offset->push_back(0);
        }
        // increase the size by logtype table's data size.
        m_uncompressed_file_size +=
                sizeof(epochtime_t) + sizeof(file_id_t) + sizeof(encoded_variable_t) * num_vars ;

        // Update metadata
        ++m_num_messages;
        m_num_variables += num_vars;

        if (timestamp < m_begin_ts) {
            m_begin_ts = timestamp;
        }
        if (timestamp > m_end_ts) {
            m_end_ts = timestamp;
        }

        m_num_uncompressed_bytes += num_uncompressed_bytes;
        m_is_metadata_clean = false;
    }

    void GLTFile::set_segment_metadata (segment_id_t segment_id,
                                        uint64_t segment_logtypes_uncompressed_pos,
                                        uint64_t segment_offset_uncompressed_pos) {
        m_segment_id = segment_id;
        m_segment_logtypes_pos = segment_logtypes_uncompressed_pos;
        m_segment_offset_pos = segment_offset_uncompressed_pos;
        m_is_metadata_clean = false;
    }
}