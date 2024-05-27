#ifndef CLP_STREAMING_ARCHIVE_WRITER_FILE_HPP
#define CLP_STREAMING_ARCHIVE_WRITER_FILE_HPP

#include <unordered_set>
#include <vector>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "../../Defs.h"
#include "../../ErrorCode.hpp"
#include "../../LogTypeDictionaryWriter.hpp"
#include "../../PageAllocatedVector.hpp"
#include "../../TimestampPattern.hpp"
#include "Segment.hpp"

namespace clp::streaming_archive::writer {
/**
 * Class representing a log file encoded in three columns - timestamps, logtype IDs, and
 * variables.
 */
class File {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "streaming_archive::writer::File operation failed";
        }
    };

    // Constructors
    File(boost::uuids::uuid const& id,
         boost::uuids::uuid const& orig_file_id,
         std::string const& orig_log_path,
         group_id_t group_id,
         size_t split_ix,
         size_t begin_message_ix)
            : m_id(id),
              m_orig_file_id(orig_file_id),
              m_orig_log_path(orig_log_path),
              m_begin_ts(cEpochTimeMax),
              m_end_ts(cEpochTimeMin),
              m_group_id(group_id),
              m_num_uncompressed_bytes(0),
              m_begin_message_ix(begin_message_ix),
              m_num_messages(0),
              m_num_variables(0),
              m_segment_id(cInvalidSegmentId),
              m_segment_timestamps_pos(0),
              m_segment_logtypes_pos(0),
              m_segment_variables_pos(0),
              m_is_split(split_ix > 0),
              m_split_ix(split_ix),
              m_segmentation_state(SegmentationState_NotInSegment),
              m_is_metadata_clean(false),
              m_is_written_out(false),
              m_is_open(false) {}

    // Destructor
    virtual ~File() = default;

    // Methods
    bool is_open() const { return m_is_open; }

    void open();

    void close() { m_is_open = false; }

    /**
     * Appends the file's columns to the given segment
     * @param logtype_dict
     * @param segment
     */
    void append_to_segment(LogTypeDictionaryWriter const& logtype_dict, Segment& segment);
    /**
     * Writes an encoded message to the respective columns and updates the metadata of the file
     * @param timestamp
     * @param logtype_id
     * @param encoded_vars
     * @param var_ids
     * @param num_uncompressed_bytes
     */
    void write_encoded_msg(
            epochtime_t timestamp,
            logtype_dictionary_id_t logtype_id,
            std::vector<encoded_variable_t> const& encoded_vars,
            std::vector<variable_dictionary_id_t> const& var_ids,
            size_t num_uncompressed_bytes
    );

    /**
     * Changes timestamp pattern in use at current message in file
     * @param pattern
     */
    void change_ts_pattern(TimestampPattern const* pattern);

    /**
     * Returns whether the file contains any timestamp pattern
     * @return true if the file contains a timestamp pattern, false otherwise
     */
    bool has_ts_pattern() const { return m_timestamp_patterns.empty() == false; }

    /**
     * @return File's begin message index
     */
    uint64_t get_begin_message_ix() const { return m_begin_message_ix; }

    /**
     * @return File's end message index
     */
    uint64_t get_end_message_ix() const { return m_begin_message_ix + m_num_messages; }

    /**
     * Gets the file's uncompressed size
     * @return File's uncompressed size in bytes
     */
    uint64_t get_num_uncompressed_bytes() const { return m_num_uncompressed_bytes; }

    /**
     * Gets the file's encoded size in bytes
     * @return Encoded size in bytes
     */
    size_t get_encoded_size_in_bytes() const {
        return m_num_messages * sizeof(epochtime_t)
               + m_num_messages * sizeof(logtype_dictionary_id_t)
               + m_num_variables * sizeof(encoded_variable_t);
    }

    /**
     * Gets the file's compression group ID
     * @return The compression group ID
     */
    group_id_t get_group_id() const { return m_group_id; }

    /**
     * Tests if the file has been moved to segment that has not yet been committed
     * @return true if in uncommitted segment, false otherwise
     */
    bool is_in_uncommitted_segment() const;
    /**
     * Marks this file as being within a committed segment
     */
    void mark_as_in_committed_segment();
    /**
     * Tests if file's current metadata is dirty
     * @return
     */
    bool is_metadata_dirty() const;
    /**
     * Marks the file's metadata as clean
     */
    void mark_metadata_as_clean();

    void set_is_split(bool is_split) { m_is_split = is_split; }

    /**
     * Gets file's original file path
     * @return file path
     */
    std::string const& get_orig_path() const { return m_orig_log_path; }

    boost::uuids::uuid const& get_orig_file_id() const { return m_orig_file_id; }

    std::string get_orig_file_id_as_string() const {
        return boost::uuids::to_string(m_orig_file_id);
    }

    boost::uuids::uuid const& get_id() const { return m_id; }

    std::string get_id_as_string() const { return boost::uuids::to_string(m_id); }

    epochtime_t get_begin_ts() const { return m_begin_ts; }

    epochtime_t get_end_ts() const { return m_end_ts; }

    std::vector<std::pair<int64_t, TimestampPattern>> const& get_timestamp_patterns() const {
        return m_timestamp_patterns;
    }

    std::string get_encoded_timestamp_patterns() const;

    uint64_t get_num_messages() const { return m_num_messages; }

    uint64_t get_num_variables() const { return m_num_variables; }

    bool is_in_segment() const { return SegmentationState_InSegment == m_segmentation_state; }

    segment_id_t get_segment_id() const { return m_segment_id; }

    uint64_t get_segment_timestamps_pos() const { return m_segment_timestamps_pos; }

    uint64_t get_segment_logtypes_pos() const { return m_segment_logtypes_pos; }

    uint64_t get_segment_variables_pos() const { return m_segment_variables_pos; }

    bool is_split() const { return m_is_split; }

    size_t get_split_ix() const { return m_split_ix; }

private:
    // Types
    typedef enum {
        SegmentationState_NotInSegment = 0,
        SegmentationState_MovingToSegment,
        SegmentationState_InSegment
    } SegmentationState;

    // Methods
    /**
     * Sets segment-related metadata to the given values
     * @param segment_id
     * @param segment_timestamps_uncompressed_pos
     * @param segment_logtypes_uncompressed_pos
     * @param segment_variables_uncompressed_pos
     */
    void set_segment_metadata(
            segment_id_t segment_id,
            uint64_t segment_timestamps_uncompressed_pos,
            uint64_t segment_logtypes_uncompressed_pos,
            uint64_t segment_variables_uncompressed_pos
    );

    // Variables
    // Metadata
    boost::uuids::uuid m_id;
    boost::uuids::uuid m_orig_file_id;

    std::string m_orig_log_path;

    epochtime_t m_begin_ts;
    epochtime_t m_end_ts;
    std::vector<std::pair<int64_t, TimestampPattern>> m_timestamp_patterns;

    group_id_t m_group_id;

    uint64_t m_num_uncompressed_bytes;

    uint64_t m_begin_message_ix;
    uint64_t m_num_messages;
    uint64_t m_num_variables;

    segment_id_t m_segment_id;
    uint64_t m_segment_timestamps_pos;
    uint64_t m_segment_logtypes_pos;
    uint64_t m_segment_variables_pos;

    bool m_is_split;
    size_t m_split_ix;

    // Data variables
    std::unique_ptr<PageAllocatedVector<epochtime_t>> m_timestamps;
    std::unique_ptr<PageAllocatedVector<logtype_dictionary_id_t>> m_logtypes;
    std::unique_ptr<PageAllocatedVector<encoded_variable_t>> m_variables;

    // State variables
    SegmentationState m_segmentation_state;
    bool m_is_metadata_clean;
    bool m_is_written_out;
    bool m_is_open;
};
}  // namespace clp::streaming_archive::writer

#endif  // CLP_STREAMING_ARCHIVE_WRITER_FILE_HPP
