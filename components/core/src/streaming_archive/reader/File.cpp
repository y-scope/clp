#include "File.hpp"

// C libraries
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// C++ libraries
#include <cassert>

// spdlog
#include <spdlog/spdlog.h>

// Project headers
#include "../../EncodedVariableInterpreter.hpp"
#include "../../Profiler.hpp"
#include "../../Stopwatch.hpp"
#include "../../Utils.hpp"
#include "../Constants.hpp"
#include "SegmentManager.hpp"

using namespace std;

namespace streaming_archive { namespace reader {
    epochtime_t File::get_begin_ts () const {
        return m_begin_ts;
    }
    epochtime_t File::get_end_ts () const {
        return m_end_ts;
    }

    ErrorCode File::open_me (const LogTypeDictionaryReader& archive_logtype_dict, MetadataDB::FileIterator& file_metadata_ix, bool read_ahead,
            const string& archive_logs_dir_path, SegmentManager& segment_manager)
    {
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
                    forward_as_tuple(num_spaces_before_ts, timestamp_format));
        }

        m_num_messages = file_metadata_ix.get_num_messages();
        m_num_variables = file_metadata_ix.get_num_variables();

        m_segment_id = file_metadata_ix.get_segment_id();
        m_is_in_segment = (cInvalidSegmentId != m_segment_id);
        if (m_is_in_segment) {
            m_segment_timestamps_decompressed_stream_pos = file_metadata_ix.get_segment_timestamps_pos();
            m_segment_logtypes_decompressed_stream_pos = file_metadata_ix.get_segment_logtypes_pos();
            m_segment_variables_decompressed_stream_pos = file_metadata_ix.get_segment_variables_pos();
        }

        m_is_split = file_metadata_ix.is_split();
        m_split_ix = file_metadata_ix.get_split_ix();

        ErrorCode error_code;

        if (m_is_in_segment) {
            uint64_t num_bytes_to_read;
            Stopwatch segment_read_stopwatch;
            Stopwatch columns_alloc_stopwatch;

            if (m_num_messages > 0) {
                if (m_num_messages > m_num_segment_msgs) {
                    // Buffers too small, so increase size to required amount
                    PROFILER_START_STOPWATCH(columns_alloc_stopwatch)
                    m_segment_timestamps = make_unique<epochtime_t[]>(m_num_messages);
                    m_segment_logtypes = make_unique<logtype_dictionary_id_t[]>(m_num_messages);
                    PROFILER_STOP_STOPWATCH(columns_alloc_stopwatch)
                    m_num_segment_msgs = m_num_messages;
                }

                num_bytes_to_read = m_num_messages*sizeof(epochtime_t);
                PROFILER_START_STOPWATCH(segment_read_stopwatch)
                error_code = segment_manager.try_read(m_segment_id, m_segment_timestamps_decompressed_stream_pos,
                                                      reinterpret_cast<char*>(m_segment_timestamps.get()), num_bytes_to_read);
                PROFILER_STOP_STOPWATCH(segment_read_stopwatch)
                if (ErrorCode_Success != error_code) {
                    close_me();
                    return error_code;
                }
                m_timestamps = m_segment_timestamps.get();

                num_bytes_to_read = m_num_messages*sizeof(logtype_dictionary_id_t);
                PROFILER_START_STOPWATCH(segment_read_stopwatch)
                error_code = segment_manager.try_read(m_segment_id, m_segment_logtypes_decompressed_stream_pos,
                                                      reinterpret_cast<char*>(m_segment_logtypes.get()), num_bytes_to_read);
                PROFILER_STOP_STOPWATCH(segment_read_stopwatch)
                if (ErrorCode_Success != error_code) {
                    close_me();
                    return error_code;
                }
                m_logtypes = m_segment_logtypes.get();
            }

            if (m_num_variables > 0) {
                if (m_num_variables > m_num_segment_vars) {
                    // Buffer too small, so increase size to required amount
                    PROFILER_START_STOPWATCH(columns_alloc_stopwatch)
                    m_segment_variables = make_unique<encoded_variable_t[]>(m_num_variables);
                    PROFILER_STOP_STOPWATCH(columns_alloc_stopwatch)
                    m_num_segment_vars = m_num_variables;
                }
                num_bytes_to_read = m_num_variables*sizeof(encoded_variable_t);
                PROFILER_START_STOPWATCH(segment_read_stopwatch)
                error_code = segment_manager.try_read(m_segment_id, m_segment_variables_decompressed_stream_pos,
                                                      reinterpret_cast<char*>(m_segment_variables.get()), num_bytes_to_read);
                PROFILER_STOP_STOPWATCH(segment_read_stopwatch)
                if (ErrorCode_Success != error_code) {
                    close_me();
                    return error_code;
                }
                m_variables = m_segment_variables.get();
            }

            PROFILER_FRAGMENTED_MEASUREMENT_INCREMENT(SegmentRead, segment_read_stopwatch.get_time_taken_in_nanoseconds())
            PROFILER_FRAGMENTED_MEASUREMENT_INCREMENT(ColumnsAlloc, columns_alloc_stopwatch.get_time_taken_in_nanoseconds())
        } else {
            void* ptr;
            string file_path = archive_logs_dir_path + m_id_as_string;
            string column_path;

            // Open timestamps file
            column_path = file_path;
            column_path += cTimestampsFileExtension;
            error_code = memory_map_file(column_path, read_ahead, m_timestamps_fd, m_timestamps_file_size, ptr);
            if (ErrorCode_Success != error_code) {
                close_me();
                return error_code;
            }
            m_timestamps = reinterpret_cast<epochtime_t*>(ptr);
            size_t num_timestamps_read = m_timestamps_file_size / sizeof(epochtime_t);
            if (num_timestamps_read < m_num_messages) {
                SPDLOG_ERROR("There are fewer timestamps on disk ({}) than the metadata ({}) indicates.", num_timestamps_read, m_num_messages);
                close_me();
                return ErrorCode_Truncated;
            }

            // Open logtype IDs file
            column_path = file_path;
            column_path += cLogTypeIdsFileExtension;
            error_code = memory_map_file(column_path, read_ahead, m_logtypes_fd, m_logtypes_file_size, ptr);
            if (ErrorCode_Success != error_code) {
                close_me();
                return error_code;
            }
            m_logtypes = reinterpret_cast<logtype_dictionary_id_t*>(ptr);
            size_t num_logtypes_read = m_logtypes_file_size / sizeof(logtype_dictionary_id_t);
            if (num_logtypes_read < m_num_messages) {
                SPDLOG_ERROR("There are fewer logtypes on disk ({}) than the metadata ({}) indicates.", num_logtypes_read, m_num_messages);
                close_me();
                return ErrorCode_Truncated;
            }

            // Open variables file
            column_path = file_path;
            column_path += cVariablesFileExtension;
            error_code = memory_map_file(column_path, read_ahead, m_variables_fd, m_variables_file_size, ptr);
            if (ErrorCode_Success != error_code) {
                close_me();
                return error_code;
            }
            m_variables = reinterpret_cast<encoded_variable_t*>(ptr);
            size_t num_variables_read = m_variables_file_size / sizeof(encoded_variable_t);
            if (num_variables_read < m_num_variables) {
                SPDLOG_ERROR("There are fewer variables on disk ({}) than the metadata ({}) indicates.", num_variables_read, m_num_variables);
                close_me();
                return ErrorCode_Truncated;
            }
        }

        m_msgs_ix = 0;
        m_variables_ix = 0;

        m_current_ts_pattern_ix = 0;
        m_current_ts_in_milli = m_begin_ts;

        return ErrorCode_Success;
    }

    void File::close_me () {
        ErrorCode error_code;

        if (m_is_in_segment) {
            m_timestamps = nullptr;
            m_logtypes = nullptr;
            m_variables = nullptr;

            m_segment_timestamps_decompressed_stream_pos = 0;
            m_segment_logtypes_decompressed_stream_pos = 0;
            m_segment_variables_decompressed_stream_pos = 0;

            m_is_in_segment = false;
        } else {
            // Unmap variables file
            if (0 != m_variables_file_size) {
                error_code = memory_unmap_file(m_variables_fd, m_variables_file_size, m_variables);
                if (ErrorCode_Success != error_code) {
                    SPDLOG_ERROR("streaming_archive::reader::File: Failed to unmap variables file, errno={}", errno);
                }
                m_variables_fd = -1;
                m_variables_file_size = 0;
                m_variables = nullptr;
            }

            // Unmap logtypes file
            if (0 != m_logtypes_file_size) {
                error_code = memory_unmap_file(m_logtypes_fd, m_logtypes_file_size, m_logtypes);
                if (ErrorCode_Success != error_code) {
                    SPDLOG_ERROR("streaming_archive::reader::File: Failed to unmap logtypes file, errno={}", errno);
                }
                m_logtypes_fd = -1;
                m_logtypes_file_size = 0;
                m_logtypes = nullptr;
            }

            // Unmap timestamps file
            if (0 != m_timestamps_file_size) {
                error_code = memory_unmap_file(m_timestamps_fd, m_timestamps_file_size, m_timestamps);
                if (ErrorCode_Success != error_code) {
                    SPDLOG_ERROR("streaming_archive::reader::File: Failed to unmap timestamps file, errno={}", errno);
                }
                m_timestamps_fd = -1;
                m_timestamps_file_size = 0;
                m_timestamps = nullptr;
            }
        }

        m_msgs_ix = 0;
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

    void File::reset_indices () {
        m_msgs_ix = 0;
        m_variables_ix = 0;
    }

    const string& File::get_orig_path () const {
        return m_orig_path;
    }

    const vector<pair<uint64_t, TimestampPattern>>& File::get_timestamp_patterns () const {
        return m_timestamp_patterns;
    }

    epochtime_t File::get_current_ts_in_milli () const {
        return m_current_ts_in_milli;
    }
    size_t File::get_current_ts_pattern_ix () const {
        return m_current_ts_pattern_ix;
    }

    void File::increment_current_ts_pattern_ix () {
        ++m_current_ts_pattern_ix;
    }

    bool File::find_message_in_time_range (epochtime_t search_begin_timestamp, epochtime_t search_end_timestamp, Message& msg) {
        bool found_msg = false;
        while (m_msgs_ix < m_num_messages && !found_msg) {
            // Get logtype
            // NOTE: We get the logtype before the timestamp since we need to use it to get the number of variables, and then advance the variable index,
            // regardless of whether the timestamp falls in the time range or not
            auto logtype_id = m_logtypes[m_msgs_ix];

            // Get number of variables in logtype
            const auto& logtype_dictionary_entry = m_archive_logtype_dict->get_entry(logtype_id);
            auto num_vars = logtype_dictionary_entry.get_num_vars();

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
                msg.set_message_number(m_msgs_ix);

                found_msg = true;
            }

            // Advance indices
            ++m_msgs_ix;
            m_variables_ix += num_vars;
        }

        return found_msg;
    }

    const SubQuery* File::find_message_matching_query (const Query& query, Message& msg) {
        const SubQuery* matching_sub_query = nullptr;
        while (m_msgs_ix < m_num_messages && nullptr == matching_sub_query) {
            auto logtype_id = m_logtypes[m_msgs_ix];

            // Get number of variables in logtype
            const auto& logtype_dictionary_entry = m_archive_logtype_dict->get_entry(logtype_id);
            auto num_vars = logtype_dictionary_entry.get_num_vars();

            for (auto sub_query : query.get_relevant_sub_queries()) {
                // Check if logtype matches search
                if (sub_query->matches_logtype(logtype_id)) {
                    // Check if timestamp matches
                    auto timestamp = m_timestamps[m_msgs_ix];
                    if (query.timestamp_is_in_search_time_range(timestamp)) {
                        // Get variables
                        if (m_variables_ix + num_vars > m_num_variables) {
                            // Logtypes not in sync with variables, so stop search
                            return nullptr;
                        }

                        msg.clear_vars();
                        auto vars_ix = m_variables_ix;
                        for (size_t i = 0; i < num_vars; ++i) {
                            auto var = m_variables[vars_ix];
                            ++vars_ix;
                            msg.add_var(var);
                        }

                        // Check if variables match
                        if (sub_query->matches_vars(msg.get_vars())) {
                            // Message matches completely, so set remaining properties
                            msg.set_logtype_id(logtype_id);
                            msg.set_timestamp(timestamp);
                            msg.set_message_number(m_msgs_ix);

                            matching_sub_query = sub_query;
                            break;
                        }
                    }
                }
            }

            // Advance indices
            ++m_msgs_ix;
            m_variables_ix += num_vars;
        }

        return matching_sub_query;
    }

    bool File::get_next_message (Message& msg) {
        if (m_msgs_ix >= m_num_messages) {
            return false;
        }

        // Get message number
        msg.set_message_number(m_msgs_ix);

        // Get timestamp
        msg.set_timestamp(m_timestamps[m_msgs_ix]);

        // Get log-type
        auto logtype_id = m_logtypes[m_msgs_ix];
        msg.set_logtype_id(logtype_id);

        // Get variables
        msg.clear_vars();
        const auto& logtype_dictionary_entry = m_archive_logtype_dict->get_entry(logtype_id);
        auto num_vars = logtype_dictionary_entry.get_num_vars();
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
} }