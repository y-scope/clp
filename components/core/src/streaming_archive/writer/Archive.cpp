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
        if (m_path.empty() == false || m_file != nullptr ||
            m_files_in_segment.empty() == false) {
            SPDLOG_ERROR("Archive not closed before being destroyed - data loss may occur");
            delete m_file;
            for (auto file : m_files_in_segment) {
                delete file;
            }
        }
    }

    void Archive::open (const UserConfig& user_config) {
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
        m_metadata_db->open(metadata_db_path.string());

        m_file_id = 0;

        m_target_segment_uncompressed_size = user_config.target_segment_uncompressed_size;
        m_next_segment_id = 0;
        m_compression_level = user_config.compression_level;

        /// TODO: add schema file size to m_stable_size???
        // Copy schema file into archive
        if (!m_schema_file_path.empty()) {
            const std::filesystem::path archive_schema_filesystem_path = archive_path / cSchemaFileName;
            try {
                const std::filesystem::path schema_filesystem_path = m_schema_file_path;
                std::filesystem::copy(schema_filesystem_path, archive_schema_filesystem_path);
            } catch (FileWriter::OperationFailed& e) {
                SPDLOG_CRITICAL("Failed to copy schema file to archive: {}", archive_schema_filesystem_path.c_str());
                throw;
            }
        }
        // Save metadata to disk
        auto metadata_file_path = archive_path / cMetadataFileName;
        try {
            m_metadata_file_writer.open(metadata_file_path.string(), FileWriter::OpenMode::CREATE_IF_NONEXISTENT_FOR_SEEKABLE_WRITING);
            // Update size before we write the metadata file so we can store the size in the metadata file
            m_stable_size += sizeof(cArchiveFormatVersion) + sizeof(m_stable_uncompressed_size) + sizeof(m_stable_size);

            m_metadata_file_writer.write_numeric_value(cArchiveFormatVersion);
            m_metadata_file_writer.write_numeric_value(m_stable_uncompressed_size);
            m_metadata_file_writer.write_numeric_value(m_stable_size);
            m_metadata_file_writer.flush();
        } catch (FileWriter::OperationFailed& e) {
            SPDLOG_CRITICAL("Failed to write archive file metadata collection in file: {}", metadata_file_path.c_str());
            throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
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
    }

    void Archive::close () {

        close_derived();
        // The file should have been closed and persisted before closing the archive.
        if (m_file != nullptr) {
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
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

        m_global_metadata_db = nullptr;

        m_stable_uncompressed_size = 0;
        m_stable_size = 0;

        m_metadata_db->close();

        m_creator_id_as_string.clear();
        m_id_as_string.clear();
        m_path.clear();
    }

    void Archive::close_file () {
        if (m_file == nullptr) {
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }
        m_file->close();
        m_file_id++;
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

    void Archive::write_dir_snapshot () {
        // Flush dictionaries
        m_logtype_dict.write_header_and_flush_to_disk();
        m_var_dict.write_header_and_flush_to_disk();
    }

    void Archive::persist_file_metadata (const vector<File*>& files) {
        if (files.empty()) {
            return;
        }

        m_metadata_db->update_files(files);

        m_global_metadata_db->update_metadata_for_files(m_id_as_string, files);

        // Mark files' metadata as clean
        for (auto file : files) {
            file->mark_metadata_as_clean();
        }
    }

    void Archive::add_empty_directories (const vector<string>& empty_directory_paths) {
        if (empty_directory_paths.empty()) {
            return;
        }

        m_metadata_db->add_empty_directories(empty_directory_paths);
    }

    size_t Archive::get_stable_uncompressed_size () const {
        size_t uncompressed_size = m_stable_uncompressed_size;

        // Add size of files in an unclosed segment
        for (auto file : m_files_in_segment) {
            uncompressed_size += file->get_num_uncompressed_bytes();
        }

        return uncompressed_size;
    }

    size_t Archive::get_stable_size () const {
        size_t on_disk_size =
                m_stable_size + m_logtype_dict.get_on_disk_size() + m_var_dict.get_on_disk_size();
        return on_disk_size;
    }

    void Archive::update_metadata () {
        auto stable_uncompressed_size = get_stable_uncompressed_size();
        auto stable_size = get_stable_size();

        m_metadata_file_writer.seek_from_current(-((int64_t)(sizeof(m_stable_uncompressed_size) + sizeof(m_stable_size))));
        m_metadata_file_writer.write_numeric_value(stable_uncompressed_size);
        m_metadata_file_writer.write_numeric_value(stable_size);

        m_global_metadata_db->update_archive_size(m_id_as_string, stable_uncompressed_size,stable_size);

        if (m_print_archive_stats_progress) {
            nlohmann::json json_msg;
            json_msg["id"] = m_id_as_string;
            json_msg["uncompressed_size"] = stable_uncompressed_size;
            json_msg["size"] = stable_size;
            std::cout << json_msg.dump(-1, ' ', true, nlohmann::json::error_handler_t::ignore) << std::endl;
        }
    }
}
