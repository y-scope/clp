#include "BufferReader.hpp"

// C++ standard libraries
#include <algorithm>
#include <cstring>

namespace {

}

BufferReader::BufferReader(char const* data, size_t data_size, size_t pos) {
    if (nullptr == data) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    m_internal_buf = data;
    m_internal_buf_size = data_size;
    m_internal_buf_pos = pos;
}

ErrorCode BufferReader::try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) {
    if (nullptr == buf && num_bytes_to_read > 0) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }

    auto remaining_data_size = get_remaining_data_size();
    if (0 == remaining_data_size) {
        return ErrorCode_EndOfFile;
    }

    num_bytes_read = std::min(remaining_data_size, num_bytes_to_read);
    auto copy_begin = m_internal_buf + m_internal_buf_pos;
    auto copy_end = copy_begin + num_bytes_read;
    std::copy(copy_begin, copy_end, buf);
    m_internal_buf_pos += num_bytes_read;
    return ErrorCode_Success;
}

ErrorCode BufferReader::try_seek_from_begin(size_t pos) {
    if (pos > m_internal_buf_size) {
        return ErrorCode_Truncated;
    }
    m_internal_buf_pos = pos;
    return ErrorCode_Success;
}

ErrorCode BufferReader::try_get_pos(size_t& pos) {
    pos = m_internal_buf_pos;
    return ErrorCode_Success;
}

ErrorCode BufferReader::try_read_to_delimiter(
        char delim,
        bool keep_delimiter,
        bool append,
        std::string& str
) {
    bool found_delim;
    size_t num_bytes_read;
    if (false == append) {
        str.clear();
    }
    return try_read_to_delimiter(delim, keep_delimiter, str, found_delim, num_bytes_read);
}

void BufferReader::peek_buffer(char const*& buf, size_t& peek_size) {
    peek_size = get_remaining_data_size();
    buf = m_internal_buf + m_internal_buf_pos;
}

ErrorCode BufferReader::try_read_to_delimiter(
        char delim,
        bool keep_delimiter,
        std::string& str,
        bool& found_delim,
        size_t& num_bytes_read
) {
    found_delim = false;
    auto const remaining_data_size = get_remaining_data_size();
    if (0 == remaining_data_size) {
        return ErrorCode_EndOfFile;
    }
    // Find the delimiter
    char const* buffer_head = m_internal_buf + m_internal_buf_pos;
    char const* delim_ptr
            = reinterpret_cast<char const*>(memchr(buffer_head, delim, remaining_data_size));

    size_t delim_pos;
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
    // append to strings
    str.append(buffer_head, num_bytes_read);
    m_internal_buf_pos = delim_pos;
    return ErrorCode_Success;
}
