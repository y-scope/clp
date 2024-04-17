// Code from CLP

#include "FileWriter.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>

#include <spdlog/spdlog.h>

using std::string;

namespace clp_s {
FileWriter::~FileWriter() {
    if (nullptr != m_file) {
        SPDLOG_ERROR("FileWriter not closed before being destroyed - may cause data loss");
    }
}

void FileWriter::write(char const* data, size_t data_length) {
    ErrorCode error_code = ErrorCodeSuccess;
    if (nullptr == m_file) {
        error_code = ErrorCodeNotInit;
    } else if (nullptr == data) {
        error_code = ErrorCodeBadParam;
    } else {
        size_t num_bytes_written = fwrite(data, sizeof(*data), data_length, m_file);
        if (num_bytes_written < data_length) {
            error_code = ErrorCodeErrno;
        }
    }
    if (ErrorCodeSuccess != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
}

void FileWriter::flush() {
#if !FLUSH_TO_DISK_ENABLED
    return;
#endif
    // Flush userspace buffers to page cache
    if (0 != fflush(m_file)) {
        SPDLOG_ERROR("fflush failed, errno={}", errno);
        throw OperationFailed(ErrorCodeErrno, __FILENAME__, __LINE__);
    }

    // Flush page cache pages to disk
    if (0 != fsync(m_fd)) {
        SPDLOG_ERROR("fdatasync failed, errno={}", errno);
        throw OperationFailed(ErrorCodeErrno, __FILENAME__, __LINE__);
    }
}

size_t FileWriter::get_pos() {
    size_t pos;
    ErrorCode error_code = try_get_pos(pos);
    if (ErrorCodeSuccess != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }

    return pos;
}

ErrorCode FileWriter::try_get_pos(size_t& pos) const {
    if (nullptr == m_file) {
        return ErrorCodeNotInit;
    }

    pos = ftello(m_file);
    if ((off_t)-1 == pos) {
        return ErrorCodeErrno;
    }

    return ErrorCodeSuccess;
}

void FileWriter::seek_from_begin(size_t pos) {
    auto error_code = try_seek_from_begin(pos);
    if (ErrorCodeSuccess != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
}

ErrorCode FileWriter::try_seek_from_begin(size_t pos) {
    if (nullptr == m_file) {
        return ErrorCodeNotInit;
    }

    int retval = fseeko(m_file, pos, SEEK_SET);
    if (0 != retval) {
        return ErrorCodeErrno;
    }

    return ErrorCodeSuccess;
}

ErrorCode FileWriter::try_seek_from_current(off_t offset) {
    if (nullptr == m_file) {
        return ErrorCodeNotInit;
    }

    int retval = fseeko(m_file, offset, SEEK_CUR);
    if (0 != retval) {
        return ErrorCodeErrno;
    }

    return ErrorCodeSuccess;
}

void FileWriter::open(string const& path, OpenMode open_mode) {
    if (nullptr != m_file) {
        throw OperationFailed(ErrorCodeNotInit, __FILENAME__, __LINE__);
    }

    switch (open_mode) {
        case OpenMode::CreateForWriting:
            m_file = fopen(path.c_str(), "wb");
            break;
        case OpenMode::CreateIfNonexistentForAppending:
            m_file = fopen(path.c_str(), "ab");
            break;
        case OpenMode::CreateIfNonexistentForSeekableWriting: {
            struct stat stat_buf = {};
            if (0 == stat(path.c_str(), &stat_buf)) {
                // File exists, so open it for seekable writing
                m_file = fopen(path.c_str(), "r+b");
            } else {
                if (ENOENT != errno) {
                    throw OperationFailed(ErrorCodeErrno, __FILENAME__, __LINE__);
                }
                // File doesn't exist, so create and open it for seekable writing
                // NOTE: We can't use the "w+" mode if the file exists since that will truncate the
                // file
                m_file = fopen(path.c_str(), "w+b");
            }

            auto retval = fseek(m_file, 0, SEEK_END);
            if (0 != retval) {
                throw OperationFailed(ErrorCodeErrno, __FILENAME__, __LINE__);
            }
            break;
        }
    }
    if (nullptr == m_file) {
        throw OperationFailed(ErrorCodeErrno, __FILENAME__, __LINE__);
    }

    m_fd = fileno(m_file);
    if (-1 == m_fd) {
        fclose(m_file);
        throw OperationFailed(ErrorCodeErrno, __FILENAME__, __LINE__);
    }
}

void FileWriter::close() {
    if (nullptr != m_file) {
        if (0 != fclose(m_file)) {
            throw OperationFailed(ErrorCodeErrno, __FILENAME__, __LINE__);
        }
        m_file = nullptr;
        m_fd = -1;
    }
}
}  // namespace clp_s
