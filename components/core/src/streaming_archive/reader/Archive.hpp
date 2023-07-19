#ifndef STREAMING_ARCHIVE_READER_ARCHIVE_HPP
#define STREAMING_ARCHIVE_READER_ARCHIVE_HPP

// C++ libraries
#include <filesystem>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <utility>

// Project headers
#include "../../ErrorCode.hpp"
#include "../../LogTypeDictionaryReader.hpp"
#include "../../Query.hpp"
#include "../../SQLiteDB.hpp"
#include "../../VariableDictionaryReader.hpp"
#include "../MetadataDB.hpp"
#include "File.hpp"
#include "Message.hpp"

namespace streaming_archive { namespace reader {
    class Archive {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

            // Methods
            const char* what () const noexcept override {
                return "streaming_archive::reader::Archive operation failed";
            }
        };

        // Methods
        /**
         * Read the metadata file
         * @param path
         * @param id
         */
        static void read_metadata_file (const std::string& path, archive_format_version_t& format_version, size_t& stable_uncompressed_size,
                                        size_t& stable_size);

        /**
         * Opens archive for reading
         * @param path
         * @throw streaming_archive::reader::Archive::OperationFailed if could not stat file or it isn't a directory or metadata is corrupted
         * @throw FileReader::OperationFailed if failed to open any dictionary
         */
        void open (const std::string& path);
        void close ();

        /**
         * Reads any new entries added to the dictionaries
         * @throw Same as LogTypeDictionary::read_from_file and VariableDictionary::read_from_file
         */
        void refresh_dictionaries ();
        const LogTypeDictionaryReader& get_logtype_dictionary () const;
        const VariableDictionaryReader& get_var_dictionary () const;

        /**
         * Opens file with given path
         * @param file
         * @param file_metadata_ix
         * @return Same as streaming_archive::reader::File::open_me
         */
        ErrorCode open_file (File& file, MetadataDB::FileIterator& file_metadata_ix);
        /**
         * Wrapper for streaming_archive::reader::File::close_me
         * @param file
         */
        void close_file (File& file);
        /**
         * Wrapper for streaming_archive::reader::File::reset_indices
         * @param file
         */
        void reset_file_indices (File& file);

        /**
         * Wrapper for streaming_archive::reader::File::find_message_in_time_range
         */
        bool find_message_in_time_range (File& file, epochtime_t search_begin_timestamp, epochtime_t search_end_timestamp, Message& msg);
        /**
         * Wrapper for streaming_archive::reader::File::find_message_matching_query
         */
        const SubQuery* find_message_matching_query (File& file, const Query& query, Message& msg);
        /**
         * Wrapper for streaming_archive::reader::File::get_next_message
         */
        bool get_next_message (File& file, Message& msg);

        /**
         * Decompresses a given message from a given file
         * @param file
         * @param compressed_msg
         * @param decompressed_msg
         * @return true if message was successfully decompressed, false otherwise
         * @throw TimestampPattern::OperationFailed if failed to insert timestamp
         */
        bool decompress_message (File& file, const Message& compressed_msg, std::string& decompressed_msg);

        void decompress_empty_directories (const std::string& output_dir);

        std::unique_ptr<MetadataDB::FileIterator> get_file_iterator () {
            return m_metadata_db.get_file_iterator(cEpochTimeMin, cEpochTimeMax, "", false, cInvalidSegmentId);
        }
        std::unique_ptr<MetadataDB::FileIterator> get_file_iterator (const std::string& file_path) {
            return m_metadata_db.get_file_iterator(cEpochTimeMin, cEpochTimeMax, file_path, false, cInvalidSegmentId);
        }
        std::unique_ptr<MetadataDB::FileIterator> get_file_iterator (epochtime_t begin_ts, epochtime_t end_ts, const std::string& file_path) {
            return m_metadata_db.get_file_iterator(begin_ts, end_ts, file_path, false, cInvalidSegmentId);
        }
        std::unique_ptr<MetadataDB::FileIterator> get_file_iterator (epochtime_t begin_ts, epochtime_t end_ts, const std::string& file_path,
                                                                     segment_id_t segment_id)
        {
            return m_metadata_db.get_file_iterator(begin_ts, end_ts, file_path, true, segment_id);
        }

    private:
        // Variables
        std::string m_id;
        std::string m_path;
        std::string m_segments_dir_path;
        LogTypeDictionaryReader m_logtype_dictionary;
        VariableDictionaryReader m_var_dictionary;

        SegmentManager m_segment_manager;

        MetadataDB m_metadata_db;
    };
} }

#endif // STREAMING_ARCHIVE_READER_ARCHIVE_HPP
