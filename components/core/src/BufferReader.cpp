#include "BufferReader.hpp"

// C++ standard libraries
#include <algorithm>

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

ErrorCode BufferReader::try_get_pos (size_t& pos) {
    pos = m_internal_buf_pos;
    return ErrorCode_Success;
}
