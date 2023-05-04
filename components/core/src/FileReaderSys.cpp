#include "FileReaderSys.hpp"

// Boost libraries
#include <boost/filesystem.hpp>

// C standard libraries
// C libraries
#include <sys/stat.h>
#include <fcntl.h>

// C++ standard libraries
#include <cerrno>

// Project headers
#include <spdlog/spdlog.h>

using std::string;

FileReaderSys::~FileReaderSys () {
    close();
    m_read_buffer.release();
    free(m_getdelim_buf);
}

ErrorCode FileReaderSys::try_get_pos (size_t& pos) {
    if (-1 == m_fd) {
        return ErrorCode_NotInit;
    }

    pos = m_file_pos;
    return ErrorCode_Success;
}

ErrorCode FileReaderSys::try_read (char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) {
    if (-1 == m_fd) {
        return ErrorCode_NotInit;
    }
    if (nullptr == buf) {
        return ErrorCode_BadParam;
    }

    size_t remaining_data = m_buffer_length - m_buffer_pos;

    if (num_bytes_to_read <= remaining_data) {
        memcpy(buf, m_read_buffer.get() + m_buffer_pos, num_bytes_to_read);
        // increment cursors
        m_buffer_pos += num_bytes_to_read;
        m_file_pos += num_bytes_to_read;
        return ErrorCode_Success;
    } else {
        // first, read everything from buffer
        size_t next_partial_read = num_bytes_to_read - remaining_data;
        memcpy(buf, m_read_buffer.get() + m_buffer_pos, num_bytes_to_read);

        // refill the buffer
        m_buffer_length = ::read(m_fd, m_read_buffer.get(), cReaderBufferSize);
        if (m_buffer_length == -1) {
            return ErrorCode_errno;
        }
        if (m_buffer_length < next_partial_read) {
            memcpy(buf + remaining_data, m_read_buffer.get(), m_buffer_length);
            m_buffer_pos = m_buffer_length;
            num_bytes_read = remaining_data + m_buffer_length;
            m_file_pos += num_bytes_read;
            return ErrorCode_EndOfFile;
        } else {
            memcpy(buf + remaining_data, m_read_buffer.get(), next_partial_read);
            m_buffer_pos = next_partial_read;
            num_bytes_read = num_bytes_to_read;
            m_file_pos += num_bytes_read;
            return ErrorCode_Success;
        }
    }
}

ErrorCode FileReaderSys::try_seek_from_begin (size_t pos) {
    if (m_fd == -1) {
        return ErrorCode_NotInit;
    }
    if (pos > m_file_pos) {
        auto offset = lseek(m_fd, pos, SEEK_SET);
        if (offset != pos) {
            return ErrorCode_errno;
        }
        m_buffer_length = 0;
        m_buffer_pos = 0;
        m_file_pos = pos;
    } else {
        // the maximum value we can seek back is m_buffer_pos;
        auto seek_back_amount = m_file_pos - pos;
        if (seek_back_amount > m_buffer_pos) {
            SPDLOG_ERROR("Can back trace anymore");
            throw;
        } else {
            m_buffer_pos = m_buffer_pos - seek_back_amount;
            m_file_pos = pos;
        }
    }
    return ErrorCode_Success;
}


ErrorCode FileReaderSys::try_open (const string& path) {
    // Cleanup in case caller forgot to call close before calling this function
    close();

    m_fd = ::open(path.c_str(), O_RDONLY);
    if (-1 == m_fd) {
        if (ENOENT == errno) {
            return ErrorCode_FileNotFound;
        }
        return ErrorCode_errno;
    }
    m_path = path;
    m_read_buffer = 0;
    m_file_pos = 0;
    m_buffer_length = 0;

    return ErrorCode_Success;
}

ErrorCode FileReaderSys::try_read_to_delimiter (char delim, bool keep_delimiter, bool append, string& str) {
    assert(-1 != m_fd);

    if (false == append) {
        str.clear();
    }

    bool found_delim {false};

    while (false == found_delim) {
        auto cursor {m_buffer_pos};
        while (cursor < m_buffer_length) {
            if (delim == m_read_buffer[cursor]) {
                found_delim = true;
                break;
            }
            cursor++;
        }
        if (found_delim) {
            // append to strings
            std::string_view substr {m_read_buffer.get() + m_buffer_pos, cursor - m_buffer_pos};
            str.append(substr);
            // increase file pos
            m_file_pos += cursor - m_buffer_pos;
            m_buffer_pos = cursor;
        } else {
            // if we didn't find a delimiter, we append the current buffer to the str and
            // read out a new buffer
            std::string_view substr {m_read_buffer.get() + m_buffer_pos, m_buffer_length - m_buffer_pos};
            str.append(substr);
            m_file_pos += m_buffer_length - m_buffer_pos;
            m_buffer_pos = 0;
            // refill the buffer
            m_buffer_length = ::read(m_fd, m_read_buffer.get(), cReaderBufferSize);
            if (m_buffer_length == -1) {
                return ErrorCode_errno;
            }
            if (m_buffer_length == 0) {
                return ErrorCode_EndOfFile;
            }
        }
    }
    return ErrorCode_Success;
}

void FileReaderSys::open (const string& path) {
    ErrorCode error_code = try_open(path);
    if (ErrorCode_Success != error_code) {
        if (ErrorCode_FileNotFound == error_code) {
            throw "File not found: " + boost::filesystem::weakly_canonical(path).string() + "\n";
        } else {
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }
    }
}

void FileReaderSys::close () {
    if (-1 != m_fd) {
        // NOTE: We don't check errors for fclose since it seems the only reason it could fail is if it was interrupted
        // by a signal
        auto res = ::close(m_fd);
        if (0 != res) {
            throw "Not sure why close fail\n";
        }
        m_fd = -1;
    }
}

ErrorCode FileReaderSys::try_fstat (struct stat& stat_buffer) {
    if (-1 == m_fd) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    auto return_value = fstat(m_fd, &stat_buffer);
    if (0 != return_value) {
        return ErrorCode_errno;
    }
    return ErrorCode_Success;
}
