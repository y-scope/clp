#ifndef STREAMING_ARCHIVE_WRITER_CLP_FILE_HPP
#define STREAMING_ARCHIVE_WRITER_CLP_FILE_HPP

#include "../File.hpp"

namespace streaming_archive::writer {
    class CLPFile : public File {
    public:
        CLPFile (const boost::uuids::uuid& id, const boost::uuids::uuid& orig_file_id,
                 const std::string& orig_log_path, group_id_t group_id, size_t split_ix) :
                    File(id, orig_file_id, orig_log_path, group_id, split_ix),
                    m_segment_timestamps_pos(0),
                    m_segment_logtypes_pos(0),
                    m_segment_variables_pos(0)
        {}

        void open_derived () override;

        void append_to_segment (const LogTypeDictionaryWriter& logtype_dict,
                                CompressedStreamOnDisk& segment) override;

        /**
         * Writes an encoded message to the respective columns and updates the metadata of the file
         * @param timestamp
         * @param logtype_id
         * @param encoded_vars
         * @param var_ids
         * @param num_uncompressed_bytes
         */
        void write_encoded_msg (epochtime_t timestamp, logtype_dictionary_id_t logtype_id,
                                const std::vector<encoded_variable_t>& encoded_vars,
                                const std::vector<variable_dictionary_id_t>& var_ids,
                                size_t num_uncompressed_bytes);

        uint64_t get_segment_timestamps_pos () const { return m_segment_timestamps_pos; }
        uint64_t get_segment_logtypes_pos () const { return m_segment_logtypes_pos; }
        uint64_t get_segment_variables_pos () const { return m_segment_variables_pos; }

    private:
        // method
        /**
         * Sets segment-related metadata to the given values
         * @param segment_id
         * @param segment_timestamps_uncompressed_pos
         * @param segment_logtypes_uncompressed_pos
         * @param segment_offset_uncompressed_pos
         */
        void set_segment_metadata (segment_id_t segment_id,
                                   uint64_t segment_timestamps_uncompressed_pos,
                                   uint64_t segment_logtypes_uncompressed_pos,
                                   uint64_t segment_variables_uncompressed_pos);

        // Data variables
        std::unique_ptr<PageAllocatedVector<epochtime_t>> m_timestamps;
        std::unique_ptr<PageAllocatedVector<logtype_dictionary_id_t>> m_logtypes;
        std::unique_ptr<PageAllocatedVector<encoded_variable_t>> m_variables;

        // metadata
        uint64_t m_segment_timestamps_pos;
        uint64_t m_segment_logtypes_pos;
        uint64_t m_segment_variables_pos;
    };
}

#endif //CLPFILE_HPP
