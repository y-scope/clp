#include "FileReader.hpp"

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

FileReader::~FileReader () {
    close();
    free(m_read_buffer);
}

ErrorCode FileReader::try_get_pos (size_t& pos) {
    if (-1 == m_fd) {
        return ErrorCode_NotInit;
    }

    pos = m_file_pos;
    return ErrorCode_Success;
}

ErrorCode FileReader::refill_reader_buffer (size_t num_bytes_to_read) {
    size_t num_bytes_read;
    num_bytes_read = ::read(m_fd, m_read_buffer, cReaderBufferSize);
    if (num_bytes_read < num_bytes_to_read) {
        reached_eof = true;
    }
    if (num_bytes_read == -1) {
        return ErrorCode_errno;
    }
    reset_buffer(m_read_buffer, num_bytes_read);

    return ErrorCode_Success;
}

ErrorCode FileReader::try_read (char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) {
    if (-1 == m_fd) {
        return ErrorCode_NotInit;
    }
    if (nullptr == buf) {
        return ErrorCode_BadParam;
    }
    // Question: should we delay the fseek?
    if (started_reading == false) {
        started_reading = true;
        auto offset = lseek(m_fd, m_file_pos, SEEK_SET);
        if (offset == -1) {
            return ErrorCode_errno;
        }
    }

    num_bytes_read = 0;
    size_t num_bytes_to_read_from_buffer {num_bytes_to_read};
    size_t num_bytes_read_from_buffer;

    // keep reading
    bool finish_reading = false;
    while (false == finish_reading) {
        auto error_code = BufferReader::try_read(buf + num_bytes_read,
                                                 num_bytes_to_read_from_buffer,
                                                 num_bytes_read_from_buffer);
        if (ErrorCode_NotInit == error_code) {
            // else, we refill the buffer
            error_code = refill_reader_buffer(cReaderBufferSize);
            if (ErrorCode_Success != error_code) {
                return error_code;
            }
        } else {
            m_file_pos += num_bytes_read_from_buffer;
            num_bytes_read += num_bytes_read_from_buffer;
            num_bytes_to_read_from_buffer -= num_bytes_read_from_buffer;
            if (ErrorCode_Success == error_code) {
                // if success, means the buffer still has enough data to read from
                finish_reading = true;
            } else if (ErrorCode_EndOfFile == error_code) {
                // if the buffer is not loaded or has been exhausted.
                // simply return
                if (reached_eof) {
                    if (num_bytes_read == 0) {
                        return ErrorCode_EndOfFile;
                    }
                    return ErrorCode_Success;
                }
                // else, we refill the buffer
                error_code = refill_reader_buffer(cReaderBufferSize);
                if (ErrorCode_Success != error_code) {
                    return error_code;
                }
            } else {
                // else some unexpected error code is encountered.
                throw OperationFailed(error_code, __FILENAME__, __LINE__);
            }
        }
    }
    return ErrorCode_Success;
}

// Maybe everytime, I should always read a page?
ErrorCode FileReader::try_seek_from_begin (size_t pos) {
    if (m_fd == -1) {
        return ErrorCode_NotInit;
    }
    //TODO: do we need to detect out of range seek?
//    struct stat st;
//    fstat(m_fd, &st);
//    off_t size = st.st_size;
//    if (pos >= size) {
//        return ErrorCode_EndOfFile;
//    }

    // if we are at A, readed something, seek to B which is on another buffer place.
    // and seek back to A, how will this be handled
    if (pos > m_file_pos) {
        auto front_seek_amount = pos - m_file_pos;
        if (front_seek_amount > m_size - m_cursor_pos) {
            // if the seek-to pos is out of buffer
            printf("Seek front on %d\n", m_fd);
            m_size = 0;
            m_cursor_pos = 0;
            m_file_pos = pos;
        } else {
            // otherwise, we can simply
            printf("simple seek front on %d\n", m_fd);
            m_buffer_pos += front_seek_amount;
            m_file_pos = pos;
        }
    } else {
        printf("Seek back on %d\n", m_fd);
        // the maximum value we can seek back is m_buffer_pos;
        if(started_reading == false) {
            m_file_pos = pos;
        } else {
            auto seek_back_amount = m_file_pos - pos;
            if (seek_back_amount > m_buffer_pos) {
                SPDLOG_ERROR("Can't back trace anymore");
                throw;
            } else {
                m_buffer_pos = m_buffer_pos - seek_back_amount;
                m_file_pos = pos;
            }
        }
    }
    return ErrorCode_Success;
}


ErrorCode FileReader::try_open (const string& path) {
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
    m_file_pos = 0;
    reached_eof = false;
    started_reading = false;

    // Buffer specific things
    reset_buffer(nullptr, 0);
    return ErrorCode_Success;
}

ErrorCode FileReader::try_read_to_delimiter (char delim, bool keep_delimiter, bool append, string& str) {
    assert(-1 != m_fd);

    if (false == append) {
        str.clear();
    }

    bool found_delim {false};

    while (false == found_delim) {
        auto cursor {m_cursor_pos};
        while (cursor < m_size && false == found_delim) {
            if (delim == m_buffer[cursor]) {
                found_delim = true;
            }
            cursor++;
        }
        // append to strings
        std::string_view substr(reinterpret_cast<const char*>(m_buffer + m_cursor_pos),
                                cursor - m_cursor_pos);
        str.append(substr);
        // increase file pos
        m_file_pos += cursor - m_cursor_pos;
        m_cursor_pos = cursor;
        if (false == found_delim) {
            if (reached_eof) {
                return ErrorCode_EndOfFile;
            }
            if (auto error_code = refill_reader_buffer(cReaderBufferSize);
                    ErrorCode_Success != error_code) {
                return error_code;
            }
        }
    }
    return ErrorCode_Success;
}

void FileReader::open (const string& path) {
    ErrorCode error_code = try_open(path);
    if (ErrorCode_Success != error_code) {
        if (ErrorCode_FileNotFound == error_code) {
            throw "File not found: " + boost::filesystem::weakly_canonical(path).string() + "\n";
        } else {
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }
    }
}

void FileReader::close () {
    if (-1 != m_fd) {
        // NOTE: We don't check errors for fclose since it seems the only reason it could fail is if it was interrupted
        // by a signal
        ::close(m_fd);
        m_fd = -1;
    }
}

ErrorCode FileReader::try_fstat (struct stat& stat_buffer) {
    if (-1 == m_fd) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    auto return_value = fstat(m_fd, &stat_buffer);
    if (0 != return_value) {
        return ErrorCode_errno;
    }
    return ErrorCode_Success;
}
