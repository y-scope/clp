#ifndef GLT_STREAMING_ARCHIVE_READER_FILE_HPP
#define GLT_STREAMING_ARCHIVE_READER_FILE_HPP

#include <list>
#include <set>
#include <vector>

#include "../../Defs.h"
#include "../../ErrorCode.hpp"
#include "../../LogTypeDictionaryReader.hpp"
#include "../../Query.hpp"
#include "../../TimestampPattern.hpp"
#include "../MetadataDB.hpp"
#include "GLTSegment.hpp"
#include "Message.hpp"

namespace glt::streaming_archive::reader {
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
            return "streaming_archive::reader::File operation failed";
        }
    };

    // Constructors
    File()
            : m_archive_logtype_dict(nullptr),
              m_begin_ts(cEpochTimeMax),
              m_end_ts(cEpochTimeMin),
              m_num_segment_msgs(0),
              m_msgs_ix(0),
              m_num_messages(0),
              m_current_ts_pattern_ix(0),
              m_current_ts_in_milli(0),
              m_logtypes_fd(-1),
              m_logtypes_file_size(0),
              m_logtypes(nullptr),
              m_offsets_fd(-1),
              m_offsets_file_size(0),
              m_segment_logtypes_decompressed_stream_pos(0),
              m_segment(nullptr),
              m_offsets(nullptr) {}

    // Methods
    std::string const& get_id_as_string() const { return m_id_as_string; }

    std::string const& get_orig_file_id_as_string() const { return m_orig_file_id_as_string; }

    epochtime_t get_begin_ts() const;
    epochtime_t get_end_ts() const;
    std::string const& get_orig_path() const;

    segment_id_t get_segment_id() const { return m_segment_id; }

    uint64_t get_num_messages() const { return m_num_messages; }

    bool is_split() const { return m_is_split; }

    // GLT specific
    /**
     * Get next message in file
     * @param msg
     * @return true if message read, false if no more messages left
     */
    bool get_next_message(Message& msg);

    /**
     * Get logtype table offset of the logtype_id
     * @param logtype_id
     * @param msg_ix
     * @return offset of the message
     */
    size_t get_msg_offset(logtype_dictionary_id_t logtype_id, size_t msg_ix);

private:
    friend class Archive;
    // Methods
    /**
     * init a file
     * @param archive_logtype_dict
     * @param file_metadata_ix
     * @return Same as SegmentManager::try_read
     * @return ErrorCode_Success on success
     */
    ErrorCode init(
            LogTypeDictionaryReader const& archive_logtype_dict,
            MetadataDB::FileIterator const& file_metadata_ix
    );

    /**
     * Opens a file with GLTSegment
     * @param archive_logtype_dict
     * @param file_metadata_ix
     * @return Same as SegmentManager::try_read
     * @return ErrorCode_Success on success
     */
    ErrorCode open_me(
            LogTypeDictionaryReader const& archive_logtype_dict,
            MetadataDB::FileIterator const& file_metadata_ix,
            GLTSegment& segment,
            Segment& message_order_table
    );
    /**
     * Closes the file
     */
    void close_me();
    /**
     * Reset positions in columns
     */
    void reset_indices();

    std::vector<std::pair<uint64_t, TimestampPattern>> const& get_timestamp_patterns() const;
    epochtime_t get_current_ts_in_milli() const;
    size_t get_current_ts_pattern_ix() const;

    void increment_current_ts_pattern_ix();

    // Variables
    LogTypeDictionaryReader const* m_archive_logtype_dict;

    epochtime_t m_begin_ts;
    epochtime_t m_end_ts;
    std::vector<std::pair<uint64_t, TimestampPattern>> m_timestamp_patterns;
    std::string m_id_as_string;
    std::string m_orig_file_id_as_string;
    std::string m_orig_path;

    segment_id_t m_segment_id;
    uint64_t m_num_segment_msgs;

    size_t m_msgs_ix;
    uint64_t m_num_messages;

    size_t m_current_ts_pattern_ix;
    epochtime_t m_current_ts_in_milli;

    size_t m_split_ix;
    bool m_is_split;

    // GLT specific
    uint64_t m_segment_logtypes_decompressed_stream_pos;
    uint64_t m_segment_offsets_decompressed_stream_pos;
    std::unique_ptr<logtype_dictionary_id_t[]> m_segment_logtypes;
    std::unique_ptr<size_t[]> m_segment_offsets;

    GLTSegment* m_segment;

    int m_logtypes_fd;
    size_t m_logtypes_file_size;
    logtype_dictionary_id_t* m_logtypes;

    int m_offsets_fd;
    size_t m_offsets_file_size;
    size_t* m_offsets;

    // for keeping the logtype table's offset
    std::unordered_map<logtype_dictionary_id_t, size_t> m_logtype_table_offsets;
};
}  // namespace glt::streaming_archive::reader

#endif  // GLT_STREAMING_ARCHIVE_READER_FILE_HPP
