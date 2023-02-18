#ifndef STREAMING_ARCHIVE_WRITER_GLT_FILE_HPP
#define STREAMING_ARCHIVE_WRITER_GLT_FILE_HPP

// C++ libraries
#include "../File.hpp"

namespace streaming_archive::writer {
    class GLTFile : public File {
    public:
        GLTFile (const boost::uuids::uuid& id, const boost::uuids::uuid& orig_file_id,
                 const std::string& orig_log_path, group_id_t group_id, size_t split_ix)
                : m_segment_logtypes_pos(0), m_segment_offset_pos(0), File(id, orig_file_id, orig_log_path, group_id, split_ix) {}

        void open_derived () override;

        void append_to_segment (const LogTypeDictionaryWriter& logtype_dict,
                                CompressedStreamOnDisk& segment) override;

        void write_encoded_msg (epochtime_t timestamp, logtype_dictionary_id_t logtype_id,
                                size_t offset,
                                const std::vector<variable_dictionary_id_t>& var_ids,
                                size_t num_uncompressed_bytes, size_t num_vars);

        uint64_t get_segment_logtypes_pos () const { return m_segment_logtypes_pos; }

        uint64_t get_segment_offset_pos () const { return m_segment_offset_pos; }

    private:
        // method
        /**
         * Sets segment-related metadata to the given values
         * @param segment_id
         * @param segment_timestamps_uncompressed_pos
         * @param segment_logtypes_uncompressed_pos
         * @param segment_offset_uncompressed_pos
         */
        void
        set_segment_metadata (segment_id_t segment_id, uint64_t segment_logtypes_uncompressed_pos,
                              uint64_t segment_offset_uncompressed_pos);

        // Data variables
        std::unique_ptr<PageAllocatedVector<logtype_dictionary_id_t>> m_logtypes;
        std::unique_ptr<PageAllocatedVector<size_t>> m_offset;

        // for keeping the logtype ids that has appeared
        std::set<logtype_dictionary_id_t> m_logtype_id_occurance;

        // metadata
        uint64_t m_segment_logtypes_pos;
        uint64_t m_segment_offset_pos;
    };
}

#endif //STREAMING_ARCHIVE_WRITER_GLT_FILE_HPP
