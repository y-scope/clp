#include "File.hpp"

// Project headers
#include "../../EncodedVariableInterpreter.hpp"

using std::string;
using std::to_string;
using std::unordered_set;

namespace streaming_archive { namespace writer {
    void File::change_ts_pattern (const TimestampPattern* pattern) {
        if (nullptr == pattern) {
            m_timestamp_patterns.emplace_back(m_num_messages, TimestampPattern());
        } else {
            m_timestamp_patterns.emplace_back(m_num_messages, *pattern);
        }
        m_is_metadata_clean = false;
    }

    bool File::is_in_uncommitted_segment () const {
        return (SegmentationState_MovingToSegment == m_segmentation_state);
    }

    void File::mark_as_in_committed_segment () {
        m_segmentation_state = SegmentationState_InSegment;
    }

    bool File::is_metadata_dirty () const {
        return !m_is_metadata_clean;
    }

    void File::mark_metadata_as_clean () {
        m_is_metadata_clean = true;
    }

    string File::get_encoded_timestamp_patterns () const {
        string encoded_timestamp_patterns;
        string encoded_timestamp_pattern;

        // TODO We could build this procedurally
        for (const auto& timestamp_pattern : m_timestamp_patterns) {
            encoded_timestamp_pattern.assign(to_string(timestamp_pattern.first));
            encoded_timestamp_pattern += ':';
            encoded_timestamp_pattern += to_string(timestamp_pattern.second.get_num_spaces_before_ts());
            encoded_timestamp_pattern += ':';
            encoded_timestamp_pattern += timestamp_pattern.second.get_format();
            encoded_timestamp_pattern += '\n';

            encoded_timestamp_patterns += encoded_timestamp_pattern;
        }

        return encoded_timestamp_patterns;
    }

    void File::append_logtype_and_var_ids_to_segment_sets (const LogTypeDictionaryWriter& logtype_dict, const logtype_dictionary_id_t* logtype_ids,
                                                           size_t num_logtypes, const encoded_variable_t* vars, size_t num_vars,
                                                           unordered_set<logtype_dictionary_id_t>& segment_logtype_ids,
                                                           unordered_set<variable_dictionary_id_t>& segment_var_ids)
    {
        size_t var_ix = 0;
        for (size_t i = 0; i < num_logtypes; ++i) {
            // Add logtype to set
            auto logtype_id = logtype_ids[i];
            segment_logtype_ids.emplace(logtype_id);

            // Get logtype dictionary entry
            auto logtype_dict_entry_ptr = logtype_dict.get_entry(logtype_id);
            auto& logtype_dict_entry = *logtype_dict_entry_ptr;

            // Get number of variables in logtype
            auto msg_num_vars = logtype_dict_entry.get_num_vars();
            if (var_ix + msg_num_vars > num_vars) {
                throw OperationFailed(ErrorCode_Corrupt, __FILENAME__, __LINE__);
            }

            // If variable is a variable dictionary ID, decode it and add it to the set
            for (size_t msg_var_ix = 0; msg_var_ix < msg_num_vars; ++msg_var_ix, ++var_ix) {
                if (LogTypeDictionaryEntry::VarDelim::NonDouble == logtype_dict_entry.get_var_delim(msg_var_ix)) {
                    auto var = vars[var_ix];
                    if (EncodedVariableInterpreter::is_var_dict_id(var)) {
                        segment_var_ids.emplace(EncodedVariableInterpreter::decode_var_dict_id(var));
                    }
                }
            }
        }
    }

    void File::increment_num_uncompressed_bytes (size_t num_bytes) {
        m_num_uncompressed_bytes += num_bytes;
        m_is_metadata_clean = false;
    }

    void File::increment_num_messages_and_variables (size_t num_messages_to_add, size_t num_variables_to_add) {
        m_num_messages += num_messages_to_add;
        m_num_variables += num_variables_to_add;
        m_is_metadata_clean = false;
    }

    void File::set_last_message_timestamp (const epochtime_t timestamp) {
        if (timestamp < m_begin_ts) {
            m_begin_ts = timestamp;
            m_is_metadata_clean = false;
        }
        if (timestamp > m_end_ts) {
            m_end_ts = timestamp;
            m_is_metadata_clean = false;
        }
    }

    void File::set_segment_metadata (segment_id_t segment_id, uint64_t segment_timestamps_uncompressed_pos, uint64_t segment_logtypes_uncompressed_pos,
                                     uint64_t segment_variables_uncompressed_pos)
    {
        m_segment_id = segment_id;
        m_segment_timestamps_pos = segment_timestamps_uncompressed_pos;
        m_segment_logtypes_pos = segment_logtypes_uncompressed_pos;
        m_segment_variables_pos = segment_variables_uncompressed_pos;
        m_is_metadata_clean = false;
    }
} }