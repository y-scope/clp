#include "File.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include "../../EncodedVariableInterpreter.hpp"
#include "../../spdlog_with_specializations.hpp"
#include "../Constants.hpp"
#include "SegmentManager.hpp"

using std::string;

namespace clp::streaming_archive::reader {
epochtime_t File::get_begin_ts() const {
    return m_begin_ts;
}

epochtime_t File::get_end_ts() const {
    return m_end_ts;
}

ErrorCode File::open_me(
        LogTypeDictionaryReader const& archive_logtype_dict,
        MetadataDB::FileIterator const& file_metadata_ix,
        SegmentManager& segment_manager
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

    m_begin_message_ix = file_metadata_ix.get_begin_message_ix();
    m_num_messages = file_metadata_ix.get_num_messages();
    m_num_variables = file_metadata_ix.get_num_variables();

    m_segment_id = file_metadata_ix.get_segment_id();
    m_segment_timestamps_decompressed_stream_pos = file_metadata_ix.get_segment_timestamps_pos();
    m_segment_logtypes_decompressed_stream_pos = file_metadata_ix.get_segment_logtypes_pos();
    m_segment_variables_decompressed_stream_pos = file_metadata_ix.get_segment_variables_pos();

    m_is_split = file_metadata_ix.is_split();
    m_split_ix = file_metadata_ix.get_split_ix();

    ErrorCode error_code;

    uint64_t num_bytes_to_read;
    if (m_num_messages > 0) {
        if (m_num_messages > m_num_segment_msgs) {
            // Buffers too small, so increase size to required amount
            m_segment_timestamps = std::make_unique<epochtime_t[]>(m_num_messages);
            m_segment_logtypes = std::make_unique<logtype_dictionary_id_t[]>(m_num_messages);
            m_num_segment_msgs = m_num_messages;
        }

        num_bytes_to_read = m_num_messages * sizeof(epochtime_t);
        error_code = segment_manager.try_read(
                m_segment_id,
                m_segment_timestamps_decompressed_stream_pos,
                reinterpret_cast<char*>(m_segment_timestamps.get()),
                num_bytes_to_read
        );
        if (ErrorCode_Success != error_code) {
            close_me();
            return error_code;
        }
        m_timestamps = m_segment_timestamps.get();

        num_bytes_to_read = m_num_messages * sizeof(logtype_dictionary_id_t);
        error_code = segment_manager.try_read(
                m_segment_id,
                m_segment_logtypes_decompressed_stream_pos,
                reinterpret_cast<char*>(m_segment_logtypes.get()),
                num_bytes_to_read
        );
        if (ErrorCode_Success != error_code) {
            close_me();
            return error_code;
        }
        m_logtypes = m_segment_logtypes.get();
    }

    if (m_num_variables > 0) {
        if (m_num_variables > m_num_segment_vars) {
            // Buffer too small, so increase size to required amount
            m_segment_variables = std::make_unique<encoded_variable_t[]>(m_num_variables);
            m_num_segment_vars = m_num_variables;
        }
        num_bytes_to_read = m_num_variables * sizeof(encoded_variable_t);
        error_code = segment_manager.try_read(
                m_segment_id,
                m_segment_variables_decompressed_stream_pos,
                reinterpret_cast<char*>(m_segment_variables.get()),
                num_bytes_to_read
        );
        if (ErrorCode_Success != error_code) {
            close_me();
            return error_code;
        }
        m_variables = m_segment_variables.get();
    }

    m_msgs_ix = 0;
    m_variables_ix = 0;

    m_current_ts_pattern_ix = 0;
    m_current_ts_in_milli = m_begin_ts;

    return ErrorCode_Success;
}

void File::close_me() {
    m_timestamps = nullptr;
    m_logtypes = nullptr;
    m_variables = nullptr;

    m_segment_timestamps_decompressed_stream_pos = 0;
    m_segment_logtypes_decompressed_stream_pos = 0;
    m_segment_variables_decompressed_stream_pos = 0;

    m_msgs_ix = 0;
    m_begin_message_ix = 0;
    m_num_messages = 0;
    m_variables_ix = 0;
    m_num_variables = 0;

    m_current_ts_pattern_ix = 0;
    m_current_ts_in_milli = 0;
    m_timestamp_patterns.clear();

    m_begin_ts = cEpochTimeMax;
    m_end_ts = cEpochTimeMin;
    m_orig_path.clear();

    m_archive_logtype_dict = nullptr;
}

void File::reset_indices() {
    m_msgs_ix = 0;
    m_variables_ix = 0;
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

bool File::find_message_in_time_range(
        epochtime_t search_begin_timestamp,
        epochtime_t search_end_timestamp,
        Message& msg
) {
    bool found_msg = false;
    while (m_msgs_ix < m_num_messages && !found_msg) {
        // Get logtype
        // NOTE: We get the logtype before the timestamp since we need to use it to get the number
        // of variables, and then advance the variable index, regardless of whether the timestamp
        // falls in the time range or not
        auto logtype_id = m_logtypes[m_msgs_ix];

        // Get number of variables in logtype
        auto const& logtype_dictionary_entry = m_archive_logtype_dict->get_entry(logtype_id);
        auto const num_vars = logtype_dictionary_entry.get_num_variables();

        auto timestamp = m_timestamps[m_msgs_ix];
        if (search_begin_timestamp <= timestamp && timestamp <= search_end_timestamp) {
            // Get variables
            if (m_variables_ix + num_vars > m_num_variables) {
                // Logtypes not in sync with variables, so stop search
                return false;
            }

            msg.clear_vars();
            auto vars_ix = m_variables_ix;
            for (size_t i = 0; i < num_vars; ++i) {
                auto var = m_variables[vars_ix];
                ++vars_ix;
                msg.add_var(var);
            }

            // Set remaining message properties
            msg.set_logtype_id(logtype_id);
            msg.set_timestamp(timestamp);
            msg.set_msg_ix(m_begin_message_ix, m_msgs_ix);

            found_msg = true;
        }

        // Advance indices
        ++m_msgs_ix;
        m_variables_ix += num_vars;
    }

    return found_msg;
}

SubQuery const* File::find_message_matching_query(Query const& query, Message& msg) {
    SubQuery const* matching_sub_query = nullptr;
    while (m_msgs_ix < m_num_messages && nullptr == matching_sub_query) {
        auto const curr_msg_ix{m_msgs_ix};
        auto logtype_id = m_logtypes[curr_msg_ix];

        // Get number of variables in logtype
        auto const& logtype_dictionary_entry = m_archive_logtype_dict->get_entry(logtype_id);
        auto const num_vars = logtype_dictionary_entry.get_num_variables();

        auto const vars_begin_ix{m_variables_ix};
        auto const vars_end_ix{m_variables_ix + num_vars};

        // Advance indices
        ++m_msgs_ix;
        m_variables_ix = vars_end_ix;

        auto const timestamp{m_timestamps[curr_msg_ix]};
        if (false == query.timestamp_is_in_search_time_range(timestamp)) {
            continue;
        }

        for (auto const* sub_query : query.get_relevant_sub_queries()) {
            if (false == sub_query->matches_logtype(logtype_id)) {
                continue;
            }

            msg.clear_vars();
            for (auto vars_ix{vars_begin_ix}; vars_ix < vars_end_ix; ++vars_ix) {
                msg.add_var(m_variables[vars_ix]);
            }
            if (false == sub_query->matches_vars(msg.get_vars())) {
                continue;
            }

            msg.set_logtype_id(logtype_id);
            msg.set_timestamp(timestamp);
            msg.set_msg_ix(m_begin_message_ix, curr_msg_ix);
            matching_sub_query = sub_query;
            break;
        }
    }

    return matching_sub_query;
}

bool File::get_next_message(Message& msg) {
    if (m_msgs_ix >= m_num_messages) {
        return false;
    }

    // Get message number
    msg.set_msg_ix(m_begin_message_ix, m_msgs_ix);

    // Get timestamp
    msg.set_timestamp(m_timestamps[m_msgs_ix]);

    // Get log-type
    auto logtype_id = m_logtypes[m_msgs_ix];
    msg.set_logtype_id(logtype_id);

    // Get variables
    msg.clear_vars();
    auto const& logtype_dictionary_entry = m_archive_logtype_dict->get_entry(logtype_id);
    auto const num_vars = logtype_dictionary_entry.get_num_variables();
    if (m_variables_ix + num_vars > m_num_variables) {
        return false;
    }
    for (size_t i = 0; i < num_vars; ++i) {
        auto var = m_variables[m_variables_ix];
        ++m_variables_ix;
        msg.add_var(var);
    }

    ++m_msgs_ix;

    return true;
}
}  // namespace clp::streaming_archive::reader
