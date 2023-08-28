#ifndef CLP_FILECOMPRESSOR_TPP
#define CLP_FILECOMPRESSOR_TPP

#include "FileCompressor.hpp"
#include "utils.hpp"

namespace clp {
template <typename encoded_variable_t>
auto FileCompressor::compress_ir_stream_by_encoding(
        size_t target_data_size_of_dicts,
        streaming_archive::writer::Archive::UserConfig& archive_user_config,
        size_t target_encoded_file_size,
        std::string const& path,
        group_id_t group_id,
        streaming_archive::writer::Archive& archive,
        ir::LogEventDeserializer<encoded_variable_t>& log_event_deserializer
) -> std::error_code {
    archive.create_and_open_file(path, group_id, m_uuid_generator(), 0);

    // We assume an IR stream only has one timestamp pattern
    auto timestamp_pattern = log_event_deserializer.get_timestamp_pattern();
    archive.change_ts_pattern(&timestamp_pattern);

    std::error_code error_code{};
    while (true) {
        auto result = log_event_deserializer.deserialize_log_event();
        if (result.has_error()) {
            auto error = result.error();
            if (std::errc::no_message_available != error) {
                error_code = error;
            }
            break;
        }

        // Split archive/encoded file if necessary before writing the new event
        if (archive.get_data_size_of_dictionaries() >= target_data_size_of_dicts) {
            split_file_and_archive(
                    archive_user_config,
                    path,
                    group_id,
                    &timestamp_pattern,
                    archive
            );
        } else if (archive.get_file().get_encoded_size_in_bytes() >= target_encoded_file_size) {
            split_file(path, group_id, &timestamp_pattern, archive);
        }

        archive.write_log_event_ir(result.value());
    }

    close_file_and_append_to_segment(archive);
    return error_code;
}
}  // namespace clp

#endif
