#include "File.hpp"

#include "../../EncodedVariableInterpreter.hpp"

using std::string;
using std::to_string;
using std::unordered_set;
using std::vector;

namespace glt::streaming_archive::writer {
void File::open() {
    if (m_is_open) {
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }
    m_logtypes = std::make_unique<PageAllocatedVector<logtype_dictionary_id_t>>();
    m_offset = std::make_unique<PageAllocatedVector<offset_t>>();
    m_is_open = true;
}

void File::append_to_segment(LogTypeDictionaryWriter const& logtype_dict, Segment& segment) {
    if (m_is_open) {
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }

    // Append files to segment
    uint64_t segment_logtypes_uncompressed_pos;
    segment.append(
            reinterpret_cast<char const*>(m_logtypes->data()),
            m_logtypes->size_in_bytes(),
            segment_logtypes_uncompressed_pos
    );
    uint64_t segment_offset_uncompressed_pos;
    segment.append(
            reinterpret_cast<char const*>(m_offset->data()),
            m_offset->size_in_bytes(),
            segment_offset_uncompressed_pos
    );
    set_segment_metadata(
            segment.get_id(),
            segment_logtypes_uncompressed_pos,
            segment_offset_uncompressed_pos
    );

    // Mark file as written out and clear in-memory columns and clear the in-memory data (except
    // metadata)
    m_logtypes.reset(nullptr);
    m_offset.reset(nullptr);
}

void File::write_encoded_msg(
        epochtime_t timestamp,
        logtype_dictionary_id_t logtype_id,
        offset_t vars_offset,
        size_t num_uncompressed_bytes,
        size_t num_vars
) {
    m_logtypes->push_back(logtype_id);

    // For each file, the offset is only needed for a
    // logtype's first occurrence. else set to 0
    // GLT TODO: create a separate id->first_offset map
    // per file to avoid storing duplicated 0
    if (m_logtype_id_occurance.count(logtype_id) == 0) {
        m_logtype_id_occurance.insert(logtype_id);
        m_offset->push_back(vars_offset);
    } else {
        m_offset->push_back(0);
    }

    // Update metadata
    ++m_num_messages;
    m_num_variables += num_vars;

    if (timestamp < m_begin_ts) {
        m_begin_ts = timestamp;
    }
    if (timestamp > m_end_ts) {
        m_end_ts = timestamp;
    }

    m_num_uncompressed_bytes += num_uncompressed_bytes;
}

void File::change_ts_pattern(TimestampPattern const* pattern) {
    if (nullptr == pattern) {
        m_timestamp_patterns.emplace_back(m_num_messages, TimestampPattern());
    } else {
        m_timestamp_patterns.emplace_back(m_num_messages, *pattern);
    }
}

string File::get_encoded_timestamp_patterns() const {
    string encoded_timestamp_patterns;
    string encoded_timestamp_pattern;

    // TODO We could build this procedurally
    for (auto const& timestamp_pattern : m_timestamp_patterns) {
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

void File::set_segment_metadata(
        segment_id_t segment_id,
        uint64_t segment_logtypes_uncompressed_pos,
        uint64_t segment_offset_uncompressed_pos
) {
    m_segment_id = segment_id;
    m_segment_logtypes_pos = segment_logtypes_uncompressed_pos;
    m_segment_offset_pos = segment_offset_uncompressed_pos;
}
}  // namespace glt::streaming_archive::writer
