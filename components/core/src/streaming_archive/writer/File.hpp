#ifndef STREAMING_ARCHIVE_WRITER_FILE_HPP
#define STREAMING_ARCHIVE_WRITER_FILE_HPP

// C++ standard libraries
#include <unordered_set>
#include <vector>

// Boost libraries
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

// Project headers
#include "../../Defs.h"
#include "../../ErrorCode.hpp"
#include "../../LogTypeDictionaryWriter.hpp"
#include "../../TimestampPattern.hpp"
#include "Segment.hpp"

namespace streaming_archive { namespace writer {
    /**
     * Abstract class representing a compressed log file encoded into three columns - timestamps, logtype IDs, and variables. Implementations need only
     * define how the column data is stored on disk.
     */
    class File {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

            // Methods
            const char* what () const noexcept override {
                return "streaming_archive::writer::File operation failed";
            }
        };

        // Constructors
        File (const boost::uuids::uuid& id, const boost::uuids::uuid& orig_file_id, const std::string& orig_log_path, group_id_t group_id, size_t split_ix) :
                m_id(id),
                m_id_as_string(boost::uuids::to_string(m_id)),
                m_orig_file_id(orig_file_id),
                m_orig_file_id_as_string(boost::uuids::to_string(m_orig_file_id)),
                m_orig_log_path(orig_log_path),
                m_begin_ts(cEpochTimeMax),
                m_end_ts(cEpochTimeMin),
                m_group_id(group_id),
                m_num_uncompressed_bytes(0),
                m_num_messages(0),
                m_num_variables(0),
                m_segment_id(cInvalidSegmentId),
                m_segment_timestamps_pos(0),
                m_segment_logtypes_pos(0),
                m_segment_variables_pos(0),
                m_is_split(split_ix > 0),
                m_split_ix(split_ix),
                m_is_metadata_clean(false),
                m_segmentation_state(SegmentationState_NotInSegment)
        {}

        // Destructor
        virtual ~File () = default;

        // Methods
        virtual bool is_open () const = 0;
        virtual void open () = 0;
        virtual void close () = 0;
        virtual void append_to_segment (const LogTypeDictionaryWriter& logtype_dict, Segment& segment,
                                        std::unordered_set<logtype_dictionary_id_t>& segment_logtype_ids,
                                        std::unordered_set<variable_dictionary_id_t>& segment_var_ids) = 0;
        virtual void cleanup_after_segment_insertion () = 0;
        virtual void write_encoded_msg (epochtime_t timestamp, logtype_dictionary_id_t logtype_id, const std::vector<encoded_variable_t>& encoded_vars,
                size_t num_uncompressed_bytes) = 0;

        /**
         * Changes timestamp pattern in use at current message in file
         * @param pattern
         */
        void change_ts_pattern (const TimestampPattern* pattern);

        /**
         * Returns whether the file contains any timestamp pattern
         * @return true if the file contains a timestamp pattern, false otherwise
         */
        bool has_ts_pattern () const { return m_timestamp_patterns.empty() == false; };

        /**
         * Gets the file's uncompressed size
         * @return File's uncompressed size in bytes
         */
        size_t get_num_uncompressed_bytes () const { return m_num_uncompressed_bytes; }

        /**
         * Gets the file's encoded size in bytes
         * @return Encoded size in bytes
         */
        size_t get_encoded_size_in_bytes () {
            return m_num_messages * sizeof(epochtime_t) + m_num_messages * sizeof(logtype_dictionary_id_t) + m_num_variables * sizeof(encoded_variable_t);
        }

        /**
         * Gets the file's compression group ID
         * @return The compression group ID
         */
        group_id_t get_group_id () const { return m_group_id; };

        /**
         * Tests if the file has been moved to segment that has not yet been comitted
         * @return true if in uncommitted segment, false otherwise
         */
        bool is_in_uncommitted_segment () const;
        /**
         * Marks this file as being within a committed segment
         */
        void mark_as_in_committed_segment ();
        /**
         * Tests if file's current metadata is dirty
         * @return
         */
        bool is_metadata_dirty () const;
        /**
         * Marks the file's metadata as clean
         */
        void mark_metadata_as_clean ();

        void set_is_split (bool is_split) { m_is_split = is_split; }

        /**
         * Gets file's original file path
         * @return file path
         */
        const std::string& get_orig_path () const { return m_orig_log_path; }
        const boost::uuids::uuid& get_orig_file_id () const { return m_orig_file_id; }
        const std::string& get_orig_file_id_as_string () const { return m_orig_file_id_as_string; }
        const boost::uuids::uuid& get_id () const { return m_id; }
        const std::string& get_id_as_string () const { return m_id_as_string; }
        epochtime_t get_begin_ts () const { return m_begin_ts; }
        epochtime_t get_end_ts () const { return m_end_ts; }
        const std::vector<std::pair<int64_t, TimestampPattern>>& get_timestamp_patterns () const { return m_timestamp_patterns; }
        std::string get_encoded_timestamp_patterns () const;
        size_t get_num_messages () const { return m_num_messages; }
        size_t get_num_variables () const { return m_num_variables; }

        bool is_in_segment () const { return SegmentationState_InSegment == m_segmentation_state; }
        segment_id_t get_segment_id () const { return m_segment_id; }
        uint64_t get_segment_timestamps_pos () const { return m_segment_timestamps_pos; }
        uint64_t get_segment_logtypes_pos () const { return m_segment_logtypes_pos; }
        uint64_t get_segment_variables_pos () const { return m_segment_variables_pos; }
        bool is_split () const { return m_is_split; }
        size_t get_split_ix () const { return m_split_ix; }

    protected:
        // Types
        typedef enum {
            SegmentationState_NotInSegment = 0,
            SegmentationState_MovingToSegment,
            SegmentationState_InSegment
        } SegmentationState;

        // Methods
        /**
         * Takes logtype and variable IDs from a file's logtype and variable columns and appends them to the given sets
         * @param logtype_dict
         * @param logtype_ids
         * @param num_logtypes
         * @param vars
         * @param num_vars
         * @param segment_logtype_ids
         * @param segment_var_ids
         */
        static void append_logtype_and_var_ids_to_segment_sets (const LogTypeDictionaryWriter& logtype_dict, const logtype_dictionary_id_t* logtype_ids,
                                                                size_t num_logtypes, const encoded_variable_t* vars, size_t num_vars,
                                                                std::unordered_set<logtype_dictionary_id_t>& segment_logtype_ids,
                                                                std::unordered_set<variable_dictionary_id_t>& segment_var_ids);


        void increment_num_uncompressed_bytes (size_t num_bytes);
        /**
         * Increments the number of messages and the number of variables by the given values
         * @param num_messages_to_add
         * @param num_variables_to_add
         */
        void increment_num_messages_and_variables (size_t num_messages_to_add, size_t num_variables_to_add);
        void set_last_message_timestamp (epochtime_t timestamp);
        /**
         * Sets segment-related metadata to the given values
         * @param segment_id
         * @param segment_timestamps_uncompressed_pos
         * @param segment_logtypes_uncompressed_pos
         * @param segment_variables_uncompressed_pos
         */
        void set_segment_metadata (segment_id_t segment_id, uint64_t segment_timestamps_uncompressed_pos, uint64_t segment_logtypes_uncompressed_pos,
                                   uint64_t segment_variables_uncompressed_pos);

        // Variables
        SegmentationState m_segmentation_state;

    private:
        // Variables
        // Metadata
        boost::uuids::uuid m_id;
        std::string m_id_as_string;
        boost::uuids::uuid m_orig_file_id;
        std::string m_orig_file_id_as_string;

        std::string m_orig_log_path;

        epochtime_t m_begin_ts;
        epochtime_t m_end_ts;
        std::vector<std::pair<int64_t, TimestampPattern>> m_timestamp_patterns;

        group_id_t m_group_id;

        uint64_t m_num_uncompressed_bytes;

        uint64_t m_num_messages;
        uint64_t m_num_variables;

        segment_id_t m_segment_id;
        uint64_t m_segment_timestamps_pos;
        uint64_t m_segment_logtypes_pos;
        uint64_t m_segment_variables_pos;

        bool m_is_split;
        size_t m_split_ix;

        // State variables
        bool m_is_metadata_clean;
    };
} }

#endif // STREAMING_ARCHIVE_WRITER_FILE_HPP
