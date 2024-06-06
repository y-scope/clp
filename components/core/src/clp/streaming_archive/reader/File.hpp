#ifndef CLP_STREAMING_ARCHIVE_READER_FILE_HPP
#define CLP_STREAMING_ARCHIVE_READER_FILE_HPP

#include <list>
#include <set>
#include <vector>

#include "../../Defs.h"
#include "../../ErrorCode.hpp"
#include "../../LogTypeDictionaryReader.hpp"
#include "../../Query.hpp"
#include "../../TimestampPattern.hpp"
#include "../MetadataDB.hpp"
#include "Message.hpp"
#include "SegmentManager.hpp"

namespace clp::streaming_archive::reader {
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
              m_segment_timestamps_decompressed_stream_pos(0),
              m_segment_logtypes_decompressed_stream_pos(0),
              m_segment_variables_decompressed_stream_pos(0),
              m_num_segment_msgs(0),
              m_num_segment_vars(0),
              m_msgs_ix(0),
              m_begin_message_ix(0),
              m_num_messages(0),
              m_variables_ix(0),
              m_num_variables(0),
              m_logtypes(nullptr),
              m_timestamps(nullptr),
              m_variables(nullptr),
              m_current_ts_pattern_ix(0),
              m_current_ts_in_milli(0) {}

    // Methods
    std::string const& get_id_as_string() const { return m_id_as_string; }

    std::string const& get_orig_file_id_as_string() const { return m_orig_file_id_as_string; }

    epochtime_t get_begin_ts() const;
    epochtime_t get_end_ts() const;
    std::string const& get_orig_path() const;

    segment_id_t get_segment_id() const { return m_segment_id; }

    uint64_t get_begin_message_ix() const { return m_begin_message_ix; }

    uint64_t get_num_messages() const { return m_num_messages; }

    bool is_split() const { return m_is_split; }

private:
    friend class Archive;

    // Methods
    /**
     * Opens file
     * @param archive_logtype_dict
     * @param file_metadata_ix
     * @param segment_manager
     * @return Same as SegmentManager::try_read
     * @return ErrorCode_Success on success
     */
    ErrorCode open_me(
            LogTypeDictionaryReader const& archive_logtype_dict,
            MetadataDB::FileIterator const& file_metadata_ix,
            SegmentManager& segment_manager
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

    /**
     * Finds message that falls in given time range
     * @param search_begin_timestamp
     * @param search_end_timestamp
     * @param msg
     * @return true if a message was found, false otherwise
     */
    bool find_message_in_time_range(
            epochtime_t search_begin_timestamp,
            epochtime_t search_end_timestamp,
            Message& msg
    );
    /**
     * Finds message matching the given query
     * @param query
     * @param msg
     * @return nullptr if no message matched
     * @return pointer to matching subquery otherwise
     */
    SubQuery const* find_message_matching_query(Query const& query, Message& msg);
    /**
     * Get next message in file
     * @param msg
     * @return true if message read, false if no more messages left
     */
    bool get_next_message(Message& msg);

    // Variables
    LogTypeDictionaryReader const* m_archive_logtype_dict;

    epochtime_t m_begin_ts;
    epochtime_t m_end_ts;
    std::vector<std::pair<uint64_t, TimestampPattern>> m_timestamp_patterns;
    std::string m_id_as_string;
    std::string m_orig_file_id_as_string;
    std::string m_orig_path;

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
    uint64_t m_begin_message_ix;
    uint64_t m_num_messages;
    size_t m_variables_ix;
    uint64_t m_num_variables;

    logtype_dictionary_id_t* m_logtypes;
    epochtime_t* m_timestamps;
    encoded_variable_t* m_variables;

    size_t m_current_ts_pattern_ix;
    epochtime_t m_current_ts_in_milli;

    size_t m_split_ix;
    bool m_is_split;
};
}  // namespace clp::streaming_archive::reader

#endif  // CLP_STREAMING_ARCHIVE_READER_FILE_HPP
