#ifndef STREAMING_ARCHIVE_WRITER_ONDISKFILE_HPP
#define STREAMING_ARCHIVE_WRITER_ONDISKFILE_HPP

// C++ standard libraries
#include <string>

// Project headers
#include "File.hpp"

namespace streaming_archive { namespace writer {
    /**
     * OnDiskFile is a streaming_archive::writer::File which stores all columns on disk until they are appended to a segment
     */
    class OnDiskFile : public File {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

            // Methods
            const char* what () const noexcept override {
                return "streaming_archive::writer::OnDiskFile operation failed";
            }
        };

        // Constructor
        /**
         * Constructs a file
         */
        OnDiskFile (const boost::uuids::uuid& id, const boost::uuids::uuid& orig_file_id, const std::string& orig_log_path,
                    const std::string& archive_log_path, group_id_t group_id, size_t split_ix);

    private:
        friend class Archive;

        // Methods that implement the File interface
        /**
         * Opens writers for each column
         * @throw FileWriter::OperationFailed if any writer fails to open
         * @throw streaming_archive::writer::OnDiskFile::OperationFailed if file is already in segment
         */
        void open () override;
        /**
         * Closes writers for all columns
         * @throw FileWriter::OperationFailed if any writer fails to close
         */
        void close () override;

        /**
         * Appends file's columns to the given segment
         * @param logtype_dict
         * @param segment
         * @param segment_logtype_ids
         * @param segment_var_ids
         * @throw streaming_archive::writer::OnDiskFile::OperationFailed if file is still open, any column could not be mapped, any column is truncated, or any
         * column fails to be appended
         */
        void append_to_segment (const LogTypeDictionaryWriter& logtype_dict, Segment& segment, std::unordered_set<logtype_dictionary_id_t>& segment_logtype_ids,
                                std::unordered_set<variable_dictionary_id_t>& segment_var_ids) override;
        /**
         * Removes file's columns from disk
         * @throw streaming_archive::writer::OnDiskFile::OperationFailed if removal of any column fails
         */
        void cleanup_after_segment_insertion () override;

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
         * @throw FileWriter::OperationFailed if any write fails
         */
        void write_encoded_msg (epochtime_t timestamp, logtype_dictionary_id_t logtype_id, const std::vector<encoded_variable_t>& encoded_vars,
                size_t num_uncompressed_bytes) override;

        // Methods specific to OnDiskFile
        /**
         * Flushes file to disk
         */
        void flush ();

        // Variables
        std::string m_archive_log_path;

        FileWriter m_timestamps_file_writer;
        FileWriter m_logtype_ids_file_writer;
        FileWriter m_variables_file_writer;

        bool m_is_open;
    };
} }

#endif // STREAMING_ARCHIVE_WRITER_ONDISKFILE_HPP
