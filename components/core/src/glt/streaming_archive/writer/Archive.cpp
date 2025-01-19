#include "Archive.hpp"

#include <sys/stat.h>

#include <filesystem>
#include <fstream>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <json/single_include/nlohmann/json.hpp>

#include "../../EncodedVariableInterpreter.hpp"
#include "../../ir/types.hpp"
#include "../../spdlog_with_specializations.hpp"
#include "../../Utils.hpp"
#include "../Constants.hpp"
#include "utils.hpp"

using glt::ir::eight_byte_encoded_variable_t;
using glt::ir::four_byte_encoded_variable_t;
using std::list;
using std::make_unique;
using std::string;
using std::unordered_set;
using std::vector;

namespace glt::streaming_archive::writer {
Archive::~Archive() {
    if (m_path.empty() == false || m_file != nullptr || m_files_in_segment.empty() == false) {
        SPDLOG_ERROR("Archive not closed before being destroyed - data loss may occur");
        delete m_file;
        for (auto file : m_files_in_segment) {
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
            cArchiveFormatVersion,
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

    m_file_id = 0;

    m_target_segment_uncompressed_size = user_config.target_segment_uncompressed_size;
    m_next_segment_id = 0;
    m_compression_level = user_config.compression_level;

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
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }

    m_global_metadata_db = user_config.global_metadata_db;

    m_global_metadata_db->open();
    m_global_metadata_db->add_archive(m_id_as_string, *m_local_metadata);
    m_global_metadata_db->close();

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

    // handle GLT specific members
    m_combine_threshold = user_config.glt_combine_threshold;
    // Save file_id to file name mapping to disk
    std::string file_id_file_path = m_path + '/' + cFileNameDictFilename;
    try {
        m_filename_dict_writer.open(
                file_id_file_path,
                FileWriter::OpenMode::CREATE_IF_NONEXISTENT_FOR_SEEKABLE_WRITING
        );
    } catch (FileWriter::OperationFailed& e) {
        SPDLOG_CRITICAL("Failed to create file: {}", file_id_file_path.c_str());
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_filename_dict_compressor.open(m_filename_dict_writer, m_compression_level);
}

void Archive::close() {
    // The file should have been closed and persisted before closing the archive.
    if (m_file != nullptr) {
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }

    // Close segments if necessary
    if (m_message_order_table.is_open()) {
        close_segment_and_persist_file_metadata(
                m_message_order_table,
                m_glt_segment,
                m_files_in_segment,
                m_logtype_ids_in_segment,
                m_var_ids_in_segment
        );
        m_logtype_ids_in_segment.clear();
        m_var_ids_in_segment.clear();
    }
    m_filename_dict_compressor.close();
    m_filename_dict_writer.close();

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

    m_global_metadata_db = nullptr;

    m_metadata_db.close();

    m_creator_id_as_string.clear();
    m_id_as_string.clear();
    m_path.clear();
}

void Archive::create_and_open_file(
        string const& path,
        group_id_t const group_id,
        boost::uuids::uuid const& orig_file_id,
        size_t split_ix
) {
    if (m_file != nullptr) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }
    m_file = new File(m_uuid_generator(), orig_file_id, path, group_id, split_ix);
    m_file->open();
    std::string file_name_to_write = path + '\n';
    m_filename_dict_compressor.write(file_name_to_write.c_str(), file_name_to_write.size());
}

void Archive::close_file() {
    if (m_file == nullptr) {
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }
    m_file->close();
    m_file_id++;
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
    size_t offset = m_glt_segment.append_to_segment(logtype_id, timestamp, m_file_id, encoded_vars);
    // Issue: the offset of var_segments is per file based. However, we still need to add the offset
    // of segments. the offset of segment is not known because we don't know if the segment should
    // be timestamped... Here for simplicity, we add the segment offset back when we close the file
    m_file->write_encoded_msg(
            timestamp,
            logtype_id,
            offset,
            num_uncompressed_bytes,
            encoded_vars.size()
    );
    // Update segment indices
    m_logtype_ids_in_segment.insert(logtype_id);
    m_var_ids_in_segment.insert_all(var_ids);
}

void Archive::write_dir_snapshot() {
    // Flush dictionaries
    m_logtype_dict.write_header_and_flush_to_disk();
    m_var_dict.write_header_and_flush_to_disk();
}

void Archive::append_file_contents_to_segment(
        Segment& segment,
        GLTSegment& glt_segment,
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
    if (segment.get_uncompressed_size() + glt_segment.get_uncompressed_size()
        >= m_target_segment_uncompressed_size)
    {
        close_segment_and_persist_file_metadata(
                segment,
                glt_segment,
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
    // GLT TODO: this open logic is counter intuitive for glt_segment
    // because the open happens after file content gets appended
    // to m_glt_segment.
    if (!m_message_order_table.is_open()) {
        m_glt_segment.open(
                m_segments_dir_path,
                m_next_segment_id,
                m_compression_level,
                m_combine_threshold
        );
        m_message_order_table.open(m_segments_dir_path, m_next_segment_id, m_compression_level);
        m_next_segment_id++;
    }
    append_file_contents_to_segment(
            m_message_order_table,
            m_glt_segment,
            m_logtype_ids_in_segment,
            m_var_ids_in_segment,
            m_files_in_segment
    );

    // Make sure file pointer is nulled and cannot be accessed outside
    m_file = nullptr;
}

void Archive::persist_file_metadata(vector<File*> const& files) {
    if (files.empty()) {
        return;
    }

    m_metadata_db.update_files(files);

    m_global_metadata_db->update_metadata_for_files(m_id_as_string, files);
}

void Archive::close_segment_and_persist_file_metadata(
        Segment& on_disk_stream,
        GLTSegment& glt_segment,
        std::vector<File*>& files,
        ArrayBackedPosIntSet<logtype_dictionary_id_t>& segment_logtype_ids,
        ArrayBackedPosIntSet<variable_dictionary_id_t>& segment_var_ids
) {
    auto segment_id = on_disk_stream.get_id();
    m_logtype_dict.index_segment(segment_id, segment_logtype_ids);
    m_var_dict.index_segment(segment_id, segment_var_ids);

    on_disk_stream.close();
    glt_segment.close();

    // GLT TODO: here the size calculation needs some attention
    m_local_metadata->increment_static_compressed_size(on_disk_stream.get_compressed_size());
    m_local_metadata->increment_static_compressed_size(glt_segment.get_compressed_size());

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

    m_global_metadata_db->open();
    persist_file_metadata(files);
    update_metadata();
    m_global_metadata_db->close();

    for (auto file : files) {
        delete file;
    }
    files.clear();
}

void Archive::add_empty_directories(vector<string> const& empty_directory_paths) {
    if (empty_directory_paths.empty()) {
        return;
    }

    m_metadata_db.add_empty_directories(empty_directory_paths);
}

uint64_t Archive::get_dynamic_compressed_size() {
    uint64_t on_disk_size = m_logtype_dict.get_on_disk_size() + m_var_dict.get_on_disk_size()
                            + m_filename_dict_compressor.get_pos();

    // GLT. Note we don't need to add size of glt_segment
    if (m_message_order_table.is_open()) {
        on_disk_size += m_message_order_table.get_compressed_size();
    }
    return on_disk_size;
}

void Archive::update_metadata() {
    m_local_metadata->set_dynamic_uncompressed_size(0);
    m_local_metadata->set_dynamic_compressed_size(get_dynamic_compressed_size());
    // Rewrite (overwrite) the metadata file
    m_metadata_file_writer.seek_from_begin(0);
    m_local_metadata->write_to_file(m_metadata_file_writer);

    m_global_metadata_db->update_archive_metadata(m_id_as_string, *m_local_metadata);

    if (m_print_archive_stats_progress) {
        nlohmann::json json_msg;
        json_msg["id"] = m_id_as_string;
        json_msg["uncompressed_size"] = m_local_metadata->get_uncompressed_size_bytes();
        json_msg["size"] = m_local_metadata->get_compressed_size_bytes();
        std::cout << json_msg.dump(-1, ' ', true, nlohmann::json::error_handler_t::ignore)
                  << std::endl;
    }
}
}  // namespace glt::streaming_archive::writer
