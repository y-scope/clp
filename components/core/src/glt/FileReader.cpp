#include "FileReader.hpp"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>

#include <boost/filesystem.hpp>

using std::string;

namespace glt {
FileReader::~FileReader() {
    close();
    free(m_getdelim_buf);
}

ErrorCode FileReader::try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) {
    if (nullptr == m_file) {
        return ErrorCode_NotInit;
    }
    if (nullptr == buf) {
        return ErrorCode_BadParam;
    }

    num_bytes_read = fread(buf, sizeof(*buf), num_bytes_to_read, m_file);
    if (num_bytes_read < num_bytes_to_read) {
        if (ferror(m_file)) {
            return ErrorCode_errno;
        } else if (feof(m_file)) {
            if (0 == num_bytes_read) {
                return ErrorCode_EndOfFile;
            }
        }
    }

    return ErrorCode_Success;
}

ErrorCode FileReader::try_seek_from_begin(size_t pos) {
    if (nullptr == m_file) {
        return ErrorCode_NotInit;
    }

    int retval = fseeko(m_file, pos, SEEK_SET);
    if (0 != retval) {
        return ErrorCode_errno;
    }

    return ErrorCode_Success;
}

ErrorCode FileReader::try_get_pos(size_t& pos) {
    if (nullptr == m_file) {
        return ErrorCode_NotInit;
    }

    pos = ftello(m_file);
    if ((off_t)-1 == pos) {
        return ErrorCode_errno;
    }

    return ErrorCode_Success;
}

ErrorCode FileReader::try_open(string const& path) {
    // Cleanup in case caller forgot to call close before calling this function
    close();

    m_file = fopen(path.c_str(), "rb");
    if (nullptr == m_file) {
        if (ENOENT == errno) {
            return ErrorCode_FileNotFound;
        }
        return ErrorCode_errno;
    }
    m_path = path;

    return ErrorCode_Success;
}

void FileReader::open(string const& path) {
    ErrorCode error_code = try_open(path);
    if (ErrorCode_Success != error_code) {
        if (ErrorCode_FileNotFound == error_code) {
            throw "File not found: " + boost::filesystem::weakly_canonical(path).string() + "\n";
        } else {
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }
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
            return ErrorCode_errno;
        } else if (feof(m_file)) {
            return ErrorCode_EndOfFile;
        }
    }
    if (false == keep_delimiter && delim == m_getdelim_buf[num_bytes_read - 1]) {
        --num_bytes_read;
    }
    str.append(m_getdelim_buf, num_bytes_read);

    return ErrorCode_Success;
}

ErrorCode FileReader::try_fstat(struct stat& stat_buffer) {
    if (nullptr == m_file) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    auto return_value = fstat(fileno(m_file), &stat_buffer);
    if (0 != return_value) {
        return ErrorCode_errno;
    }
    return ErrorCode_Success;
}
}  // namespace glt
