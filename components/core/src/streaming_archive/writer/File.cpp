#include "File.hpp"

// Project headers
#include "../../EncodedVariableInterpreter.hpp"

using std::string;
using std::to_string;
using std::unordered_set;
using std::vector;

namespace streaming_archive::writer {
    void File::open () {
        if (m_is_written_out) {
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }
        m_is_open = true;
        open_derived();
    }

    void File::change_ts_pattern (const TimestampPattern* pattern) {
        if (nullptr == pattern) {
            m_timestamp_patterns.emplace_back(m_num_messages, TimestampPattern());
        } else {
            m_timestamp_patterns.emplace_back(m_num_messages, *pattern);
        }
        m_is_metadata_clean = false;
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
}
