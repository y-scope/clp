#ifndef STREAMING_ARCHIVE_READER_FILE_HPP
#define STREAMING_ARCHIVE_READER_FILE_HPP

// C++ libraries
#include <list>
#include <set>
#include <vector>

// Project headers
#include "../../Defs.h"
#include "../../ErrorCode.hpp"
#include "../../LogTypeDictionaryReader.hpp"
#include "../../Query.hpp"
#include "../../TimestampPattern.hpp"
#include "../MetadataDB.hpp"
#include "Message.hpp"
#include "SegmentManager.hpp"

namespace streaming_archive { namespace reader {
    class File {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

            // Methods
            const char* what () const noexcept override {
                return "streaming_archive::reader::File operation failed";
            }
        };

        // Constructors
        File () :
            m_archive_logtype_dict(nullptr),
            m_begin_ts(cEpochTimeMax),
            m_end_ts(cEpochTimeMin),
            m_is_in_segment(false),
            m_segment_timestamps_decompressed_stream_pos(0),
            m_segment_logtypes_decompressed_stream_pos(0),
            m_segment_variables_decompressed_stream_pos(0),
            m_num_segment_msgs(0),
            m_num_segment_vars(0),
            m_msgs_ix(0),
            m_num_messages(0),
            m_variables_ix(0),
            m_num_variables(0),
            m_logtypes_fd(-1),
            m_logtypes_file_size(0),
            m_logtypes(nullptr),
            m_timestamps_fd(-1),
            m_timestamps_file_size(0),
            m_timestamps(nullptr),
            m_variables_fd(-1),
            m_variables_file_size(0),
            m_variables(nullptr),
            m_current_ts_pattern_ix(0),
            m_current_ts_in_milli(0)
        {}

        // Methods
        const std::string& get_id_as_string() const { return m_id_as_string; }
        const std::string& get_orig_file_id_as_string() const { return m_orig_file_id_as_string; }
        epochtime_t get_begin_ts () const;
        epochtime_t get_end_ts () const;
        const std::string& get_orig_path () const;
        bool is_in_segment () const { return m_is_in_segment; }
        segment_id_t get_segment_id () const { return m_segment_id; }
        uint64_t get_num_messages () const { return m_num_messages; }
        bool is_split () const { return m_is_split; }

    private:
        friend class Archive;

        // Methods
        /**
         * Opens file
         * @param archive_logtype_dict
         * @param file_metadata_ix
         * @param read_ahead Whether to read-ahead in the file (if possible)
         * @param archive_logs_dir_path Path to directory where logs are stored on disk in this archive
         * @param segment_manager Segment manager for when file is stored in a segment
         * @return FileReader::try_open's error codes on failure to open metadata
         * @return ErrorCode::Failure_Metadata_Corrupted on metadata loading error
         * @return ErrorCode::Errno on error
         * @return ErrorCode::FileNotFound if a column's file was not found
         * @return ErrorCode::Truncated if metadata did not contain all required data or if column in segment was truncated
         * @return ErrorCode::Success on success
         * @throw FileReader::OperationFailed on any read failure
         * @throw Same as streaming_archive::reader::SegmentManager::read
         */
        ErrorCode open_me (const LogTypeDictionaryReader& archive_logtype_dict, MetadataDB::FileIterator& file_metadata_ix, bool read_ahead,
                const std::string& archive_logs_dir_path, SegmentManager& segment_manager);
        /**
         * Closes the file
         */
        void close_me ();
        /**
         * Reset positions in columns
         */
        void reset_indices ();

        const std::vector<std::pair<uint64_t, TimestampPattern>>& get_timestamp_patterns () const;
        epochtime_t get_current_ts_in_milli () const;
        size_t get_current_ts_pattern_ix () const;

        void increment_current_ts_pattern_ix ();

        /**
         * Finds message that falls in given time range
         * @param search_begin_timestamp
         * @param search_end_timestamp
         * @param msg
         * @return true if a message was found, false otherwise
         */
        bool find_message_in_time_range (epochtime_t search_begin_timestamp, epochtime_t search_end_timestamp, Message& msg);
        /**
         * Finds message matching the given query
         * @param query
         * @param msg
         * @return nullptr if no message matched
         * @return pointer to matching subquery otherwise
         */
        const SubQuery* find_message_matching_query (const Query& query, Message& msg);
        /**
         * Get next message in file
         * @param msg
         * @return true if message read, false if no more messages left
         */
        bool get_next_message (Message& msg);

        // Variables
        const LogTypeDictionaryReader* m_archive_logtype_dict;

        epochtime_t m_begin_ts;
        epochtime_t m_end_ts;
        std::vector<std::pair<uint64_t, TimestampPattern>> m_timestamp_patterns;
        std::string m_id_as_string;
        std::string m_orig_file_id_as_string;
        std::string m_orig_path;

        bool m_is_in_segment;
        segment_id_t m_segment_id;
        uint64_t m_segment_timestamps_decompressed_stream_pos;
        uint64_t m_segment_logtypes_decompressed_stream_pos;
        uint64_t m_segment_variables_decompressed_stream_pos;
        std::unique_ptr<epochtime_t[]> m_segment_timestamps;
        std::unique_ptr<logtype_dictionary_id_t[]> m_segment_logtypes;
        uint64_t m_num_segment_msgs;
        std::unique_ptr<encoded_variable_t[]> m_segment_variables;
        uint64_t m_num_segment_vars;

        size_t m_msgs_ix;
        uint64_t m_num_messages;
        size_t m_variables_ix;
        uint64_t m_num_variables;

        int m_logtypes_fd;
        size_t m_logtypes_file_size;
        logtype_dictionary_id_t* m_logtypes;

        int m_timestamps_fd;
        size_t m_timestamps_file_size;
        epochtime_t* m_timestamps;

        int m_variables_fd;
        size_t m_variables_file_size;
        encoded_variable_t* m_variables;

        size_t m_current_ts_pattern_ix;
        epochtime_t m_current_ts_in_milli;

        size_t m_split_ix;
        bool m_is_split;
    };
} }

#endif // STREAMING_ARCHIVE_READER_FILE_HPP
