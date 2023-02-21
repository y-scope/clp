#ifndef STREAMING_ARCHIVE_GLT_ARCHIVE_HPP
#define STREAMING_ARCHIVE_GLT_ARCHIVE_HPP

// Project headers
#include "../Archive.hpp"
#include "../Segment.hpp"
#include "GLTFile.hpp"
#include "GLTSegment.hpp"

namespace streaming_archive::writer {
    class GLTArchive : public Archive {
    public:

        // Constructors
        GLTArchive ();

        // Methods
        void open_derived(const UserConfig& user_config) override;
        void close_derived () override;

        void create_and_open_file (const std::string& path, group_id_t group_id,
                                   const boost::uuids::uuid& orig_file_id,
                                   size_t split_ix) override;

        void write_msg (epochtime_t timestamp, const std::string& message,
                        size_t num_uncompressed_bytes) override;

        void write_msg_using_schema (compressor_frontend::Token*& uncompressed_msg,
                                     uint32_t uncompressed_msg_pos, bool has_delimiter,
                                     bool has_timestamp) override;

        void append_file_to_segment () override;

        size_t get_stable_size () const override;

        /**
         * Closes a given segment, persists the metadata of the files in the segment
         * @param segment
         * @param files
         * @param segment_logtype_ids
         * @param segment_var_ids
         * @throw Same as streaming_archive::writer::CompressedStreamOnDisk::close
         * @throw Same as streaming_archive::writer::Archive::persist_file_metadata
         */
        void close_segment_and_persist_file_metadata (Segment& message_order_table,
                                                      GLTSegment& glt_segment,
                                                      std::vector<File*>& files,
                                                      ArrayBackedPosIntSet<logtype_dictionary_id_t>& segment_logtype_ids,
                                                      ArrayBackedPosIntSet<variable_dictionary_id_t>& segment_var_ids);

    private:

        // Methods
        /**
         * Appends the message order table of the current encoded file to the given segment
         * @param segment
         * @param logtype_ids_in_segment
         * @param var_ids_in_segment
         * @param files_in_segment
         */
        void
        append_file_contents_to_segment (Segment& message_order_table, GLTSegment& glt_segment,
                                         ArrayBackedPosIntSet<logtype_dictionary_id_t>& logtype_ids_in_segment,
                                         ArrayBackedPosIntSet<variable_dictionary_id_t>& var_ids_in_segment,
                                         std::vector<File*>& files_in_segment);

        double m_combined_threshold;
        // TODO: remove this after file id is integrated
        // into the database schema
        FileWriter m_filename_dict_writer;

        GLTSegment m_glt_segment;
        GLTFile* m_glt_file;
        Segment m_message_order_table;

    };
}
#endif //STREAMING_ARCHIVE_GLT_ARCHIVE_HPP
