#include "BufferedFileReader.hpp"

#include <fcntl.h>

#include <cerrno>

#include <boost/filesystem.hpp>

#include "math_utils.hpp"

using std::string;

namespace glt {
namespace {
/**
 * Reads from the given file descriptor
 * @param fd
 * @param buf
 * @param num_bytes_to_read
 * @param num_bytes_read
 * @return ErrorCode_errno on error
 * @return ErrorCode_EndOfFile on EOF
 * @return ErrorCode_Success on success
 */
auto read_into_buffer(int fd, char* buf, size_t num_bytes_to_read, size_t& num_bytes_read)
        -> ErrorCode;

auto read_into_buffer(int fd, char* buf, size_t num_bytes_to_read, size_t& num_bytes_read)
        -> ErrorCode {
    num_bytes_read = 0;
    while (true) {
        auto const bytes_read = ::read(fd, buf, num_bytes_to_read);
        if (0 == bytes_read) {
            break;
        }
        if (bytes_read < 0) {
            return ErrorCode_errno;
        }

        buf += bytes_read;
        num_bytes_read += bytes_read;
        num_bytes_to_read -= bytes_read;
        if (num_bytes_read == num_bytes_to_read) {
            return ErrorCode_Success;
        }
    }
    if (0 == num_bytes_read) {
        return ErrorCode_EndOfFile;
    }
    return ErrorCode_Success;
}
}  // namespace

BufferedFileReader::BufferedFileReader(size_t base_buffer_size) {
    if (base_buffer_size % cMinBufferSize != 0) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    m_base_buffer_size = base_buffer_size;
    m_buffer.resize(m_base_buffer_size);
}

BufferedFileReader::~BufferedFileReader() {
    close();
}

auto BufferedFileReader::try_open(string const& path) -> ErrorCode {
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
    m_buffer_reader.emplace(m_buffer.data(), 0);
    m_highest_read_pos = 0;
    return ErrorCode_Success;
}

void BufferedFileReader::open(string const& path) {
    auto const error_code = try_open(path);
    if (ErrorCode_Success != error_code) {
        if (ErrorCode_FileNotFound == error_code) {
            throw OperationFailed(
                    error_code,
                    __FILENAME__,
                    __LINE__,
                    "File not found: " + boost::filesystem::weakly_canonical(path).string()
            );
        }
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
}

auto BufferedFileReader::close() -> void {
    if (-1 == m_fd) {
        return;
    }

    if (m_checkpoint_pos.has_value()) {
        m_buffer.resize(m_base_buffer_size);
        m_checkpoint_pos.reset();
    }

    // NOTE: We don't check errors for close since, in the read case, it seems the only reason it
    // could fail is if it was interrupted by a signal
    ::close(m_fd);
    m_fd = -1;
}

auto BufferedFileReader::try_refill_buffer_if_empty() -> ErrorCode {
    if (-1 == m_fd) {
        return ErrorCode_NotInit;
    }
    if (m_buffer_reader->get_buffer_size() > 0) {
        return ErrorCode_Success;
    }
    return refill_reader_buffer(m_base_buffer_size);
}

void BufferedFileReader::refill_buffer_if_empty() {
    auto error_code = try_refill_buffer_if_empty();
    if (ErrorCode_Success != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
}

auto BufferedFileReader::try_peek_buffered_data(char const*& buf, size_t& peek_size) const
        -> ErrorCode {
    if (-1 == m_fd) {
        return ErrorCode_NotInit;
    }
    m_buffer_reader->peek_buffer(buf, peek_size);
    return ErrorCode_Success;
}

void BufferedFileReader::peek_buffered_data(char const*& buf, size_t& peek_size) const {
    auto error_code = try_peek_buffered_data(buf, peek_size);
    if (ErrorCode_Success != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
}

auto BufferedFileReader::set_checkpoint() -> size_t {
    if (m_checkpoint_pos.has_value() && m_checkpoint_pos < m_file_pos
        && m_buffer_reader->get_buffer_size() != m_base_buffer_size)
    {
        drop_content_before_current_pos();
    }
    m_checkpoint_pos = m_file_pos;
    return m_file_pos;
}

auto BufferedFileReader::clear_checkpoint() -> void {
    if (false == m_checkpoint_pos.has_value()) {
        return;
    }

    auto error_code = try_seek_from_begin(m_highest_read_pos);
    if (ErrorCode_Success != error_code) {
        // Should never happen
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
    drop_content_before_current_pos();
    m_checkpoint_pos.reset();
}

auto BufferedFileReader::try_get_pos(size_t& pos) -> ErrorCode {
    if (-1 == m_fd) {
        return ErrorCode_NotInit;
    }
    pos = m_file_pos;
    return ErrorCode_Success;
}

auto BufferedFileReader::try_seek_from_begin(size_t pos) -> ErrorCode {
    if (-1 == m_fd) {
        return ErrorCode_NotInit;
    }
    if (pos == m_file_pos) {
        return ErrorCode_Success;
    }

    auto seek_lower_bound = m_checkpoint_pos.has_value() ? m_checkpoint_pos.value() : m_file_pos;
    if (pos < seek_lower_bound) {
        return ErrorCode_Unsupported;
    }

    auto error_code = m_buffer_reader->try_seek_from_begin(get_buffer_relative_pos(pos));
    if (ErrorCode_Truncated == error_code) {
        if (false == m_checkpoint_pos.has_value()) {
            // If checkpoint is not set, simply move the file_pos and invalidate
            // the buffer reader
            auto offset = lseek(m_fd, static_cast<off_t>(pos), SEEK_SET);
            if (-1 == offset) {
                return ErrorCode_errno;
            }
            m_buffer_reader.emplace(m_buffer.data(), 0);
            m_buffer_begin_pos = pos;
        } else {
            auto const num_bytes_to_refill = pos - get_buffer_end_pos();
            error_code = refill_reader_buffer(num_bytes_to_refill);
            if (ErrorCode_EndOfFile == error_code) {
                return ErrorCode_Truncated;
            }
            if (ErrorCode_Success != error_code) {
                return error_code;
            }
            error_code = m_buffer_reader->try_seek_from_begin(get_buffer_relative_pos(pos));
            if (ErrorCode_Success != error_code) {
                return error_code;
            }
        }
    } else if (ErrorCode_Success != error_code) {
        return error_code;
    }
    update_file_pos(pos);
    return ErrorCode_Success;
}

auto BufferedFileReader::try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read)
        -> ErrorCode {
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
        size_t bytes_read{0};
        auto error_code = m_buffer_reader->try_read(buf, num_bytes_to_read, bytes_read);
        if (ErrorCode_Success == error_code) {
            buf += bytes_read;
            num_bytes_read += bytes_read;
            num_bytes_to_read -= bytes_read;
            update_file_pos(m_file_pos + bytes_read);
            if (0 == num_bytes_to_read) {
                break;
            }
        } else if (ErrorCode_EndOfFile != error_code) {
            return error_code;
        }

        error_code = refill_reader_buffer(m_base_buffer_size);
        if (ErrorCode_EndOfFile == error_code) {
            break;
        }
        if (ErrorCode_Success != error_code) {
            return error_code;
        }
    }
    if (0 == num_bytes_read) {
        return ErrorCode_EndOfFile;
    }
    return ErrorCode_Success;
}

auto
BufferedFileReader::try_read_to_delimiter(char delim, bool keep_delimiter, bool append, string& str)
        -> ErrorCode {
    if (-1 == m_fd) {
        return ErrorCode_NotInit;
    }
    if (false == append) {
        str.clear();
    }
    bool found_delim{false};
    size_t total_num_bytes_read{0};
    while (true) {
        size_t num_bytes_read{0};
        if (auto ret_code = m_buffer_reader->try_read_to_delimiter(
                    delim,
                    keep_delimiter,
                    str,
                    found_delim,
                    num_bytes_read
            );
            ret_code != ErrorCode_Success && ret_code != ErrorCode_EndOfFile)
        {
            return ret_code;
        }
        update_file_pos(m_file_pos + num_bytes_read);
        total_num_bytes_read += num_bytes_read;
        if (found_delim) {
            break;
        }

        auto error_code = refill_reader_buffer(m_base_buffer_size);
        if (ErrorCode_EndOfFile == error_code) {
            if (0 == total_num_bytes_read) {
                return ErrorCode_EndOfFile;
            }
            break;
        }
        if (ErrorCode_Success != error_code) {
            return error_code;
        }
    }
    return ErrorCode_Success;
}

auto BufferedFileReader::refill_reader_buffer(size_t num_bytes_to_refill) -> ErrorCode {
    auto const buffer_end_pos = get_buffer_end_pos();
    auto const data_size = m_buffer_reader->get_buffer_size();
    auto const available_buffer_space = m_buffer.size() - data_size;

    size_t num_bytes_to_read{0};
    size_t next_buffer_pos{0};
    auto next_buffer_begin_pos = m_buffer_begin_pos;
    if (m_checkpoint_pos.has_value()) {
        num_bytes_to_read = int_round_up_to_multiple(
                buffer_end_pos + num_bytes_to_refill,
                m_base_buffer_size
        );
        // Grow the buffer if necessary
        if (num_bytes_to_read > available_buffer_space) {
            m_buffer.resize(data_size + num_bytes_to_read);
        }
        next_buffer_pos = data_size;
    } else {
        num_bytes_to_read = m_base_buffer_size - (buffer_end_pos % m_base_buffer_size);
        if (num_bytes_to_read > available_buffer_space) {
            // Advance the entire buffer since we don't grow the buffer if there's no checkpoint
            next_buffer_pos = 0;
            next_buffer_begin_pos = buffer_end_pos;
        } else {
            next_buffer_pos = data_size;
        }
    }

    size_t num_bytes_read{0};
    auto error_code
            = read_into_buffer(m_fd, &m_buffer[next_buffer_pos], num_bytes_to_read, num_bytes_read);
    if (error_code != ErrorCode_Success && ErrorCode_EndOfFile != error_code) {
        return error_code;
    }
    // NOTE: We still want to set the buffer reader if no bytes were read on EOF
    m_buffer_reader.emplace(m_buffer.data(), next_buffer_pos + num_bytes_read, next_buffer_pos);
    m_buffer_begin_pos = next_buffer_begin_pos;
    return error_code;
}

auto BufferedFileReader::drop_content_before_current_pos() -> void {
    auto buffer_reader_pos = m_buffer_reader->get_pos();
    auto const new_data_size = m_buffer_reader->get_buffer_size() - buffer_reader_pos;
    auto const new_buffer_size = int_round_up_to_multiple(new_data_size, m_base_buffer_size);

    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + static_cast<long>(buffer_reader_pos));
    m_buffer.resize(new_buffer_size);
    m_buffer_begin_pos += buffer_reader_pos;

    m_buffer_reader.emplace(m_buffer.data(), new_data_size);
}

auto BufferedFileReader::update_file_pos(size_t pos) -> void {
    m_file_pos = pos;
    m_highest_read_pos = std::max(m_file_pos, m_highest_read_pos);
}
}  // namespace glt
