#include "Archive.hpp"

#include <sys/stat.h>

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <log_surgeon/Constants.hpp>
#include <log_surgeon/LogEvent.hpp>
#include <log_surgeon/LogParser.hpp>
#include <log_surgeon/Token.hpp>
#include <nlohmann/json.hpp>

#include <clp/Defs.h>
#include <clp/EncodedVariableInterpreter.hpp>
#include <clp/ir/LogEvent.hpp>
#include <clp/ir/types.hpp>
#include <clp/streaming_archive/Constants.hpp>
#include <clp/streaming_archive/writer/utils.hpp>
#include <clp/TimestampPattern.hpp>

namespace clp::streaming_archive::writer {
using clp::ir::eight_byte_encoded_variable_t;
using clp::ir::four_byte_encoded_variable_t;
using std::string;
using std::vector;

Archive::~Archive() {
    if (m_path.empty() == false || m_file != nullptr
        || m_files_with_timestamps_in_segment.empty() == false
        || m_files_without_timestamps_in_segment.empty() == false)
    {
        SPDLOG_ERROR("Archive not closed before being destroyed - data loss may occur");
        delete m_file;
        for (auto file : m_files_with_timestamps_in_segment) {
            delete file;
        }
        for (auto file : m_files_without_timestamps_in_segment) {
            delete file;
        }
    }
}

void Archive::open(UserConfig const& user_config) {
    int retval;

    m_id = user_config.id;
    m_id_as_string = boost::uuids::to_string(m_id);
    m_creator_id = user_config.creator_id;
    m_creator_id_as_string = boost::uuids::to_string(m_creator_id);
    m_creation_num = user_config.creation_num;
    m_print_archive_stats_progress = user_config.print_archive_stats_progress;

    std::error_code std_error_code;

    // Ensure path doesn't already exist
    std::filesystem::path archive_path
            = std::filesystem::path(user_config.output_dir) / m_id_as_string;
    bool path_exists = std::filesystem::exists(archive_path, std_error_code);
    if (path_exists) {
        SPDLOG_ERROR("Archive path already exists: {}", archive_path.c_str());
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }
    auto const& archive_path_string = archive_path.string();
    m_local_metadata = std::make_optional<ArchiveMetadata>(
            cArchiveFormatVersion::Version,
            m_creator_id_as_string,
            m_creation_num
    );

    // Create internal directories if necessary
    retval = mkdir(archive_path_string.c_str(), 0750);
    if (0 != retval) {
        SPDLOG_ERROR("Failed to create {}, errno={}", archive_path_string.c_str(), errno);
        throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
    }

    // Get archive directory's file descriptor
    int archive_dir_fd = ::open(archive_path_string.c_str(), O_RDONLY);
    if (-1 == archive_dir_fd) {
        SPDLOG_ERROR(
                "Failed to get file descriptor for {}, errno={}",
                archive_path_string.c_str(),
                errno
        );
        throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
    }

    // Create segments directory
    m_segments_dir_path = archive_path_string;
    m_segments_dir_path += '/';
    m_segments_dir_path += cSegmentsDirname;
    m_segments_dir_path += '/';
    retval = mkdir(m_segments_dir_path.c_str(), 0750);
    if (0 != retval) {
        SPDLOG_ERROR("Failed to create {}, errno={}", m_segments_dir_path.c_str(), errno);
        throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
    }

    // Get segments directory's file descriptor
    m_segments_dir_fd = ::open(m_segments_dir_path.c_str(), O_RDONLY);
    if (-1 == m_segments_dir_fd) {
        SPDLOG_ERROR(
                "Failed to open file descriptor for {}, errno={}",
                m_segments_dir_path.c_str(),
                errno
        );
        throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
    }

    // Create metadata database
    auto metadata_db_path = archive_path / cMetadataDBFileName;
    m_metadata_db.open(metadata_db_path.string());

    m_target_segment_uncompressed_size = user_config.target_segment_uncompressed_size;
    m_next_segment_id = 0;
    m_compression_level = user_config.compression_level;

    /// TODO: add schema file size to m_stable_size???
    // Copy schema file into archive
    if (!m_schema_file_path.empty()) {
        std::filesystem::path const archive_schema_filesystem_path = archive_path / cSchemaFileName;
        try {
            std::filesystem::path const schema_filesystem_path = m_schema_file_path;
            std::filesystem::copy(schema_filesystem_path, archive_schema_filesystem_path);
        } catch (FileWriter::OperationFailed& e) {
            SPDLOG_CRITICAL(
                    "Failed to copy schema file to archive: {}",
                    archive_schema_filesystem_path.c_str()
            );
            throw;
        }
    }

    // Save metadata to disk
    auto metadata_file_path = archive_path / cMetadataFileName;
    try {
        m_metadata_file_writer.open(
                metadata_file_path.string(),
                FileWriter::OpenMode::CREATE_IF_NONEXISTENT_FOR_SEEKABLE_WRITING
        );
        m_local_metadata->write_to_file(m_metadata_file_writer);
        m_metadata_file_writer.flush();
    } catch (FileWriter::OperationFailed& e) {
        SPDLOG_CRITICAL(
                "Failed to write archive file metadata collection in file: {}",
                metadata_file_path.c_str()
        );
        throw;
    }

    m_global_metadata_db = user_config.global_metadata_db;

    m_file = nullptr;

    // Open log-type dictionary
    string logtype_dict_path = archive_path_string + '/' + cLogTypeDictFilename;
    string logtype_dict_segment_index_path
            = archive_path_string + '/' + cLogTypeSegmentIndexFilename;
    m_logtype_dict
            .open(logtype_dict_path, logtype_dict_segment_index_path, cLogtypeDictionaryIdMax);

    // Open variable dictionary
    string var_dict_path = archive_path_string + '/' + cVarDictFilename;
    string var_dict_segment_index_path = archive_path_string + '/' + cVarSegmentIndexFilename;
    m_var_dict.open(var_dict_path, var_dict_segment_index_path, cVariableDictionaryIdMax);

#if FLUSH_TO_DISK_ENABLED
    // fsync archive directory now that everything in the archive directory has been created
    if (fsync(archive_dir_fd) != 0) {
        SPDLOG_ERROR("Failed to fsync {}, errno={}", archive_path_string.c_str(), errno);
        throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
    }
#endif
    if (::close(archive_dir_fd) != 0) {
        // We've already fsynced, so this error shouldn't affect us. Therefore, just log it.
        SPDLOG_WARN(
                "Error when closing file descriptor for {}, errno={}",
                archive_path_string.c_str(),
                errno
        );
    }

    m_path = archive_path_string;
}

void Archive::close() {
    // The file should have been closed and persisted before closing the archive.
    if (m_file != nullptr) {
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }

    // Close segments if necessary
    if (m_segment_for_files_with_timestamps.is_open()) {
        close_segment_and_persist_file_metadata(
                m_segment_for_files_with_timestamps,
                m_files_with_timestamps_in_segment,
                m_logtype_ids_in_segment_for_files_with_timestamps,
                m_var_ids_in_segment_for_files_with_timestamps
        );
        m_logtype_ids_in_segment_for_files_with_timestamps.clear();
        m_var_ids_in_segment_for_files_with_timestamps.clear();
    }
    if (m_segment_for_files_without_timestamps.is_open()) {
        close_segment_and_persist_file_metadata(
                m_segment_for_files_without_timestamps,
                m_files_without_timestamps_in_segment,
                m_logtype_ids_in_segment_for_files_without_timestamps,
                m_var_ids_in_segment_for_files_without_timestamps
        );
        m_logtype_ids_in_segment_for_files_without_timestamps.clear();
        m_var_ids_in_segment_for_files_without_timestamps.clear();
    }

    // Persist all metadata including dictionaries
    write_dir_snapshot();

    m_logtype_dict.close();
    m_logtype_dict_entry.clear();
    m_var_dict.close();

    if (::close(m_segments_dir_fd) != 0) {
        // We've already fsynced, so this error shouldn't affect us. Therefore, just log it.
        SPDLOG_WARN("Error when closing segments directory file descriptor, errno={}", errno);
    }
    m_segments_dir_fd = -1;
    m_segments_dir_path.clear();

    m_metadata_file_writer.close();

    update_global_metadata();
    m_global_metadata_db = nullptr;

    for (auto* file : m_file_metadata_for_global_update) {
        delete file;
    }
    m_file_metadata_for_global_update.clear();

    if (m_print_archive_stats_progress) {
        print_archive_stats_progress();
    }

    m_metadata_db.close();

    m_creator_id_as_string.clear();
    m_id_as_string.clear();
    m_path.clear();
}

void Archive::create_and_open_file(
        string const& path,
        group_id_t const group_id,
        boost::uuids::uuid const& orig_file_id,
        size_t split_ix,
        size_t begin_message_ix
) {
    if (m_file != nullptr) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }
    m_file = new File(m_uuid_generator(), orig_file_id, path, group_id, split_ix, begin_message_ix);
    m_file->open();
}

void Archive::close_file() {
    if (m_file == nullptr) {
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }
    m_file->close();
}

File const& Archive::get_file() const {
    if (m_file == nullptr) {
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }
    return *m_file;
}

void Archive::set_file_is_split(bool is_split) {
    if (m_file == nullptr) {
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }
    m_file->set_is_split(is_split);
}

void Archive::change_ts_pattern(TimestampPattern const* pattern) {
    if (m_file == nullptr) {
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }
    m_file->change_ts_pattern(pattern);
}

void
Archive::write_msg(epochtime_t timestamp, string const& message, size_t num_uncompressed_bytes) {
    // Encode message and add components to dictionaries
    vector<encoded_variable_t> encoded_vars;
    vector<variable_dictionary_id_t> var_ids;
    EncodedVariableInterpreter::encode_and_add_to_dictionary(
            message,
            m_logtype_dict_entry,
            m_var_dict,
            encoded_vars,
            var_ids
    );
    logtype_dictionary_id_t logtype_id;
    m_logtype_dict.add_entry(m_logtype_dict_entry, logtype_id);

    m_file->write_encoded_msg(timestamp, logtype_id, encoded_vars, var_ids, num_uncompressed_bytes);

    update_segment_indices(logtype_id, var_ids);
}

auto Archive::add_token_to_dicts(
        log_surgeon::LogEventView const& log_view,
        log_surgeon::Token token_view
) -> void {
    auto const* type_ids{token_view.get_type_ids()};
    if (nullptr == type_ids || type_ids->empty()) {
        throw std::runtime_error("Token has no type IDs: " + token_view.to_string());
    }
    auto const token_type{type_ids->at(0)};
    switch (token_type) {
        case static_cast<int>(log_surgeon::SymbolId::TokenNewline):
        case static_cast<int>(log_surgeon::SymbolId::TokenUncaughtString): {
            m_logtype_dict_entry.add_static_text(token_view.to_string());
            break;
        }
        case static_cast<int>(log_surgeon::SymbolId::TokenInt): {
            encoded_variable_t encoded_var{};
            if (false
                == EncodedVariableInterpreter::convert_string_to_representable_integer_var(
                        token_view.to_string(),
                        encoded_var
                ))
            {
                variable_dictionary_id_t id{};
                m_var_dict.add_entry(token_view.to_string(), id);
                m_var_ids.push_back(id);
                encoded_var = EncodedVariableInterpreter::encode_var_dict_id(id);
                m_logtype_dict_entry.add_dictionary_var();
            } else {
                m_logtype_dict_entry.add_int_var();
            }
            m_encoded_vars.push_back(encoded_var);
            break;
        }
        case static_cast<int>(log_surgeon::SymbolId::TokenFloat): {
            encoded_variable_t encoded_var{};
            if (false
                == EncodedVariableInterpreter::convert_string_to_representable_float_var(
                        token_view.to_string(),
                        encoded_var
                ))
            {
                variable_dictionary_id_t id{};
                m_var_dict.add_entry(token_view.to_string(), id);
                m_var_ids.push_back(id);
                encoded_var = EncodedVariableInterpreter::encode_var_dict_id(id);
                m_logtype_dict_entry.add_dictionary_var();
            } else {
                m_logtype_dict_entry.add_float_var();
            }
            m_encoded_vars.push_back(encoded_var);
            break;
        }
        default: {
            // If there are no capture groups the entire variable token is stored as a variable.
            // If the variable token contains capture groups, we break the token up by storing
            // each capture as a variable and any substrings surrounding the capture as part of
            // the logtype. Capture repetition currently does not work so we explicitly only
            // store the first capture.

            auto const& lexer{log_view.get_log_parser().m_lexer};
            auto captures{lexer.get_captures_from_rule_id(token_type)};
            if (false == captures.has_value()) {
                variable_dictionary_id_t id{};
                m_var_dict.add_entry(token_view.to_string(), id);
                m_var_ids.push_back(id);
                m_encoded_vars.push_back(EncodedVariableInterpreter::encode_var_dict_id(id));
                m_logtype_dict_entry.add_dictionary_var();
                break;
            }

            auto capture_pos_result{
                    log_view.get_capture_position(token_view, captures.value().at(0))
            };
            if (capture_pos_result.has_error()) {
                SPDLOG_ERROR(
                        "Capture group match not found for token {} ({} {}). Storing token as "
                        "static text.",
                        log_view.get_log_parser().get_id_symbol(token_type),
                        capture_pos_result.error().category().name(),
                        capture_pos_result.error().message()
                );
                m_logtype_dict_entry.add_static_text(token_view.to_string_view());
                break;
            }
            auto const [capture_start_pos, capture_end_pos]{capture_pos_result.value()};

            auto const before_capture{
                    token_view.get_sub_token(token_view.get_start_pos(), capture_start_pos)
                            .to_string_view()
            };
            m_logtype_dict_entry.add_static_text(before_capture);

            variable_dictionary_id_t id{};
            m_var_dict.add_entry(
                    token_view.get_sub_token(capture_start_pos, capture_end_pos).to_string_view(),
                    id
            );
            m_var_ids.push_back(id);
            m_encoded_vars.push_back(EncodedVariableInterpreter::encode_var_dict_id(id));
            m_logtype_dict_entry.add_dictionary_var();

            auto const after_capture{
                    token_view.get_sub_token(capture_end_pos, token_view.get_end_pos())
                            .to_string_view()
            };
            m_logtype_dict_entry.add_static_text(after_capture);
            break;
        }
    }
}

auto Archive::write_msg_using_schema(log_surgeon::LogEventView const& log_view) -> void {
    epochtime_t timestamp{0};
    TimestampPattern const* timestamp_pattern{nullptr};
    auto const& log_buf{log_view.get_log_output_buffer()};
    if (log_buf->has_header()) {
        size_t start{};
        size_t end{};
        timestamp_pattern = TimestampPattern::search_known_ts_patterns(
                log_buf->get_mutable_token(0).to_string(),
                timestamp,
                start,
                end
        );
        if (nullptr == timestamp_pattern) {
            throw(std::runtime_error(
                    "Schema contains a timestamp regex that matches "
                    + log_buf->get_mutable_token(0).to_string()
                    + " which does not match any known timestamp pattern."
            ));
        }
        if (m_old_ts_pattern != timestamp_pattern) {
            change_ts_pattern(timestamp_pattern);
            m_old_ts_pattern = const_cast<TimestampPattern*>(timestamp_pattern);
        }
    } else {
        timestamp_pattern = nullptr;
    }
    if (get_data_size_of_dictionaries() >= m_target_data_size_of_dicts) {
        split_file_and_archive(
                m_archive_user_config,
                m_path_for_compression,
                m_group_id,
                timestamp_pattern,
                *this
        );
    } else if (m_file->get_encoded_size_in_bytes() >= m_target_encoded_file_size) {
        split_file(m_path_for_compression, m_group_id, timestamp_pattern, *this);
    }
    m_encoded_vars.clear();
    m_var_ids.clear();
    m_logtype_dict_entry.clear();

    size_t num_uncompressed_bytes{0};
    // Timestamp is included in the uncompressed message size
    auto start_pos{log_buf->get_token(0).get_start_pos()};
    if (timestamp_pattern == nullptr) {
        start_pos = log_buf->get_token(1).get_start_pos();
    }
    auto const end_pos{log_buf->get_token(log_buf->pos() - 1).get_end_pos()};
    if (start_pos <= end_pos) {
        num_uncompressed_bytes = end_pos - start_pos;
    } else {
        num_uncompressed_bytes = log_buf->get_token(0).get_buffer_size() - start_pos + end_pos;
    }
    for (auto token_idx{1}; token_idx < log_buf->pos(); token_idx++) {
        auto token_view{log_buf->get_token(token_idx)};
        auto const* type_ids{token_view.get_type_ids()};
        if (nullptr == type_ids || type_ids->empty()) {
            throw std::runtime_error("Token has no type IDs: " + token_view.to_string());
        }
        auto const token_type{type_ids->at(0)};
        if (log_buf->has_delimiters() && (timestamp_pattern != nullptr || token_idx > 1)
            && token_type != static_cast<int>(log_surgeon::SymbolId::TokenUncaughtString)
            && token_type != static_cast<int>(log_surgeon::SymbolId::TokenNewline))
        {
            m_logtype_dict_entry.add_static_text(token_view.get_delimiter());
            token_view.increment_start_pos();
        }
        add_token_to_dicts(log_view, token_view);
    }
    if (false == m_logtype_dict_entry.get_value().empty()) {
        logtype_dictionary_id_t logtype_id{};
        m_logtype_dict.add_entry(m_logtype_dict_entry, logtype_id);
        m_file->write_encoded_msg(
                timestamp,
                logtype_id,
                m_encoded_vars,
                m_var_ids,
                num_uncompressed_bytes
        );

        update_segment_indices(logtype_id, m_var_ids);
    }
}

template <typename encoded_variable_t>
void Archive::write_log_event_ir(ir::LogEvent<encoded_variable_t> const& log_event) {
    vector<eight_byte_encoded_variable_t> encoded_vars;
    vector<variable_dictionary_id_t> var_ids;
    size_t original_num_bytes{0};
    EncodedVariableInterpreter::encode_and_add_to_dictionary(
            log_event.get_message(),
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

    update_segment_indices(logtype_id, var_ids);
}

void Archive::write_dir_snapshot() {
    // Flush dictionaries
    m_logtype_dict.write_header_and_flush_to_disk();
    m_var_dict.write_header_and_flush_to_disk();
}

void Archive::update_segment_indices(
        logtype_dictionary_id_t logtype_id,
        vector<variable_dictionary_id_t> const& var_ids
) {
    if (m_file->has_ts_pattern()) {
        m_logtype_ids_in_segment_for_files_with_timestamps.insert(logtype_id);
        m_var_ids_in_segment_for_files_with_timestamps.insert_all(var_ids);
    } else {
        m_logtype_ids_for_file_with_unassigned_segment.insert(logtype_id);
        m_var_ids_for_file_with_unassigned_segment.insert(var_ids.cbegin(), var_ids.cend());
    }
}

void Archive::append_file_contents_to_segment(
        Segment& segment,
        ArrayBackedPosIntSet<logtype_dictionary_id_t>& logtype_ids_in_segment,
        ArrayBackedPosIntSet<variable_dictionary_id_t>& var_ids_in_segment,
        vector<File*>& files_in_segment
) {
    if (!segment.is_open()) {
        segment.open(m_segments_dir_path, m_next_segment_id++, m_compression_level);
    }

    m_file->append_to_segment(m_logtype_dict, segment);
    files_in_segment.emplace_back(m_file);
    m_local_metadata->increment_static_uncompressed_size(m_file->get_num_uncompressed_bytes());
    m_local_metadata->expand_time_range(m_file->get_begin_ts(), m_file->get_end_ts());

    // Close current segment if its uncompressed size is greater than the target
    if (segment.get_uncompressed_size() >= m_target_segment_uncompressed_size) {
        close_segment_and_persist_file_metadata(
                segment,
                files_in_segment,
                logtype_ids_in_segment,
                var_ids_in_segment
        );
        logtype_ids_in_segment.clear();
        var_ids_in_segment.clear();
    }
}

void Archive::append_file_to_segment() {
    if (m_file == nullptr) {
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }

    if (m_file->has_ts_pattern()) {
        m_logtype_ids_in_segment_for_files_with_timestamps.insert_all(
                m_logtype_ids_for_file_with_unassigned_segment
        );
        m_var_ids_in_segment_for_files_with_timestamps.insert_all(
                m_var_ids_for_file_with_unassigned_segment
        );
        append_file_contents_to_segment(
                m_segment_for_files_with_timestamps,
                m_logtype_ids_in_segment_for_files_with_timestamps,
                m_var_ids_in_segment_for_files_with_timestamps,
                m_files_with_timestamps_in_segment
        );
    } else {
        m_logtype_ids_in_segment_for_files_without_timestamps.insert_all(
                m_logtype_ids_for_file_with_unassigned_segment
        );
        m_var_ids_in_segment_for_files_without_timestamps.insert_all(
                m_var_ids_for_file_with_unassigned_segment
        );
        append_file_contents_to_segment(
                m_segment_for_files_without_timestamps,
                m_logtype_ids_in_segment_for_files_without_timestamps,
                m_var_ids_in_segment_for_files_without_timestamps,
                m_files_without_timestamps_in_segment
        );
    }
    m_logtype_ids_for_file_with_unassigned_segment.clear();
    m_var_ids_for_file_with_unassigned_segment.clear();
    // Make sure file pointer is nulled and cannot be accessed outside
    m_file = nullptr;
}

void Archive::persist_file_metadata(vector<File*> const& files) {
    if (files.empty()) {
        return;
    }

    m_metadata_db.update_files(files);

    // Mark files' metadata as clean
    for (auto file : files) {
        file->mark_metadata_as_clean();
    }
}

void Archive::close_segment_and_persist_file_metadata(
        Segment& segment,
        std::vector<File*>& files,
        ArrayBackedPosIntSet<logtype_dictionary_id_t>& segment_logtype_ids,
        ArrayBackedPosIntSet<variable_dictionary_id_t>& segment_var_ids
) {
    auto segment_id = segment.get_id();
    m_logtype_dict.index_segment(segment_id, segment_logtype_ids);
    m_var_dict.index_segment(segment_id, segment_var_ids);

    segment.close();

    m_local_metadata->increment_static_compressed_size(segment.get_compressed_size());

#if FLUSH_TO_DISK_ENABLED
    // fsync segments directory to flush segment's directory entry
    if (fsync(m_segments_dir_fd) != 0) {
        SPDLOG_ERROR("Failed to fsync {}, errno={}", m_segments_dir_path.c_str(), errno);
        throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
    }
#endif

    // Flush dictionaries
    m_logtype_dict.write_header_and_flush_to_disk();
    m_var_dict.write_header_and_flush_to_disk();

    for (auto file : files) {
        file->mark_as_in_committed_segment();
        m_file_metadata_for_global_update.emplace_back(file);
    }

    persist_file_metadata(files);
    update_local_metadata();

    files.clear();
}

void Archive::add_empty_directories(vector<string> const& empty_directory_paths) {
    if (empty_directory_paths.empty()) {
        return;
    }

    m_metadata_db.add_empty_directories(empty_directory_paths);
}

uint64_t Archive::get_dynamic_compressed_size() {
    uint64_t on_disk_size = m_logtype_dict.get_on_disk_size() + m_var_dict.get_on_disk_size();

    // Add size of unclosed segments
    if (m_segment_for_files_with_timestamps.is_open()) {
        on_disk_size += m_segment_for_files_with_timestamps.get_compressed_size();
    }
    if (m_segment_for_files_without_timestamps.is_open()) {
        on_disk_size += m_segment_for_files_without_timestamps.get_compressed_size();
    }

    return on_disk_size;
}

auto Archive::print_archive_stats_progress() -> void {
    nlohmann::json json_msg;
    json_msg["id"] = m_id_as_string;
    json_msg["uncompressed_size"] = m_local_metadata->get_uncompressed_size_bytes();
    json_msg["size"] = m_local_metadata->get_compressed_size_bytes();
    std::cout << json_msg.dump(-1, ' ', true, nlohmann::json::error_handler_t::ignore) << std::endl;
}

void Archive::update_local_metadata() {
    m_local_metadata->set_dynamic_uncompressed_size(0);
    m_local_metadata->set_dynamic_compressed_size(get_dynamic_compressed_size());
    // Rewrite (overwrite) the metadata file
    m_metadata_file_writer.seek_from_begin(0);
    m_local_metadata->write_to_file(m_metadata_file_writer);
}

auto Archive::update_global_metadata() -> void {
    m_global_metadata_db->open();
    if (false == m_local_metadata.has_value()) {
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_global_metadata_db->add_archive(m_id_as_string, m_local_metadata.value());
    m_global_metadata_db->update_metadata_for_files(
            m_id_as_string,
            m_file_metadata_for_global_update
    );
    m_global_metadata_db->close();
}

// Explicitly declare template specializations so that we can define the template methods in this
// file
template void Archive::write_log_event_ir<eight_byte_encoded_variable_t>(
        ir::LogEvent<eight_byte_encoded_variable_t> const& log_event
);
template void Archive::write_log_event_ir<four_byte_encoded_variable_t>(
        ir::LogEvent<four_byte_encoded_variable_t> const& log_event
);
}  // namespace clp::streaming_archive::writer
