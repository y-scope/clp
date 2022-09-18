#include "Archive.hpp"
#include "../../clp/utils.hpp"

// C libraries
#include <sys/stat.h>

// C++ libraries
#include <iostream>
#include <fstream>
#include <filesystem>

// Boost libraries
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

// json
#include "../../../submodules/json/single_include/nlohmann/json.hpp"

// spdlog
#include <spdlog/spdlog.h>

// Project headers
#include "../../EncodedVariableInterpreter.hpp"
#include "../../Profiler.hpp"
#include "../../Utils.hpp"
#include "../Constants.hpp"
#include "../../compressor_frontend/LogParser.hpp"

using std::list;
using std::make_unique;
using std::string;
using std::unordered_set;
using std::vector;

namespace streaming_archive::writer {
    Archive::~Archive () {
        if (m_path.empty() == false || m_file != nullptr || m_files_with_timestamps_in_segment.empty() == false ||
                m_files_without_timestamps_in_segment.empty() == false)
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

    std::string Archive::open (const UserConfig& user_config) {
        int retval;

        m_id = user_config.id;
        m_id_as_string = boost::uuids::to_string(m_id);
        m_creator_id = user_config.creator_id;
        m_creator_id_as_string = boost::uuids::to_string(m_creator_id);
        m_creation_num = user_config.creation_num;
        m_print_archive_stats_progress = user_config.print_archive_stats_progress;

        std::error_code std_error_code;

        // Ensure path doesn't already exist
        std::filesystem::path archive_path = std::filesystem::path(user_config.output_dir) / m_id_as_string;
        bool path_exists = std::filesystem::exists(archive_path, std_error_code);
        if (path_exists) {
            SPDLOG_ERROR("Archive path already exists: {}", archive_path.c_str());
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }
        const auto& archive_path_string = archive_path.string();
        m_stable_uncompressed_size = 0;
        m_stable_size = 0;

        // Create internal directories if necessary
        retval = mkdir(archive_path_string.c_str(), 0750);
        if (0 != retval) {
            SPDLOG_ERROR("Failed to create {}, errno={}", archive_path_string.c_str(), errno);
            throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
        }

        // Get archive directory's file descriptor
        int archive_dir_fd = ::open(archive_path_string.c_str(), O_RDONLY);
        if (-1 == archive_dir_fd) {
            SPDLOG_ERROR("Failed to get file descriptor for {}, errno={}", archive_path_string.c_str(), errno);
            throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
        }

        // Create logs directory
        m_logs_dir_path = archive_path_string;
        m_logs_dir_path += '/';
        m_logs_dir_path += cLogsDirname;
        m_logs_dir_path += '/';
        retval = mkdir(m_logs_dir_path.c_str(), 0750);
        if (0 != retval) {
            SPDLOG_ERROR("Failed to create {}, errno={}", m_logs_dir_path.c_str(), errno);
            throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
        }

        // Get logs directory's file descriptor
        m_logs_dir_fd = ::open(m_logs_dir_path.c_str(), O_RDONLY);
        if (-1 == m_logs_dir_fd) {
            SPDLOG_ERROR("Failed to open file descriptor for {}, errno={}", m_logs_dir_path.c_str(), errno);
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
            SPDLOG_ERROR("Failed to open file descriptor for {}, errno={}", m_segments_dir_path.c_str(), errno);
            throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
        }

        // Create metadata database
        auto metadata_db_path = archive_path / cMetadataDBFileName;
        m_metadata_db.open(metadata_db_path.string());

        m_next_file_id = 0;

        m_target_segment_uncompressed_size = user_config.target_segment_uncompressed_size;
        m_next_segment_id = 0;
        m_compression_level = user_config.compression_level;

        // Copy schema file into archive
        if (!m_schema_file_path.empty()) {
            const std::filesystem::path archive_schema_filesystem_path = archive_path / cSchemaFileName;
            try {
                const std::filesystem::path m_schema_filesystem_path = m_schema_file_path;
                std::filesystem::copy(m_schema_filesystem_path, archive_schema_filesystem_path);
                m_stable_size += m_schema_file_size;
            } catch (FileWriter::OperationFailed& e) {
                SPDLOG_CRITICAL("Failed to copy schema file to archive: {}", archive_schema_filesystem_path.c_str());
                throw;
            }
        }

        // Save metadata to disk
        auto metadata_file_path = archive_path / cMetadataFileName;
        try {
            size_t schema_original_file_path_size = m_schema_original_file_path.size();
            m_metadata_file_writer.open(metadata_file_path.string(), FileWriter::OpenMode::CREATE_IF_NONEXISTENT_FOR_SEEKABLE_WRITING);
            // Update size before we write the metadata file so we can store the size in the metadata file
            m_stable_size += sizeof(cArchiveFormatVersion) + sizeof(m_stable_uncompressed_size) + sizeof(m_stable_size) + sizeof(m_schema_checksum);
            m_stable_size += sizeof(schema_original_file_path_size) + m_schema_original_file_path.size() * sizeof(char) + sizeof(m_schema_last_edited);

            m_metadata_file_writer.write_numeric_value(cArchiveFormatVersion);
            m_metadata_file_writer.write_numeric_value(m_schema_checksum);
            m_metadata_file_writer.write_numeric_value(schema_original_file_path_size);
            m_metadata_file_writer.write_string(m_schema_original_file_path);
            m_metadata_file_writer.write_numeric_value(m_schema_last_edited);
            m_metadata_file_writer.write_numeric_value(m_stable_uncompressed_size);
            m_metadata_file_writer.write_numeric_value(m_stable_size);
            m_metadata_file_writer.flush();
        } catch (FileWriter::OperationFailed& e) {
            SPDLOG_CRITICAL("Failed to write archive file metadata collection in file: {}", metadata_file_path.c_str());
            throw;
        }

        m_global_metadata_db = user_config.global_metadata_db;

        m_global_metadata_db->open();
        m_global_metadata_db->add_archive(m_id_as_string, m_stable_uncompressed_size, m_stable_size, m_creator_id_as_string, m_creation_num);
        m_global_metadata_db->close();

        m_file = nullptr;

        // Open log-type dictionary
        string logtype_dict_path = archive_path_string + '/' + cLogTypeDictFilename;
        string logtype_dict_segment_index_path = archive_path_string + '/' + cLogTypeSegmentIndexFilename;
        m_logtype_dict.open(logtype_dict_path, logtype_dict_segment_index_path, cLogtypeDictionaryIdMax);

        // Open variable dictionary
        string var_dict_path = archive_path_string + '/' + cVarDictFilename;
        string var_dict_segment_index_path = archive_path_string + '/' + cVarSegmentIndexFilename;
        m_var_dict.open(var_dict_path, var_dict_segment_index_path,
                        EncodedVariableInterpreter::get_var_dict_id_range_end() - EncodedVariableInterpreter::get_var_dict_id_range_begin());

        #if FLUSH_TO_DISK_ENABLED
            // fsync archive directory now that everything in the archive directory has been created
            if (fsync(archive_dir_fd) != 0) {
                SPDLOG_ERROR("Failed to fsync {}, errno={}", archive_path_string.c_str(), errno);
                throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
            }
        #endif
        if (::close(archive_dir_fd) != 0) {
            // We've already fsynced, so this error shouldn't affect us. Therefore, just log it.
            SPDLOG_WARN("Error when closing file descriptor for {}, errno={}", archive_path_string.c_str(), errno);
        }

        m_path = archive_path_string;
        return archive_path.string();
    }

    void Archive::close () {
        // The file should have been closed and persisted before closing the archive.
        if (m_file != nullptr) {
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }

        // Close segments if necessary
        if (m_segment_for_files_with_timestamps.is_open()) {
            close_segment_and_persist_file_metadata(m_segment_for_files_with_timestamps, m_files_with_timestamps_in_segment,
                                                    m_logtype_ids_in_segment_for_files_with_timestamps, m_var_ids_in_segment_for_files_with_timestamps);
            m_logtype_ids_in_segment_for_files_with_timestamps.clear();
            m_var_ids_in_segment_for_files_with_timestamps.clear();
        }
        if (m_segment_for_files_without_timestamps.is_open()) {
            close_segment_and_persist_file_metadata(m_segment_for_files_without_timestamps, m_files_without_timestamps_in_segment,
                                                    m_logtype_ids_in_segment_for_files_without_timestamps, m_var_ids_in_segment_for_files_without_timestamps);
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

        if (::close(m_logs_dir_fd) != 0) {
            // We've already fsynced, so this error shouldn't affect us. Therefore, just log it.
            SPDLOG_WARN("Error when closing logs directory file descriptor, errno={}", errno);
        }
        m_logs_dir_fd = -1;
        m_logs_dir_path.clear();

        m_metadata_file_writer.close();

        m_global_metadata_db = nullptr;

        m_stable_uncompressed_size = 0;
        m_stable_size = 0;

        m_metadata_db.close();

        m_creator_id_as_string.clear();
        m_id_as_string.clear();
        m_path.clear();
    }

    void Archive::create_and_open_file (const string& path, const group_id_t group_id, const boost::uuids::uuid& orig_file_id, size_t split_ix) {
        if (m_file != nullptr) {
            throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
        }
        m_file = new File(m_uuid_generator(), orig_file_id, path, group_id, split_ix);
        m_file->open();
    }

    void Archive::close_file () {
        if (m_file == nullptr) {
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }
        m_file->close();
    }

    const File& Archive::get_file () const {
        if (m_file == nullptr) {
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }
        return *m_file;
    }

    void Archive::set_file_is_split (bool is_split) {
        if (m_file == nullptr) {
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }
        m_file->set_is_split(is_split);
    }

    void Archive::change_ts_pattern (const TimestampPattern* pattern) {
        if (m_file == nullptr) {
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }
        m_file->change_ts_pattern(pattern);
    }

    void Archive::write_msg (epochtime_t timestamp, const string& message, size_t num_uncompressed_bytes) {
        // Encode message and add components to dictionaries
        vector<encoded_variable_t> encoded_vars;
        vector<variable_dictionary_id_t> var_ids;
        EncodedVariableInterpreter::encode_and_add_to_dictionary(message, m_logtype_dict_entry, m_var_dict, encoded_vars, var_ids);
        logtype_dictionary_id_t logtype_id;
        m_logtype_dict.add_entry(m_logtype_dict_entry, logtype_id);

        m_file->write_encoded_msg(timestamp, logtype_id, encoded_vars, var_ids, num_uncompressed_bytes);

        // Update segment indices
        if (m_file->has_ts_pattern()) {
            m_logtype_ids_in_segment_for_files_with_timestamps.insert(logtype_id);
            m_var_ids_in_segment_for_files_with_timestamps.insert_all(var_ids);
        } else {
            m_logtype_ids_for_file_with_unassigned_segment.insert(logtype_id);
            m_var_ids_for_file_with_unassigned_segment.insert(var_ids.cbegin(), var_ids.cend());
        }
    }
    
    void Archive::write_msg_using_schema (compressor_frontend::Token*& uncompressed_msg, uint32_t uncompressed_msg_pos, const bool has_delimiter,
                                          const bool has_timestamp) {
        epochtime_t timestamp = 0;
        TimestampPattern* timestamp_pattern = nullptr;
        if (has_timestamp) {
            size_t start;
            size_t end;
            timestamp_pattern = (TimestampPattern*) TimestampPattern::search_known_ts_patterns(
                    uncompressed_msg[0].get_string(), timestamp, start, end);
            if (old_ts_pattern != *timestamp_pattern) {
                change_ts_pattern(timestamp_pattern);
                old_ts_pattern = *timestamp_pattern;
            }
            assert(nullptr != timestamp_pattern);
        }
        if (get_data_size_of_dictionaries() >= m_target_data_size_of_dicts) {
            clp::split_file_and_archive(m_archive_user_config, m_path_for_compression, m_group_id, timestamp_pattern, *this);
        } else if (m_file->get_encoded_size_in_bytes() >= m_target_encoded_file_size) {
            clp::split_file(m_path_for_compression, m_group_id, timestamp_pattern, *this);
        }

        m_encoded_vars.clear();
        m_var_ids.clear();
        m_logtype_dict_entry.clear();

        size_t num_uncompressed_bytes = 0;
        // Timestamp is included in the uncompressed message size
        uint32_t start_pos = uncompressed_msg[0].start_pos;
        if (timestamp_pattern == nullptr) {
            start_pos = uncompressed_msg[1].start_pos;
        }
        uint32_t end_pos = uncompressed_msg[uncompressed_msg_pos - 1].end_pos;
        if (start_pos <= end_pos) {
            num_uncompressed_bytes = end_pos - start_pos;
        } else {
            num_uncompressed_bytes = *uncompressed_msg[0].buffer_size_ptr - start_pos + end_pos;
        }
        for (uint32_t i = 1; i < uncompressed_msg_pos; i++) {
            compressor_frontend::Token& token = uncompressed_msg[i];
            int token_type = token.type_ids->at(0);
            if (has_delimiter && token_type != (int) compressor_frontend::SymbolID::TokenUncaughtStringID &&
                token_type != (int) compressor_frontend::SymbolID::TokenNewlineId) {
                m_logtype_dict_entry.add_constant(token.get_delimiter(), 0, 1);
                if (token.start_pos == *token.buffer_size_ptr - 1) {
                    token.start_pos = 0;
                } else {
                    token.start_pos++;
                }
            }
            switch (token_type) {
                case (int) compressor_frontend::SymbolID::TokenNewlineId: 
                case (int) compressor_frontend::SymbolID::TokenUncaughtStringID: {
                    m_logtype_dict_entry.add_constant(token.get_string(), 0, token.get_length());
                    break;
                }
                case (int) compressor_frontend::SymbolID::TokenIntId: {
                    encoded_variable_t encoded_var;
                    if (!EncodedVariableInterpreter::convert_string_to_representable_integer_var(token.get_string(), encoded_var)) {
                        variable_dictionary_id_t id;
                        m_var_dict.add_entry(token.get_string(), id);
                        encoded_var = EncodedVariableInterpreter::encode_var_dict_id(id);
                    }
                    m_logtype_dict_entry.add_non_double_var();
                    m_encoded_vars.push_back(encoded_var);
                    break;
                }
                case (int) compressor_frontend::SymbolID::TokenDoubleId: {
                    encoded_variable_t encoded_var;
                    if (!EncodedVariableInterpreter::convert_string_to_representable_double_var(token.get_string(), encoded_var)) {
                        variable_dictionary_id_t id;
                        m_var_dict.add_entry(token.get_string(), id);
                        encoded_var = EncodedVariableInterpreter::encode_var_dict_id(id);
                        m_logtype_dict_entry.add_non_double_var();
                    } else {
                        m_logtype_dict_entry.add_double_var();
                    }
                    m_encoded_vars.push_back(encoded_var);
                    break;
                }
                default: {
                    // Variable string looks like a dictionary variable, so encode it as so
                    encoded_variable_t encoded_var;
                    variable_dictionary_id_t id;
                    m_var_dict.add_entry(token.get_string(), id);
                    encoded_var = EncodedVariableInterpreter::encode_var_dict_id(id);
                    m_var_ids.push_back(id);

                    m_logtype_dict_entry.add_non_double_var();
                    m_encoded_vars.push_back(encoded_var);
                    break;
                }
            }
        }
        if (!m_logtype_dict_entry.get_value().empty()) {
            logtype_dictionary_id_t logtype_id;
            m_logtype_dict.add_entry(m_logtype_dict_entry, logtype_id);
            m_file->write_encoded_msg(timestamp, logtype_id, m_encoded_vars, m_var_ids, num_uncompressed_bytes);

            // Update segment indices
            if (m_file->has_ts_pattern()) {
                m_logtype_ids_in_segment_for_files_with_timestamps.insert(logtype_id);
                m_var_ids_in_segment_for_files_with_timestamps.insert_all(m_var_ids);
            } else {
                m_logtype_ids_for_file_with_unassigned_segment.insert(logtype_id);
                m_var_ids_for_file_with_unassigned_segment.insert(m_var_ids.cbegin(), m_var_ids.cend());
            }
        }
    }

    void Archive::write_dir_snapshot () {
        #if FLUSH_TO_DISK_ENABLED
            // fsync logs directory to flush new files' directory entries
            if (0 != fsync(m_logs_dir_fd)) {
                SPDLOG_ERROR("Failed to fsync {}, errno={}", m_logs_dir_path.c_str(), errno);
                throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
            }
        #endif

        // Flush dictionaries
        m_logtype_dict.write_header_and_flush_to_disk();
        m_var_dict.write_header_and_flush_to_disk();
    }

    void Archive::append_file_contents_to_segment (Segment& segment, ArrayBackedPosIntSet<logtype_dictionary_id_t>& logtype_ids_in_segment,
                                                   ArrayBackedPosIntSet<variable_dictionary_id_t>& var_ids_in_segment, vector<File*>& files_in_segment)
    {
        if (!segment.is_open()) {
            segment.open(m_segments_dir_path, m_next_segment_id++, m_compression_level);
        }

        m_file->append_to_segment(m_logtype_dict, segment);
        files_in_segment.emplace_back(m_file);

        // Close current segment if its uncompressed size is greater than the target
        if (segment.get_uncompressed_size() >= m_target_segment_uncompressed_size) {
            close_segment_and_persist_file_metadata(segment, files_in_segment, logtype_ids_in_segment, var_ids_in_segment);
            logtype_ids_in_segment.clear();
            var_ids_in_segment.clear();
        }
    }

    void Archive::append_file_to_segment () {
        if (m_file == nullptr) {
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }

        if (m_file->has_ts_pattern()) {
            m_logtype_ids_in_segment_for_files_with_timestamps.insert_all(m_logtype_ids_for_file_with_unassigned_segment);
            m_var_ids_in_segment_for_files_with_timestamps.insert_all(m_var_ids_for_file_with_unassigned_segment);
            append_file_contents_to_segment(m_segment_for_files_with_timestamps, m_logtype_ids_in_segment_for_files_with_timestamps,
                                            m_var_ids_in_segment_for_files_with_timestamps, m_files_with_timestamps_in_segment);
        } else {
            m_logtype_ids_in_segment_for_files_without_timestamps.insert_all(m_logtype_ids_for_file_with_unassigned_segment);
            m_var_ids_in_segment_for_files_without_timestamps.insert_all(m_var_ids_for_file_with_unassigned_segment);
            append_file_contents_to_segment(m_segment_for_files_without_timestamps, m_logtype_ids_in_segment_for_files_without_timestamps,
                                            m_var_ids_in_segment_for_files_without_timestamps, m_files_without_timestamps_in_segment);
        }
        m_logtype_ids_for_file_with_unassigned_segment.clear();
        m_var_ids_for_file_with_unassigned_segment.clear();
        // Make sure file pointer is nulled and cannot be accessed outside
        m_file = nullptr;
    }

    void Archive::persist_file_metadata (const vector<File*>& files) {
        if (files.empty()) {
            return;
        }

        m_metadata_db.update_files(files);

        m_global_metadata_db->update_metadata_for_files(m_id_as_string, files);

        // Mark files' metadata as clean
        for (auto file : files) {
            file->mark_metadata_as_clean();
        }
    }

    void Archive::close_segment_and_persist_file_metadata (Segment& segment, std::vector<File*>& files,
                                                           ArrayBackedPosIntSet<logtype_dictionary_id_t>& segment_logtype_ids,
                                                           ArrayBackedPosIntSet<variable_dictionary_id_t>& segment_var_ids)
    {
        auto segment_id = segment.get_id();
        m_logtype_dict.index_segment(segment_id, segment_logtype_ids);
        m_var_dict.index_segment(segment_id, segment_var_ids);

        m_stable_size += segment.get_compressed_size();

        segment.close();

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
        }

        m_global_metadata_db->open();
        persist_file_metadata(files);
        update_metadata();
        m_global_metadata_db->close();

        for (auto file : files) {
            m_stable_uncompressed_size += file->get_num_uncompressed_bytes();
            delete file;
        }
        files.clear();
    }

    void Archive::add_empty_directories (const vector<string>& empty_directory_paths) {
        if (empty_directory_paths.empty()) {
            return;
        }

        m_metadata_db.add_empty_directories(empty_directory_paths);
    }

    size_t Archive::get_stable_uncompressed_size () const {
        size_t uncompressed_size = m_stable_uncompressed_size;

        // Add size of files in an unclosed segment
        for (auto file : m_files_with_timestamps_in_segment) {
            uncompressed_size += file->get_num_uncompressed_bytes();
        }
        for (auto file : m_files_without_timestamps_in_segment) {
            uncompressed_size += file->get_num_uncompressed_bytes();
        }

        return uncompressed_size;
    }

    size_t Archive::get_stable_size () const {
        size_t on_disk_size = m_stable_size + m_logtype_dict.get_on_disk_size() + m_var_dict.get_on_disk_size();

        // Add size of unclosed segment
        if (m_segment_for_files_without_timestamps.is_open()) {
            on_disk_size += m_segment_for_files_without_timestamps.get_compressed_size();
        }

        return on_disk_size;
    }

    void Archive::update_metadata () {
        auto stable_uncompressed_size = get_stable_uncompressed_size();
        auto stable_size = get_stable_size();

        m_metadata_file_writer.seek_from_current(-((int64_t)(sizeof(m_stable_uncompressed_size) + sizeof(m_stable_size))));
        m_metadata_file_writer.write_numeric_value(stable_uncompressed_size);
        m_metadata_file_writer.write_numeric_value(stable_size);

        m_global_metadata_db->update_archive_size(m_id_as_string, stable_uncompressed_size, stable_size);

        if (m_print_archive_stats_progress) {
            nlohmann::json json_msg;
            json_msg["id"] = m_id_as_string;
            json_msg["uncompressed_size"] = stable_uncompressed_size;
            json_msg["size"] = stable_size;
            std::cout << json_msg.dump(-1, ' ', true, nlohmann::json::error_handler_t::ignore) << std::endl;
        }
    }
}
