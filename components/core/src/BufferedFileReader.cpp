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

static ErrorCode try_read_into_buffer(int fd, char* buffer, size_t num_bytes_to_read,
                                      size_t& num_bytes_read);

BufferedFileReader::BufferedFileReader () {
    m_file_pos = 0;
    m_fd = -1;
    m_checkpoint_enabled = false;
    if (auto error_code = set_buffer_size(cDefaultBufferSize);
        ErrorCode_Success != error_code) {
        SPDLOG_ERROR("Failed to init reader buffer size to be {}", cDefaultBufferSize);
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
    m_buffer = make_unique<char[]>(m_buffer_size);
}

BufferedFileReader::~BufferedFileReader () {
    close();
}

size_t BufferedFileReader::remaining_data_size () const {
    if (m_data_size == 0) {
        return 0;
    }
    assert(m_data_size >= cursor_pos());
    return m_data_size - cursor_pos();
}

ErrorCode BufferedFileReader::try_get_pos (size_t& pos) {
    if (-1 == m_fd) {
        return ErrorCode_NotInit;
    }
    pos = m_file_pos;
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
        if (m_data_size > refill_size) {
            m_buffer = make_unique<char[]>(refill_size);
        }
        auto error_code = try_read_into_buffer(m_fd, m_buffer.get(),
                                               refill_size, num_bytes_refilled);
        if (error_code != ErrorCode_Success) {
            return error_code;
        }
        m_buffer_begin_pos = m_file_pos & m_buffer_aligned_mask;
        m_data_size = num_bytes_refilled;
    } else {
        // Messy way of copying data from old buffer to new buffer
        auto new_buffer = make_unique<char[]>(m_data_size + refill_size);
        memcpy(new_buffer.get(), m_buffer.get(), m_data_size);
        auto error_code = try_read_into_buffer(m_fd, &new_buffer[m_data_size], refill_size,
                                               num_bytes_refilled);
        m_buffer = std::move(new_buffer);
        if (error_code != ErrorCode_Success) {
            return error_code;
        }
        m_data_size += num_bytes_refilled;

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
    // keep reading until enough data is read or an eof is seen
    while (true) {
        auto avaiable_bytes_for_read = std::min(num_bytes_to_read_from_buffer,
                                                remaining_data_size());
        memcpy(buf + num_bytes_read, buffer_head(), avaiable_bytes_for_read);

        num_bytes_to_read_from_buffer -= avaiable_bytes_for_read;
        num_bytes_read += avaiable_bytes_for_read;

        m_file_pos += avaiable_bytes_for_read;
        if (num_bytes_to_read_from_buffer == 0) {
            break;
        }
        // refill the buffer if more bytes are to be read
        auto error_code = refill_reader_buffer(m_buffer_size);
        if (ErrorCode_EndOfFile == error_code) {
           break;
        } else if (ErrorCode_Success != error_code) {
            return error_code;
        }
    }

    if (num_bytes_read == 0) {
        return ErrorCode_EndOfFile;
    }
    return ErrorCode_Success;
}

ErrorCode BufferedFileReader::try_seek_from_begin (size_t pos) {
    if (m_fd == -1) {
        return ErrorCode_NotInit;
    }
    if (pos == m_file_pos) {
        return ErrorCode_Success;
    }

    if (pos <= m_file_pos) {
        if (false == m_checkpoint_enabled) {
            SPDLOG_ERROR("Error: Seek back when checkpoint is not enabled");
            return ErrorCode_Failure;
        }
        if (pos < m_checkpoint_pos) {
            SPDLOG_ERROR("Error: trying to seek to {} which is ahead of checkpoint: {}",
                         pos, m_checkpoint_pos);
            return ErrorCode_Failure;
        }
    } else {
        auto buffer_available_data = remaining_data_size();
        auto seek_distance = pos - m_file_pos;
        if (seek_distance <= buffer_available_data) {
            m_file_pos = pos;
            return ErrorCode_Success;
        }
        // Handle the case where buffer doesn't contain enough data for seek
        if (false == m_checkpoint_enabled) {
            m_buffer_begin_pos = pos & m_buffer_aligned_mask;
            auto offset = lseek(m_fd, m_buffer_begin_pos, SEEK_SET);
            if (offset == -1) {
                return ErrorCode_errno;
            }
            // invalidate buffered_data
            m_data_size = 0;
        } else {
            // Note: we can safely assume that m_size will be a multiple of
            // m_reader_buffer_size. if m_size is not a multiple of
            // m_reader_buffer_size, if must mean the file has reached EoF
            // and the code will throw an error anyway
            size_t num_bytes_to_refill = pos - m_buffer_begin_pos + m_data_size;
            size_t quantizied_refill_size =
                    (1 + ((num_bytes_to_refill - 1) >> m_buffer_exp)) << m_buffer_exp;
            size_t num_bytes_refilled {0};
            auto error_code = refill_reader_buffer(quantizied_refill_size, num_bytes_refilled);
            if (ErrorCode_EndOfFile == error_code || num_bytes_refilled < num_bytes_to_refill) {
                SPDLOG_ERROR("not expecting to seek pass the Entire file");
                throw OperationFailed(ErrorCode_EndOfFile, __FILENAME__, __LINE__);
            }
            if (ErrorCode_Success != error_code) {
                return error_code;
            }
        }
    }

    m_file_pos = pos;
    return ErrorCode_Success;
}

ErrorCode BufferedFileReader::peek_buffered_data (size_t size_to_peek, const char*& data_ptr,
                                                  size_t& peek_size) {
    if (-1 == m_fd) {
        return ErrorCode_NotInit;
    }
    // Refill the buffer if necessary
    if (0 == m_data_size) {
        auto error_code = refill_reader_buffer(m_buffer_size);
        if (ErrorCode_Success != error_code) {
            data_ptr = nullptr;
            peek_size = 0;
            return error_code;
        }
    }
    peek_size = std::min(size_to_peek, remaining_data_size());
    data_ptr = reinterpret_cast<const char*>(buffer_head());
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
    m_data_size = 0;
    m_buffer_begin_pos = 0;
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
        // find the pointer pointing to the delimiter
        const auto* delim_ptr = reinterpret_cast<const char*>(memchr(buffer_head(), delim,
                                                                     remaining_data_size()));
        if (delim_ptr != nullptr) {
            delim_pos = (delim_ptr - m_buffer.get()) + 1;
            found_delim = true;
        } else {
            delim_pos = m_data_size;
        }
        // append to strings
        size_t str_length = delim_pos - cursor_pos();
        str.append(reinterpret_cast<const char*>(buffer_head()), str_length);

        m_file_pos += str_length;
        if (false == found_delim) {
            if (auto error_code = refill_reader_buffer(m_buffer_size);
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
}

size_t BufferedFileReader::mark_pos() {
    if (true == m_checkpoint_enabled) {
        SPDLOG_ERROR("I haven't carefully think about whether we should allow this or not");
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_checkpoint_pos = m_file_pos;
    m_checkpoint_enabled = true;
    return m_file_pos;
}

void BufferedFileReader::reset_checkpoint () {
    // alternatively, we can keep claiming back the memory
    if (false == m_checkpoint_enabled) {
        return;
    }
    if (m_data_size != m_buffer_size) {
        // allocate new buffer for buffered data that hasn't been seek passed
        auto copy_pos = cursor_pos() & m_buffer_aligned_mask;
        m_data_size -= copy_pos;
        // Use a quantized size for the new buffer size
        auto new_buffer_size = (1 + ((m_data_size - 1) >> m_buffer_exp)) << m_buffer_exp;

        auto new_buffer = make_unique<char[]>(new_buffer_size);
        memcpy(new_buffer.get(), &m_buffer[copy_pos], m_data_size);
        m_buffer = std::move(new_buffer);
        m_buffer_begin_pos += copy_pos;
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

    m_buffer_exp = static_cast<size_t>(exponent);
    m_buffer_size = buffer_size;
    m_buffer_aligned_mask = ~(m_buffer_size - 1);
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
            m_buffer = make_unique<char[]>(m_buffer_size);
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

static ErrorCode try_read_into_buffer(int fd, char* buffer, size_t num_bytes_to_read,
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
