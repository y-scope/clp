#ifndef STREAMING_ARCHIVE_WRITER_INMEMORYFILE_HPP
#define STREAMING_ARCHIVE_WRITER_INMEMORYFILE_HPP

// Project headers
#include "../../PageAllocatedVector.hpp"
#include "File.hpp"

namespace streaming_archive { namespace writer {
    /**
     * InMemoryFile is a streaming_archive::writer::File which keeps all columns in memory until they are appended to a segment
     */
    class InMemoryFile : public File {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

            // Methods
            const char* what () const noexcept override {
                return "streaming_archive::writer::InMemoryFile operation failed";
            }
        };

        // Constructor
        /**
         * Constructs a file
         */
        InMemoryFile (const boost::uuids::uuid& id, const boost::uuids::uuid& orig_file_id, const std::string& orig_log_path,
                      const std::string& archive_log_path, group_id_t group_id, size_t split_ix);

    private:
        friend class Archive;

        // Methods that implement the File interface
        /**
         * Opens the file
         * @throw streaming_archive::writer::InMemoryFile::OperationFailed if file is already in segment
         */
        void open () override;
        /**
         * Closes the file
         */
        void close () override;
        /**
         * Tests if file is open
         * @return true if open, false otherwise
         */
        bool is_open () const override;

        /**
         * Writes an encoded message to the respective columns and updates the metadata of the file
         * @param timestamp
         * @param logtype_id
         * @param encoded_vars
         * @param num_uncompressed_bytes
         */
        void write_encoded_msg (epochtime_t timestamp, logtype_dictionary_id_t logtype_id, const std::vector<encoded_variable_t>& encoded_vars,
                size_t num_uncompressed_bytes) override;

        /**
         * Appends file's columns to the given segment
         * @param logtype_dict
         * @param segment
         * @param segment_logtype_ids
         * @param segment_var_ids
         * @throw streaming_archive::writer::InMemoryFile::OperationFailed if file is still open or any column fails to be appended
         */
        void append_to_segment (const LogTypeDictionaryWriter& logtype_dict, Segment& segment, std::unordered_set<logtype_dictionary_id_t>& segment_logtype_ids,
                                std::unordered_set<variable_dictionary_id_t>& segment_var_ids) override;
        /**
         * Cleans up any data after inserting the file into the segment
         */
        void cleanup_after_segment_insertion () override;

        // Methods specific to InMemoryFile
        /**
         * Writes the in-memory file to disk
         * @throw FileWriter::OperationFailed on open, write, or close failure
         */
        void write_to_disk ();

        // Variables
        std::string m_archive_log_path;

        PageAllocatedVector<epochtime_t> m_timestamps;
        PageAllocatedVector<logtype_dictionary_id_t> m_logtypes;
        PageAllocatedVector<encoded_variable_t> m_variables;

        bool m_is_written_out;

        bool m_is_open;
    };
} }

#endif // STREAMING_ARCHIVE_WRITER_INMEMORYFILE_HPP
