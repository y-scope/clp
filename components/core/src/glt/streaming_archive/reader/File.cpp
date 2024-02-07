#include "File.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include "../../EncodedVariableInterpreter.hpp"
#include "../../spdlog_with_specializations.hpp"
#include "../Constants.hpp"
#include "SegmentManager.hpp"

using std::string;

namespace glt::streaming_archive::reader {
epochtime_t File::get_begin_ts() const {
    return m_begin_ts;
}

epochtime_t File::get_end_ts() const {
    return m_end_ts;
}

ErrorCode File::init(
        LogTypeDictionaryReader const& archive_logtype_dict,
        MetadataDB::FileIterator const& file_metadata_ix
) {
    m_archive_logtype_dict = &archive_logtype_dict;

    // Populate metadata from database document
    file_metadata_ix.get_id(m_id_as_string);
    file_metadata_ix.get_orig_file_id(m_orig_file_id_as_string);
    file_metadata_ix.get_path(m_orig_path);
    m_begin_ts = file_metadata_ix.get_begin_ts();
    m_end_ts = file_metadata_ix.get_end_ts();

    string encoded_timestamp_patterns;
    file_metadata_ix.get_timestamp_patterns(encoded_timestamp_patterns);
    size_t begin_pos = 0;
    size_t end_pos;
    string timestamp_format;
    while (true) {
        end_pos = encoded_timestamp_patterns.find_first_of(':', begin_pos);
        if (string::npos == end_pos) {
            // Done
            break;
        }
        size_t msg_num = strtoull(&encoded_timestamp_patterns[begin_pos], nullptr, 10);
        begin_pos = end_pos + 1;

        end_pos = encoded_timestamp_patterns.find_first_of(':', begin_pos);
        if (string::npos == end_pos) {
            // Unexpected truncation
            throw OperationFailed(ErrorCode_Corrupt, __FILENAME__, __LINE__);
        }
        uint8_t num_spaces_before_ts = strtol(&encoded_timestamp_patterns[begin_pos], nullptr, 10);
        begin_pos = end_pos + 1;

        end_pos = encoded_timestamp_patterns.find_first_of('\n', begin_pos);
        if (string::npos == end_pos) {
            // Unexpected truncation
            throw OperationFailed(ErrorCode_Corrupt, __FILENAME__, __LINE__);
        }
        timestamp_format.assign(encoded_timestamp_patterns, begin_pos, end_pos - begin_pos);
        begin_pos = end_pos + 1;

        m_timestamp_patterns.emplace_back(
                std::piecewise_construct,
                std::forward_as_tuple(msg_num),
                forward_as_tuple(num_spaces_before_ts, timestamp_format)
        );
    }

    m_num_messages = file_metadata_ix.get_num_messages();
    m_segment_id = file_metadata_ix.get_segment_id();

    m_is_split = file_metadata_ix.is_split();
    m_split_ix = file_metadata_ix.get_split_ix();

    m_msgs_ix = 0;

    m_current_ts_pattern_ix = 0;
    m_current_ts_in_milli = m_begin_ts;

    return ErrorCode_Success;
}

ErrorCode File::open_me(
        LogTypeDictionaryReader const& archive_logtype_dict,
        MetadataDB::FileIterator const& file_metadata_ix,
        GLTSegment& segment,
        Segment& message_order_table
) {
    File::init(archive_logtype_dict, file_metadata_ix);
    m_segment_logtypes_decompressed_stream_pos = file_metadata_ix.get_segment_logtypes_pos();
    m_segment_offsets_decompressed_stream_pos = file_metadata_ix.get_segment_offset_pos();

    if (cInvalidSegmentId == m_segment_id) {
        SPDLOG_ERROR("Unexpected invalid segment id");
        return ErrorCode_Truncated;
    }

    uint64_t num_bytes_to_read;
    if (m_num_messages > 0) {
        if (m_num_messages > m_num_segment_msgs) {
            // Buffers too small, so increase size to required amount
            m_segment_logtypes = std::make_unique<logtype_dictionary_id_t[]>(m_num_messages);
            m_segment_offsets = std::make_unique<size_t[]>(m_num_messages);
            m_num_segment_msgs = m_num_messages;
        }

        num_bytes_to_read = m_num_messages * sizeof(logtype_dictionary_id_t);
        ErrorCode error_code = message_order_table.try_read(
                m_segment_logtypes_decompressed_stream_pos,
                reinterpret_cast<char*>(m_segment_logtypes.get()),
                num_bytes_to_read
        );
        if (ErrorCode_Success != error_code) {
            close_me();
            return error_code;
        }
        m_logtypes = m_segment_logtypes.get();
        num_bytes_to_read = m_num_messages * sizeof(size_t);
        error_code = message_order_table.try_read(
                m_segment_offsets_decompressed_stream_pos,
                reinterpret_cast<char*>(m_segment_offsets.get()),
                num_bytes_to_read
        );
        if (ErrorCode_Success != error_code) {
            close_me();
            return error_code;
        }
        m_offsets = m_segment_offsets.get();
    }

    m_segment = &segment;

    return ErrorCode_Success;
}

void File::close_me() {
    m_segment_logtypes_decompressed_stream_pos = 0;
    m_segment_offsets_decompressed_stream_pos = 0;
    m_logtype_table_offsets.clear();

    m_msgs_ix = 0;
    m_num_messages = 0;

    m_current_ts_pattern_ix = 0;
    m_current_ts_in_milli = 0;
    m_timestamp_patterns.clear();

    m_begin_ts = cEpochTimeMax;
    m_end_ts = cEpochTimeMin;
    m_orig_path.clear();

    m_archive_logtype_dict = nullptr;
}

size_t File::get_msg_offset(logtype_dictionary_id_t logtype_id, size_t msg_ix) {
    if (m_logtype_table_offsets.find(logtype_id) == m_logtype_table_offsets.end()) {
        m_logtype_table_offsets[logtype_id] = m_offsets[msg_ix];
    }
    size_t return_value = m_logtype_table_offsets[logtype_id];
    m_logtype_table_offsets[logtype_id] += 1;
    return return_value;
}

bool File::get_next_message(Message& msg) {
    if (m_msgs_ix >= m_num_messages) {
        return false;
    }

    // Get message number
    msg.set_message_number(m_msgs_ix);

    // Get log-type
    auto logtype_id = m_logtypes[m_msgs_ix];
    msg.set_logtype_id(logtype_id);

    // Get variables
    msg.clear_vars();
    auto const& logtype_dictionary_entry = m_archive_logtype_dict->get_entry(logtype_id);

    // Get timestamp
    auto variable_offset = get_msg_offset(logtype_id, m_msgs_ix);
    auto timestamp = m_segment->get_timestamp_at_offset(logtype_id, variable_offset);
    msg.set_timestamp(timestamp);

    auto const num_vars = logtype_dictionary_entry.get_num_variables();
    if (num_vars > 0) {
        // The behavior here slight changed. the function will throw an error
        // if the attempt to load variable fails
        m_segment->get_variable_row_at_offset(logtype_id, variable_offset, msg);
    }

    ++m_msgs_ix;

    return true;
}

void File::reset_indices() {
    m_msgs_ix = 0;
}

string const& File::get_orig_path() const {
    return m_orig_path;
}

std::vector<std::pair<uint64_t, TimestampPattern>> const& File::get_timestamp_patterns() const {
    return m_timestamp_patterns;
}

epochtime_t File::get_current_ts_in_milli() const {
    return m_current_ts_in_milli;
}

size_t File::get_current_ts_pattern_ix() const {
    return m_current_ts_pattern_ix;
}

void File::increment_current_ts_pattern_ix() {
    ++m_current_ts_pattern_ix;
}
}  // namespace glt::streaming_archive::reader
