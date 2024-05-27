#include "File.hpp"

#include "../../EncodedVariableInterpreter.hpp"

using std::string;
using std::to_string;
using std::unordered_set;
using std::vector;

namespace clp::streaming_archive::writer {
void File::open() {
    if (m_is_written_out) {
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }
    m_timestamps = std::make_unique<PageAllocatedVector<epochtime_t>>();
    m_logtypes = std::make_unique<PageAllocatedVector<logtype_dictionary_id_t>>();
    m_variables = std::make_unique<PageAllocatedVector<encoded_variable_t>>();
    m_is_open = true;
}

void File::append_to_segment(LogTypeDictionaryWriter const& logtype_dict, Segment& segment) {
    if (m_is_open) {
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }

    // Append files to segment
    uint64_t segment_timestamps_uncompressed_pos;
    segment.append(
            reinterpret_cast<char const*>(m_timestamps->data()),
            m_timestamps->size_in_bytes(),
            segment_timestamps_uncompressed_pos
    );
    uint64_t segment_logtypes_uncompressed_pos;
    segment.append(
            reinterpret_cast<char const*>(m_logtypes->data()),
            m_logtypes->size_in_bytes(),
            segment_logtypes_uncompressed_pos
    );
    uint64_t segment_variables_uncompressed_pos;
    segment.append(
            reinterpret_cast<char const*>(m_variables->data()),
            m_variables->size_in_bytes(),
            segment_variables_uncompressed_pos
    );
    set_segment_metadata(
            segment.get_id(),
            segment_timestamps_uncompressed_pos,
            segment_logtypes_uncompressed_pos,
            segment_variables_uncompressed_pos
    );
    m_segmentation_state = SegmentationState_MovingToSegment;

    // Mark file as written out and clear in-memory columns and clear the in-memory data (except
    // metadata)
    m_is_written_out = true;
    m_timestamps.reset(nullptr);
    m_logtypes.reset(nullptr);
    m_variables.reset(nullptr);
}

void File::write_encoded_msg(
        epochtime_t timestamp,
        logtype_dictionary_id_t logtype_id,
        vector<encoded_variable_t> const& encoded_vars,
        vector<variable_dictionary_id_t> const& var_ids,
        size_t num_uncompressed_bytes
) {
    m_timestamps->push_back(timestamp);
    m_logtypes->push_back(logtype_id);
    m_variables->push_back_all(encoded_vars);

    // Update metadata
    ++m_num_messages;
    m_num_variables += encoded_vars.size();

    if (timestamp < m_begin_ts) {
        m_begin_ts = timestamp;
    }
    if (timestamp > m_end_ts) {
        m_end_ts = timestamp;
    }

    m_num_uncompressed_bytes += num_uncompressed_bytes;
    m_is_metadata_clean = false;
}

void File::change_ts_pattern(TimestampPattern const* pattern) {
    if (nullptr == pattern) {
        m_timestamp_patterns.emplace_back(m_num_messages, TimestampPattern());
    } else {
        m_timestamp_patterns.emplace_back(m_num_messages, *pattern);
    }
    m_is_metadata_clean = false;
}

bool File::is_in_uncommitted_segment() const {
    return (SegmentationState_MovingToSegment == m_segmentation_state);
}

void File::mark_as_in_committed_segment() {
    m_segmentation_state = SegmentationState_InSegment;
}

bool File::is_metadata_dirty() const {
    return !m_is_metadata_clean;
}

void File::mark_metadata_as_clean() {
    m_is_metadata_clean = true;
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
        uint64_t segment_timestamps_uncompressed_pos,
        uint64_t segment_logtypes_uncompressed_pos,
        uint64_t segment_variables_uncompressed_pos
) {
    m_segment_id = segment_id;
    m_segment_timestamps_pos = segment_timestamps_uncompressed_pos;
    m_segment_logtypes_pos = segment_logtypes_uncompressed_pos;
    m_segment_variables_pos = segment_variables_uncompressed_pos;
    m_is_metadata_clean = false;
}
}  // namespace clp::streaming_archive::writer
