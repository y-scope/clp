#include "BufferedFileReader.hpp"

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

using std::make_unique;
using std::move;
using std::string;

BufferedFileReader::BufferedFileReader () {
    m_file_pos = 0;
    m_fd = -1;
    m_checkpoint_enabled = false;
    if (ErrorCode_Success != set_buffer_size(DefaultBufferSize)) {
        throw "Failed to init reader buffer size\n";
    }
    m_read_buffer = make_unique<int8_t[]>(m_reader_buffer_size);
}

BufferedFileReader::~BufferedFileReader () {
    close();
}

ErrorCode BufferedFileReader::try_get_pos (size_t& pos) {
    if (-1 == m_fd) {
        return ErrorCode_NotInit;
    }
    pos = m_file_pos;
    return ErrorCode_Success;
}

static ErrorCode try_read_into_buffer(int fd, int8_t* buffer, size_t num_bytes_to_read,
                                      size_t& num_bytes_read) {
    num_bytes_read = 0;
    // keep reading from the fd until seeing a 0, which means eof
    while (true) {
        auto bytes_read = ::read(fd, buffer + num_bytes_read, num_bytes_to_read);
        if (bytes_read == -1) {
            return ErrorCode_errno;
        }
        if (bytes_read == 0) {
            break;
        }
        num_bytes_read += bytes_read;
        num_bytes_to_read -= bytes_read;
        if (num_bytes_to_read == 0) {
            return ErrorCode_Success;
        }
    }
    if (num_bytes_read == 0) {
        return ErrorCode_EndOfFile;
    }
    return ErrorCode_Success;
}

ErrorCode BufferedFileReader::refill_reader_buffer (size_t refill_size) {
    size_t num_bytes_refilled;
    return refill_reader_buffer (refill_size, num_bytes_refilled);
}

ErrorCode BufferedFileReader::refill_reader_buffer (size_t refill_size,
                                                    size_t& num_bytes_refilled) {
    num_bytes_refilled = 0;
    if (false == m_checkpoint_enabled) {
        // recover from a previous reset
        if (m_size > refill_size) {
            m_read_buffer = make_unique<int8_t[]>(refill_size);
        }
        auto error_code = try_read_into_buffer(m_fd, m_read_buffer.get(),
                                               refill_size, num_bytes_refilled);
        if (error_code != ErrorCode_Success) {
            return error_code;
        }
        m_cursor_pos = 0;
        if (m_data == nullptr) {
            m_cursor_pos = m_file_pos & m_reader_buffer_cursor_mask;
            m_data = m_read_buffer.get();
        }
        m_size = num_bytes_refilled;

    } else {
        // Messy way of copying data from old buffer to new buffer
        auto new_buffer = make_unique<int8_t[]>(m_size + refill_size);
        memcpy(new_buffer.get(), m_read_buffer.get(), m_size);
        m_read_buffer = std::move(new_buffer);
        auto error_code = try_read_into_buffer(m_fd, m_read_buffer.get() + m_size, refill_size,
                                               num_bytes_refilled);
        if (error_code != ErrorCode_Success) {
            return error_code;
        }

        if (nullptr == m_data) {
            // if buffer is uninitialized, reset the cursor_pos
            // in case it's after a seek
            m_cursor_pos = m_file_pos & m_reader_buffer_cursor_mask;
        }
        m_data = m_read_buffer.get();
        m_size += num_bytes_refilled;

    }
    return ErrorCode_Success;
}

ErrorCode BufferedFileReader::try_read (char* buf, size_t num_bytes_to_read,
                                        size_t& num_bytes_read) {
    if (-1 == m_fd) {
        return ErrorCode_NotInit;
    }
    if (nullptr == buf) {
        return ErrorCode_BadParam;
    }

    num_bytes_read = 0;
    size_t num_bytes_to_read_from_buffer {num_bytes_to_read};
    size_t num_bytes_read_from_buffer {0};
    // keep reading until enough data is read or an eof is seen
    while (true) {
        auto error_code = BufferReader::try_read(buf + num_bytes_read,
                                                 num_bytes_to_read_from_buffer,
                                                 num_bytes_read_from_buffer);
        if (ErrorCode_Success == error_code ||
            ErrorCode_EndOfFile == error_code ||
            ErrorCode_NotInit == error_code) {
            m_file_pos += num_bytes_read_from_buffer;
            num_bytes_read += num_bytes_read_from_buffer;
            num_bytes_to_read_from_buffer -= num_bytes_read_from_buffer;
            if (num_bytes_to_read_from_buffer == 0) {
                break;
            }
            // refill the buffer if more bytes are to be read
            error_code = refill_reader_buffer(m_reader_buffer_size);
            if (ErrorCode_EndOfFile == error_code) {
                if (num_bytes_read == 0) {
                    return ErrorCode_EndOfFile;
                } else {
                    break;
                }
            }
            else if (ErrorCode_Success != error_code) {
                return error_code;
            }
        } else {
            return error_code;
        }
    }
    return ErrorCode_Success;
}

ErrorCode BufferedFileReader::try_seek_from_begin (size_t pos) {
    if (m_fd == -1) {
        return ErrorCode_NotInit;
    }
    // early return path
    if (pos == m_file_pos) {
        return ErrorCode_Success;
    }

    if (pos <= m_file_pos) {
        if (false == m_checkpoint_enabled) {
            SPDLOG_ERROR("Seek back not allowed when checkpoint is not enabled");
            return ErrorCode_Failure;
        }
        if (pos < m_checkpoint_pos) {
            SPDLOG_ERROR("Seek back before the checkpoint is not supported");
            return ErrorCode_Failure;
        }
        m_cursor_pos -= (m_file_pos - pos);
        m_file_pos = pos;
    } else {
        auto buffer_available_data = m_size - m_cursor_pos;
        auto seek_distance = pos - m_file_pos;
        if (seek_distance <= buffer_available_data) {
            m_cursor_pos += seek_distance;
            m_file_pos = pos;
        } else if (false == m_checkpoint_enabled) {
            auto buffer_aligned_pos = pos & m_reader_buffer_aligned_mask;
            auto offset = lseek(m_fd, buffer_aligned_pos, SEEK_SET);
            if (offset == -1) {
                return ErrorCode_errno;
            }
            // invalidate buffered_data
            reset_buffer(nullptr, 0);
            m_file_pos = pos;
        } else {
            size_t num_bytes_to_refill = seek_distance - buffer_available_data;
            size_t num_bytes_refilled {0};
            while (true) {
                auto error_code = refill_reader_buffer(m_reader_buffer_size, num_bytes_refilled);
                if (ErrorCode_EndOfFile == error_code) {
                    SPDLOG_ERROR("not expecting to seek pass the Entire file");
                    throw;
                }
                else if (ErrorCode_Success != error_code) {
                    return error_code;
                }
                if (num_bytes_to_refill <= m_reader_buffer_size) {
                    m_file_pos = pos;
                    m_cursor_pos += seek_distance;
                    break;
                }
                num_bytes_to_refill -= num_bytes_refilled;
            }
        }
    }
    return ErrorCode_Success;
}


ErrorCode BufferedFileReader::try_open (const string& path) {
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
    reset_buffer(m_read_buffer.get(), 0);
    return ErrorCode_Success;
}

ErrorCode BufferedFileReader::try_read_to_delimiter (char delim, bool keep_delimiter,
                                             bool append, string& str) {
    if (-1 == m_fd) {
        return ErrorCode_NotInit;
    }

    if (false == append) {
        str.clear();
    }

    bool found_delim {false};
    size_t delim_pos;
    while (false == found_delim) {
        auto remaining_data_size = m_size - m_cursor_pos;
        // find the pointer pointing to the delimiter
        const auto* delim_ptr =
                reinterpret_cast<const int8_t*>(memchr(m_data + m_cursor_pos,
                                                       delim, remaining_data_size));
        if (delim_ptr != nullptr) {
            delim_pos = (delim_ptr - m_data) + 1;
            found_delim = true;
        } else {
            delim_pos = m_size;
        }
        // append to strings
        size_t copy_length = delim_pos - m_cursor_pos;
        std::string_view substr(reinterpret_cast<const char*>(m_data + m_cursor_pos),
                                copy_length);
        str.append(substr);
        // increment file pos to the delimiter or the end of file
        m_file_pos += copy_length;
        m_cursor_pos = delim_pos;
        if (false == found_delim) {
            if (auto error_code = refill_reader_buffer(m_reader_buffer_size);
                    ErrorCode_Success != error_code) {
                return error_code;
            }
        }
    }
    return ErrorCode_Success;
}

void BufferedFileReader::open (const string& path) {
    ErrorCode error_code = try_open(path);
    if (ErrorCode_Success != error_code) {
        if (ErrorCode_FileNotFound == error_code) {
            throw "File not found: " + boost::filesystem::weakly_canonical(path).string() + "\n";
        } else {
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }
    }
}

void BufferedFileReader::revert_pos() {
    if (false == m_checkpoint_enabled) {
        SPDLOG_ERROR("Checkpoint is not enabled");
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_file_pos = m_checkpoint_pos;
    m_cursor_pos = m_checkpointed_buffer_pos;
}

size_t BufferedFileReader::mark_pos() {
    if (true == m_checkpoint_enabled) {
        SPDLOG_ERROR("I haven't carefully think about whether we should allow this or not");
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_checkpoint_pos = m_file_pos;
    m_checkpointed_buffer_pos = m_cursor_pos;
    m_checkpoint_enabled = true;
    return m_file_pos;
}

void BufferedFileReader::reset_checkpoint () {
    // alternatively, we can keep claiming back the memory
    if (false == m_checkpoint_enabled) {
        return;
    }
    if (m_size != m_reader_buffer_size) {
        // allocate new buffer for buffered data that hasn't been seek passed
        auto copy_pos = m_cursor_pos & m_reader_buffer_aligned_mask;
        auto copy_size = m_size - copy_pos;
        // Use a quantized size for the new buffer size
        auto new_buffer_size = (1 + ((copy_size - 1) >> m_reader_buffer_exp))
                                        << m_reader_buffer_exp;

        auto new_buffer = make_unique<int8_t[]>(new_buffer_size);
        memcpy(new_buffer.get(), m_data + copy_pos, copy_size);
        m_read_buffer = std::move(new_buffer);
        m_data = m_read_buffer.get();

        m_size = copy_size;
        m_cursor_pos -= copy_pos;
    }
    m_checkpoint_enabled = false;
}

ErrorCode BufferedFileReader::set_buffer_size (size_t buffer_size) {
    if (m_fd != -1) {
        SPDLOG_ERROR("Buffer size can not be changed when the file is open");
        return ErrorCode_Failure;
    }
    if (buffer_size == 0) {
        SPDLOG_ERROR("Buffer size can not be set to 0");
        return ErrorCode_BadParam;
    }
    if (buffer_size % 4096 != 0) {
        SPDLOG_ERROR("Buffer size {} is not a multiple of page size", buffer_size);
        return ErrorCode_BadParam;
    }
    // Calculate the logarithm base 2 of the number
    double exponent = log(buffer_size) / log(2);
    if (ceil(exponent) != floor(exponent)) {
        SPDLOG_ERROR("Buffer size {} is not a power of 2", buffer_size);
        return ErrorCode_BadParam;
    }

    m_reader_buffer_exp = static_cast<size_t>(exponent);
    m_reader_buffer_size = buffer_size;
    m_reader_buffer_aligned_mask = ~(m_reader_buffer_size - 1);
    m_reader_buffer_cursor_mask = m_reader_buffer_size - 1;
    return ErrorCode_Success;
}

void BufferedFileReader::close () {
    if (-1 != m_fd) {
        // NOTE: We don't check errors for fclose since it seems
        // the only reason it could fail is if it was interrupted by a signal
        ::close(m_fd);
        m_fd = -1;

        if (m_checkpoint_enabled) {
            SPDLOG_DEBUG("close file without resetting checkpoint");
            m_read_buffer = make_unique<int8_t[]>(m_reader_buffer_size);
            m_checkpoint_enabled = false;
        }
    }
}

ErrorCode BufferedFileReader::try_fstat (struct stat& stat_buffer) const {
    if (-1 == m_fd) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    auto return_value = fstat(m_fd, &stat_buffer);
    if (0 != return_value) {
        return ErrorCode_errno;
    }
    return ErrorCode_Success;
}
