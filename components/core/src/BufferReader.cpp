#include "BufferReader.hpp"

#include <algorithm>
#include <cstring>

BufferReader::BufferReader(char const* data, size_t data_size, size_t pos) {
    if (nullptr == data) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    m_internal_buf = data;
    m_internal_buf_size = data_size;
    m_internal_buf_pos = pos;
}

auto BufferReader::try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read)
        -> ErrorCode {
    if (nullptr == buf && num_bytes_to_read > 0) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }

    auto remaining_data_size = get_remaining_data_size();
    if (0 == remaining_data_size) {
        return ErrorCode_EndOfFile;
    }

    num_bytes_read = std::min(remaining_data_size, num_bytes_to_read);
    const auto* copy_begin = m_internal_buf + m_internal_buf_pos;
    const auto* copy_end = copy_begin + num_bytes_read;
    std::copy(copy_begin, copy_end, buf);
    m_internal_buf_pos += num_bytes_read;
    return ErrorCode_Success;
}

auto BufferReader::try_seek_from_begin(size_t pos) -> ErrorCode {
    if (pos > m_internal_buf_size) {
        return ErrorCode_Truncated;
    }
    m_internal_buf_pos = pos;
    return ErrorCode_Success;
}

auto BufferReader::try_get_pos(size_t& pos) -> ErrorCode {
    pos = m_internal_buf_pos;
    return ErrorCode_Success;
}

auto BufferReader::try_read_to_delimiter(
        char delim,
        bool keep_delimiter,
        bool append,
        std::string& str
) -> ErrorCode {
    if (false == append) {
        str.clear();
    }
    bool found_delim{false};
    size_t num_bytes_read{0};
    return try_read_to_delimiter(delim, keep_delimiter, str, found_delim, num_bytes_read);
}

auto BufferReader::peek_buffer(char const*& buf, size_t& peek_size) -> void {
    peek_size = get_remaining_data_size();
    buf = m_internal_buf + m_internal_buf_pos;
}

auto BufferReader::try_read_to_delimiter(
        char delim,
        bool keep_delimiter,
        std::string& str,
        bool& found_delim,
        size_t& num_bytes_read
) -> ErrorCode {
    found_delim = false;
    auto const remaining_data_size = get_remaining_data_size();
    if (0 == remaining_data_size) {
        return ErrorCode_EndOfFile;
    }

    // Find the delimiter
    auto const* buffer_head = m_internal_buf + m_internal_buf_pos;
    auto* delim_ptr = static_cast<char const*>(memchr(buffer_head, delim, remaining_data_size));

    size_t delim_pos{0};
    if (delim_ptr != nullptr) {
        delim_pos = (delim_ptr - m_internal_buf) + 1;
        num_bytes_read = delim_pos - m_internal_buf_pos;
        if (false == keep_delimiter && delim == m_internal_buf[delim_pos - 1]) {
            --num_bytes_read;
        }
        found_delim = true;
    } else {
        delim_pos = m_internal_buf_size;
        num_bytes_read = remaining_data_size;
    }
    str.append(buffer_head, num_bytes_read);
    m_internal_buf_pos = delim_pos;
    return ErrorCode_Success;
}
