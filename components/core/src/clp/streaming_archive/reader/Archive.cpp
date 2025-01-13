#include "Archive.hpp"

#include <sys/stat.h>

#include <cstring>
#include <fstream>
#include <vector>

#include <boost/filesystem.hpp>

#include "../../EncodedVariableInterpreter.hpp"
#include "../../spdlog_with_specializations.hpp"
#include "../../Utils.hpp"
#include "../ArchiveMetadata.hpp"
#include "../Constants.hpp"

using std::string;
using std::unordered_set;
using std::vector;

namespace clp::streaming_archive::reader {
void Archive::open(string const& path) {
    // Determine whether path is file or directory
    struct stat path_stat = {};
    char const* path_c_str = path.c_str();
    if (0 != stat(path_c_str, &path_stat)) {
        SPDLOG_ERROR("Failed to stat {}, errno={}", path_c_str, errno);
        throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
    }
    if (!S_ISDIR(path_stat.st_mode)) {
        SPDLOG_ERROR("{} is not a directory", path_c_str);
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }
    m_path = path;

    // Read the metadata file
    string metadata_file_path = path + '/' + cMetadataFileName;
    archive_format_version_t format_version{};
    try {
        FileReader file_reader{metadata_file_path};
        ArchiveMetadata const metadata{file_reader};
        format_version = metadata.get_archive_format_version();
    } catch (TraceableException& traceable_exception) {
        auto error_code = traceable_exception.get_error_code();
        if (ErrorCode_errno == error_code) {
            SPDLOG_CRITICAL(
                    "streaming_archive::reader::Archive: Failed to read archive metadata file "
                    "{} at {}:{} - errno={}",
                    metadata_file_path.c_str(),
                    traceable_exception.get_filename(),
                    traceable_exception.get_line_number(),
                    errno
            );
        } else {
            SPDLOG_CRITICAL(
                    "streaming_archive::reader::Archive: Failed to read archive metadata file "
                    "{} at {}:{} - error={}",
                    metadata_file_path.c_str(),
                    traceable_exception.get_filename(),
                    traceable_exception.get_line_number(),
                    error_code
            );
        }
        throw;
    }

    // Check archive matches format version
    if (cArchiveFormatVersion != format_version) {
        SPDLOG_ERROR("streaming_archive::reader::Archive: Archive uses an unsupported format.");
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }

    auto metadata_db_path = boost::filesystem::path(path) / cMetadataDBFileName;
    if (false == boost::filesystem::exists(metadata_db_path)) {
        SPDLOG_ERROR(
                "streaming_archive::reader::Archive: Metadata DB not found: {}",
                metadata_db_path.string()
        );
        throw OperationFailed(ErrorCode_FileNotFound, __FILENAME__, __LINE__);
    }
    m_metadata_db.open(metadata_db_path.string());

    // Open log-type dictionary
    string logtype_dict_path = m_path;
    logtype_dict_path += '/';
    logtype_dict_path += cLogTypeDictFilename;
    string logtype_segment_index_path = m_path;
    logtype_segment_index_path += '/';
    logtype_segment_index_path += cLogTypeSegmentIndexFilename;
    m_logtype_dictionary.open(logtype_dict_path, logtype_segment_index_path);

    // Open variables dictionary
    string var_dict_path = m_path;
    var_dict_path += '/';
    var_dict_path += cVarDictFilename;
    string var_segment_index_path = m_path;
    var_segment_index_path += '/';
    var_segment_index_path += cVarSegmentIndexFilename;
    m_var_dictionary.open(var_dict_path, var_segment_index_path);

    // Open segment manager
    m_segments_dir_path = m_path;
    m_segments_dir_path += '/';
    m_segments_dir_path += cSegmentsDirname;
    m_segments_dir_path += '/';
    m_segment_manager.open(m_segments_dir_path);

    // Open segment list
    string segment_list_path = m_segments_dir_path;
    segment_list_path += cSegmentListFilename;
}

void Archive::close() {
    m_logtype_dictionary.close();
    m_var_dictionary.close();
    m_segment_manager.close();
    m_segments_dir_path.clear();
    m_metadata_db.close();
    m_path.clear();
}

void Archive::refresh_dictionaries() {
    m_logtype_dictionary.read_new_entries();
    m_var_dictionary.read_new_entries();
}

ErrorCode Archive::open_file(File& file, MetadataDB::FileIterator const& file_metadata_ix) {
    return file.open_me(m_logtype_dictionary, file_metadata_ix, m_segment_manager);
}

void Archive::close_file(File& file) {
    file.close_me();
}

void Archive::reset_file_indices(streaming_archive::reader::File& file) {
    file.reset_indices();
}

LogTypeDictionaryReader const& Archive::get_logtype_dictionary() const {
    return m_logtype_dictionary;
}

VariableDictionaryReader const& Archive::get_var_dictionary() const {
    return m_var_dictionary;
}

bool Archive::find_message_in_time_range(
        File& file,
        epochtime_t search_begin_timestamp,
        epochtime_t search_end_timestamp,
        Message& msg
) {
    return file.find_message_in_time_range(search_begin_timestamp, search_end_timestamp, msg);
}

SubQuery const* Archive::find_message_matching_query(File& file, Query const& query, Message& msg) {
    return file.find_message_matching_query(query, msg);
}

bool Archive::get_next_message(File& file, Message& msg) {
    return file.get_next_message(msg);
}

bool
Archive::decompress_message(File& file, Message const& compressed_msg, string& decompressed_msg) {
    if (false == decompress_message_without_ts(compressed_msg, decompressed_msg)) {
        return false;
    }

    // Determine which timestamp pattern to use
    auto const& timestamp_patterns = file.get_timestamp_patterns();
    if (!timestamp_patterns.empty()
        && compressed_msg.get_ix_in_file_split()
                   >= timestamp_patterns[file.get_current_ts_pattern_ix()].first)
    {
        while (true) {
            if (file.get_current_ts_pattern_ix() >= timestamp_patterns.size() - 1) {
                // Already at last timestamp pattern
                break;
            }
            auto next_patt_start_message_num
                    = timestamp_patterns[file.get_current_ts_pattern_ix() + 1].first;
            if (compressed_msg.get_ix_in_file_split() < next_patt_start_message_num) {
                // Not yet time for next timestamp pattern
                break;
            }
            file.increment_current_ts_pattern_ix();
        }
        timestamp_patterns[file.get_current_ts_pattern_ix()].second.insert_formatted_timestamp(
                compressed_msg.get_ts_in_milli(),
                decompressed_msg
        );
    }

    return true;
}

bool
Archive::decompress_message_without_ts(Message const& compressed_msg, string& decompressed_msg) {
    decompressed_msg.clear();

    // Build original message content
    auto const logtype_id = compressed_msg.get_logtype_id();
    auto const& logtype_entry = m_logtype_dictionary.get_entry(logtype_id);
    if (false
        == EncodedVariableInterpreter::decode_variables_into_message(
                logtype_entry,
                m_var_dictionary,
                compressed_msg.get_vars(),
                decompressed_msg
        ))
    {
        SPDLOG_ERROR(
                "streaming_archive::reader::Archive: Failed to decompress variables from "
                "logtype id {}",
                compressed_msg.get_logtype_id()
        );
        return false;
    }

    return true;
}

void Archive::decompress_empty_directories(string const& output_dir) {
    boost::filesystem::path output_dir_path = boost::filesystem::path(output_dir);

    string path;
    auto ix_ptr = m_metadata_db.get_empty_directory_iterator();
    for (auto& ix = *ix_ptr; ix.has_next(); ix.next()) {
        ix.get_path(path);
        auto empty_directory_path = output_dir_path / path;
        auto error_code = create_directory_structure(empty_directory_path.string(), 0700);
        if (ErrorCode_Success != error_code) {
            SPDLOG_ERROR(
                    "Failed to create directory structure {}, errno={}",
                    empty_directory_path.string().c_str(),
                    errno
            );
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }
    }
}
}  // namespace clp::streaming_archive::reader
