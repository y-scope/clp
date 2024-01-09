// Code from CLP

#include "FileReader.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>

using std::string;

namespace clp_s {
FileReader::~FileReader() {
    close();
    free(m_getdelim_buf);
}

ErrorCode FileReader::try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) {
    if (nullptr == m_file) {
        return ErrorCodeNotInit;
    }
    if (nullptr == buf) {
        return ErrorCodeBadParam;
    }

    num_bytes_read = fread(buf, sizeof(*buf), num_bytes_to_read, m_file);
    if (num_bytes_read < num_bytes_to_read) {
        if (ferror(m_file)) {
            return ErrorCodeErrno;
        } else if (feof(m_file)) {
            if (0 == num_bytes_read) {
                return ErrorCodeEndOfFile;
            }
        }
    }

    return ErrorCodeSuccess;
}

ErrorCode FileReader::try_seek_from_begin(size_t pos) {
    if (nullptr == m_file) {
        return ErrorCodeNotInit;
    }

    int retval = fseeko(m_file, pos, SEEK_SET);
    if (0 != retval) {
        return ErrorCodeErrno;
    }

    return ErrorCodeSuccess;
}

ErrorCode FileReader::try_get_pos(size_t& pos) {
    if (nullptr == m_file) {
        return ErrorCodeNotInit;
    }

    pos = ftello(m_file);
    if ((off_t)-1 == pos) {
        return ErrorCodeErrno;
    }

    return ErrorCodeSuccess;
}

ErrorCode FileReader::try_open(string const& path) {
    // Cleanup in case caller forgot to call close before calling this function
    close();

    m_file = fopen(path.c_str(), "rb");
    if (nullptr == m_file) {
        if (ENOENT == errno) {
            return ErrorCodeFileNotFound;
        }
        return ErrorCodeErrno;
    }

    return ErrorCodeSuccess;
}

void FileReader::open(string const& path) {
    ErrorCode error_code = try_open(path);
    if (ErrorCodeSuccess != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
}

void FileReader::close() {
    if (m_file != nullptr) {
        // NOTE: We don't check errors for fclose since it seems the only reason it could fail is if
        // it was interrupted by a signal
        fclose(m_file);
        m_file = nullptr;
    }
}

ErrorCode
FileReader::try_read_to_delimiter(char delim, bool keep_delimiter, bool append, string& str) {
    assert(nullptr != m_file);

    if (false == append) {
        str.clear();
    }
    ssize_t num_bytes_read = getdelim(&m_getdelim_buf, &m_getdelim_buf_len, delim, m_file);
    if (num_bytes_read < 1) {
        if (ferror(m_file)) {
            return ErrorCodeErrno;
        } else if (feof(m_file)) {
            return ErrorCodeEndOfFile;
        }
    }
    if (false == keep_delimiter && delim == m_getdelim_buf[num_bytes_read - 1]) {
        --num_bytes_read;
    }
    str.append(m_getdelim_buf, num_bytes_read);

    return ErrorCodeSuccess;
}

ErrorCode FileReader::try_read_exact_length(char* buf, size_t num_bytes) {
    size_t num_bytes_read;
    auto error_code = try_read(buf, num_bytes, num_bytes_read);
    if (ErrorCodeSuccess != error_code) {
        return error_code;
    }
    if (num_bytes_read < num_bytes) {
        return ErrorCodeTruncated;
    }

    return ErrorCodeSuccess;
}

size_t FileReader::get_pos() {
    size_t pos;
    ErrorCode error_code = try_get_pos(pos);
    if (ErrorCodeSuccess != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }

    return pos;
}

void FileReader::seek_from_begin(size_t pos) {
    ErrorCode error_code = try_seek_from_begin(pos);
    if (ErrorCodeSuccess != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
}
}  // namespace clp_s
