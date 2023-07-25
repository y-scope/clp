#include "BufferReader.hpp"

// C++ standard libraries
#include <algorithm>

#include <stdio.h>
#include <string.h>

BufferReader::BufferReader (const char* data, size_t data_size) {
    if (nullptr == data || 0 == data_size) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    m_internal_buf = data;
    m_internal_buf_size = data_size;
}

ErrorCode BufferReader::try_read (char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) {
    if (nullptr == buf && num_bytes_to_read > 0) {
        return ErrorCode_BadParam;
    }

    auto remaining_data_size = m_internal_buf_size - m_internal_buf_pos;
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

ErrorCode BufferReader::try_seek_from_begin (size_t pos) {
    if (pos > m_internal_buf_size) {
        return ErrorCode_OutOfBounds;
    }
    m_internal_buf_pos = pos;
    return ErrorCode_Success;
}

ErrorCode BufferReader::try_seek_from_current (off_t offset) {
    if (m_internal_buf_pos + offset > m_internal_buf_size) {
        return ErrorCode_OutOfBounds;
    }
    m_internal_buf_pos += offset;
    return ErrorCode_Success;
}

ErrorCode BufferReader::try_get_pos (size_t& pos) {
    pos = m_internal_buf_pos;
    return ErrorCode_Success;
}

void BufferReader::peek_buffer (size_t size_to_peek, const char*& data_ptr, size_t& peek_size) {
    peek_size = std::min(size_to_peek, m_internal_buf_size - m_internal_buf_pos);
    data_ptr = m_internal_buf + m_internal_buf_pos;
}

ErrorCode BufferReader::try_read_to_delimiter (char delim, bool keep_delimiter, bool append,
                                               std::string& str, size_t& length) {

    if (false == append) {
        str.clear();
    }
    // find the pointer pointing to the delimiter
    const char* buffer_head = m_internal_buf + m_internal_buf_pos;
    const char* delim_ptr = reinterpret_cast<const char*>(
            memchr(buffer_head, delim, m_internal_buf_size - m_internal_buf_pos)
    );
    ErrorCode ret_code;
    size_t delim_pos;
    if (delim_ptr != nullptr) {
        delim_pos = (delim_ptr - m_internal_buf) + 1;
        ret_code = ErrorCode_Success;
    } else {
        delim_pos = m_internal_buf_size;
        ret_code = ErrorCode_EndOfFile;
    }
    // append to strings
    length = delim_pos - m_internal_buf_pos;
    if (false == keep_delimiter && delim == m_internal_buf[delim_pos - 1]) {
        --length;
    }
    str.append(buffer_head, length);
    m_internal_buf_pos = delim_pos;
    return ret_code;
}