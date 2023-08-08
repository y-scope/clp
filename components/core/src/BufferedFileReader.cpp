#include "BufferedFileReader.hpp"

// Boost libraries
#include <boost/filesystem.hpp>

// C standard libraries
#include <fcntl.h>

// C++ standard libraries
#include <cerrno>

// Project headers
#include <spdlog/spdlog.h>

using std::make_unique;
using std::move;
using std::string;

namespace {
    ErrorCode try_read_into_buffer(int fd, char* buf, size_t num_bytes_to_read,
                                   size_t& num_bytes_read) {
        num_bytes_read = 0;
        while (true) {
            const auto bytes_read = ::read(fd, buf, num_bytes_to_read);
            if (bytes_read > 0) {
                buf += bytes_read;
                num_bytes_read += bytes_read;
                num_bytes_to_read -= bytes_read;
                if (num_bytes_read == num_bytes_to_read) {
                    return ErrorCode_Success;
                }
            } else if (0 == bytes_read) {
                break;
            } else {
                return ErrorCode_errno;
            }
        }
        if (0 == num_bytes_read) {
            return ErrorCode_EndOfFile;
        }
        return ErrorCode_Success;
    }
}

BufferedFileReader::BufferedFileReader (size_t base_buffer_size) {
    if (base_buffer_size % cMinBufferSize != 0) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    m_base_buffer_size = base_buffer_size;
    m_buffer_size = m_base_buffer_size;
    m_buffer = make_unique<char[]>(m_buffer_size);
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

ErrorCode BufferedFileReader::try_seek_from_begin (size_t pos) {
    if (-1 == m_fd) {
        return ErrorCode_NotInit;
    }
    if (pos == m_file_pos) {
        return ErrorCode_Success;
    }

    if (m_checkpoint_pos.has_value() == false) {
        if (pos < m_file_pos) {
            return ErrorCode_Failure;
        }
        if (ErrorCode_Success ==
            m_buffer_reader->try_seek_from_begin(get_buffer_relative_pos(pos))){
            m_file_pos = pos;
            m_highest_read_pos = std::max(m_highest_read_pos, m_file_pos);
            return ErrorCode_Success;
        }
        // if checkpoint is not set, simply move the file_pos and invalidate the buffer reader
        auto offset = lseek(m_fd, static_cast<__off64_t>(pos), SEEK_SET);
        if (-1 == offset) {
            return ErrorCode_errno;
        }
        m_buffer_reader.emplace(m_buffer.get(), 0);
        m_buffer_begin_pos = pos;
    } else {
        if (pos < m_checkpoint_pos) {
            return ErrorCode_Failure;
        }
        if (ErrorCode_Success ==
            m_buffer_reader->try_seek_from_begin(get_buffer_relative_pos(pos))) {
            m_file_pos = pos;
            m_highest_read_pos = std::max(m_highest_read_pos, m_file_pos);
            return ErrorCode_Success;
        }

        size_t num_bytes_to_refill = pos - get_buffer_end_pos();
        auto error_code = refill_reader_buffer(num_bytes_to_refill);
        if (ErrorCode_EndOfFile == error_code) {
            throw OperationFailed(ErrorCode_EndOfFile, __FILENAME__, __LINE__);
        }
        if (ErrorCode_Success != error_code) {
            return error_code;
        }
        if (ErrorCode_Success != m_buffer_reader->try_seek_from_begin(get_buffer_relative_pos(pos))) {
            throw OperationFailed(ErrorCode_EndOfFile, __FILENAME__, __LINE__);
        }
    }
    m_file_pos = pos;
    m_highest_read_pos = std::max(m_highest_read_pos, m_file_pos);
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
    if (num_bytes_to_read == 0) {
        return ErrorCode_Success;
    }

    num_bytes_read = 0;
    while (true) {
        size_t bytes_read {0};
        auto error_code = m_buffer_reader->try_read(buf, num_bytes_to_read, bytes_read);
        if (ErrorCode_Success == error_code) {
            buf += bytes_read;
            num_bytes_read += bytes_read;
            num_bytes_to_read -= bytes_read;
            m_file_pos += bytes_read;
            if (0 == num_bytes_to_read) {
                break;
            }
        } else if (ErrorCode_EndOfFile != error_code) {
            return error_code;
        }

        error_code = refill_reader_buffer(m_base_buffer_size);
        if (ErrorCode_EndOfFile == error_code) {
            break;
        } else if (ErrorCode_Success != error_code) {
            return error_code;
        }
    }
    if (0 == num_bytes_read) {
        return ErrorCode_EndOfFile;
    }
    m_highest_read_pos = std::max(m_highest_read_pos, m_file_pos);
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
    size_t total_append_length {0};
    while (false == found_delim) {
        size_t length {0};
        if (auto ret_code = m_buffer_reader->try_read_to_delimiter(delim, keep_delimiter, str, found_delim, length);
            ret_code != ErrorCode_Success && ret_code != ErrorCode_EndOfFile) {
            return ret_code;
        }
        m_file_pos += length;
        total_append_length += length;

        if (false == found_delim) {
            auto error_code = refill_reader_buffer(m_base_buffer_size);
            if (ErrorCode_EndOfFile == error_code) {
                if (total_append_length == 0) {
                    return ErrorCode_EndOfFile;
                }
                return ErrorCode_Success;
            } else if (ErrorCode_Success != error_code) {
                return error_code;
            }
        }
    }
    m_highest_read_pos = std::max(m_highest_read_pos, m_file_pos);
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
    m_buffer_begin_pos = 0;
    m_buffer_reader.emplace(m_buffer.get(), 0);
    m_highest_read_pos = 0;
    return ErrorCode_Success;
}

void BufferedFileReader::open (const string& path) {
    ErrorCode error_code = try_open(path);
    if (ErrorCode_Success != error_code) {
        if (ErrorCode_FileNotFound == error_code) {
            SPDLOG_ERROR("File not found: {}", boost::filesystem::weakly_canonical(path).string());
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        } else {
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }
    }
}

void BufferedFileReader::close () {
    if (-1 != m_fd) {
        // NOTE: We don't check errors for fclose since it seems
        // the only reason it could fail is if it was interrupted by a signal
        ::close(m_fd);
        m_fd = -1;

        if (m_checkpoint_pos.has_value()) {
            m_buffer_size = m_base_buffer_size;
            m_buffer = make_unique<char[]>(m_buffer_size);
            m_checkpoint_pos.reset();
        }
    }
}

size_t BufferedFileReader::set_checkpoint() {
    if (m_checkpoint_pos.has_value() && m_checkpoint_pos < m_file_pos) {
        if (m_buffer_reader->get_buffer_size() != m_base_buffer_size) {
            // allocate new buffer for buffered data starting from pos
            resize_buffer_from_pos(m_buffer_reader->get_pos());
            m_buffer_reader->seek_from_begin(get_buffer_relative_pos(m_file_pos));
        }
    }
    m_checkpoint_pos = m_file_pos;
    return m_file_pos;
}

void BufferedFileReader::clear_checkpoint () {
    if (false == m_checkpoint_pos.has_value()) {
        return;
    }

    m_file_pos = m_highest_read_pos;
    resize_buffer_from_pos(get_buffer_relative_pos(m_file_pos));
    m_checkpoint_pos.reset();
}

ErrorCode BufferedFileReader::peek_buffered_data (const char*& buf,
                                                  size_t& peek_size) {
    if (-1 == m_fd) {
        return ErrorCode_NotInit;
    }
    // Refill the buffer if it is not loaded yet
    if (0 == m_buffer_reader->get_buffer_size()) {
        auto error_code = refill_reader_buffer(m_base_buffer_size);
        if (ErrorCode_Success != error_code) {
            buf = nullptr;
            peek_size = 0;
            return error_code;
        }
    }
    m_buffer_reader->peek_buffer(buf, peek_size);
    return ErrorCode_Success;
}

size_t BufferedFileReader::quantize_to_buffer_size (size_t size) const {
    if (size == 0) {
        return 0;
    }
    return (1 + ((size - 1) / m_base_buffer_size)) * m_base_buffer_size;
}

ErrorCode BufferedFileReader::refill_reader_buffer (size_t num_bytes_to_refill) {
    size_t num_bytes_refilled = 0;

    const auto buffer_end_pos = get_buffer_end_pos();
    const auto data_size = m_buffer_reader->get_buffer_size();

    size_t num_bytes_alignment = m_base_buffer_size - (buffer_end_pos % m_base_buffer_size);
    if (m_checkpoint_pos.has_value()) {
        while (num_bytes_alignment < num_bytes_to_refill) {
            num_bytes_alignment += m_base_buffer_size;
        }
    }
    // Don't extend the underlying buffer if enough space is available
    if (num_bytes_alignment < m_buffer_size - data_size) {
        auto error_code = try_read_into_buffer(m_fd, m_buffer.get() + data_size,
                                               num_bytes_alignment, num_bytes_refilled);
        if (error_code != ErrorCode_Success) {
            return error_code;
        }
        m_buffer_reader.emplace(m_buffer.get(), num_bytes_refilled + data_size, data_size);
        return ErrorCode_Success;
    }

    if (false == m_checkpoint_pos.has_value()) {
        auto error_code = try_read_into_buffer(m_fd, m_buffer.get(),
                                               num_bytes_alignment, num_bytes_refilled);
        if (error_code != ErrorCode_Success) {
            return error_code;
        }
        m_buffer_begin_pos = buffer_end_pos;
        m_buffer_reader.emplace(m_buffer.get(), num_bytes_refilled);
    } else {
        // Messy way of copying data from old buffer to new buffer
        m_buffer_size = data_size + num_bytes_alignment;
        auto new_buffer = make_unique<char[]>(m_buffer_size);
        memcpy(new_buffer.get(), m_buffer.get(), data_size);

        // Read data to the new buffer, with offset = data_size
        auto error_code = try_read_into_buffer(m_fd, &new_buffer[data_size], num_bytes_alignment,
                                               num_bytes_refilled);
        if (error_code != ErrorCode_Success) {
            return error_code;
        }
        m_buffer = move(new_buffer);
        m_buffer_reader.emplace(m_buffer.get(), data_size + num_bytes_refilled, m_buffer_reader->get_pos());
    }
    return ErrorCode_Success;
}

void BufferedFileReader::resize_buffer_from_pos (size_t pos) {
    if (pos > m_buffer_reader->get_buffer_size()) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }

    const auto copy_size = m_buffer_reader->get_buffer_size() - pos;
    // Use a quantized size for the underlying buffer size
    m_buffer_size = quantize_to_buffer_size(copy_size);
    auto new_buffer = make_unique<char[]>(m_buffer_size);
    memcpy(new_buffer.get(), &m_buffer[pos], copy_size);
    m_buffer = move(new_buffer);
    m_buffer_begin_pos += pos;

    m_buffer_reader.emplace(m_buffer.get(), copy_size);
}