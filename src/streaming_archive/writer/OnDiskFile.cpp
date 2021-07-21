#include "OnDiskFile.hpp"

// C standard libraries
#include <unistd.h>

// C++ standard libraries
#include <climits>

// spdlog
#include <spdlog/spdlog.h>

// Project headers
#include "../../EncodedVariableInterpreter.hpp"
#include "../../Utils.hpp"
#include "../Constants.hpp"

using std::string;
using std::unordered_set;
using std::vector;

namespace streaming_archive { namespace writer {
    OnDiskFile::OnDiskFile (const boost::uuids::uuid& id, const boost::uuids::uuid& orig_file_id, const string& orig_log_path, const string& archive_log_path,
                            const group_id_t group_id, size_t split_ix) : File(id, orig_file_id, orig_log_path, group_id, split_ix), m_is_open(false)
    {
        m_archive_log_path = archive_log_path;
        m_archive_log_path += get_id_as_string();
    }

    void OnDiskFile::open () {
        if (SegmentationState_NotInSegment != m_segmentation_state) {
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }

        // Open timestamps file writer
        string path = m_archive_log_path;
        path += cTimestampsFileExtension;
        m_timestamps_file_writer.open(path, FileWriter::OpenMode::CREATE_IF_NONEXISTENT_FOR_APPENDING);

        // Open logtype IDs file writer
        path = m_archive_log_path;
        path += cLogTypeIdsFileExtension;
        m_logtype_ids_file_writer.open(path, FileWriter::OpenMode::CREATE_IF_NONEXISTENT_FOR_APPENDING);

        // Open variables file writer
        path = m_archive_log_path;
        path += cVariablesFileExtension;
        m_variables_file_writer.open(path, FileWriter::OpenMode::CREATE_IF_NONEXISTENT_FOR_APPENDING);

        m_is_open = true;
    }

    void OnDiskFile::close () {
        flush();
        m_variables_file_writer.close();
        m_logtype_ids_file_writer.close();
        m_timestamps_file_writer.close();
        m_is_open = false;
    }

    bool OnDiskFile::is_open () const {
        return m_is_open;
    }

    void OnDiskFile::write_encoded_msg (const epochtime_t timestamp, const logtype_dictionary_id_t logtype_id, const vector<encoded_variable_t>& encoded_vars,
            size_t num_uncompressed_bytes)
    {
        m_timestamps_file_writer.write_numeric_value(timestamp);
        m_logtype_ids_file_writer.write_numeric_value(logtype_id);
        for (const auto encoded_var : encoded_vars) {
            m_variables_file_writer.write_numeric_value(encoded_var);
        }
        increment_num_messages_and_variables(1, encoded_vars.size());

        set_last_message_timestamp(timestamp);
        increment_num_uncompressed_bytes(num_uncompressed_bytes);
    }

    void OnDiskFile::append_to_segment (const LogTypeDictionaryWriter& logtype_dict, Segment& segment,
                                        unordered_set<logtype_dictionary_id_t>& segment_logtype_ids, unordered_set<variable_dictionary_id_t>& segment_var_ids)
    {
        if (m_is_open) {
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }

        ErrorCode error_code;

        // Map timestamps file
        string path = m_archive_log_path;
        path += cTimestampsFileExtension;
        int timestamps_fd;
        size_t timestamps_file_size;
        void* timestamps_ptr;
        error_code = memory_map_file(path, true, timestamps_fd, timestamps_file_size, timestamps_ptr);
        if (ErrorCode_Success != error_code) {
            SPDLOG_ERROR("Failed to map timestamps file");
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }
        if (timestamps_file_size % sizeof(epochtime_t) != 0) {
            SPDLOG_ERROR("Timestamps file is truncated");
            throw OperationFailed(ErrorCode_Truncated, __FILENAME__, __LINE__);
        }
        size_t num_read_timestamps = timestamps_file_size / sizeof(epochtime_t);
        if (num_read_timestamps < get_num_messages()) {
            SPDLOG_ERROR("There are fewer timestamps on disk ({}) than the metadata ({}) indicates.", num_read_timestamps, get_num_messages());
            throw OperationFailed(ErrorCode_Truncated, __FILENAME__, __LINE__);
        }

        // Map logtypes file
        path = m_archive_log_path;
        path += cLogTypeIdsFileExtension;
        int logtypes_fd;
        size_t logtypes_file_size;
        void* logtypes_ptr;
        error_code = memory_map_file(path, true, logtypes_fd, logtypes_file_size, logtypes_ptr);
        if (ErrorCode_Success != error_code) {
            SPDLOG_ERROR("Failed to map logtypes file");
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }
        if (logtypes_file_size % sizeof(logtype_dictionary_id_t) != 0) {
            SPDLOG_ERROR("Logtypes file is truncated");
            throw OperationFailed(ErrorCode_Truncated, __FILENAME__, __LINE__);
        }
        size_t num_read_logtypes = logtypes_file_size / sizeof(logtype_dictionary_id_t);
        if (num_read_logtypes < get_num_messages()) {
            SPDLOG_ERROR("There are fewer logtypes on disk ({}) than the metadata ({}) indicates.", num_read_logtypes, get_num_messages());
            throw OperationFailed(ErrorCode_Truncated, __FILENAME__, __LINE__);
        }

        // Map variables file
        path = m_archive_log_path;
        path += cVariablesFileExtension;
        int variables_fd;
        size_t variables_file_size;
        void* variables_ptr;
        error_code = memory_map_file(path, true, variables_fd, variables_file_size, variables_ptr);
        if (ErrorCode_Success != error_code) {
            SPDLOG_ERROR("Failed to map variables file");
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }
        if (variables_file_size % sizeof(encoded_variable_t) != 0) {
            SPDLOG_ERROR("Variables file is truncated");
            throw OperationFailed(ErrorCode_Truncated, __FILENAME__, __LINE__);
        }
        size_t num_read_variables = variables_file_size / sizeof(encoded_variable_t);
        if (num_read_variables < get_num_variables()) {
            SPDLOG_ERROR("There are fewer variables on disk ({}) than the metadata ({}) indicates.", num_read_variables, get_num_variables());
            throw OperationFailed(ErrorCode_Truncated, __FILENAME__, __LINE__);
        }

        // Add file's logtype and variable IDs to respective segment sets
        auto logtype_ids = reinterpret_cast<logtype_dictionary_id_t*>(logtypes_ptr);
        size_t num_logtypes = logtypes_file_size / sizeof(logtype_dictionary_id_t);
        auto variables = reinterpret_cast<encoded_variable_t*>(variables_ptr);
        size_t num_vars = variables_file_size / sizeof(encoded_variable_t);
        append_logtype_and_var_ids_to_segment_sets(logtype_dict, logtype_ids, num_logtypes, variables, num_vars, segment_logtype_ids, segment_var_ids);

        // Append files to segment
        uint64_t segment_timestamps_uncompressed_pos;
        segment.append(reinterpret_cast<const char*>(timestamps_ptr), timestamps_file_size, segment_timestamps_uncompressed_pos);
        uint64_t segment_logtypes_uncompressed_pos;
        segment.append(reinterpret_cast<const char*>(logtypes_ptr), logtypes_file_size, segment_logtypes_uncompressed_pos);
        uint64_t segment_variables_uncompressed_pos;
        segment.append(reinterpret_cast<const char*>(variables_ptr), variables_file_size, segment_variables_uncompressed_pos);
        set_segment_metadata(segment.get_id(), segment_timestamps_uncompressed_pos, segment_logtypes_uncompressed_pos, segment_variables_uncompressed_pos);
        m_segmentation_state = SegmentationState_MovingToSegment;

        // Unmap timestamps file
        error_code = memory_unmap_file(timestamps_fd, timestamps_file_size, timestamps_ptr);
        if (ErrorCode_Success != error_code) {
            SPDLOG_ERROR("Failed to unmap timestamps file");
        }

        // Unmap logtypes file
        error_code = memory_unmap_file(logtypes_fd, logtypes_file_size, logtypes_ptr);
        if (ErrorCode_Success != error_code) {
            SPDLOG_ERROR("Failed to unmap logtypes file");
        }

        // Unmap variables file
        error_code = memory_unmap_file(variables_fd, variables_file_size, variables_ptr);
        if (ErrorCode_Success != error_code) {
            SPDLOG_ERROR("Failed to unmap variables file");
        }
    }

    void OnDiskFile::cleanup_after_segment_insertion () {
        string path = m_archive_log_path;
        path += cTimestampsFileExtension;
        int retval = unlink(path.c_str());
        if (0 != retval) {
            throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
        }

        path = m_archive_log_path;
        path += cLogTypeIdsFileExtension;
        retval = unlink(path.c_str());
        if (0 != retval) {
            throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
        }

        path = m_archive_log_path;
        path += cVariablesFileExtension;
        retval = unlink(path.c_str());
        if (0 != retval) {
            throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
        }
    }

    void OnDiskFile::flush () {
        m_timestamps_file_writer.flush();
        m_logtype_ids_file_writer.flush();
        m_variables_file_writer.flush();
    }
} }
