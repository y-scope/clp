#include "LibarchiveReader.hpp"

// libarchive
#include <archive_entry.h>

// Project headers
#include "Defs.h"
#include "spdlog_with_specializations.hpp"

ErrorCode LibarchiveReader::try_open (size_t buffer_length, const char* buffer, FileReader& file_reader, const std::string& path_if_compressed_file) {
    // Create and initialize internal libarchive
    m_archive = archive_read_new();
    if (nullptr == m_archive) {
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }

    auto return_value = archive_read_support_filter_all(m_archive);
    if (ARCHIVE_OK != return_value) {
        SPDLOG_DEBUG("Failed to enable all filters for libarchive - {}", archive_error_string(m_archive));
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }

    // NOTE: We rely on libarchive trying to interpret the archive as raw last (since that's our intent as well)
    return_value = archive_read_support_format_all(m_archive);
    if (ARCHIVE_OK != return_value) {
        SPDLOG_DEBUG("Failed to enable all formats for libarchive - {}", archive_error_string(m_archive));
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    return_value = archive_read_support_format_raw(m_archive);
    if (ARCHIVE_OK != return_value) {
        SPDLOG_DEBUG("Failed to enable raw format for libarchive - {}", archive_error_string(m_archive));
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }

    // Copy initial buffer content
    m_buffer.resize(buffer_length);
    memcpy(m_buffer.data(), buffer, buffer_length);
    m_initial_buffer_content_exhausted = m_buffer.empty();

    m_file_reader = &file_reader;

    m_filename_if_compressed = path_if_compressed_file;


    return_value = archive_read_open2(m_archive, this, libarchive_open_callback, libarchive_read_callback, libarchive_skip_callback,
                                      libarchive_close_callback);
    if (ARCHIVE_OK != return_value) {
        SPDLOG_DEBUG("Failed to open libarchive - {}", archive_error_string(m_archive));
        release_resources();
        return ErrorCode_Failure;
    }

    return ErrorCode_Success;
}

void LibarchiveReader::close () {
    if (nullptr == m_archive) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    auto return_value = archive_read_close(m_archive);
    if (ARCHIVE_OK != return_value) {
        SPDLOG_ERROR("Failed to close libarchive - {}", archive_error_string(m_archive));
    }

    release_resources();
}

ErrorCode LibarchiveReader::try_read_next_header () {
    if (nullptr == m_archive) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    auto return_value = archive_read_next_header(m_archive, &m_archive_entry);
    if (ARCHIVE_OK != return_value) {
        if (ARCHIVE_EOF == return_value) {
            return ErrorCode_EndOfFile;
        }
        SPDLOG_DEBUG("Failed to read libarchive header - {}", archive_error_string(m_archive));
        return ErrorCode_Failure;
    }

    return ErrorCode_Success;
}

void LibarchiveReader::open_file_reader (LibarchiveFileReader& libarchive_file_reader) {
    if (nullptr == m_archive) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }
    if (get_entry_file_type() != AE_IFREG) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }

    libarchive_file_reader.open(m_archive, m_archive_entry);
}

mode_t LibarchiveReader::get_entry_file_type () const {
    if (nullptr == m_archive_entry) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    return archive_entry_filetype(m_archive_entry);
}

const char* LibarchiveReader::get_path () const {
    if (nullptr == m_archive_entry) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    if (ARCHIVE_FORMAT_RAW == archive_format(m_archive)) {
        return m_filename_if_compressed.c_str();
    } else {
        return archive_entry_pathname(m_archive_entry);
    }
}

int LibarchiveReader::libarchive_open_callback (struct archive* archive, void* client_data) {
    auto& libarchive_reader = *reinterpret_cast<LibarchiveReader*>(client_data);

    libarchive_reader.libarchive_open_callback();

    return ARCHIVE_OK;
}

int LibarchiveReader::libarchive_close_callback (struct archive* archive, void* client_data) {
    auto& libarchive_reader = *reinterpret_cast<LibarchiveReader*>(client_data);

    libarchive_reader.libarchive_close_callback();

    return ARCHIVE_OK;
}

la_ssize_t LibarchiveReader::libarchive_read_callback (struct archive* archive, void* client_data, const void** buffer) {
    auto& libarchive_reader = *reinterpret_cast<LibarchiveReader*>(client_data);

    size_t num_bytes_read = 0;
    auto error_code = libarchive_reader.libarchive_read_callback(buffer, num_bytes_read);
    if (ErrorCode_Success != error_code) {
        switch (error_code) {
            case ErrorCode_NotInit:
                archive_set_error(archive, EINVAL, "Underlying file is not open.");
                return -1;
            case ErrorCode_BadParam:
                archive_set_error(archive, ENOMEM, "Unknown error.");
                return -1;
            case ErrorCode_errno:
                archive_set_error(archive, errno, "%s", strerror(errno));
                return -1;
            case ErrorCode_EndOfFile:
                return 0;
            default:
                archive_set_error(archive, ENOENT, "Unhandled error code.");
                return -1;
        }
    }

    return num_bytes_read;
}

la_int64_t LibarchiveReader::libarchive_skip_callback (struct archive* archive, void* client_data, off_t request) {
    auto& libarchive_reader = *reinterpret_cast<LibarchiveReader*>(client_data);

    size_t num_bytes_skipped;
    auto error_code = libarchive_reader.libarchive_skip_callback(request, num_bytes_skipped);
    if (ErrorCode_Success != error_code) {
        if (ErrorCode_errno == error_code) {
            archive_set_error(archive, errno, "Failed to skip.");
        }
        return ARCHIVE_FATAL;
    }

    return num_bytes_skipped;
}

void LibarchiveReader::libarchive_open_callback () {
    m_is_opened_by_libarchive = true;
}

void LibarchiveReader::libarchive_close_callback () {
    m_is_opened_by_libarchive = false;
}

ErrorCode LibarchiveReader::libarchive_read_callback (const void** buffer, size_t& num_bytes_read) {
    if (false == m_is_opened_by_libarchive) {
        return ErrorCode_NotInit;
    }

    if (false == m_initial_buffer_content_exhausted) {
        *buffer = m_buffer.data();
        num_bytes_read = m_buffer.size();
        m_initial_buffer_content_exhausted = true;
    } else {
        constexpr size_t cTargetBufferLength = 4096;
        m_buffer.resize(cTargetBufferLength);
        auto error_code = m_file_reader->try_read(m_buffer.data(), cTargetBufferLength, num_bytes_read);
        if (ErrorCode_Success != error_code) {
            return error_code;
        }
        if (num_bytes_read < cTargetBufferLength) {
            m_buffer.resize(num_bytes_read);
        }
    }

    return ErrorCode_Success;
}

ErrorCode LibarchiveReader::libarchive_skip_callback (off_t num_bytes_to_skip, size_t& num_bytes_skipped) {
    // Get current position
    size_t pos;
    auto error_code = m_file_reader->try_get_pos(pos);
    if (ErrorCode_Success != error_code) {
        return error_code;
    }

    // Calculate desired position, ensuring its within the file
    size_t desired_pos = pos + num_bytes_to_skip;
    struct stat stat_buffer = {};
    error_code = m_file_reader->try_fstat(stat_buffer);
    if (ErrorCode_Success != error_code) {
        return error_code;
    }
    if (desired_pos > stat_buffer.st_size) {
        desired_pos = stat_buffer.st_size;
    }

    // Seek to desired position
    error_code = m_file_reader->try_seek_from_begin(desired_pos);
    if (ErrorCode_Success != error_code) {
        return error_code;
    }

    num_bytes_skipped = desired_pos - pos;

    return ErrorCode_Success;
}

void LibarchiveReader::release_resources () {
    auto return_value = archive_read_free(m_archive);
    if (ARCHIVE_OK != return_value) {
        SPDLOG_ERROR("Failed to destroy libarchive - {}", archive_error_string(m_archive));
    }
    m_archive = nullptr;

    m_file_reader = nullptr;
    m_buffer.clear();
}
