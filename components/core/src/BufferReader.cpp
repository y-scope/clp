#include "BufferReader.hpp"

// C++ standard libraries
#include <algorithm>
#include <cstring>

BufferReader::BufferReader (const char* data, size_t data_size) {
    if (data == nullptr || data_size == 0) {
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_data = data;
    m_data_size = data_size;
    m_cursor_pos = 0;
}

ErrorCode BufferReader::try_read (char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) {
    if (nullptr == buf && num_bytes_to_read > 0) {
        return ErrorCode_BadParam;
    }

    auto remaining_data_size = m_data_size - m_cursor_pos;
    if (remaining_data_size == 0) {
        return ErrorCode_EndOfFile;
    }

    num_bytes_read = std::min(remaining_data_size, num_bytes_to_read);
    auto copy_begin = m_data + m_cursor_pos;
    auto copy_end = copy_begin + num_bytes_read;
    std::copy(copy_begin, copy_end, buf);
    m_cursor_pos += num_bytes_read;
    return ErrorCode_Success;
}

[[nodiscard]] ErrorCode BufferReader::try_seek_from_begin (size_t pos) {
    if (pos > m_data_size) {
        return ErrorCode_OutOfBounds;
    }
    m_cursor_pos = pos;
    return ErrorCode_Success;
}

[[nodiscard]] ErrorCode BufferReader::try_get_pos (size_t& pos) {
    pos = m_cursor_pos;
    return ErrorCode_Success;
}
