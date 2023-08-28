#ifndef STREAMING_ARCHIVE_WRITER_ARCHIVE_TPP
#define STREAMING_ARCHIVE_WRITER_ARCHIVE_TPP

#include <vector>

#include "../../EncodedVariableInterpreter.hpp"
#include "../../ir/LogEvent.hpp"

namespace streaming_archive::writer {
template <typename encoded_variable_t>
void Archive::write_log_event_ir(ir::LogEvent<encoded_variable_t> const& log_event) {
    std::vector<ffi::eight_byte_encoded_variable_t> encoded_vars;
    std::vector<variable_dictionary_id_t> var_ids;
    size_t original_num_bytes{0};
    EncodedVariableInterpreter::encode_and_add_to_dictionary(
            log_event,
            m_logtype_dict_entry,
            m_var_dict,
            encoded_vars,
            var_ids,
            original_num_bytes
    );

    logtype_dictionary_id_t logtype_id{cLogtypeDictionaryIdMax};
    m_logtype_dict.add_entry(m_logtype_dict_entry, logtype_id);

    m_file->write_encoded_msg(
            log_event.get_timestamp(),
            logtype_id,
            encoded_vars,
            var_ids,
            original_num_bytes
    );

    // TODO deduplicate
    // Update segment indices
    if (m_file->has_ts_pattern()) {
        m_logtype_ids_in_segment_for_files_with_timestamps.insert(logtype_id);
        m_var_ids_in_segment_for_files_with_timestamps.insert_all(var_ids);
    } else {
        m_logtype_ids_for_file_with_unassigned_segment.insert(logtype_id);
        m_var_ids_for_file_with_unassigned_segment.insert(var_ids.cbegin(), var_ids.cend());
    }
}
}  // namespace streaming_archive::writer

#endif  // STREAMING_ARCHIVE_WRITER_ARCHIVE_TPP
